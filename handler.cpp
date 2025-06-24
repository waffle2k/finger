#include "handler.hpp"
#include <filesystem>
#include <fstream>
#include <string_view>

const std::filesystem::path kPATH{"/var/finger/users/"};

std::string process(const std::string &username) {
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
  std::filesystem::path planPath = kPATH / username;

  // Check if the plan file exists
  if (!std::filesystem::exists(planPath)) {
    // If no plan file exists, return just the username
    return username;
  }

  // Try to read the plan file
  try {
    std::ifstream planFile(planPath);
    if (!planFile.is_open()) {
      return username;
    }

    std::string content;
    std::string line;
    while (std::getline(planFile, line)) {
      content += line + "\n";
    }

    // Return the plan content with proper line endings
    if (!content.empty() && content.back() == '\n') {
      content.pop_back(); // Remove the last newline
      content += "\r\n";
      return content;
    } else if (!content.empty()) {
      content += "\r\n";
      return content;
    } else {
      // If file exists but is empty, return just the username
      return username;
    }
  } catch (...) {
    // If there's any error reading the file, just return the username
    return username;
  }
}
