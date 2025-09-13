#pragma once

#include <string_view>
#include <format>
#include <vector>

// https://beej.us/guide/bgnet/html/

/*
Сама идея:
Сервер использует 2 сокета: TCP и UDP
По TCP происходит "handshake" и настройка шифрования
После этого идёт подсоединение к UDP


*/

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

const size_t BFR_SZ = 1024;

#ifdef _WIN32
typedef SOCKET xsocket;
#else
typedef int xsocket;
#endif

#define FQDN_SZ 256

class NetworkStuff{
public:
    explicit NetworkStuff() = default;
    void Init(NetworkMode mode);
    ~NetworkStuff();
    void run();
};

