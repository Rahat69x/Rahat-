/**
 * src/net/net_compat.h
 * Cross-platform network socket compatibility layer (F8, C2).
 * Supports BSD sockets on POSIX and Winsock2 on Windows.
 */

#ifndef NET_COMPAT_H
#define NET_COMPAT_H

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define INVALID_SOCKET_VAL INVALID_SOCKET
#define SOCKET_ERR_VAL SOCKET_ERROR
#define CLOSESOCKET(s) closesocket(s)

static inline int net_init(void) {
    WSADATA wsa;
    return (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) ? 0 : -1;
}

static inline void net_cleanup(void) {
    WSACleanup();
}

static inline int net_set_nonblocking(socket_t s) {
    u_long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode);
}

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
typedef int socket_t;
#define INVALID_SOCKET_VAL (-1)
#define SOCKET_ERR_VAL (-1)
#define CLOSESOCKET(s) close(s)

static inline int net_init(void) {
    signal(SIGPIPE, SIG_IGN);
    return 0;
}

static inline void net_cleanup(void) {}

static inline int net_set_nonblocking(socket_t s) {
    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(s, F_SETFL, flags | O_NONBLOCK);
}
#endif

#endif /* NET_COMPAT_H */
