#include "network_stuff.h"
#include "useful_stuff.h"

#include <cstdio>
#include "useful_stuff.h"
#include <print>
#include <iostream>

extern LogLvl LOGLVL;

#ifdef _WIN32
inline bool xaddress_in_use(xsocket sockfd){
    char yes = 1;
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == SOCKET_ERROR;
}
#else
inline bool xaddress_in_use(xsocket sockfd){
    int yes = 1;
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1;
}
#endif

void NetworkStuff::Init(NetworkMode mode){
    this->mode = mode;
#ifdef _WIN32
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0){
        // можно было б бросить исключение, но пока обойдусь такой затычкой
        LOG(ERR, "WSAStartup failed");
        exit(1);
    }

    if (LOBYTE(wsaData.wVersion) != 2 ||
        HIBYTE(wsaData.wVersion) != 2){
        
        LOG(ERR, "Version 2.2 of Winsock not available");
        WSACleanup();
        exit(2);
    }
#endif
    LOG(TRACE, "Network startup: SUCCESS");
}

NetworkStuff::~NetworkStuff(void){
#ifdef _WIN32
    WSACleanup();
#endif
    LOG(TRACE, "Network cleanup: SUCCESS");
}

void NetworkStuff::run(){
}


