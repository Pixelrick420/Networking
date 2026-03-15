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

void freeUDPClient(Client *client) {
    close(client->fd);
    free(client);
}

Client *createUDPClient() {
    Client *client = (Client *)malloc(sizeof(Client));
    memset(&client->server, 0, sizeof(client->server));

    client->fd = socket(AF_INET, SOCK_DGRAM, 0);
    client->server.sin_family = AF_INET;
    client->server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &client->server.sin_addr);

    if (client->fd < 0) {
        freeUDPClient(client);
        return NULL;
    }
    return client;
}

int main() {
    char buffer[SIZE] = {0};
    Client *client = createUDPClient();

    if (!client) {
        perror("creation failed");
        return 1;
    }

    printf("Sending to server\n");

    char *msg = "Hello Server";
    socklen_t len = sizeof(client->server);
    sendto(client->fd, msg, strlen(msg), 0, (struct sockaddr *)&client->server,
           len);
    recvfrom(client->fd, buffer, SIZE, 0, (struct sockaddr *)&client->server,
             &len);
    printf("Server: %s\n", buffer);

    freeUDPClient(client);
    return 0;
}
