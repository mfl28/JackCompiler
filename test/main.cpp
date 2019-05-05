#include <gtest/gtest.h>

std::string testFilesPath;

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    // Require the test-files directory path to be passed as a parameter:
    assert(argc == 2);
    testFilesPath = argv[1];
    return RUN_ALL_TESTS();
}