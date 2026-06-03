#include <gtest/gtest.h>
#include <tesults_gtest/tesults_gtest.h>

TEST(CalculatorTest, Addition) {
    tesults_gtest::description("Verifies basic integer addition");
    EXPECT_EQ(2 + 2, 4);
}

TEST(CalculatorTest, Subtraction) {
    tesults_gtest::custom("operands", "10 - 3");
    EXPECT_EQ(10 - 3, 7);
}

TEST(CalculatorTest, Division) {
    tesults_gtest::step({"setup", "pass", "Initialise values", ""});
    tesults_gtest::step({"divide", "pass", "10 / 2", ""});
    EXPECT_EQ(10 / 2, 5);
}

int main(int argc, char* argv[]) {
    auto cfg = tesults_gtest::config_from_args(argc, argv);
    testing::InitGoogleTest(&argc, argv);
    if (!cfg.target.empty()) {
        testing::UnitTest::GetInstance()->listeners().Append(
            new tesults_gtest::TesultsListener(cfg)
        );
    }
    return RUN_ALL_TESTS();
}
