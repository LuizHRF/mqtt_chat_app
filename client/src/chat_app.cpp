#include "../include/chat_app.hpp"
#include "../include/utils.hpp"
#include "../include/mqtt_client.hpp"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <nlohmann/json.hpp>
#include <memory>

void ChatApp::run() {
    std::string input;
    std::unique_ptr<MqttClient> client = nullptr;

    while (true) {
        char* line = readline("> ");
        if (line) {
            input = line;
            if (!input.empty()) add_history(line);
            free(line);
        }
        std::cout << "\33[1A\33[2K\r";  
        std::cout.flush();

        if (input.rfind("/register", 0) == 0) {
            // Exemplo: /register luiz senha123
            auto [user, pass] = parseRegister(input);
            std::string cmd = "mosquitto_passwd -b broker/users.db " + user + " " + pass;
            int ret = std::system(cmd.c_str());
            std::cout << (ret == 0 ? "✅ Usuário cadastrado!\n" : "❌ Erro ao cadastrar.\n");

        }
        else if (input.rfind("/login", 0) == 0) {

            auto [user, pass] = parseLogin(input);
            client = std::make_unique<MqttClient>("tcp://localhost:1883", user);

            if (client->connect(user, pass)) {
                system("clear");
                std::cout << "✅ Login bem-sucedido!\n";

                client->subscribe("global/USERS");
                client->subscribe(user + "_Control");
                client->subscribe("global/GROUPS");
                client->currentTopic = "NONE";
                

            } else {
                std::cout << "❌ Erro de login.\n";
            }
        }
        else if (input.rfind("/join", 0) == 0) {
            if (client) {

                auto[group_name, group_owner] = parseGroup(input);
                if (group_name.empty() || group_owner.empty()){
                    std::cout << "Forneça um nome de grupo e ID de usuário válidos!\n";
                    continue;
                }

                client->publish_request(group_owner + "_Control", group_name, MESSAGE_TYPE_GROUP_REQUEST, 1);
                std::cout << "\nA solicitação para entrar no grupo " << group_name << " foi enviada para " << group_owner << ".\nSe o grupo existir e " << group_owner<< " for o líder, ele poderá analisar sua solicitação!\n\n";


            } else {
                std::cout << "⚠️ Você precisa estar logado primeiro.\n";
            }
        }
        else if (input == "/help") {
            help();
        }

        else if (input == "/availablegroups"){
            if (client) {
                client->display_known_groups();
            } else {
                std::cout << "⚠️ Você precisa estar logado primeiro.\n";
            }
        }

        else if (input.rfind("/talk", 0) == 0){
            if (client) {
                std::string user_id = input.substr(6);
                if (user_id.empty()) {
                    std::cout << "❌ Por favor, forneça um ID de usuário.\n";
                } else {

                    std::string newTopic = "chat/" + user_id + "_" + client->getUsername() + "_" + getCurrentTimestamp();
                    client->subscribe(newTopic);
                    client->publish_request(user_id + "_Control", newTopic, MESSAGE_TYPE_CHAT_REQUEST, 1);

                    //std::cout << "Tópico atual: " << client->currentTopic << "\n";
                    system("clear");
                    //std::cout << "Sua solicitação foi enviada para " << user_id << " [" << client->currentTopic << "]\n";
                    printWithColor("Sua solicitação foi enviada para ", "blue", false);
                    printWithColor(user_id, "yellow", true);
                    printWithColor(" [" + newTopic + "]\n", "blue", false);
                }
            } else {
                std::cout << "⚠️ Você precisa estar logado primeiro.\n";
            }
        }

        else if (input.rfind("/creategroup", 0) == 0){
            if (client){

                std::string group_name = input.substr(13);
                if (group_name.empty()){
                    std::cout << "❌ Por favor, forneça um ID de usuário.\n";
                } else {
                    std::string topic = "global/GROUPS/" + group_name + "_-_" + getCurrentTimestamp();


                    nlohmann::json new_group;
                    new_group["name"] = group_name;
                    new_group["topic"] = topic;
                    new_group["timestamp"] = getCurrentTimestamp();
                    new_group["members"] = {client->getUsername()};

                    client->groupsIHost.push_back(new_group);

                    client->subscribe(topic);
                    client->publish_request("global/GROUPS", group_name + "::" + client->getUsername(), MESSAGE_TYPE_NEWGROUP, 1);
                    std::cout << "Novo grupo ";
                    printWithColor(group_name, "yellow", true);
                    std::cout << " criado com sucesso!\n";

                }

            } else {
                std::cout << "⚠️ Você precisa estar logado primeiro.\n";
            }
        }
        else if (input ==  "/myrequests") {
            if (client) {

                auto [topic, sender] = client->showRequests();
                if (topic.empty()) continue;

                client->currentTopic = topic;
                client->subscribe(client->currentTopic);

                system("clear");
                std::cout << "Você está conversando com " << sender << "\n";

                talk(client.get());

            } else {
                std::cout << "⚠️ Você precisa estar logado primeiro.\n";
            }
        }

        else if (input ==  "/mychats") {
            if (client) {

                std::string topic = client->display_pending_chats();
                if (topic.empty()) continue;

                std::string groupName = parseGroupTopic(topic);

                system("clear");
                
                std::cout << "Você está no chat de grupo ";
                printWithColor(groupName+"\n", "yellow", true);

                client->currentTopic = topic;
                talk(client.get());

            } else {
                std::cout << "⚠️ Você precisa estar logado primeiro.\n";
            }
        }

        else if (input ==  "/userstats") {
            if (client) {
                client->display_user_status();
            } else {
                std::cout << "Você precisa estar logado primeiro.\n";
            }
        }

        else if (input == "/exit") {
            if (client) {
                client->disconnect();
                client.reset(); 
            }
            
            std::cout << "👋 Saindo do chat...\n";
            break;
        }
        else {
            std::cout << "⚠️ Comando Inválido. Utilize /help para ver a lista de comandos.\n";
        }
    }
}

void talk(MqttClient* client) {
    client->subscribe(client->currentTopic);

    std::cout << "Digite ";
    printWithColor("/exit", "red", true);
    std::cout << " para sair do chat.\n";
    client->display_pending_messages(client->currentTopic);
    std::string input;
    while (true) {
        char* line = readline("> ");
        if (line) {
            input = line;
            if (!input.empty()) add_history(line);
            free(line);
        }
        std::cout << "\33[1A\33[2K\r";
        std::cout.flush();

        if (input.empty()) continue;   

        if (input == "/exit") {
            client->publish_message(client->currentTopic, "{Saiu da conversa}");
            client->currentTopic = "NONE";
            system("clear");
            break;
        } else {
            client->publish_message(client->currentTopic, input);
        }
    }
}