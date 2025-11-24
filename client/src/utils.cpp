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

std::pair<std::string, std::string> parseGroup(const std::string& input) {
    std::istringstream iss(input);
    std::string cmd, group, by, owner;
    iss >> cmd >> group >> by >> owner;
    if (owner.empty() || owner[0] != '@') {
        return {"", ""};
    }
    return {group, owner};
}

std::string parseGroupTopic(const std::string& input) {  // global/GROUPS/amigos_-_2025-10-30_20:01:20
    const std::string prefix = "global/GROUPS/";

    // Garante que o prefixo está presente
    if (input.rfind(prefix, 0) != 0) {
        throw std::invalid_argument("Tópico inválido: prefixo ausente");
    }

    // Pega a parte depois de "global/GROUPS/"
    std::string after_prefix = input.substr(prefix.size());

    // Procura o separador "_-_"
    size_t pos = after_prefix.find("_-_");
    if (pos == std::string::npos) {
        throw std::invalid_argument("Tópico inválido: separador '_-_' não encontrado");
    }

    // Extrai o nome do grupo
    std::string group_name = after_prefix.substr(0, pos);

    return group_name;
}

void help() {
    system("clear");
    printWithColor("Comandos disponíveis:\n", "black", false);

    // Helper lambda to print each command line
    auto printCommand = [](const std::string& cmd, const std::string& params, const std::string& desc) {
        printWithColor(cmd, "blue", true);
        if (!params.empty()) {
            std::cout << " ";
            printWithColor(params, "black", true);
        }
        std::cout << " - ";
        printWithColor(desc + "\n", "cyan", false);
    };

    printCommand("/register", "<username> <password>     ", "Registrar um novo usuário");
    printCommand("/login", "<username> <password>        ", "Fazer login");
    printCommand("/talk", "@<user_id>                    ", "Envia solicitação para um usuário");
    printCommand("/join", "<group_name> by @<group_owner>", "Entra em um grupo de chat");
    printCommand("/creategroup", "<group_name>           ", "Cria um novo grupo de chat");
    printCommand("/availablegroups", "                   ", "Lista os grupos disponíveis");
    printCommand("/myrequests", "                        ", "Lista suas solicitações");
    printCommand("/mychats", "                           ", "Lista seus chats pendentes");
    printCommand("/userstats", "                         ", "Lista o status dos usuários");
    printCommand("/exit", "                              ", "Sai do chat ou da aplicação"); 
    printCommand("/whoami", "                            ", "Mostra qual usário está logado"); 


    // std::cout << "/register <username> <password> - Registrar um novo usuário\n";
    // std::cout << "/login <username> <password>    - Fazer login\n";
    // std::cout << "/talk <user_id>                 - Evia solicitação para um usuário\n";
    // std::cout << "/join <group_name>              - Entrar em um grupo de chat (não implementado)\n";
    // std::cout << "/creategroup <group_name>       - Criar um novo grupo de chat (não implementado)\n";
    // std::cout << "/myrequests                     - Listar suas solicitações\n";
    // std::cout << "/mychats                        - Listar seus chats pendentes\n";
    // std::cout << "/userstats                      - Listar usuários e seus status\n";
    // std::cout << "/exit                           - Sair do chat\n";
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

void printWithColor(const std::string& text, const std::string& color, bool bold) {

    std::string code;
    std::string reset = "\033[0m";

    if (color == "red") {
        code = "\033[31m";
    } else if (color == "green") {
        code = "\033[32m";
    } else if (color == "yellow") {
        code = "\033[33m";
    } else if (color == "blue") {
        code = "\033[34m";
    } else if (color == "magenta") {
        code = "\033[35m";
    } else if (color == "cyan") {
        code = "\033[36m";
    } else if (color == "white") {
        code = "\033[37m";
    } else if (color == "black") {
        code = "\033[30m";
    } else if (color == "gray") {
        code = "\033[90m";
    } else {
        code = reset;
    }

    if (bold) {
        code = "\033[1;" + code.substr(2);
    }
    std::string final = code + text + reset;
    std::cout << final;
}

void displayMessage(MyMessage message) {
    printWithColor("[" + message.timestamp + "] ", "white", true);
    std::cout << message.sender << ": " << message.text << std::endl;
}