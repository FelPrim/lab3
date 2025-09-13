// Отвечает за то, что делает сервер
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define TPORT 23230
#define TPORTSTR "23230"
#define UPORT 23231
#define UPORTSTR "23231"
#define BUF_SZ 1024
#define FQDN_SZ 256
#define CONMAX SOMAXCONN

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

int main(int argc, char *argv[]){
    
    // Открытие портов
    int tfd;//, ufd;

    // tcp
    {
    struct sockaddr_in hints, *result;
    memset(&hints, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    getaddrinfo(NULL, TPORTSTR, &hints, &result);

    if ((tfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) < 0)
        goto tsocket_error;
  
    int opt = 1;
    if (setsockopt(tfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        goto tsetsockopt_error;

    if (bind(tfd, result->ai_addr, result->ai_addrlen) < 0)
        goto tbind_error;
    
    if (listen(tfd, CONMAX) < 0)
        goto tlisten_error;

    freeaddrinfo(result);

    int flags = fcntl(tfd, F_GETFL, 0);
    if (flags < 0)
        goto tftcntl_getfl_error;
    if (fcntl(tfd, F_SETFL, flags | O_NONBLOCK) < 0)
        goto tftcntl_setfl_error;

    }
    // udp
    /*
    {
    struct sockaddr_in hints, *result;
    memset(&hints, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    getaddrinfo(NULL, UPORTSTR, &hints, &result);

    if ((ufd = socket(result->ai_family, result->ai_socktype, result->ai_protocol)) < 0)
        goto usocket_error;
  
    int opt = 1;
    if (setsockopt(ufd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        goto usetsocket_error;

    if (bind(ufd, result->ai_addr, result->ai_addrlen) < 0)
        goto ubind_error;

    if (listen(ufd, CONMAX) < 0)
        goto ulisten_error;

    freeaddrinfo(result);

    int flags = fcntl(ufd, F_GETFL, 0);
    if (flags < 0)
        goto uftcntl_getfl_error;
    if (fcntl(ufd, F_SETFL, flags | O_NONBLOCK) < 0)
        goto uftcntl_setfl_error;

    }
    */
    int epfd = epoll_create1(0);
    if (epfd < 0)
        goto epfd_error;
    unsigned int eventslen = 0, eventscap = 64;
    static_assert(events_cap >= 1); //2);
    struct epoll_event *events = (struct epoll_event*) calloc(eventscap*sizeof(struct epoll_event)); 
    events[0].events = EPOLLIN | EPOLLOUT; // не уверен
    events[0].data.fd = tfd;
   // events[1].events = EPOLLIN | EPOLLOUT;
   // events[1].data.fd = ufd;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, events) < 0)
        goto tepoll_ctl_error;
    //if (epoll_ctl(epfd, EPOLL_CTL_ADD, ufd, events+1) < 0)
    //    goto uepoll_ctl_error;
    
    int *clients = (int*) calloc((eventscap-2)*sizeof(int));
    int client_count = 0;

    eventslen = 2;

    puts("server started");
    printf("tfd: %d\n", tfd);//"ufd: %d\n", tfd, ufd);

    // Теперь надо сделать асинхронную часть с приёмом, анализом и ответом на запросы
    while (1){
        int n = epoll_wait(epfd, events, eventslen, -1);
        if  (unlikely(n < 0)){
            if (errno == EINTR) continue;
            goto epoll_wait_error;
        }
        for (int i = 0; i < n; ++i){
            inf fd = events[i].data.fd;
            
            if (fd == tfd){
                // новое подключение
                struct sockaddr_in cli_addr;
                socklen_t cli_len = sizeof(cli_addr);
                int conn_fd = accept(tfd, (struct sockaddr*)&cli_addr, &cli_len);
                if (unlikely(conn_fd < 0)){
                    perror("accept");
                    continue;
                }

                {
                int flags = fcntl(conn_fd, F_GETFL, 0);
                if (unlikely(flags < 0)) //return -1
                fcntl(conn_fd, F_SETFL, flags | O_NONBLOCK);
                }
                //if (eventslen > eventscap){
                //}
                events[eventslen].events = EPOLLIN | EPOLLRDHUP; // WTF?
                events[eventslen].data.fd = conn_fd;
                if (unlikely(epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, events+eventslen) < 0)){
                    perror("epoll ctl: conn_fd");
                    close(conn_fd);
                    continue;
                }
                eventslen++; 
                clients[client_count] = conn_fd;
                client_count++;

                char msg[128];
                snprintf(msg, sizeof(msg) "New client connected! (fd=%d)\n", conn_fd);

                for (int j = 0; j < client_count; ++j)
                    send(clients[j], msg, strlen(msg), 0);

                printf("Client fd=%d connected.\n", conn_fd);
                
            } else {
                // client_socket_event
                if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)){
                    // client disconnected
                    printf("Client fd=%d disconnected.\n", fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);

                    for (int j = 0; j < client_count; ++j){
                        if (clients[j] == fd){
                            clients[j] = clients[client_count-1];
                            client_count--;
                            eventslen--;
                            break;
                        }
                    }

                    continue;
                }
                if (likely(events[i].events & EPOLLIN){
                    har buf[512];
                    ssize_t count = recv(fd, buf, sizeof(buf), 0);
                    if (unlikely(count <= 0)){
                        //client closed connection
                        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                        close(fd);
                        for (int j = 0; j < client_count; ++j){
                            if (clients[j] == fd){
                                clients[j] = clients[client_count-1];
                                client_count--;
                                eventslen--;
                                break;
                            }
                        }

                        continue;
                    }
                    buf[count] = '\0';
                    printf("From client fd=%d: %s\n", fd, buf);
                }

            }
        }
    }

    for (int j = 0; j < eventslen; ++j)
        close(events[j].data.fd);
    close(epfd);
    free(events);
    free(clients);

    return 0;

tsocket_error:
    perror("tcp socket");
    return 1;
tsetsockopt_error:
    perror("tcp setsockopt");
    close(tfd);
    return 2;
tbind_error:
    perror("tcp bind");
    close(tfd);
    return 3;
tlisten_error:
    perror("tcp listen");
    close(tfd);
    return 4;
/*usocket_error:
    perror("udp socket");
    close(tfd);
    return 5;
usetsockopt_error:
    perror("udp setsockopt");
    close(tfd);
    close(ufd);
    return 6;
ubind_error:
    perror("udp bind");
    close(tfd);
    close(ufd);
    return 7;
ulisten_error:
    perror("udp listen");
    close(tfd);
    close(ufd);
    return 8;
*/
tftcntl_getfl_error:
    perror("tcp fcntl(F_GETFL)");
    close(tfd);
    return 9;
tftcntl_setfl_error:
    perror("tcp fctnl(F_GETFL)");
    close(tfd);
    return 10;
/*
uftcntl_getfl_error:
    perror("udp fcntl(F_GETFL)");
    close(tfd);
    close(ufd);
    return 11;
uftcntl_setfl_error:
    perror("udp fctnl(F_GETFL)");
    close(tfd);
    close(ufd);
    return 12;
*/
epfd_error:
    perror("epoll_create1");
    close(tfd);
//    close(ufd);
    return 13;
tepoll_ctl_error:
    perror("tcp epoll_ctl");
    close(tfd);
//    close(ufd);
    return 14;
//uepoll_ctl_error:
//    perror("udp epoll_ctl");
//    close(tfd);
//    close(ufd);
//    return 15;
epoll_wait_error:
    perror("epoll_wait");
    // должен циклом проходить
    close(tfd);
//    close(ufd);
    return 16;
}
