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
enum class NetworkMode{SERVER, CLIENT, CLIENTANDSERVER};

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
private:
    NetworkMode mode = NetworkMode::CLIENT;
};

class Server{
public:
    explicit Server() = default;
    ~Server() = default;
    void run();
private:
    char TCPPORT[6];
    char UDPPORT[6];

};

class Client{
public:
    explicit Client(void) = default;
    ~Client() = default;
    void run();
private:
    
};

class ServerAndClient{
public:
    explicit ServerAndClient(void) = default;
    ~ServerAndClient() = default;
    void run();
private:
};

template<>
struct std::formatter<NetworkMode>: std::formatter<std::string_view> {
    auto format(NetworkMode mode, std::format_context& ctx) const {
        std::string_view str;
        switch (mode) {
            case NetworkMode::CLIENT:  str = "CLIENT"; break;
            case NetworkMode::SERVER:  str = "SERVER"; break;
            case NetworkMode::CLIENTANDSERVER: str = "CLIENTANDSERVER"; break;
            default:                   str = "UNKNOWN"; break;
        }
        return std::formatter<std::string_view>::format(str, ctx);
    }
};
