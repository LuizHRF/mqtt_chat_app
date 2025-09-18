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
        std::cout << "> ";
        std::getline(std::cin, input);
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
                client->currentTopic = "NONE";
                

            } else {
                std::cout << "❌ Erro de login.\n";
            }
        }
        else if (input.rfind("/join", 0) == 0) {
            if (client) {
                std::string group = input.substr(6);
                if (group.empty()) group = "chat/global";
                client->currentTopic = "chat/" + group;
                client->subscribe(client->currentTopic);
                system("clear");
                std::cout << "Entrou na sala: " << client->currentTopic << "\n";
            } else {
                std::cout << "⚠️ Você precisa estar logado primeiro.\n";
            }
        }
        else if (input == "/help") {
            help();
        }
        else if (input.rfind("/talk", 0) == 0){
            if (client) {
                std::string user_id = input.substr(6);
                if (user_id.empty()) {
                    std::cout << "❌ Por favor, forneça um ID de usuário.\n";
                } else {

                    client->currentTopic = "chat/" + user_id + "_" + client->getUsername() + "_" + getCurrentTimestamp();
                    client->publish_request(user_id + "_Control", client->currentTopic, MESSAGE_TYPE_CHAT_REQUEST, 1);

                    //std::cout << "Tópico atual: " << client->currentTopic << "\n";
                    system("clear");
                    //std::cout << "Sua solicitação foi enviada para " << user_id << " [" << client->currentTopic << "]\n";
                    printWithColor("Sua solicitação foi enviada para ", "blue", false);
                    printWithColor(user_id, "yellow", true);
                    printWithColor(" [" + client->currentTopic + "]\n", "blue", false);

                    talk(client.get());
                    
                }
            } else {
                std::cout << "⚠️ Você precisa estar logado primeiro.\n";
            }
        }
        else if (input ==  "/myrequests") {
            if (client) {

                auto [topic, sender] = showRequests(client->myRequests);
                if (topic.empty()) continue;

                client->currentTopic = topic;
                client->subscribe(client->currentTopic);

                system("clear");
                std::cout << "Você está conversando com " << sender << " [" << client->currentTopic << "]\n";

                talk(client.get());

            } else {
                std::cout << "⚠️ Você precisa estar logado primeiro.\n";
            }
        }

        else if (input ==  "/mychats") {
            if (client) {

                std::string topic = client->display_pending_chats();
                if (topic.empty()) continue;

                std::cout << "Entrando no chat " << topic << "\n";
                client->currentTopic = topic;
                talk(client.get());

            } else {
                std::cout << "⚠️ Você precisa estar logado primeiro.\n";
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

std::pair<std::string, std::string> showRequests(const std::vector<nlohmann::json>& requests) {
    if (requests.empty()) {
        std::cout << "Você não tem solicitações pendentes.\n";
        return {"", ""};
    } else {
        while(true){
            system("clear");
            std::cout << "SUAS SOLICITAÇÕES\nDigite o número da solicitação para interagir ou utilize /exit para voltar.\n\n";
            int  i = 1;
            for (const auto& req: requests) {
                try {
                    auto j = req;
                    MyMessage message = MyMessage::from_json(j);
                    std::cout << i << ". " << message.sender << " gostaria de ";
                    if (message.type == MESSAGE_TYPE_CHAT_REQUEST) {
                        std::cout << "iniciar uma conversa com você! ";
                    } else if (message.type == MESSAGE_TYPE_GROUP_REQUEST) {
                        std::cout << "entrar no seu grupo! ";
                    } else {
                        std::cout << "fazer algo desconhecido! ";
                    }
                    std::cout << "[" << message.timestamp << "]\n";
                    i++;
                } catch (...) {
                    std::cout << i << ". Mensagem inválida: " << req << "\n";
                    i++;
                }
            }
            std::cout << "> ";
            std::string input;
            std::getline(std::cin, input);
            std::cout << "\33[1A\33[2K\r";
            std::cout.flush();

            if (input == "/exit") {
                system("clear");
                return {"", ""};
            }
            else {
                int requestNumber = std::stoi(input);
                if (requestNumber > 0 && requestNumber <= requests.size()) {
                    auto selectedRequest = requests[requestNumber - 1];
                    std::cout << "Digite 1 para aceitar a solicitação ou 0 para recusar\n";
                    std::cout << "> ";
                    std::string response;
                    std::getline(std::cin, response);
                    std::cout << "\33[1A\33[2K\r";
                    std::cout.flush();

                    if (response == "1") {
                        std::cout << "✅ Solicitação aceita.\n Você será redirecionado ao chat\n";
                        return {selectedRequest["text"], selectedRequest["sender"]};

                    } else if (response == "0") {
                        std::cout << "❌ Solicitação recusada.\n";
                        return {"", ""};
                    } else {
                        std::cout << "Resposta inválida.\n";
                        continue;
                    }
                } else {
                    std::cout << "Número de solicitação inválido.\n";
                    continue;
                }
            }
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
        std::cout << "> ";
        std::getline(std::cin, input);
        std::cout << "\33[1A\33[2K\r";
        std::cout.flush();

        if (input.empty()) continue;   

        if (input == "/exit") {
            client->publish_message(client->currentTopic, "{Saiu da conversa}");

            //UNSIBSCRIBE DO TÓPICO ??
            //client->unsubscribe(client->currentTopic);
            client->currentTopic = "NONE";
            system("clear");
            break;
        } else {
            client->publish_message(client->currentTopic, input);
        }
    }
}