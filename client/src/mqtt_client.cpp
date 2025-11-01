#include "../include/mqtt_client.hpp"
#include "../include/utils.hpp"
#include <typeinfo>

ChatCallback::ChatCallback(MqttClient *client) : mqttClient(client) {}

void ChatCallback::connected(const std::string &cause)
{
    std::cout << "Conectado ao broker. " << cause << std::endl;

    // Envia uma mensagem fixa para um tópico a cada X segundos em uma thread separada
    std::thread([this]()
                {
        const std::string topic = "global/USERS";
        const int interval_seconds = 10; // ajuste o intervalo conforme necessário

        while (mqttClient->isConnected()) {
            mqttClient->publish_request(topic, "Online", MESSAGE_TYPE_STATUS, 1);
        
            if (!mqttClient->groupsIHost.empty()){
                for (const auto& g: mqttClient->groupsIHost) {
                    const std::string g_name = g["name"];
                    mqttClient->publish_request("global/GROUPS", g_name + "::" + mqttClient->getUsername(), MESSAGE_TYPE_NEWGROUP, 1);
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
            
        } })
        .detach();
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

        if (message.type == MESSAGE_TYPE_MESSAGE)
        {

            if (msg->get_topic() == mqttClient->currentTopic || mqttClient->getUsername() == message.sender)
            {
                displayMessage(message);
            }
            else
            {

                mqttClient->myMessages[msg->get_topic()].push_back(j);
            }
        }
        else if (message.type == MESSAGE_TYPE_STATUS)
        {

            int status = (message.text == "Online") ? 1 : 0;
            mqttClient->userStatus[message.sender] = status;
        }
        else if (message.type == MESSAGE_TYPE_NEWGROUP)
        {

            mqttClient->knownGroups[message.text] = j;
        }
        else if (message.type == MESSAGE_TYPE_GROUP_REQUEST)
        {

            for (const auto& g : mqttClient->groupsIHost) {

                if (g["name"] == message.text){

                    std::cout << "Novo pedido de " << message.sender << " para participar do seu grupo " << message.text << std::endl;

                    mqttClient->myRequests.push_back(j);

                    break;

                }

            }

        }
        else if (message.type == MESSAGE_TYPE_CHAT_REQUEST)
        {

            std::cout << "Novo pedido de chat de " << message.sender << ": " << message.text << std::endl;
            mqttClient->myRequests.push_back(j);
        }
        else if (message.type == MESSAGE_TYPE_GROUPACCEPTANCE)
        {

            mqttClient->subscribe_async(message.text);
            std::cout << message.sender << " aceitou seu pedido para entrar no grupo!\n";

            MyMessage join_msg = MyMessage(message.sender, getCurrentTimestamp(), "Bem vindo ao grupo!", MESSAGE_TYPE_MESSAGE);
            nlohmann::json m = join_msg.to_json();
            mqttClient->myMessages[message.text].push_back(m);
        }
        else
        {

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

        auto payload = msg.to_json_string();
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

bool MqttClient::publish_request(const std::string &topic, const std::string &message, int type, int qos)
{
    try
    {

        MyMessage msg(username_, getCurrentTimestamp(), message, type);

        auto payload = msg.to_json_string(); // serializa para string
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
        //std::cout << "Inscrito no tópico: " << topic << std::endl;
        return true;
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "❌ Erro ao inscrever-se: " << exc.what() << std::endl;
        return false;
    }
}

bool MqttClient::subscribe_async(const std::string &topic, int qos)
{
    try
    {
        client.subscribe(topic, qos);
        //std::cout << "Inscrito no tópico (async): " << topic << std::endl;
        return true;
    }
    catch (const mqtt::exception &exc)
    {
        std::cerr << "❌ Erro ao inscrever-se (async): " << exc.what() << std::endl;
        return false;
    }
}

void MqttClient::disconnect()
{
    try
    {

        MyMessage msg(username_, getCurrentTimestamp(), "Offline", MESSAGE_TYPE_STATUS);
        auto payload = msg.to_json_string();
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

void MqttClient::display_pending_messages(std::string topic)
{
    if (myMessages.find(topic) != myMessages.end())
    {

        std::vector<nlohmann::json> messages = myMessages[topic];
        for (const auto &msg : messages)
        {
            displayMessage(MyMessage::from_json(msg));
        }

        myMessages[topic].clear();
    }
}

std::string MqttClient::display_pending_chats()
{

    std::unordered_map<std::string, std::string> chats;

    for (const auto &[topic, messages] : myMessages)
    {
        std::cout << "Analizando tópico: " << topic << std::endl;
        auto [usr1, usr2, time] = parse_chat_topic(topic);

        if (usr1.empty() || usr2.empty() || time.empty())
        {

            int pos = topic.rfind("global/GROUPS/");
            if (pos != std::string::npos)
            {
                std::string group_name = topic.substr(pos + 14);
                int pos2 = group_name.rfind("_-_");
                if (pos2 != std::string::npos)
                {
                    group_name = group_name.substr(0, pos2);
                    group_name = group_name.append(" [Grupo]");
                    chats[group_name] = topic;
                }
            }
        }
        else
        {

            std::string other_user = (usr1 == username_) ? usr2 : usr1;
            chats[other_user] = topic;
        }
    }

    for(const auto g: groupsIHost){

        std::string name = g["name"];
        name = name.append(" [Grupo]");
        chats[name] = g["topic"];

    }

    if (chats.empty())
    {
        std::cout << "Nenhum chat pendente." << std::endl;
        return "";
    }

    std::cout << "Digite o número para acessar o chat" << std::endl;
    std::cout << "Chats pendentes:" << std::endl;
    int index = 1;
    for (const auto &[user, topic] : chats)
    {
        std::cout << index++ << ". " << user << std::endl;
    }
    std::string input;
    char *line = readline("> ");
    if (line)
    {
        input = line;
        if (!input.empty())
            add_history(line);
        free(line);
    }
    std::cout << "\33[1A\33[2K\r";
    std::cout.flush();
    int chatNumber = std::stoi(input);
    if (chatNumber > 0 && chatNumber < index)
    {
        return std::next(chats.begin(), chatNumber - 1)->second;
    }
    else
    {
        std::cout << "Número inválido." << std::endl;
        return "";
    }
    return "";
};

void MqttClient::display_user_status()
{
    std::cout << "Status dos usuários:" << std::endl;
    for (const auto &[user, status] : userStatus)
    {
        printWithColor(user + ": ", "cyan", false);
        if (status == 1)
        {
            printWithColor("Online", "green", true);
        }
        else
        {
            printWithColor("Offline", "red", true);
        }
        std::cout << std::endl;
    }
}

void MqttClient::display_known_groups()
{

    system("clear");
    std::cout << "Grupos conhecidos:" << std::endl;

    for (const auto &g : knownGroups)
    {

        std::string group_name, owner;
        std::string key = g.first;
        size_t delim_pos = key.find("::");
        if (delim_pos != std::string::npos)
        {
            group_name = key.substr(0, delim_pos);
            owner = key.substr(delim_pos + 2);
        }
        else
        {
            group_name = key;
            owner = "";
        }

        std::cout << "Grupo: " << group_name << " | Host: " << owner << std::endl;
    }
}

std::pair<std::string, std::string> MqttClient::showRequests()
{

    auto &requests = myRequests;

    if (requests.empty())
    {
        std::cout << "Você não tem solicitações pendentes.\n";
        return {"", ""};
    }
    else
    {
        while (true)
        {
            system("clear");
            std::cout << "SUAS SOLICITAÇÕES\nDigite o número da solicitação para interagir ou utilize /exit para voltar.\n\n";
            int i = 1;
            for (const auto &req : requests)
            {
                try
                {
                    auto j = req;
                    MyMessage message = MyMessage::from_json(j);
                    std::cout << i << ". " << message.sender << " gostaria de ";
                    if (message.type == MESSAGE_TYPE_CHAT_REQUEST)
                    {
                        std::cout << "iniciar uma conversa com você! ";
                    }
                    else if (message.type == MESSAGE_TYPE_GROUP_REQUEST)
                    {
                        std::cout << "entrar no seu grupo! ";
                    }
                    else
                    {
                        std::cout << "fazer algo desconhecido! ";
                    }
                    std::cout << "[" << message.timestamp << "]\n";
                    i++;
                }
                catch (...)
                {
                    std::cout << i << ". Mensagem inválida: " << req << "\n";
                    i++;
                }
            }
            std::string input;
            char *line = readline("> ");
            if (line)
            {
                input = line;
                if (!input.empty())
                    add_history(line);
                free(line);
            }
            std::cout << "\33[1A\33[2K\r";
            std::cout.flush();

            if (input == "/exit")
            {
                system("clear");
                return {"", ""};
            }
            else
            {
                int requestNumber = std::stoi(input);
                if (requestNumber > 0 && requestNumber <= requests.size())
                {
                    auto selectedRequest = requests[requestNumber - 1];
                    std::cout << "Digite 1 para aceitar a solicitação ou 0 para recusar\n";
                    std::string response;
                    char *line = readline("> ");
                    if (line)
                    {
                        response = line;
                        if (!response.empty())
                            add_history(line);
                        free(line);
                    }
                    std::cout << "\33[1A\33[2K\r";
                    std::cout.flush();

                    MyMessage msg = MyMessage::from_json(selectedRequest);

                    if (msg.type == MESSAGE_TYPE_GROUP_REQUEST)
                    {

                        if (response == "1")
                        {
                            std::cout << "✅ Solicitação aceita. Agora ele(a) faz parte do grupo!\n";

                            std::string topic = accept_new_member(msg.text, msg.sender);
                            publish_message(topic, "Bem-vindo ao grupo " + msg.text + ", " + msg.sender + "!", 1);
                            requests.erase(requests.begin()+requestNumber-1);

                            return {"", ""};
                        }
                        else if (response == "0")
                        {
                            std::cout << "❌ Solicitação recusada.\n";
                            requests.erase(requests.begin()+requestNumber-1);
                            return {"", ""};
                        }
                        else
                        {
                            std::cout << "Resposta inválida.\n";
                            continue;
                        }
                    }
                    else
                    {

                        if (response == "1")
                        {
                            std::cout << "✅ Solicitação aceita.\n Você será redirecionado ao chat\n";
                            requests.erase(requests.begin()+requestNumber-1);
                            return {selectedRequest["text"], selectedRequest["sender"]};
                        }
                        else if (response == "0")
                        {
                            std::cout << "❌ Solicitação recusada.\n";
                            requests.erase(requests.begin()+requestNumber-1);
                            return {"", ""};
                        }
                        else
                        {
                            std::cout << "Resposta inválida.\n";
                            continue;
                        }
                    }

                }
                else
                {
                    std::cout << "Número de solicitação inválido.\n";
                    continue;
                }
            }
        }
    }
}

std::string MqttClient::accept_new_member(const std::string &groupName, const std::string &new_member)
{

    std::string topic = "";

    for (const auto &g : groupsIHost)
    {
        if (g["name"].get<std::string>() == groupName)
        {

            topic = g["topic"].get<std::string>();
            // std::cout << topic << std::endl;

            break;
        }
    }

    if (topic.empty())
    {
        std::cout << "Grupo não encontrado ou você não é o host do grupo.\n";
        return "";
    }

    publish_request(new_member + "_Control", topic, MESSAGE_TYPE_GROUPACCEPTANCE, 1);
    return topic;
}