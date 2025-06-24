#include "handler.hpp"
#include <gtest/gtest.h>

// Test fixture for process function tests
class ProcessTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Valid username tests
TEST_F(ProcessTest, ValidUsernameSimple) {
  std::string result = process("john");
  EXPECT_EQ(result, "john");
}

TEST_F(ProcessTest, ValidUsernameWithNumbers) {
  std::string result = process("user123");
  EXPECT_EQ(result, "user123");
}

TEST_F(ProcessTest, ValidUsernameWithUnderscore) {
  std::string result = process("user_name");
  EXPECT_EQ(result, "user_name");
}

TEST_F(ProcessTest, ValidUsernameWithHyphen) {
  std::string result = process("user-name");
  EXPECT_EQ(result, "user-name");
}

TEST_F(ProcessTest, ValidUsernameEmptyString) {
  std::string result = process("");
  EXPECT_EQ(result, "");
}

// Directory traversal tests
TEST_F(ProcessTest, DirectoryTraversalBasicDotDotSlash) {
  std::string result = process("user../file");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

TEST_F(ProcessTest, DirectoryTraversalBasicDotDotBackslash) {
  std::string result = process("user..\\file");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

TEST_F(ProcessTest, DirectoryTraversalURLEncodedLowercase2e2e2f) {
  std::string result = process("user%2e%2e%2ffile");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

TEST_F(ProcessTest, DirectoryTraversalURLEncodedLowercase2e2e5c) {
  std::string result = process("user%2e%2e%5cfile");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

TEST_F(ProcessTest, DirectoryTraversalURLEncodedUppercase2E2E2F) {
  std::string result = process("user%2E%2E%2Ffile");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

TEST_F(ProcessTest, DirectoryTraversalURLEncodedUppercase2E2E5C) {
  std::string result = process("user%2E%2E%5Cfile");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

TEST_F(ProcessTest, DirectoryTraversalMixedDotDot2f) {
  std::string result = process("user..%2ffile");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

TEST_F(ProcessTest, DirectoryTraversalMixedDotDot5c) {
  std::string result = process("user..%5cfile");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

TEST_F(ProcessTest, DirectoryTraversalMixedDotDot2F) {
  std::string result = process("user..%2Ffile");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

TEST_F(ProcessTest, DirectoryTraversalMixedDotDot5C) {
  std::string result = process("user..%5Cfile");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

// Path detection tests
TEST_F(ProcessTest, PathDetectionForwardSlash) {
  std::string result = process("user/name");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Path detected") != std::string::npos);
}

TEST_F(ProcessTest, PathDetectionForwardSlashAtStart) {
  std::string result = process("/username");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Path detected") != std::string::npos);
}

TEST_F(ProcessTest, PathDetectionForwardSlashAtEnd) {
  std::string result = process("username/");
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Path detected") != std::string::npos);
}

// Edge cases
TEST_F(ProcessTest, SingleDot) {
  std::string result = process(".");
  EXPECT_EQ(result, ".");
}

TEST_F(ProcessTest, DoubleDotWithoutSlash) {
  std::string result = process("..");
  EXPECT_EQ(result, "..");
}

TEST_F(ProcessTest, ContainsDotButNotTraversal) {
  std::string result = process("user.name");
  EXPECT_EQ(result, "user.name");
}

TEST_F(ProcessTest, BackslashWithoutDots) {
  std::string result = process("user\\name");
  EXPECT_EQ(result, "user\\name");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
