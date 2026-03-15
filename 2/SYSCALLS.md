# Linux Network Programming System Calls

## Overview

<br>

Network system calls are the interface through which user-space programs communicate with the Linux kernel to perform networking operations. Linux supports TCP/IP as its native network transport, and most network operations are based on the **BSD sockets interface**. Sockets allow processes on different computers — or even on the same host — to exchange data through a network.

A **system call** is a programmatic request by a user-space application to ask the Linux kernel to perform tasks that require higher privileges, such as creating network sockets, binding to ports, or sending/receiving data. The CPU switches from user mode to kernel mode, the kernel executes the required function, and control returns to the application with the result.

<br>

---

## Socket Lifecycle (TCP Server vs Client)

```
SERVER                          CLIENT
  |                               |
socket()                        socket()
  |                               |
bind()                            |
  |                               |
listen()                          |
  |                               |
accept() <───────────────────> connect() 
  |                               |
send()/recv()  <──────────>  send()/recv()
  |                               |
close()                         close()
```

<br>

---

## 1. `socket()` — Create a Socket

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
int socket(int domain, int type, int protocol);
```

### Parameters

| Parameter  | Description |
|------------|-------------|
| `domain`   | Communication domain (address family). Common values: `AF_INET` (IPv4), `AF_INET6` (IPv6), `AF_UNIX` (local IPC). |
| `type`     | Socket type. Common values: `SOCK_STREAM` (TCP), `SOCK_DGRAM` (UDP), `SOCK_RAW` (raw packets). |
| `protocol` | Protocol to use. Usually `0` to auto-select based on domain and type. Use `IPPROTO_TCP` or `IPPROTO_UDP` explicitly if needed. |

### Return Value
- On **success**: a non-negative integer file descriptor.
- On **error**: `-1`, and `errno` is set appropriately.

### Description

`socket()` creates a new socket and returns a file descriptor that refers to it. The socket itself has no address assigned to it at this stage. It is the starting point for all network communication — both for clients and servers.

### Example
```c
int sockfd = socket(AF_INET, SOCK_STREAM, 0);
if (sockfd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
}
```

<br>

---

## 2. `bind()` — Assign an Address to a Socket

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | File descriptor of the socket (from `socket()`). |
| `addr`    | Pointer to a `sockaddr` structure containing the local address and port to bind to. For IPv4, cast a `struct sockaddr_in *` to `struct sockaddr *`. |
| `addrlen` | Size of the address structure in bytes (e.g., `sizeof(struct sockaddr_in)`). |

### Return Value
- On **success**: `0`.
- On **error**: `-1`, and `errno` is set.

### Description

`bind()` assigns a local IP address and port number to a socket. It is primarily used by servers to declare which address and port they will listen on. Clients may optionally call `bind()` to choose a specific local port, but usually leave this to the OS.

### The `sockaddr_in` Structure (IPv4)
```c
struct sockaddr_in {
    sa_family_t    sin_family;  // AF_INET
    in_port_t      sin_port;    // Port number (network byte order)
    struct in_addr sin_addr;    // IP address
};
```

### Example
```c
struct sockaddr_in server_addr;
memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family      = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;      // Bind to all interfaces
server_addr.sin_port        = htons(8080);     // Port 8080

if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
}
```

<br>

---

## 3. `listen()` — Mark Socket as Passive (Server-side)

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
int listen(int sockfd, int backlog);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | File descriptor of a bound socket. |
| `backlog` | Maximum length of the queue for pending connections. If the queue is full, new connections may be refused. Typical value: `5` to `128`. |

### Return Value
- On **success**: `0`.
- On **error**: `-1`, and `errno` is set.

### Description

`listen()` marks a socket as a **passive socket** — one that will be used to accept incoming connections. It is used only by **TCP servers**, after `bind()`. It does not block; it just enables the socket to accept connections, maintaining a queue of pending clients.

### Example
```c
if (listen(sockfd, 10) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
}
```

<br>

---

## 4. `accept()` — Accept an Incoming Connection

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | The listening socket file descriptor. |
| `addr`    | Pointer to a `sockaddr` structure that will be filled with the connecting client's address. Pass `NULL` if you don't need this info. |
| `addrlen` | Pointer to the size of `addr`. Must be initialized to the buffer size before calling; will be updated to actual size. |

### Return Value
- On **success**: a new non-negative file descriptor for the accepted connection (a separate socket for this client).
- On **error**: `-1`, and `errno` is set.

### Description

`accept()` **blocks** until a client connects. When a client connects, it extracts the first connection on the queue, creates a new connected socket, and returns its file descriptor. The original `sockfd` continues listening for more connections. This new descriptor is what you use to communicate with that specific client.

### Example
```c
struct sockaddr_in client_addr;
socklen_t addrlen = sizeof(client_addr);

