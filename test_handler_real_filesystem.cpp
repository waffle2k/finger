#include "handler.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

// Test fixture for real filesystem integration tests
class RealFilesystemTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a unique temporary directory for this test run
    temp_dir =
        std::filesystem::temp_directory_path() /
        ("finger_test_" +
         std::to_string(
             std::chrono::duration_cast<std::chrono::nanoseconds>(
                 std::chrono::high_resolution_clock::now().time_since_epoch())
                 .count()));

    // Create the temporary directory
    std::filesystem::create_directories(temp_dir);

    // Set up test base path within temp directory
    test_base_path = temp_dir / "users";
    std::filesystem::create_directories(test_base_path);
  }

  void TearDown() override {
    // Clean up all temporary files and directories
    if (std::filesystem::exists(temp_dir)) {
      std::filesystem::remove_all(temp_dir);
    }
  }

  // Helper method to create a test file with specified content
  void createTestFile(const std::string &filename, const std::string &content) {
    std::filesystem::path file_path = test_base_path / filename;
    std::ofstream file(file_path);
    if (file.is_open()) {
      file << content;
      file.close();
    }
  }

  // Helper method to create a test directory
  void createTestDirectory(const std::string &dirname) {
    std::filesystem::path dir_path = test_base_path / dirname;
    std::filesystem::create_directories(dir_path);
  }

  // Helper method to check if a file exists in the test directory
  bool testFileExists(const std::string &filename) {
    return std::filesystem::exists(test_base_path / filename);
  }

  std::filesystem::path temp_dir;
  std::filesystem::path test_base_path;
  RealFilesystemWrapper real_fs;
};

// Test RealFilesystemWrapper exists() method
TEST_F(RealFilesystemTest, ExistsMethodWithRealFile) {
  // Create a test file
  createTestFile("testuser", "Test content");

  // Test that exists() returns true for existing file
  std::filesystem::path file_path = test_base_path / "testuser";
  EXPECT_TRUE(real_fs.exists(file_path));
}

TEST_F(RealFilesystemTest, ExistsMethodWithNonExistentFile) {
  // Test that exists() returns false for non-existent file
  std::filesystem::path file_path = test_base_path / "nonexistent";
  EXPECT_FALSE(real_fs.exists(file_path));
}

// Test RealFilesystemWrapper read_file() method
TEST_F(RealFilesystemTest, ReadFileWithSimpleContent) {
  std::string expected_content = "Hello, World!";
  createTestFile("simple", expected_content);

  std::filesystem::path file_path = test_base_path / "simple";
  std::string result = real_fs.read_file(file_path);

  // RealFilesystemWrapper adds \r\n at the end
  EXPECT_EQ(result, expected_content + "\r\n");
}

TEST_F(RealFilesystemTest, ReadFileWithMultilineContent) {
  std::string expected_content = "Line 1\nLine 2\nLine 3";
  createTestFile("multiline", expected_content);

  std::filesystem::path file_path = test_base_path / "multiline";
  std::string result = real_fs.read_file(file_path);

  // RealFilesystemWrapper processes line by line and adds \r\n
  EXPECT_EQ(result, "Line 1\nLine 2\nLine 3\r\n");
}

TEST_F(RealFilesystemTest, ReadFileWithEmptyFile) {
  createTestFile("empty", "");

  std::filesystem::path file_path = test_base_path / "empty";
  std::string result = real_fs.read_file(file_path);

  // Empty files return empty string
  EXPECT_EQ(result, "");
}

TEST_F(RealFilesystemTest, ReadFileNonExistentFile) {
  std::filesystem::path file_path = test_base_path / "nonexistent";
  std::string result = real_fs.read_file(file_path);

  // Non-existent files return empty string
  EXPECT_EQ(result, "");
}

// Integration tests with process() function using real filesystem
TEST_F(RealFilesystemTest, ProcessWithExistingUserFile) {
  std::string user_content = "John Doe\nSoftware Engineer\nLoves C++";
  createTestFile("johndoe", user_content);

  std::string result = process("johndoe", real_fs, test_base_path);
  EXPECT_EQ(result, "John Doe\nSoftware Engineer\nLoves C++\r\n");
}

TEST_F(RealFilesystemTest, ProcessWithNonExistentUserFile) {
  std::string result = process("nonexistentuser", real_fs, test_base_path);
  EXPECT_EQ(result, "nonexistentuser");
}

TEST_F(RealFilesystemTest, ProcessWithEmptyUserFile) {
  createTestFile("emptyuser", "");

  std::string result = process("emptyuser", real_fs, test_base_path);
  EXPECT_EQ(result, "emptyuser");
}

TEST_F(RealFilesystemTest, ProcessWithComplexUserInfo) {
  std::string user_content = "Name: Alice Smith\n"
                             "Title: Senior Developer\n"
                             "Department: Engineering\n"
                             "Office: Building A, Room 123\n"
                             "Phone: +1-555-0123\n"
                             "Email: alice.smith@company.com\n"
                             "Projects: Project Alpha, Project Beta\n"
                             "Skills: C++, Python, JavaScript";
  createTestFile("alice", user_content);

  std::string result = process("alice", real_fs, test_base_path);
  EXPECT_EQ(result, user_content + "\r\n");
}

// Test with different file permissions and edge cases
TEST_F(RealFilesystemTest, ProcessWithSingleLineFile) {
  std::string user_content = "Simple one-line user info";
  createTestFile("simpleuser", user_content);

  std::string result = process("simpleuser", real_fs, test_base_path);
  EXPECT_EQ(result, user_content + "\r\n");
}

TEST_F(RealFilesystemTest, ProcessWithFileContainingOnlyNewlines) {
  std::string user_content = "\n\n\n";
  createTestFile("newlineuser", user_content);

  std::string result = process("newlineuser", real_fs, test_base_path);
  // RealFilesystemWrapper reads line by line, so the last empty line after the
  // final \n is not read as a separate line, resulting in "\n\n\r\n"
  EXPECT_EQ(result, "\n\n\r\n");
}

// Test directory traversal protection with real filesystem
TEST_F(RealFilesystemTest, ProcessDirectoryTraversalProtectionWithRealFS) {
  // Create a file outside the base path that shouldn't be accessible
  std::filesystem::path outside_file = temp_dir / "secret.txt";
  std::ofstream secret_file(outside_file);
  secret_file << "Secret content";
  secret_file.close();

  // Try to access it via directory traversal
  std::string result = process("../secret", real_fs, test_base_path);
  EXPECT_TRUE(result.find("InvalidInput:") == 0);
  EXPECT_TRUE(result.find("Directory traversal detected") != std::string::npos);
}

// Test with custom base path
TEST_F(RealFilesystemTest, ProcessWithCustomBasePath) {
  // Create a different base directory
  std::filesystem::path custom_base = temp_dir / "custom_users";
  std::filesystem::create_directories(custom_base);

  // Create a user file in the custom base
  std::filesystem::path user_file = custom_base / "customuser";
  std::ofstream file(user_file);
  file << "Custom base path user";
  file.close();

  std::string result = process("customuser", real_fs, custom_base);
  EXPECT_EQ(result, "Custom base path user\r\n");
}

// Test filesystem wrapper with various file types
TEST_F(RealFilesystemTest, ReadFileWithSpecialCharacters) {
  std::string special_content = "User with special chars: àáâãäåæçèéêë";
  createTestFile("specialuser", special_content);

  std::filesystem::path file_path = test_base_path / "specialuser";
  std::string result = real_fs.read_file(file_path);

  EXPECT_EQ(result, special_content + "\r\n");
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
