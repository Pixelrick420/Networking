#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define SIZE 1024

typedef struct sockaddr_in Address;
typedef struct Client {
    int fd;
    Address server;
} Client;

void freeTCPclient(Client *client) {
    close(client->fd);
    free(client);
}

Client *createTCPServer() {
    Client *client = (Client *)malloc(sizeof(Client));
    memset(&client->server, 0, sizeof(client->server));

    client->fd = socket(AF_INET, SOCK_STREAM, 0);
    client->server.sin_family = AF_INET;
    client->server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &client->server.sin_addr);

    if (client->fd < 0) {
        freeTCPclient(client);
        return NULL;
    }
    return client;
}

int main() {
    char buffer[SIZE] = {0};
    Client *client = createTCPServer();

    if (!client) {
        perror("creation failed");
        return 1;
    } else if (connect(client->fd, (struct sockaddr *)&client->server,
                       sizeof(client->server)) < 0) {
        perror("connect failed");
        return 1;
    }

    printf("Connected to server\n");

    char *msg = "Hello Server";
    send(client->fd, msg, strlen(msg), 0);
    recv(client->fd, buffer, SIZE, 0);
    printf("Server: %s\n", buffer);

    freeTCPclient(client);
    return 0;
}