int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);
if (client_fd < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
}
printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
```

<br>

---

## 5. `connect()` — Connect to a Remote Socket (Client-side)

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | File descriptor of the client socket. |
| `addr`    | Pointer to a `sockaddr` structure holding the server's IP address and port. |
| `addrlen` | Size of the address structure. |

### Return Value
- On **success**: `0`.
- On **error**: `-1`, and `errno` is set.

### Description

`connect()` is used by a **client** to establish a connection to a server. For TCP (`SOCK_STREAM`), it initiates the three-way handshake. For UDP (`SOCK_DGRAM`), it just sets the default peer address for `send()`/`recv()` — no actual connection is made.

### Example
```c
struct sockaddr_in server_addr;
memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family = AF_INET;
server_addr.sin_port   = htons(8080);
inet_pton(AF_INET, "192.168.1.1", &server_addr.sin_addr);

if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("connect");
    exit(EXIT_FAILURE);
}
```

<br>

---

## 6. `send()` — Send Data on a Connected Socket

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | File descriptor of a connected socket. |
| `buf`     | Pointer to the data buffer to send. |
| `len`     | Number of bytes to send. |
| `flags`   | Control flags. `0` for default behavior. Common flags: `MSG_DONTWAIT` (non-blocking), `MSG_NOSIGNAL` (suppress SIGPIPE). |

### Return Value
- On **success**: number of bytes actually sent (may be less than `len`).
- On **error**: `-1`, and `errno` is set.

### Description

`send()` is used to transmit data on a **connected** socket (TCP). It may not send all bytes at once; always check the return value and loop if necessary. It is equivalent to `write()` when `flags` is `0`, but provides more control via flags.

### Example
```c
const char *message = "Hello, Server!";
ssize_t bytes_sent = send(sockfd, message, strlen(message), 0);
if (bytes_sent < 0) {
    perror("send");
}
```

<br>

---

## 7. `recv()` — Receive Data from a Connected Socket

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | File descriptor of the connected socket. |
| `buf`     | Buffer where received data will be stored. |
| `len`     | Maximum number of bytes to receive. |
| `flags`   | Control flags. `0` for default. Common flags: `MSG_PEEK` (peek without removing from queue), `MSG_WAITALL` (wait until full buffer is received). |

### Return Value
- On **success**: number of bytes received. `0` means the connection was closed by the peer.
- On **error**: `-1`, and `errno` is set.

### Description

`recv()` reads incoming data from a connected socket into a buffer. It blocks by default until data is available. A return value of `0` indicates the peer has closed the connection — an important condition to check for in all network programs.

### Example
```c
char buffer[1024];
ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
if (bytes_received < 0) {
    perror("recv");
} else if (bytes_received == 0) {
    printf("Client disconnected.\n");
} else {
    buffer[bytes_received] = '\0';
    printf("Received: %s\n", buffer);
}
```

<br>

---

## 8. `sendto()` — Send Data to a Specific Address (UDP)

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
```

### Parameters

| Parameter   | Description |
|-------------|-------------|
| `sockfd`    | UDP socket file descriptor. |
| `buf`       | Data buffer to send. |
| `len`       | Length of the data. |
| `flags`     | Control flags (usually `0`). |
| `dest_addr` | Pointer to the destination address structure. |
| `addrlen`   | Size of the destination address structure. |

### Return Value
- On **success**: number of bytes sent.
- On **error**: `-1`, and `errno` is set.

### Description

