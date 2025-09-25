#ifndef CHAT_APP_HPP
#define CHAT_APP_HPP

#include <string>
#include <memory>
#include <vector>
#include <readline/readline.h>
#include <readline/history.h>
#include <nlohmann/json.hpp>

class MqttClient;

class ChatApp {
public:
    ChatApp() = default;
    void run();
};

std::pair<std::string, std::string> showRequests(const std::vector<nlohmann::json>& requests);
void talk(MqttClient* client);

#endif // CHAT_APP_HPP

