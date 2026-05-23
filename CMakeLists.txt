cmake_minimum_required(VERSION 3.14)
project(gtest-tesults VERSION 1.0.0 LANGUAGES CXX)

add_library(gtest_tesults INTERFACE)
target_include_directories(gtest_tesults INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(gtest_tesults INTERFACE cxx_std_17)

# Enable automatic listener registration via environment variables.
# Intended for use with gtest_main (e.g. ROS 2 / ament_add_gtest) where a
# custom main() is not available. Do not combine with config_from_args.
option(GTEST_TESULTS_AUTO_REGISTER "Auto-register Tesults listener via environment variables" OFF)
if(GTEST_TESULTS_AUTO_REGISTER)
    target_compile_definitions(gtest_tesults INTERFACE GTEST_TESULTS_AUTO_REGISTER=1)
endif()

if(BUILD_EXAMPLES)
    find_package(GTest REQUIRED)
    find_package(tesults CONFIG QUIET)
    if(NOT tesults_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            tesults
            GIT_REPOSITORY https://github.com/tesults/cpp.git
            GIT_TAG        main
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(tesults)
    endif()
    add_executable(gtest_tesults_example examples/main.cpp)
    target_link_libraries(gtest_tesults_example PRIVATE
        gtest_tesults
        tesults
        GTest::gtest
    )
endif()