`sendto()` is used for **connectionless** (UDP) sockets. Unlike `send()`, the destination address is specified in each call since UDP has no established connection. It can also be used on TCP sockets if the `dest_addr` is `NULL` (equivalent to `send()`).

### Example
```c
struct sockaddr_in dest;
dest.sin_family = AF_INET;
dest.sin_port   = htons(9090);
inet_pton(AF_INET, "192.168.1.2", &dest.sin_addr);

sendto(sockfd, "ping", 4, 0, (struct sockaddr *)&dest, sizeof(dest));
```

<br>

---

## 9. `recvfrom()` — Receive Data and Capture Sender's Address (UDP)

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
```

### Parameters

| Parameter  | Description |
|------------|-------------|
| `sockfd`   | UDP socket file descriptor. |
| `buf`      | Buffer to receive data into. |
| `len`      | Maximum bytes to read. |
| `flags`    | Control flags (usually `0`). |
| `src_addr` | Filled with the sender's address upon return. Pass `NULL` to ignore. |
| `addrlen`  | Pointer to size of `src_addr`; updated on return. |

### Return Value
- On **success**: number of bytes received.
- On **error**: `-1`, and `errno` is set.

### Description

`recvfrom()` is the UDP counterpart to `recv()`. It receives a datagram and additionally captures the **sender's address** in `src_addr`. This is essential in UDP servers that need to know who sent the data so they can respond.

### Example
```c
struct sockaddr_in sender;
socklen_t sender_len = sizeof(sender);
char buf[512];

recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&sender, &sender_len);
printf("Received from %s\n", inet_ntoa(sender.sin_addr));
```

<br>

---

## 10. `setsockopt()` — Set Socket Options

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
int setsockopt(int sockfd, int level, int optname,
               const void *optval, socklen_t optlen);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | The socket file descriptor. |
| `level`   | Protocol level at which the option resides. Use `SOL_SOCKET` for socket-level options, `IPPROTO_TCP` for TCP options. |
| `optname` | The option name (e.g., `SO_REUSEADDR`, `SO_KEEPALIVE`, `SO_RCVBUF`). |
| `optval`  | Pointer to the value to set. |
| `optlen`  | Size of `optval`. |

### Return Value
- On **success**: `0`.
- On **error**: `-1`, and `errno` is set.

### Description

`setsockopt()` controls the behavior of a socket. It is used to fine-tune socket behavior, such as allowing address reuse, setting buffer sizes, enabling keep-alive, setting timeouts, etc.

### Common Options

| Option         | Level        | Effect |
|----------------|-------------|--------|
| `SO_REUSEADDR` | `SOL_SOCKET` | Allow reuse of local addresses (avoids "address already in use" error). |
| `SO_KEEPALIVE` | `SOL_SOCKET` | Enable periodic keep-alive probes on TCP connections. |
| `SO_RCVBUF`    | `SOL_SOCKET` | Set receive buffer size. |
| `SO_SNDBUF`    | `SOL_SOCKET` | Set send buffer size. |
| `SO_LINGER`    | `SOL_SOCKET` | Control close behavior when data is pending. |
| `TCP_NODELAY`  | `IPPROTO_TCP` | Disable Nagle's algorithm for lower latency. |

### Example
```c
int opt = 1;
// Allow address reuse — essential for servers to restart quickly
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

<br>

---

## 11. `getsockopt()` — Get Socket Options

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
int getsockopt(int sockfd, int level, int optname,
               void *optval, socklen_t *optlen);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | The socket file descriptor. |
| `level`   | Protocol level (e.g., `SOL_SOCKET`, `IPPROTO_TCP`). |
| `optname` | The option to retrieve. |
| `optval`  | Buffer to store the retrieved option value. |
| `optlen`  | Pointer to the size of `optval`; updated on return to actual size. |

### Return Value
- On **success**: `0`.
- On **error**: `-1`, and `errno` is set.

### Description

`getsockopt()` retrieves the current value of a socket option. It is the read counterpart to `setsockopt()`. Useful for querying socket state, checking buffer sizes, or reading error conditions.

### Example
```c
int optval;
socklen_t optlen = sizeof(optval);
getsockopt(sockfd, SOL_SOCKET, SO_TYPE, &optval, &optlen);
printf("Socket type: %d\n", optval);  // e.g., 1 = SOCK_STREAM
```

