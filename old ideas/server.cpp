#include "useful_stuff.h"

#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <print>

extern LogLvl LOGLVL;

struct Application{
    explicit Application(int argc, char * argv[]);
    ~Application();
    void run();

};


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

int main(int argc, char* argv[]){
    
    return 0;
}
