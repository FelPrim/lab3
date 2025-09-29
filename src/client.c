#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "network_stuff.h"
#define SITE "marrs73.ru"
#define TPORTSTR "23230"
#define BUFSZ 1024

int main(int argc, char *argv[]){
    network_start();
    
    xsocket tfd;

    {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    getaddrinfo(SITE, TPORTSTR, &hints, &result);

    if ((tfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) < 0){
        freeaddrinfo(result);
        perror("TCP socket");
        return 1;
    }

    if (address_in_use(tfd)){
        freeaddrinfo(result);
        perror("TCP address_in_use");
        xclose(tfd);
        return 2;
    }

    //if (bind(tfd, result->ai_addr, result->ai_addrlen) < 0){
    //    freeaddrinfo(result);
    //    perror("TCP bind");
    //    xclose(tfd);
    //    return 3;
    //}
    
    //if (make_nonblocking(tfd)){
    //    freeaddrinfo(result);
    //    perror("TCP make_nonblocking");
    //    xclose(tfd);
    //    return 4;
    //}

    if (connect(tfd, result->ai_addr, result->ai_addrlen) < 0){
        freeaddrinfo(result);
        perror("TCP connect");
        xclose(tfd);
        return 4;
    }
    freeaddrinfo(result);
    }
    puts("Client has connected");
    
    while (1){
        char buffer[BUFSZ];
        ssize_t count = recv(tfd, buffer, BUFSZ-1, 0);
        if (count > 0){
            buffer[count] = '\0';
            puts(buffer);
            continue;
        }
        if (count == 0){
            fprintf(stderr, "Server closed connection\n");
            break;
        }
        if (!recv_errorhandling()){
            perror("TCP recv");
            break;
        }
    }
    
    network_finish();
    return 0;
}
