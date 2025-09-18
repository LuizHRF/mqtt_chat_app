
# MQTT CHAT

Aplicação de bate-papo simples via terminal, utilizando o protocolo MQTT com o broker Mosquitto rodando localmente. 

Permite conversasa individuais [em grupo no futuro]

## Funcionalidades

- Registro e login de usuários
- Envio e recebimento de mensagens em tempo real
- Envio de solicitação de conversa
- Bate papo direto (*Direct Message*)
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
```

## Como compilar

Execute o comando abaixo na raiz do projeto:

```bash
g++ client/src/*.cpp -o chat_app.exe -lpaho-mqttpp3 -lpaho-mqtt3as -lpthread
```

## Como executar

1. Inicie o broker Mosquitto localmente:
    ```bash
    mosquitto -c broker/mosquitto.conf
    ```

*Nota: Após cadastrar um usuário, pode ser necessário reiniciar o broker para que ele aceite a conexão (?)*

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

- `/register <identificador> <senha>` — Registrar novo usuário
- `/login <identificador> <senha>` — Fazer login
- `/talk @<identificador>` — Solicitar conversa com alguém
- `/myrequests` — Visualizar as solicitações de mensagem pra você
- `/exit` — Sair do chat
- Qualquer outra mensagem será enviada para a sala atual

## Observações

- O cadastro de usuários é feito localmente no arquivo `broker/users.db`.
- As mensagens são trocadas via tópicos MQTT, cada sala corresponde a um tópico.
- O aplicativo é apenas para fins de estudo/demonstração.

---

Empasse atual: A persistência de mensagens não funciona corretamente a nível de aplicação.

1. É necessário que um usuário já tenha aceito a *request* de outro usuário para que as mensagens fiquem salvas e sejam entregues  
[Talvez mudar  estrutura de tópicos e aassinar um tópico mais genérico atrves dos coringas. Desta forma as mensagens enviads antes da request ser aceit ficam salvas no broker]  ou  
[Apenas mandara request e abrir o chat somente se a pessoa aceitar]
2. Uma vez que a *request* é aceita ela some e, em logins posteriores, não é possível acessar a mesma conversa (portanto não recebndo as mensagens persistentes)  
[Mudar a estrutura 'myRequests' para que represente um conjunto, assim, toda mensagem pode possuir uma request que substituirá a última, tornando a conversa acessível sempre que houverem mensagens] ou  
[Verificar no broker quais os tópicos cujo um cliente está incrito para pdoer acessar as conversas antigas]