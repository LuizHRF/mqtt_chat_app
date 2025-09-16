#include "../include/utils.hpp"
#include "../include/mqtt_client.hpp"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <ctime>

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_time);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H:%M:%S");
    return oss.str();
}

std::pair<std::string, std::string> parseRegister(const std::string& input) {
    std::istringstream iss(input);
    std::string cmd, user, pass;
    iss >> cmd >> user >> pass;
    user = '@' + user;
    return {user, pass};
}

std::pair<std::string, std::string> parseLogin(const std::string& input) {
    std::istringstream iss(input);
    std::string cmd, user, pass;
    iss >> cmd >> user >> pass;
    user = '@' + user;
    return {user, pass};
}

void help() {
    system("clear");
    std::cout << "Comandos disponíveis:\n";
    std::cout << "/register <username> <password> - Registrar um novo usuário\n";
    std::cout << "/login <username> <password>    - Fazer login\n";
    std::cout << "/talk <user_id>                 - Conversar com um usuário\n";
    std::cout << "/join <group_name>              - Entrar em um grupo de chat (padrão: global)\n";
    std::cout << "/creategroup <group_name>       - Criar um novo grupo de chat\n";
    std::cout << "/myrequests                     - Listar suas solicitações\n";
    std::cout << "/exit                           - Sair do chat\n";

}

void displayMessage(MyMessage message) {
    std::cout << "[" << message.timestamp << "] " << message.sender << ": " << message.text << std::endl;
}

std::tuple<std::string, std::string, std::string> parse_chat_topic(std::string topic){  //chat/@maria_@luiz2025-09-16 12:50:12
    if (topic.rfind("chat/", 0) == 0) {
        std::string chat_part = topic.substr(5); // Remove "chat/"
        size_t underscore_pos = chat_part.find('_');
        if (underscore_pos != std::string::npos) {
            std::string user1 = chat_part.substr(0, underscore_pos);
            std::string rest = chat_part.substr(underscore_pos + 1);
            size_t second_underscore_pos = rest.find('_');
            if (second_underscore_pos != std::string::npos) {
                std::string user2 = rest.substr(0, second_underscore_pos);
                std::string timestamp = rest.substr(second_underscore_pos + 1);
                return {user1, user2, timestamp};
            } else {
                return {user1, rest, ""}; // No timestamp
            }
        }
    }
    return {"", "", ""}; // Invalid format
}