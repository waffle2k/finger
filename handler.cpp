#include "handler.hpp"
#include <filesystem>
#include <fstream>
#include <string_view>

bool RealFilesystemWrapper::exists(const std::filesystem::path &path) const {
  return std::filesystem::exists(path);
}

std::string
RealFilesystemWrapper::read_file(const std::filesystem::path &path) const {
  std::ifstream file(path);
  if (!file.is_open()) {
    return "";
  }

  std::string content;
  std::string line;

  while (std::getline(file, line)) {
    content += line + "\n";
  }

  if (content.empty()) {
    return "";
  }

  // Return the content with proper line endings
  if (content.back() == '\n') {
    content.pop_back(); // Remove the last newline
    content += "\r\n";
    return content;
  }

  content += "\r\n";
  return content;
}

std::string process(const std::string &username) {
  RealFilesystemWrapper fs;
  return process(username, fs, kPATH);
}

std::string process(const std::string &username, const IFilesystemWrapper &fs,
                    const std::filesystem::path &basepath) {
  try {
    // Check for directory traversal patterns
    if (username.find("../") != std::string::npos ||
        username.find("..\\") != std::string::npos ||
        username.find("%2e%2e%2f") != std::string::npos ||
        username.find("%2e%2e%5c") != std::string::npos ||
        username.find("%2E%2E%2F") != std::string::npos ||
        username.find("%2E%2E%5C") != std::string::npos ||
        username.find("..%2f") != std::string::npos ||
        username.find("..%5c") != std::string::npos ||
        username.find("..%2F") != std::string::npos ||
        username.find("..%5C") != std::string::npos) {
      throw InvalidInput("Directory traversal detected in username");
    }

    if (username.find("/") != std::string::npos) {
      throw InvalidInput("Path detected in username");
    }
  } catch (InvalidInput &e) {
    return std::string("InvalidInput: ") + e.what() + std::string("\r\n");
  }

  // Attempt to open the plan file (if any) and return the contents as a string
  std::filesystem::path planPath = basepath / username;

  // Check if the plan file exists using the filesystem wrapper
  if (!fs.exists(planPath)) {
    // If no plan file exists, return just the username
    return username;
  }

  // Try to read the plan file using the filesystem wrapper
  std::string content = fs.read_file(planPath);

  if (content.empty()) {
    // If file exists but is empty or couldn't be read, return just the username
    return username;
  }

  return content;
}
