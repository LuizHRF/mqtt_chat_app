#include "../include/mqtt_client.hpp"
#include "../include/utils.hpp"

ChatCallback::ChatCallback(MqttClient* client) : mqttClient(client) {}

void ChatCallback::connected(const std::string &cause)
{
    std::cout << "Conectado ao broker. " << cause << std::endl;

    // Envia uma mensagem fixa para um tópico a cada X segundos em uma thread separada
    std::thread([this]() {
        const std::string topic = "global/USERS";
        const int interval_seconds = 10; // ajuste o intervalo conforme necessário

        while (mqttClient->isConnected()) {
            mqttClient->publish_request(topic, "Online", MESSAGE_TYPE_STATUS, 1);
        
            if (!mqttClient->groupsIHost.empty()){
                for (const auto& g: mqttClient->groupsIHost)

                    mqttClient->publish_request("global/GROUPS", g["name"]+"::"+mqttClient->getUsername(), MESSAGE_TYPE_NEWGROUP, 1);
            }

            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
            
        }
    }).detach();
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


            if (msg->get_topic() == mqttClient->currentTopic || mqttClient->getUsername() == message.sender) {
                displayMessage(message);
            }else{
                
                mqttClient->myMessages[msg->get_topic()].push_back(j);
            }

        } else if (message.type == MESSAGE_TYPE_STATUS) {

            int status = (message.text == "Online") ? 1 : 0;
            mqttClient->userStatus[message.sender] = status;
        
        }else if (message.type == MESSAGE_TYPE_NEWGROUP){

            mqttClient->knownGroups.push_back(j);
            
        } else if (message.type == MESSAGE_TYPE_GROUP_REQUEST) {

            std::cout << "Novo pedido de " << message.sender << " para participar do seu grupo " << message.text << std::endl;
            mqttClient->myRequests.push_back(j);

        } else if (message.type == MESSAGE_TYPE_CHAT_REQUEST) {

            std::cout << "Novo pedido de chat de " << message.sender << ": " << message.text << std::endl;
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
        connOpts.set_clean_session(false);
        std::cout << "Conectando ao broker em " << serverAddress << "..." << std::endl;
        auto tok = client.connect(connOpts);
        tok->wait();
        std::cout << "✅ Conectado com sucesso!" << std::endl;
        
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

bool MqttClient::publish_request(const std::string &topic, const std::string &message,  int type, int qos)
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
        //  std::cout << "Inscrito no tópico: " << topic << std::endl;
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

        MyMessage msg(username_, getCurrentTimestamp(), "Offline", MESSAGE_TYPE_STATUS);
        auto payload = msg.to_json();
        mqtt::message_ptr pubmsg = mqtt::make_message("global/USERS", payload);
        pubmsg->set_qos(1);
        client.publish(pubmsg);

        client.disconnect()->wait();
        std::cout << "🔌 Desconectado do broker." << std::endl;
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "❌ Erro ao desconectar: " << exc.what() << std::endl;
    }
}

void MqttClient::display_pending_messages(std::string topic){
    if (myMessages.find(topic) != myMessages.end()) {
        
        std::vector<nlohmann::json> messages = myMessages[topic];
        for (const auto& msg : messages) {
            displayMessage(MyMessage::from_json(msg));
        }
        myMessages[topic].clear(); 
    }
}

std::string MqttClient::display_pending_chats(){
    
    //ler todas as mensagens em myMessages e
    //agrupar por tópico

    std::unordered_map<std::string, std::string> chats; // @remetente, topico_da_conversa

    for (const auto& [topic, messages] : myMessages) {
        auto [usr1, usr2, time] = parse_chat_topic(topic);
        std::string other_user = (usr1 == username_) ? usr2 : usr1;
        chats[other_user] = topic;
    }

    if (chats.empty()) {
        std::cout << "Nenhum chat pendente." << std::endl;
        return "";
    }

    std::cout << "Digite o número para acessar o chat" << std::endl;
    std::cout << "Chats pendentes:" << std::endl;
    int index = 1;
    for (const auto& [user, topic] : chats) {
        std::cout << index++ << ". " << user << std::endl;
    }
    std::string input;
    char* line = readline("> ");
    if (line) {
        input = line;
        if (!input.empty()) add_history(line);
        free(line);
}
    std::cout << "\33[1A\33[2K\r";
    std::cout.flush();
    int chatNumber = std::stoi(input);
    if (chatNumber > 0 && chatNumber < index) {
        return std::next(chats.begin(), chatNumber - 1)->second;
    } else {
        std::cout << "Número inválido." << std::endl;
        return "";
    }
    return "";


};   

void MqttClient::display_user_status(){
    std::cout << "Status dos usuários:" << std::endl;
    for (const auto& [user, status] : userStatus) {
        printWithColor(user + ": ", "cyan", false);
        if (status == 1) {
            printWithColor("Online", "green", true);
        } else {
            printWithColor("Offline", "red", true);
        }
        std::cout << std::endl;
    }
}

void MqttClient::display_known_groups(){

}