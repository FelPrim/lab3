#pragma once
// оборачивает функции взаимодействия с ОС таким образом, что в client.c остаётся только логика

#ifdef __cplusplus
extern "C" {
#endif

// includes
#ifdef _WIN32
// windows.h инклудит winsock.h, а я хочу иметь возможность использовать новейший winsock2 (1994)
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <sys/epoll.h>
    #include <openssl/ssl.h>
    #include <openssl/err.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    typedef SOCKET xsocket;
    #define xclose closesocket
#else
    typedef int xsocket;
    #define xclose close
#endif

void network_start(){
#ifdef _WIN32
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
        perror("WSAStartup");
        exit(1);
    }

    if (LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2){
        
        perror("Version 2.2 of Winsock not available");
        WSACleanup();
        exit(2);
    }
#endif
}

void network_finish(){
#ifdef _WIN32
    WSACleanup();
#endif
}

bool address_in_use(xsocket sockfd){
#ifdef _WIN32
   char yes = 1;
   return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == SOCKET_ERROR;
#else
   int yes = 1;
   return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1;
#endif
}

bool make_nonblocking(xsocket sockfd){
#ifdef _WIN32
    unsigned long ul = 1;
    return ioctlsocket(sockfd, FIONBIO, &ul) == SOCKET_ERROR;
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0)
        return false;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0;
#endif
}

bool recv_errorhandling(){
#ifdef _WIN32
    int err = WSAGetLastError();
    return err == WSAEINTR || err == WSAEWOULDBLOCK;
#else
    return errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}


#ifdef __cplusplus
}
#endif
