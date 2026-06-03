// tesults_gtest.h — Google Test event listener for Tesults
#pragma once

#include <tesults/tesults.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace tesults_gtest {

// ── Enhanced reporting storage ────────────────────────────────────────────────

namespace detail {

struct EnhancedData {
    std::vector<std::string> files;
    std::string              desc;
    std::map<std::string, std::string> custom;
    std::vector<tesults::Step>         steps;
};

inline std::mutex& enhanced_mutex() {
    static std::mutex m;
    return m;
}

inline std::map<std::string, EnhancedData>& enhanced_store() {
    static std::map<std::string, EnhancedData> store;
    return store;
}

inline std::string current_key() {
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    if (!info) return "";
    return std::string(info->test_suite_name()) + "/" + info->name();
}

} // namespace detail

// Call from inside a TEST body to attach a file to the current test case.
inline void file(const std::string& path) {
    auto key = detail::current_key();
    if (key.empty()) return;
    std::lock_guard<std::mutex> lock(detail::enhanced_mutex());
    detail::enhanced_store()[key].files.push_back(path);
}

// Call from inside a TEST body to add a custom field to the current test case.
inline void custom(const std::string& name, const std::string& value) {
    auto key = detail::current_key();
    if (key.empty()) return;
    std::lock_guard<std::mutex> lock(detail::enhanced_mutex());
    detail::enhanced_store()[key].custom["_" + name] = value;
}

// Call from inside a TEST body to set a description for the current test case.
inline void description(const std::string& value) {
    auto key = detail::current_key();
    if (key.empty()) return;
    std::lock_guard<std::mutex> lock(detail::enhanced_mutex());
    detail::enhanced_store()[key].desc = value;
}

// Call from inside a TEST body to add a step to the current test case.
inline void step(tesults::Step s) {
    auto key = detail::current_key();
    if (key.empty()) return;
    std::lock_guard<std::mutex> lock(detail::enhanced_mutex());
    detail::enhanced_store()[key].steps.push_back(std::move(s));
}

// ── Listener ──────────────────────────────────────────────────────────────────

class TesultsListener : public testing::EmptyTestEventListener {
public:
    struct Config {
        std::string target;
        std::string files_path;
        std::string build_name;
        std::string build_result;
        std::string build_desc;
        std::string build_reason;
    };

    explicit TesultsListener(Config config) : config_(std::move(config)) {}

    void OnTestStart(const testing::TestInfo& info) override {
        start_times_[key_for(info)] = now_ms();
    }

    void OnTestEnd(const testing::TestInfo& info) override {
        tesults::Case tc;
        tc.name  = info.name();
        tc.suite = info.test_suite_name();
        tc.end   = now_ms();

        auto it = start_times_.find(key_for(info));
        if (it != start_times_.end()) {
            tc.start = it->second;
            start_times_.erase(it);
        }

        const auto* result = info.result();
        if (result->Passed()) {
            tc.result = "pass";
        } else if (result->Failed()) {
            tc.result = "fail";
            std::ostringstream reason;
            for (int i = 0; i < result->total_part_count(); ++i) {
                const auto& part = result->GetTestPartResult(i);
                if (part.failed()) {
                    reason << part.message() << "\n";
                }
            }
            tc.reason = reason.str();
        } else {
            tc.result = "unknown";
        }

        // Merge enhanced reporting data
        {
            std::lock_guard<std::mutex> lock(detail::enhanced_mutex());
            auto eit = detail::enhanced_store().find(key_for(info));
            if (eit != detail::enhanced_store().end()) {
                auto& ed = eit->second;
                for (auto& f : ed.files)        tc.files.push_back(f);
                for (auto& [k, v] : ed.custom)  tc.custom[k] = v;
                if (!ed.desc.empty())            tc.desc  = ed.desc;
                if (!ed.steps.empty())           tc.steps = ed.steps;
                detail::enhanced_store().erase(eit);
            }
        }

        // Legacy files-path directory scanning
        if (!config_.files_path.empty()) {
            for (auto& f : collect_files(tc.suite, tc.name)) {
                tc.files.push_back(f);
            }
        }

        cases_.push_back(std::move(tc));
    }

    void OnTestProgramEnd(const testing::UnitTest&) override {
        if (!config_.build_name.empty()) {
            tesults::Case build;
            build.name      = config_.build_name;
            build.suite     = "[build]";
            build.result    = valid_result(config_.build_result);
            build.rawResult = config_.build_result;
            if (!config_.build_desc.empty())   build.desc   = config_.build_desc;
            if (!config_.build_reason.empty()) build.reason = config_.build_reason;
            if (!config_.files_path.empty()) {
                build.files = collect_files("[build]", config_.build_name);
            }
            cases_.push_back(std::move(build));
        }

        tesults::Data data;
        data.target             = config_.target;
        data.cases              = cases_;
        data.integrationName    = "tesults-gtest";
        data.integrationVersion = "1.0.2";
        data.testFramework      = "googletest";

        std::cout << "Tesults results uploading..." << std::endl;
        auto resp = tesults::upload(data);
        std::cout << "success: " << (resp.success ? "true" : "false") << "\n";
        std::cout << "message: " << resp.message << "\n";
        for (auto& w : resp.warnings) std::cout << "warning: " << w << "\n";
        for (auto& e : resp.errors)   std::cout << "error: "   << e << "\n";
    }

private:
    Config config_;
    std::vector<tesults::Case> cases_;
    std::map<std::string, long long> start_times_;

