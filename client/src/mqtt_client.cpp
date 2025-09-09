#include "../include/mqtt_client.hpp"
#include "../include/utils.hpp"

ChatCallback::ChatCallback(MqttClient* client) : mqttClient(client) {}

void ChatCallback::connected(const std::string &cause)
{
    std::cout << "Conectado ao broker. " << cause << std::endl;
}

void ChatCallback::connection_lost(const std::string &cause)
{
    std::cout << "⚠️ Conexão perdida. Causa: " << cause << std::endl;
}

void ChatCallback::message_arrived(mqtt::const_message_ptr msg) 
{
    try
    {
        auto j = nlohmann::json::parse(msg->get_payload());
        MyMessage message = MyMessage::from_json(j);

        if (message.type == MESSAGE_TYPE_MESSAGE) {

            if (mqttClient->currentTopic == msg->get_topic())  //Se o usuario estiver na conversa
                std::cout << "[" << message.timestamp << "] " << message.sender << ": " << message.text << std::endl;

        } else if (message.type == MESSAGE_TYPE_STATUS) {

            //mqttClient->myRequests.push_back(j);

        } else if (message.type == MESSAGE_TYPE_CHAT_REQUEST) {

            std::cout << "🔔 Pedido de chat de " << message.sender << ": " << message.text << std::endl;
            mqttClient->myRequests.push_back(j);

        } else {

            std::cout << "❓ Mensagem desconhecida: " << msg->get_payload() << std::endl;

        }
    }
    catch (...)
    {
        std::cout << msg->get_payload() << std::endl;
    }
}

void ChatCallback::delivery_complete(mqtt::delivery_token_ptr token) {}


MqttClient::MqttClient(const std::string &server, const std::string &username)
    : serverAddress(server), username_(username), client(server, username), cb(ChatCallback(this))
{
    connOpts.set_clean_session(false);
    client.set_callback(cb);
}

bool MqttClient::connect(const std::string &user, const std::string &pass)
{
    try
    {
        connOpts.set_user_name(user);
        connOpts.set_password(pass);

        std::cout << "Conectando ao broker em " << serverAddress << "..." << std::endl;
        auto tok = client.connect(connOpts);
        tok->wait();
        std::cout << "✅ Conectado com sucesso!" << std::endl;
        
        MyMessage msg(username_, getCurrentTimestamp(), "Online", MESSAGE_TYPE_STATUS);

        client.publish("global/USERS", msg.to_json(), 1, true);
        return true;
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "❌ Erro ao conectar: " << exc.what() << std::endl;
        return false;
    }
}

bool MqttClient::publish_message(const std::string &topic, const std::string &message, int qos)
{
    try
    {
        MyMessage msg(username_, getCurrentTimestamp(), message, MESSAGE_TYPE_MESSAGE);

        auto payload = msg.to_json();
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
        pubmsg->set_qos(qos);

        client.publish(pubmsg);
        return true;
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "❌ Erro ao enviar mensagem: " << exc.what() << std::endl;
        return false;
    }
}

bool MqttClient::publish_request(const std::string &topic, const std::string &message, int qos = 1, int type)
{
    try
    {

        MyMessage msg(username_, getCurrentTimestamp(), message, type);

        auto payload = msg.to_json(); // serializa para string
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, payload);
        pubmsg->set_qos(qos);
        client.publish(pubmsg);
        return true;
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "❌ Erro ao publicar: " << exc.what() << std::endl;
        return false;
    }
}

bool MqttClient::subscribe(const std::string &topic, int qos)
{
    try
    {
        client.subscribe(topic, qos)->wait();
        std::cout << "Inscrito no tópico: " << topic << std::endl;
        return true;
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "❌ Erro ao inscrever-se: " << exc.what() << std::endl;
        return false;
    }
}

void MqttClient::disconnect()
{
    try
    {
        client.publish("global/USERS", username_ + " is Offline", 1, false);
        client.disconnect()->wait();
        std::cout << "🔌 Desconectado do broker." << std::endl;
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "❌ Erro ao desconectar: " << exc.what() << std::endl;
    }
}
