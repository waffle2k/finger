#pragma once

#include <string>
#include <exception>

#include <stdexcept>

class InvalidInput : public std::runtime_error
{
public:
    InvalidInput(const std::string& what = "") : std::runtime_error(what) {}
};


std::string process(const std::string &username);