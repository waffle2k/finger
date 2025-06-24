#include "handler.hpp"

std::string process(const std::string &username){
    try{
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

        if (username.find("/") != std::string::npos){
            throw InvalidInput("Path detected in username");
        }
    }
    catch(InvalidInput&e){
        return std::string("InvalidInput: ") + e.what() + std::string("\r\n");
    }
    return username;
}
