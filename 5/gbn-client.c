#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define SIZE 1024
#define FRAMES 8

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
    printf("[GBN] Receiver connected.\n\n");

    char buf[SIZE], ack[SIZE];
    int expected = 0;
    int nak_sent = 0;

    while (expected < FRAMES) {
        memset(buf, 0, SIZE);
        int bytes = recv(client->fd, buf, SIZE, 0);
        if (bytes <= 0)
            break;

        if (strncmp(buf, "END_WINDOW", 10) == 0) {

            if (nak_sent) {
                nak_sent = 0;
            } else {
                int last = atoi(buf + 11);
                printf("[GBN] Receiver: Window done → sending cumulative ACK "
                       "%d\n\n",
                       last);
                snprintf(ack, SIZE, "ACK:%d", last);
                send(client->fd, ack, strlen(ack), 0);
                expected = last + 1;
            }
        } else if (strncmp(buf, "DROP", 4) == 0) {
            int seq = atoi(buf + 5);
            printf("[GBN] Receiver: Frame %d lost (simulated drop) → sending "
                   "NAK %d\n",
                   seq, seq);
            printf("[GBN] Receiver: Discarding any further out-of-order frames "
                   "in this window\n\n");
            snprintf(ack, SIZE, "NAK:%d", seq);
            send(client->fd, ack, strlen(ack), 0);
            nak_sent = 1;
        } else if (strncmp(buf, "FRAME", 5) == 0) {
            int seq;
            char data[SIZE];
            sscanf(buf, "FRAME:%d:%s", &seq, data);

            if (nak_sent) {

                printf("[GBN] Receiver: Discarding out-of-order frame %d\n",
                       seq);
            } else if (seq == expected) {
                printf("[GBN] Receiver: Accepted frame %d [%s]\n", seq, data);
                expected++;
            } else {
                printf("[GBN] Receiver: Unexpected frame %d (expected %d), "
                       "discarding\n",
                       seq, expected);
            }
        }
    }

    printf("[GBN] Receiver: All %d frames received.\n", FRAMES);
    freeClient(client);
    return 0;
}
