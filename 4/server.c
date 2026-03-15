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
} Server;

void freeUDPServer(Server *server) {
    if (server->fd >= 0) {
        close(server->fd);
    }
    free(server);
}

Server *createUDPServer() {
    Server *server = (Server *)malloc(sizeof(Server));
    memset(&server->address, 0, sizeof(server->address));

    server->fd = socket(AF_INET, SOCK_DGRAM, 0);
    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(PORT);

    if (server->fd < 0) {
        freeUDPServer(server);
        return NULL;
    }

    if (bind(server->fd, (struct sockaddr *)&server->address,
             sizeof(server->address)) < 0) {
        freeUDPServer(server);
        return NULL;
    }

    return server;
}

int main() {
    Server *server = createUDPServer();
    if (!server) {
        perror("Server creation failed");
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    char buffer[SIZE] = {0};
    Address client;
    socklen_t len = sizeof(client);

    int bytes =
        recvfrom(server->fd, buffer, SIZE, 0, (struct sockaddr *)&client, &len);
    if (bytes < 0) {
        perror("recvfrom failed");
        freeUDPServer(server);
        return 1;
    }

    printf("Client: %s\n", buffer);
    const char *msg = "Hello from server";
    sendto(server->fd, msg, strlen(msg), 0, (struct sockaddr *)&client, len);
    printf("Reply sent to client\n");

    freeUDPServer(server);
    return 0;
}
