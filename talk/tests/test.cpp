#include "gtest/gtest.h"
#include "logger.h"

int main(int argc, char **argv) {
    file_log_init("build/test.log");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
