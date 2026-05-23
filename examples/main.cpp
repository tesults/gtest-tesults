#include <gtest/gtest.h>
#include <gtest_tesults/gtest_tesults.h>

TEST(CalculatorTest, Addition) {
    gtest_tesults::description("Verifies basic integer addition");
    EXPECT_EQ(2 + 2, 4);
}

TEST(CalculatorTest, Subtraction) {
    gtest_tesults::custom("operands", "10 - 3");
    EXPECT_EQ(10 - 3, 7);
}

TEST(CalculatorTest, Division) {
    gtest_tesults::step({"setup", "pass", "Initialise values", ""});
    gtest_tesults::step({"divide", "pass", "10 / 2", ""});
    EXPECT_EQ(10 / 2, 5);
}

int main(int argc, char* argv[]) {
    auto cfg = gtest_tesults::config_from_args(argc, argv);
    testing::InitGoogleTest(&argc, argv);
    if (!cfg.target.empty()) {
        testing::UnitTest::GetInstance()->listeners().Append(
            new gtest_tesults::TesultsListener(cfg)
        );
    }
    return RUN_ALL_TESTS();
}