<br>

---

## 12. `getsockname()` — Get Local Socket Address

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | The socket file descriptor. |
| `addr`    | Buffer filled with the local address bound to the socket. |
| `addrlen` | Pointer to size of `addr`; updated to actual size on return. |

### Return Value
- On **success**: `0`.
- On **error**: `-1`, and `errno` is set.

### Description

`getsockname()` returns the current local address to which `sockfd` is bound. Useful when you bind to port `0` (letting the OS choose a port) and need to find out which port was assigned.

### Example
```c
struct sockaddr_in local_addr;
socklen_t len = sizeof(local_addr);
getsockname(sockfd, (struct sockaddr *)&local_addr, &len);
printf("Local port: %d\n", ntohs(local_addr.sin_port));
```

<br>

---

## 13. `getpeername()` — Get Remote Peer's Address

### Header
```c
#include <sys/types.h>
#include <sys/socket.h>
```

### Function Signature
```c
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | A connected socket file descriptor. |
| `addr`    | Buffer filled with the remote peer's address. |
| `addrlen` | Pointer to size of `addr`; updated on return. |

### Return Value
- On **success**: `0`.
- On **error**: `-1`, and `errno` is set.

### Description

`getpeername()` returns the address of the remote peer connected to `sockfd`. It can be called after `accept()` (server) or `connect()` (client) to find out who is on the other end of the connection.

### Example
```c
struct sockaddr_in peer_addr;
socklen_t len = sizeof(peer_addr);
getpeername(client_fd, (struct sockaddr *)&peer_addr, &len);
printf("Peer IP: %s, Port: %d\n",
       inet_ntoa(peer_addr.sin_addr),
       ntohs(peer_addr.sin_port));
```

<br>

---

## 14. `shutdown()` — Partially or Fully Close a Connection

### Header
```c
#include <sys/socket.h>
```

### Function Signature
```c
int shutdown(int sockfd, int how);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `sockfd`  | The connected socket. |
| `how`     | Specifies what to shut down: `SHUT_RD` (stop receiving), `SHUT_WR` (stop sending), `SHUT_RDWR` (stop both). |

### Return Value
- On **success**: `0`.
- On **error**: `-1`, and `errno` is set.

### Description

`shutdown()` disables part or all of a duplex connection without closing the file descriptor. Unlike `close()`, it allows fine-grained control — for example, signaling the peer that no more data will be sent (half-close) while still being able to receive.

### Example
```c
// Signal end of sending but keep receiving open
shutdown(sockfd, SHUT_WR);
```

<br>

---

## 15. `close()` — Close a Socket

### Header
```c
#include <unistd.h>
```

### Function Signature
```c
int close(int fd);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `fd`      | File descriptor of the socket to close. |

### Return Value
- On **success**: `0`.
- On **error**: `-1`, and `errno` is set.

### Description

`close()` releases the socket file descriptor and associated resources. For TCP, if this is the last reference to the socket, the connection teardown (FIN sequence) is initiated. Always close sockets when done to avoid resource leaks (file descriptor exhaustion).

### Example
```c
close(client_fd);
close(sockfd);
```

<br>

---

## 16. `select()` — Monitor Multiple File Descriptors

### Header
```c
#include <sys/select.h>
```

### Function Signature
```c
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
```

### Parameters

| Parameter   | Description |
|-------------|-------------|
| `nfds`      | Highest file descriptor + 1 to monitor. |
| `readfds`   | Set of fds to watch for readability. |
| `writefds`  | Set of fds to watch for writability. |
| `exceptfds` | Set of fds to watch for exceptional conditions. |
| `timeout`   | Maximum time to wait. `NULL` = block indefinitely. `{0,0}` = non-blocking poll. |

### Return Value
- **Positive**: number of ready file descriptors.
- **`0`**: timeout expired.
- **`-1`**: error, and `errno` is set.

### Description

`select()` allows a program to monitor multiple file descriptors simultaneously, waiting until one or more are ready for I/O. This is essential for building servers that handle multiple clients without multi-threading. The `fd_set` is manipulated using macros: `FD_ZERO`, `FD_SET`, `FD_CLR`, `FD_ISSET`.

### Example
```c
fd_set readfds;
FD_ZERO(&readfds);
FD_SET(sockfd, &readfds);

