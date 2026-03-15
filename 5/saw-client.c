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

void freeClient(Client *client) {
    close(client->fd);
    free(client);
}

Client *createClient() {
    Client *client = (Client *)malloc(sizeof(Client));
    memset(&client->server, 0, sizeof(client->server));

    client->fd = socket(AF_INET, SOCK_STREAM, 0);
    client->server.sin_family = AF_INET;
    client->server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &client->server.sin_addr);

    if (client->fd < 0) {
        freeClient(client);
        return NULL;
    }
    return client;
}

int main() {
    Client *client = createClient();
    if (!client) {
        perror("creation failed");
        return 1;
    }

    if (connect(client->fd, (struct sockaddr *)&client->server,
                sizeof(client->server)) < 0) {
        perror("connect failed");
        return 1;
    }
    printf("[SAW] Receiver connected to sender.\n\n");

    char buf[SIZE];
    char ack[SIZE];

    while (1) {
        memset(buf, 0, SIZE);
        int bytes = recv(client->fd, buf, SIZE, 0);
        if (bytes <= 0)
            break;

        if (strncmp(buf, "DROP", 4) == 0) {
            int seq = atoi(buf + 5);
            printf("[SAW] Receiver: Frame %d not received (simulated drop) → "
                   "sending NAK %d\n\n",
                   seq, seq);
            snprintf(ack, SIZE, "NAK:%d", seq);
            send(client->fd, ack, strlen(ack), 0);
        } else if (strncmp(buf, "FRAME", 5) == 0) {

            int seq;
            char data[SIZE];
            sscanf(buf, "FRAME:%d:%s", &seq, data);
            printf("[SAW] Receiver: Got frame %d [%s] → sending ACK %d\n\n",
                   seq, data, seq);
            snprintf(ack, SIZE, "ACK:%d", seq);
            send(client->fd, ack, strlen(ack), 0);
        } else {
            break;
        }
    }

    printf("[SAW] Receiver: All frames received.\n");
    freeClient(client);
    return 0;
}
