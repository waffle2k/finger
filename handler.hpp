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
  bool exists(const std::filesystem::path &path) const override;
  std::string read_file(const std::filesystem::path &path) const override;
};

class InvalidInput : public std::runtime_error {
public:
  InvalidInput(const std::string &what = "") : std::runtime_error(what) {}
};

const std::filesystem::path kPATH{"/var/finger/users/"};

std::string process(const std::string &username);
std::string process(const std::string &username, const IFilesystemWrapper &fs,
                    const std::filesystem::path &basepath = kPATH);