struct timeval tv = {5, 0}; // 5-second timeout
int ret = select(sockfd + 1, &readfds, NULL, NULL, &tv);
if (ret > 0 && FD_ISSET(sockfd, &readfds)) {
    // Socket is ready to read
}
```

<br>

---

## 17. `poll()` — Wait for Events on File Descriptors

### Header
```c
#include <poll.h>
```

### Function Signature
```c
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `fds`     | Array of `pollfd` structures, each specifying a file descriptor and events to watch. |
| `nfds`    | Number of items in the `fds` array. |
| `timeout` | Timeout in milliseconds. `-1` = block indefinitely, `0` = return immediately. |

### The `pollfd` Structure
```c
struct pollfd {
    int   fd;       // File descriptor to watch
    short events;   // Events to watch for (POLLIN, POLLOUT, POLLERR)
    short revents;  // Events that actually occurred (set by kernel)
};
```

### Return Value
- **Positive**: number of fds with returned events.
- **`0`**: timeout.
- **`-1`**: error, and `errno` is set.

### Description

`poll()` is a more flexible alternative to `select()`. It does not have the 1024 fd limit of `select()` (by default) and uses a cleaner API. Each `pollfd` structure in the array specifies which events to monitor, and the kernel fills in `revents` with what actually happened.

### Example
```c
struct pollfd fds[2];
fds[0].fd     = server_fd;
fds[0].events = POLLIN;   // Watch for incoming connections

fds[1].fd     = client_fd;
fds[1].events = POLLIN;   // Watch for incoming data

int ret = poll(fds, 2, 3000);  // 3-second timeout
if (fds[0].revents & POLLIN) {
    // New connection on server_fd
}
```

<br>

---

## Summary Table

| System Call     | Use Case                                          | TCP | UDP |
|-----------------|---------------------------------------------------|-----|-----|
| `socket()`      | Create a socket endpoint                          | ✅  | ✅  |
| `bind()`        | Assign local address/port                         | ✅  | ✅  |
| `listen()`      | Mark socket as passive (server)                   | ✅  | ❌  |
| `accept()`      | Accept incoming connection (blocks until client)  | ✅  | ❌  |
| `connect()`     | Connect to a remote server                        | ✅  | ✅* |
| `send()`        | Send data over connected socket                   | ✅  | ✅* |
| `recv()`        | Receive data from connected socket                | ✅  | ✅* |
| `sendto()`      | Send datagram to a specific address               | ❌  | ✅  |
| `recvfrom()`    | Receive datagram and get sender's address         | ❌  | ✅  |
| `setsockopt()`  | Set socket options (reuse, keepalive, etc.)       | ✅  | ✅  |
| `getsockopt()`  | Read socket options                               | ✅  | ✅  |
| `getsockname()` | Get local socket's address                        | ✅  | ✅  |
| `getpeername()` | Get remote peer's address                         | ✅  | ❌  |
| `shutdown()`    | Half-close or fully close send/receive            | ✅  | ❌  |
| `close()`       | Release socket and free resources                 | ✅  | ✅  |
| `select()`      | Monitor multiple fds (portable, limited to 1024)  | ✅  | ✅  |
| `poll()`        | Monitor multiple fds (more scalable than select)  | ✅  | ✅  |

> \* UDP with `connect()` sets a default peer; subsequent `send()`/`recv()` use that peer.

<br>

---

## Byte Order Helper Functions

Network protocols use **big-endian** (network byte order). Host machines may be little-endian. Always convert before putting values on the wire:

```c
#include <arpa/inet.h>

uint16_t htons(uint16_t hostshort);   // Host to network (short/port)
uint16_t ntohs(uint16_t netshort);    // Network to host (short/port)
uint32_t htonl(uint32_t hostlong);    // Host to network (long/IP)
uint32_t ntohl(uint32_t netlong);     // Network to host (long/IP)

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
int inet_pton(int af, const char *src, void *dst);
```

<br>

---
