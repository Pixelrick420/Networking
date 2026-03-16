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

typedef struct sockaddr_in Address;
typedef struct Server {
    Address address;
    int fd;
    int con;
} Server;

void freeTCPServer(Server *server) {
    if (server->con >= 0) {
        close(server->con);
    }
    if (server->fd >= 0) {
        close(server->fd);
    }
    free(server);
}

Server *createTCPServer() {
    Server *server = (Server *)malloc(sizeof(Server));
    memset(&server->address, 0, sizeof(server->address));
    server->con = -1;

    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(PORT);

    if (server->fd < 0) {
        freeTCPServer(server);
        return NULL;
    }

    if (bind(server->fd, (struct sockaddr *)&server->address,
             sizeof(server->address)) < 0) {
        freeTCPServer(server);
        return NULL;
    }

    if (listen(server->fd, 3) < 0) {
        freeTCPServer(server);
        return NULL;
    }

    return server;
}

void sendToClient(Server *server, char *msg) {
    send(server->con, msg, strlen(msg), 0);
    printf("sent response: %s\n", msg);
}

void recvFromClient(Server *server, char *buffer) {
    memset(buffer, 0, SIZE);
    int received = recv(server->con, buffer, SIZE - 2, 0);
    buffer[received] = '\0';
    printf("\n\nclient: %s\n", buffer);
}

int main() {
    Server *server = createTCPServer();
    if (!server) {
        perror("Server creation failed");
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    char buffer[SIZE] = {0};
    Address client;
    socklen_t len = sizeof(client);

    server->con = accept(server->fd, (struct sockaddr *)&client, &len);
    if (server->con < 0) {
        perror("accept failed");
        freeTCPServer(server);
        return 1;
    }

    printf("Client connected\n");

    recvFromClient(server, buffer);
    sendToClient(server, "Opened file");
    FILE *fp = fopen(buffer, "w");
    int n;

    while ((n = recv(server->con, buffer, SIZE, 0)) > 0) {
        fwrite(buffer, 1, n, fp);
    }

    freeTCPServer(server);
    return 0;
}
