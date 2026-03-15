#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define SIZE 1024
#define FRAMES 8
#define WINDOW 4
#define DROP_FRAME 2

typedef struct sockaddr_in Address;
typedef struct Server {
    Address address;
    int fd;
    int con;
} Server;

void freeServer(Server *server) {
    if (server->con >= 0)
        close(server->con);
    if (server->fd >= 0)
        close(server->fd);
    free(server);
}

Server *createServer() {
    Server *server = (Server *)malloc(sizeof(Server));
    memset(&server->address, 0, sizeof(server->address));
    server->con = -1;

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (server->fd < 0) {
        freeServer(server);
        return NULL;
    }
    if (bind(server->fd, (struct sockaddr *)&server->address,
             sizeof(server->address)) < 0) {
        freeServer(server);
        return NULL;
    }
    if (listen(server->fd, 3) < 0) {
        freeServer(server);
        return NULL;
    }
    return server;
}

int main() {
    Server *server = createServer();
    if (!server) {
        perror("Server creation failed");
        return 1;
    }

    printf("[SR] Sender waiting on port %d... (window=%d, frames=%d)\n\n", PORT,
           WINDOW, FRAMES);

    Address client;
    socklen_t len = sizeof(client);
    server->con = accept(server->fd, (struct sockaddr *)&client, &len);
    if (server->con < 0) {
        perror("accept failed");
        freeServer(server);
        return 1;
    }
    printf("[SR] Receiver connected.\n\n");

    char buf[SIZE];
    int acked[FRAMES];
    memset(acked, 0, sizeof(acked));
    int dropped = 0;
    int base = 0;

    while (base < FRAMES) {
        int window_end = base + WINDOW;
        if (window_end > FRAMES)
            window_end = FRAMES;

        printf("[SR] Sender: Sending window [%d - %d]\n", base, window_end - 1);

        for (int seq = base; seq < window_end; seq++) {
            if (acked[seq])
                continue;

            if (seq == DROP_FRAME && !dropped) {
                printf("[SR] Sender: Simulating DROP of frame %d\n", seq);
                snprintf(buf, SIZE, "DROP:%d", seq);
                dropped = 1;
            } else {
                snprintf(buf, SIZE, "FRAME:%d:Data-%d", seq, seq);
                printf("[SR] Sender: Sent frame %d\n", seq);
            }
            send(server->con, buf, strlen(buf), 0);
        }

        snprintf(buf, SIZE, "END_WINDOW:%d", window_end - 1);
        send(server->con, buf, strlen(buf), 0);

        int got_nak = 0;
        int nak_seq = -1;

        for (int i = base; i < window_end; i++) {
            memset(buf, 0, SIZE);
            int bytes = recv(server->con, buf, SIZE, 0);
            if (bytes <= 0)
                break;

            if (strncmp(buf, "ACK", 3) == 0) {
                int seq = atoi(buf + 4);
                printf("[SR] Sender: Received ACK %d\n", seq);
                acked[seq] = 1;
            } else if (strncmp(buf, "NAK", 3) == 0) {
                nak_seq = atoi(buf + 4);
                got_nak = 1;
                printf("[SR] Sender: Received NAK %d → will retransmit ONLY "
                       "frame %d\n",
                       nak_seq, nak_seq);
            }
        }

        if (got_nak) {
            printf("\n[SR] Sender: Retransmitting only frame %d\n", nak_seq);
            snprintf(buf, SIZE, "FRAME:%d:Data-%d", nak_seq, nak_seq);
            send(server->con, buf, strlen(buf), 0);

            snprintf(buf, SIZE, "END_RETRANSMIT:%d", nak_seq);
            send(server->con, buf, strlen(buf), 0);

            memset(buf, 0, SIZE);
            recv(server->con, buf, SIZE, 0);
            if (strncmp(buf, "ACK", 3) == 0) {
                int seq = atoi(buf + 4);
                printf("[SR] Sender: Received ACK %d (retransmit)\n", seq);
                acked[seq] = 1;
            }
        }

        while (base < FRAMES && acked[base])
            base++;
        printf("[SR] Sender: Window base advanced to %d\n\n", base);
    }

    printf("[SR] Sender: All %d frames acknowledged. Done.\n", FRAMES);
    freeServer(server);
    return 0;
}
