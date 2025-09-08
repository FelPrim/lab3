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
    LOG(INFO, "MODE: {}", mode);
    switch(mode){
        case NetworkMode::SERVER:
        {
            Server server;
            server.run();
            break;
        }
        case NetworkMode::CLIENT:
        {   
            Client client;
            client.run();
            break;
        }
        case NetworkMode::CLIENTANDSERVER:
        {
            ServerAndClient both;
            both.run();
            break;
        }
    }
}

void Server::run(){
    // пока без ui_stuff
    port_input:
    PORT = 0;
    std::println("Input port number: ");
    std::cin >> PORT;
    if (PORT > 49151 or PORT < 1024){
        std::println("Invalid port number. Port must be in ({}, {})", 1024, 49151);
        goto port_input;
    }
     = {
        .ai_family = AF_INET,
        .ai_flags = AI_PASSIVE,
        .ai_protocol = IPPROTO_UDP
    };
    struct addrinfo* servinfo;
    int status = getaddrinfo(IP_STR, PORT_STR, &hints, &servinfo);
    switch (status){
        case 0:
            print_addrinfo_list(servinfo);
            std::println("Print 'y' if you want to change port. Press any other key to continue.");
            // дать возможность покинуть программу
            char confirm;
            std::cin>>confirm;
            if (confirm == 'y')
                goto port_input;
            break;
        case 1:
            LOG(ERR, "Temporary DNS failure. Try again");
            goto port_input;
        default:
            LOG(ERR, "getaddrinfo error: {}", gai_strerror(status));
            goto port_input;

    }

    xsocket Listen;
    struct addrinfo* result;
    result = servinfo;
    for (; result != NULL; result = result->ai_next){
        if ((Listen = socket(
            result->ai_family,
            result->ai_socktype,
            result->ai_protocol
            )) == -1
           ){
            
            LOG(ERR, "socket");
            continue;
        }

        if (xaddress_in_use(&Listen)){
            LOG(ERR, "setsockopt");
            continue;
        }

        if (bind(Listen, result->ai_addr, result->ai_addrlen) == -1){
            xclosesocket(Listen);
            LOG(ERR, "bind");
            continue;
        }

        LOG(INFO, "Binded socket woth following address information:");
        print_addrinfo(result);
        break;
    }

    if (result == NULL){
        LOG(ERR, "Failed to bind socket");
        exit(3);
    }


}

void Client::run(){
    
}

void ServerAndClient::run(){
    
}

