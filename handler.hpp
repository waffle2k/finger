#pragma once

#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

class IFilesystemWrapper {
public:
  virtual ~IFilesystemWrapper() = default;
  virtual bool exists(const std::filesystem::path &path) const = 0;
  virtual std::string read_file(const std::filesystem::path &path) const = 0;
};

class RealFilesystemWrapper : public IFilesystemWrapper {
public:
  bool exists(const std::filesystem::path &path) const override {
    return std::filesystem::exists(path);
  }

  std::string read_file(const std::filesystem::path &path) const override {
    std::ifstream file(path);
    if (!file.is_open()) {
      return "";
    }

    std::string content;
    std::string line;
    while (std::getline(file, line)) {
      content += line + "\n";
    }

    // Return the content with proper line endings
    if (!content.empty() && content.back() == '\n') {
      content.pop_back(); // Remove the last newline
      content += "\r\n";
      return content;
    } else if (!content.empty()) {
      content += "\r\n";
      return content;
    }

    return "";
  }
};

class InvalidInput : public std::runtime_error {
public:
  InvalidInput(const std::string &what = "") : std::runtime_error(what) {}
};

std::string process(const std::string &username);
std::string process(const std::string &username, const IFilesystemWrapper &fs);
