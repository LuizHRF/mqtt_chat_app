#pragma once
#include <string>
#include <utility>

#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>

class myMessage {
public:
    std::string user;
    std::string content;
    std::string timestamp;

    myMessage(const std::string& user, const std::string& content, const std::string& timestamp)
        : user(user), content(content), timestamp(timestamp) {}
};

std::pair<std::string, std::string> parseRegister(const std::string& input);
std::pair<std::string, std::string> parseLogin(const std::string& input);


std::string getCurrentTimestamp();

void help();