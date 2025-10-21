
# MQTT CHAT

Aplicação de bate-papo simples via terminal, utilizando o protocolo MQTT com o broker Mosquitto rodando localmente. 

Permite conversasa individuais e em grupo

## Funcionalidades

- Registro e login de usuários (no próprio broker)
- Envio e recebimento de mensagens *one on one*
- Envio de solicitação de conversa
- Envio de solicitação para ingresso em bate papos de grupo
- Gerenciamento de solicitações
- Salvamento de mensagens não vizualizadas
- Interação através de comandos básicos via terminal

## Requisitos

- **Mosquitto** (broker MQTT) instalado e rodando localmente
- **Biblioteca Paho MQTT C++** (`libpaho-mqttpp3`, `libpaho-mqtt3as`) [Disponível aqui](https://github.com/eclipse-paho/paho.mqtt.cpp)
- **g++** (compilador C++)

## Instalação das dependências

No Ubuntu/Debian, execute:

```bash
sudo apt update
sudo apt install mosquitto libpaho-mqttpp3-dev libpaho-mqtt3as-dev g++
sudo apt-get install libreadline-dev
```

## Para compilar

Execute o comando abaixo na raiz do projeto:

```bash
g++ client/src/*.cpp -o chat_app.exe -lpaho-mqttpp3 -lpaho-mqtt3as -lpthread -lreadline
```

## Para executar

1. Inicie o broker Mosquitto localmente utilizando as configurações na pasta ``broker``:
    ```bash
    mosquitto -c broker/mosquitto.conf
    ```

*Nota: Após cadastrar um usuário, pode ser necessário reiniciar o broker para que ele aceite a conexão*

2. Execute o aplicativo de chat (em diferentes terminais):
    ```bash
    ./chat_app
    ```

3. Solicite uma conversa:
    ```bash
    /talk @maria
    ```
Lembre-se de adicionar o **@** na frente do identificador do usuário que deseja conversar!

## Comandos disponíveis

- ``/help`` - Mostra todos os comandos disponíveis
- `/register <identificador> <senha>` — Registrar novo usuário
- `/login <identificador> <senha>` — Fazer login
- `/talk @<identificador>` — Solicitar conversa com alguém
- `/creategroup <nome>` - Cria um grupo e define você como líder
- `/join <nome_do_grupo> by @<id_lider>` - Solicita a entrada em um grupo 'nome_do_grupo' liderado por 'id_lider'
- `/mychats` - Mostra as mensgaens pendentes que você possui
- `/myrequests` — Gerenciar as solicitações de mensagem e grupos enviadas para você
- `/userstats` - Mostra o status (online/offline) dos usuários conhecidos
- `/availablegroups` - Mostra os grupos conhecidos e o nome de seus líderes
- `/exit` — Sair do chat
- Qualquer outra mensagem será enviada para a sala atual

## Observações

- O cadastro de usuários é feito localmente no arquivo `broker/users.db`.
- As mensagens são trocadas via tópicos MQTT, cada conversa corresponde a um tópico (id_user_1_id_user_2_timestamp).
- O aplicativo é apenas para fins de estudo/demonstração.

---

*Luiz Henrique Rigo Faccio*  
*2211100003*