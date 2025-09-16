#pragma once
#include "mqtt/async_client.h"
#include <string>
#include <memory>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>

#define MESSAGE_TYPE_INVALID -1
#define MESSAGE_TYPE_STATUS 0
#define MESSAGE_TYPE_GROUP_REQUEST 1
#define MESSAGE_TYPE_CHAT_REQUEST 2
#define MESSAGE_TYPE_MESSAGE 3

class MqttClient;

class ChatCallback : public virtual mqtt::callback
{
private:

    MqttClient* mqttClient;

public:
    ChatCallback(MqttClient* client);
    void connected(const std::string &cause) override;
    void connection_lost(const std::string &cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr token) override;
};
 
class MyMessage {
public:
    std::string sender;
    std::string timestamp;
    std::string text;
    int type; 

    MyMessage(const std::string& snd, const std::string& time, const std::string& txt, int tp)
        : sender(snd), timestamp(time), text(txt), type(tp) {}

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["sender"] = sender;
        j["timestamp"] = timestamp;
        j["text"] = text;
        j["type"] = std::to_string(type);
        //std::cout << "Parsing message to json: " << sender << ", " << timestamp << ", " << text << ", " << type << std::endl;
        return j.dump();
    }

    static MyMessage from_json(const nlohmann::json& j) {
        return MyMessage(
            j.value("sender", ""),
            j.value("timestamp", ""),
            j.value("text", ""),
            std::stoi(j.value("type", "-1"))
        );
    }
};

class MqttClient
{
private:
    std::string serverAddress;
    std::string clientId;
    std::string username_;
    mqtt::async_client client;
    mqtt::connect_options connOpts;
    ChatCallback cb;

public:
    MqttClient(const std::string &server, const std::string &username);

    
    bool connect(const std::string &user, const std::string &pass);
    bool publish_message(const std::string &topic, const std::string &message, int qos = 1);
    bool publish_request(const std::string &topic, const std::string &message, int type, int qos = 1);
    bool subscribe(const std::string &topic, int qos = 1);
    void disconnect();
    void display_pending_messages(std::string topic);
    std::string display_pending_chats();  //myChats será uma leitura de myMessages verificando os tópicos existentes - retorna o topico que s desej conversar

    std::string getUsername() const { return username_; }
    std::vector<nlohmann::json> myRequests;
    std::unordered_map<std::string, std::vector<nlohmann::json>> myMessages;
    //std::vector<std::tuple<std::string, std::string, std::string>> myChats; // topic, target_user
    std::string currentTopic;
};

