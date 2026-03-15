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
    printf("[SR] Receiver connected.\n\n");

    char buf[SIZE], ack[SIZE];
    int received[FRAMES];
    memset(received, 0, sizeof(received));

    int nak_seq = -1;
    int in_retransmit = 0;

    while (1) {
        memset(buf, 0, SIZE);
        int bytes = recv(client->fd, buf, SIZE, 0);
        if (bytes <= 0)
            break;

        if (strncmp(buf, "END_WINDOW", 10) == 0 && !in_retransmit) {

            if (nak_seq == -1) {
                int last = atoi(buf + 11);

                (void)last;
            }

        } else if (strncmp(buf, "END_RETRANSMIT", 14) == 0) {
            in_retransmit = 0;
            nak_seq = -1;

            printf("[SR] Receiver: Delivering buffered frames in order:\n");
            for (int i = 0; i < FRAMES; i++) {
                if (received[i])
                    printf("  → Delivered frame %d\n", i);
            }
            printf("\n");

        } else if (strncmp(buf, "DROP", 4) == 0) {
            int seq = atoi(buf + 5);
            printf("[SR] Receiver: Frame %d lost (simulated drop) → buffering "
                   "hole, sending NAK %d\n",
                   seq, seq);
            nak_seq = seq;
            snprintf(ack, SIZE, "NAK:%d", seq);
            send(client->fd, ack, strlen(ack), 0);

        } else if (strncmp(buf, "FRAME", 5) == 0) {
            int seq;
            char data[SIZE];
            sscanf(buf, "FRAME:%d:%s", &seq, data);

            if (in_retransmit) {

                printf("[SR] Receiver: Received retransmitted frame %d [%s]\n",
                       seq, data);
                received[seq] = 1;
                snprintf(ack, SIZE, "ACK:%d", seq);
                send(client->fd, ack, strlen(ack), 0);
            } else if (nak_seq >= 0 && seq != nak_seq) {

                printf("[SR] Receiver: Buffering out-of-order frame %d [%s]\n",
                       seq, data);
                received[seq] = 1;
                snprintf(ack, SIZE, "ACK:%d", seq);
                send(client->fd, ack, strlen(ack), 0);
            } else {
                printf("[SR] Receiver: Accepted frame %d [%s]\n", seq, data);
                received[seq] = 1;
                snprintf(ack, SIZE, "ACK:%d", seq);
                send(client->fd, ack, strlen(ack), 0);
            }

            int all = 1;
            for (int i = 0; i < FRAMES; i++)
                if (!received[i]) {
                    all = 0;
                    break;
                }
            if (all) {
                printf(
                    "\n[SR] Receiver: All %d frames received and delivered.\n",
                    FRAMES);
                break;
            }
        }
    }

    freeClient(client);
    return 0;
}