    static std::string key_for(const testing::TestInfo& info) {
        return std::string(info.test_suite_name()) + "/" + info.name();
    }

    static long long now_ms() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    static std::string valid_result(const std::string& r) {
        if (r == "pass" || r == "fail") return r;
        return "unknown";
    }

    std::vector<std::string> collect_files(const std::string& suite, const std::string& name) {
        std::vector<std::string> files;
        try {
            std::filesystem::path dir =
                std::filesystem::path(config_.files_path) / suite / name;
            if (std::filesystem::is_directory(dir)) {
                for (auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
                    if (entry.is_regular_file()) {
                        files.push_back(entry.path().string());
                    }
                }
            }
        } catch (...) {}
        return files;
    }
};

// ── Registration flag (forward declaration) ───────────────────────────────────
namespace detail {
inline bool& listener_registered() {
    static bool v = false;
    return v;
}
} // namespace detail

// ── Config parsing ────────────────────────────────────────────────────────────

// Load key=value pairs from a config file. Lines beginning with # are comments.
inline std::map<std::string, std::string> load_config_file(const std::string& path) {
    std::map<std::string, std::string> kv;
    std::ifstream f(path);
    if (!f.is_open()) return kv;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key   = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        kv[key] = value;
    }
    return kv;
}

// Call this BEFORE testing::InitGoogleTest so gtest does not see unknown flags.
// Removes --tesults-* flags from argv in-place and decrements argc accordingly.
// If --tesults-config is provided and --tesults-target matches a key in that
// file, the key is replaced with the corresponding token value.
inline TesultsListener::Config config_from_args(int& argc, char* argv[]) {
    TesultsListener::Config cfg;
    std::string config_path;
    int new_argc = 1;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        bool consumed = false;
        auto try_extract = [&](const char* prefix, std::string& out) -> bool {
            if (arg.rfind(prefix, 0) == 0) {
                out = arg.substr(std::strlen(prefix));
                consumed = true;
                return true;
            }
            return false;
        };
        try_extract("--tesults-target=",            cfg.target)       ||
        try_extract("--tesults-config=",            config_path)      ||
        try_extract("--tesults-files=",             cfg.files_path)   ||
        try_extract("--tesults-build-name=",        cfg.build_name)   ||
        try_extract("--tesults-build-result=",      cfg.build_result) ||
        try_extract("--tesults-build-description=", cfg.build_desc)   ||
        try_extract("--tesults-build-reason=",      cfg.build_reason);
        if (!consumed) {
            argv[new_argc++] = argv[i];
        }
    }
    argc = new_argc;
    // Only fall back to env var if auto_register hasn't already used it
    if (cfg.target.empty() && !detail::listener_registered()) {
        const char* env = std::getenv("TESULTS_TARGET");
        if (env) cfg.target = env;
    }
    // If a config file was provided, treat target as a key and look up the token
    if (!config_path.empty() && !cfg.target.empty()) {
        auto kv = load_config_file(config_path);
        auto it = kv.find(cfg.target);
        if (it != kv.end()) {
            cfg.target = it->second;
        }
    }
    return cfg;
}

// ── Environment-variable based config ────────────────────────────────────────

// Build a Config entirely from environment variables. Supported variables:
//   TESULTS_TARGET       — token or key name (required for upload)
//   TESULTS_CONFIG       — path to key=value config file for key lookup
//   TESULTS_FILES        — path to files generated by tests
//   TESULTS_BUILD_NAME   — build name
//   TESULTS_BUILD_RESULT — build result (pass/fail/unknown)
//   TESULTS_BUILD_DESCRIPTION — build description
//   TESULTS_BUILD_REASON — build failure reason
inline TesultsListener::Config config_from_env() {
    TesultsListener::Config cfg;
    auto getenv_str = [](const char* name) -> std::string {
        const char* v = std::getenv(name);
        return v ? v : "";
    };
    cfg.target       = getenv_str("TESULTS_TARGET");
    cfg.files_path   = getenv_str("TESULTS_FILES");
    cfg.build_name   = getenv_str("TESULTS_BUILD_NAME");
    cfg.build_result = getenv_str("TESULTS_BUILD_RESULT");
    cfg.build_desc   = getenv_str("TESULTS_BUILD_DESCRIPTION");
    cfg.build_reason = getenv_str("TESULTS_BUILD_REASON");
    // Key lookup via config file
    std::string config_path = getenv_str("TESULTS_CONFIG");
    if (!config_path.empty() && !cfg.target.empty()) {
        auto kv = load_config_file(config_path);
        auto it = kv.find(cfg.target);
        if (it != kv.end()) cfg.target = it->second;
    }
    return cfg;
}

// ── Auto-registration ─────────────────────────────────────────────────────────

// Registers the Tesults listener using environment variables. Called
// automatically at static-initialisation time (before main()) whenever
// TESULTS_TARGET is set. config_from_args skips the TESULTS_TARGET env-var
// fallback if this has already fired, preventing double-registration.
inline void auto_register() {
    auto cfg = config_from_env();
    if (cfg.target.empty()) return;
    testing::UnitTest::GetInstance()->listeners().Append(
        new TesultsListener(cfg)
    );
    detail::listener_registered() = true;
}

namespace detail {
struct AutoRegister {
    AutoRegister() { tesults_gtest::auto_register(); }
};
inline const AutoRegister auto_register_instance;
} // namespace detail

} // namespace tesults_gtest
