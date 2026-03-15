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
#define FRAMES 6
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

    printf("[SAW] Sender waiting on port %d...\n", PORT);

    Address client;
    socklen_t len = sizeof(client);
    server->con = accept(server->fd, (struct sockaddr *)&client, &len);
    if (server->con < 0) {
        perror("accept failed");
        freeServer(server);
        return 1;
    }
    printf("[SAW] Receiver connected.\n\n");

    char buf[SIZE];
    int dropped = 0;

    for (int seq = 0; seq < FRAMES;) {

        if (seq == DROP_FRAME && !dropped) {
            printf("[SAW] Sender: Simulating DROP of frame %d (not sending)\n",
                   seq);
            dropped = 1;

            snprintf(buf, SIZE, "DROP:%d", seq);
            send(server->con, buf, strlen(buf), 0);
        } else {
            snprintf(buf, SIZE, "FRAME:%d:Data-%d", seq, seq);
            send(server->con, buf, strlen(buf), 0);
            printf("[SAW] Sender: Sent frame %d\n", seq);
        }

        memset(buf, 0, SIZE);
        int bytes = recv(server->con, buf, SIZE, 0);
        if (bytes <= 0)
            break;

        if (strncmp(buf, "ACK", 3) == 0) {
            int ack_seq = atoi(buf + 4);
            printf("[SAW] Sender: Received ACK %d → moving to next frame\n\n",
                   ack_seq);
            seq++;
        } else if (strncmp(buf, "NAK", 3) == 0) {
            int nak_seq = atoi(buf + 4);
            printf(
                "[SAW] Sender: Received NAK %d → retransmitting frame %d\n\n",
                nak_seq, nak_seq);
        }
    }

    printf("[SAW] Sender: All %d frames acknowledged. Done.\n", FRAMES);
    freeServer(server);
    return 0;
}
