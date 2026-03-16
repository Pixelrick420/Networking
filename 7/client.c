#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define SIZE 1024
#define IP "127.0.0.1"

typedef struct sockaddr_in Address;
typedef struct Client {
    int fd;
    Address server;
} Client;

void freeTCPclient(Client *client) {
    close(client->fd);
    free(client);
}

Client *createTCPClient() {
    Client *client = (Client *)malloc(sizeof(Client));
    memset(&client->server, 0, sizeof(client->server));

    client->fd = socket(AF_INET, SOCK_STREAM, 0);
    client->server.sin_family = AF_INET;
    client->server.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &client->server.sin_addr);

    if (client->fd < 0) {
        freeTCPclient(client);
        return NULL;
    }
    return client;
}

void sendToServer(Client *client, char *msg) {
    send(client->fd, msg, strlen(msg), 0);
    printf("\n\nsent %s packet to server\n", msg);
}

void recvFromServer(Client *client, char *buffer) {
    memset(buffer, 0, SIZE);

    int received = recv(client->fd, buffer, SIZE - 1, 0);
    buffer[received] = '\0';
    printf("server: %s\n", buffer);
}

int main() {
    char buffer[SIZE] = {0};
    Client *client = createTCPClient();

    if (!client) {
        perror("creation failed");
        return 1;
    } else if (connect(client->fd, (struct sockaddr *)&client->server,
                       sizeof(client->server)) < 0) {
        perror("connect failed");
        return 1;
    }

    sendToServer(client, "CONNECT");
    recvFromServer(client, buffer);

    sendToServer(client, "HELO");
    recvFromServer(client, buffer);

    sendToServer(client, "SENDER");
    recvFromServer(client, buffer);

    sendToServer(client, "RECIEVER");
    recvFromServer(client, buffer);

    sendToServer(client, "DATA");
    recvFromServer(client, buffer);

    sendToServer(client, "QUIT");
    recvFromServer(client, buffer);

    freeTCPclient(client);
    return 0;
}
