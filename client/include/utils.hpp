#pragma once
#include <string>
#include <utility>
#include "mqtt_client.hpp"

#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime>

std::pair<std::string, std::string> parseRegister(const std::string& input);
std::pair<std::string, std::string> parseLogin(const std::string& input);
std::pair<std::string, std::string> parseGroup(const std::string& input);
std::string parseGroupTopic(const std::string& input);

std::string getCurrentTimestamp();

void help();
void displayMessage(MyMessage message);
std::tuple<std::string, std::string, std::string> parse_chat_topic(std::string topic);

void printWithColor(const std::string& text, const std::string& color, bool bold = false);