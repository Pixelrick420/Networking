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

    char filename[SIZE];
    printf("Enter local filename:");
    scanf("%s", filename);

    FILE *fp = fopen(filename, "r");
    printf("Enter remote filename:");
    scanf("%s", filename);
    sendToServer(client, filename);
    recvFromServer(client, buffer);

    printf("Sending File..\n");
    int n;
    while ((n = fread(buffer, 1, SIZE, fp)) > 0) {
        send(client->fd, buffer, n, 0);
    }
    printf("File Sent\n");
    freeTCPclient(client);
    return 0;
}
