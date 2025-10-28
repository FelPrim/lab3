#include "crossplatform_networking.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
static int ws_initialized = 0;
#endif

void csetup(void) {
    // Инициализация обработки сигналов
    csignal_handle_init();

#ifdef _WIN32
    // Инициализация Winsock
    if (!ws_initialized) {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            exit(1);
        }

        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            WSACleanup();
            exit(2);
        }
        ws_initialized = 1;
    }
#endif
}

void cfree(void) {
    // Восстановление обработчиков сигналов
    csignal_handle_cleanup();

#ifdef _WIN32
    // Очистка Winsock
    if (ws_initialized) {
        WSACleanup();
        ws_initialized = 0;
    }
#endif
}

// Сетевые функции-заглушки
struct csockaddr* cgetaddrinfo(const char* node, const char* service, int params) {
    (void)params;
    struct csockaddr* result = malloc(sizeof(struct csockaddr));
    if (!result) return NULL;
    
    memset(&result->addr, 0, sizeof(result->addr));
    result->addr.sin_family = AF_INET;
    
    if (node && strcmp(node, "localhost") == 0) {
        result->addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else {
        result->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    
    if (service) {
        result->addr.sin_port = htons(atoi(service));
    }
    
    return result;
}

void cfreeaddrinfo(struct csockaddr* addr) {
    free(addr);
}

csocket cgetsocket(const struct csockaddr* info) {
    (void)info;
#ifdef _WIN32
    return socket(AF_INET, SOCK_STREAM, 0);
#else
    return socket(AF_INET, SOCK_STREAM, 0);
#endif
}

int cbind(csocket sockfd, const struct csockaddr* info) {
    if (!info) return CSOCKET_ERROR;
    return bind(sockfd, (struct sockaddr*)&info->addr, sizeof(info->addr));
}

int creuseaddr(csocket sockfd) {
    int reuse = 1;
#ifdef _WIN32
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
#else
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif
}

int cconnect(csocket sockfd, const struct csockaddr* info) {
    if (!info) return CSOCKET_ERROR;
    return connect(sockfd, (struct sockaddr*)&info->addr, sizeof(info->addr));
}

int clisten(csocket sockfd, int backlog) {
    return listen(sockfd, backlog);
}

csocket caccept(csocket sockfd, struct csockaddr* info) {
    if (!info) return CSOCKET_INVALID;
    socklen_t addrlen = sizeof(info->addr);
#ifdef _WIN32
    return accept(sockfd, (struct sockaddr*)&info->addr, &addrlen);
#else
    return accept(sockfd, (struct sockaddr*)&info->addr, &addrlen);
#endif
}

int csend(csocket sockfd, const void* msg, int len) {
#ifdef _WIN32
    return send(sockfd, (const char*)msg, len, DEFAULT_SENDFLAGS);
#else
    return send(sockfd, msg, len, DEFAULT_SENDFLAGS);
#endif
}

int crecv(csocket sockfd, void* buf, int len) {
#ifdef _WIN32
    return recv(sockfd, (char*)buf, len, DEFAULT_RECVFLAGS);
#else
    return recv(sockfd, buf, len, DEFAULT_RECVFLAGS);
#endif
}

int csendto(csocket sockfd, const void* msg, int len, const struct csockaddr* to) {
    if (!to) return CSOCKET_ERROR;
#ifdef _WIN32
    return sendto(sockfd, (const char*)msg, len, DEFAULT_SENDTOFLAGS, 
                 (const struct sockaddr*)&to->addr, sizeof(to->addr));
#else
    return sendto(sockfd, msg, len, DEFAULT_SENDTOFLAGS,
                 (const struct sockaddr*)&to->addr, sizeof(to->addr));
#endif
}

int crecvfrom(csocket sockfd, void* buf, int len, struct csockaddr* from) {
    if (!from) return CSOCKET_ERROR;
    socklen_t addrlen = sizeof(from->addr);
#ifdef _WIN32
    return recvfrom(sockfd, (char*)buf, len, DEFAULT_RECVFROMFLAGS,
                   (struct sockaddr*)&from->addr, &addrlen);
#else
    return recvfrom(sockfd, buf, len, DEFAULT_RECVFROMFLAGS,
                   (struct sockaddr*)&from->addr, &addrlen);
#endif
}