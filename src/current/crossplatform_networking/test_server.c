#include "crossplatform_networking.h"
#include <stdio.h>
#include <string.h>

#define ADDRESS "localhost"
#define UPORT "4321"
#define TPORT "5432"

void* udpserver_logic(void *arg){
    struct croutine_args *args = (struct croutine_args*) arg;
    int id = args->id;
    struct csockaddr myinfo;
    {
        struct csockaddr* tmp = cgetaddrinfo(ADDRESS, UPORT, CUDP | CSERVER);
        //if (tmp) {
        //    memcpy(&myinfo, tmp, sizeof(myinfo));
        //    cfreeaddrinfo(tmp);
        //}
    }
    csocket udp_echo = cgetsocket(&myinfo);
    creuseaddr(udp_echo);
    cbind(udp_echo, &myinfo);
    struct csockaddr client;
    
    while (ccanrun(id)){
        char buf[2048];
        int numbytes = crecvfrom(udp_echo, buf, 2047, &client);
        if (numbytes < 0){
            continue;
        }
        buf[numbytes] = '\0';
        printf("UDP RECVD: %s\n", buf);
        csendto(udp_echo, buf, numbytes, &client);
    }
    
    cclose(udp_echo);
    cstop(id);
    return NULL;
}

void* tcpserver_logic(void *arg){
    struct croutine_args *args = (struct croutine_args*) arg;
    int id = args->id;
    struct csockaddr myinfo;
    {
        struct csockaddr* tmp = cgetaddrinfo(ADDRESS, TPORT, CTCP | CSERVER);
       // if (tmp) {
       //    memcpy(&myinfo, tmp, sizeof(myinfo));
       //    cfreeaddrinfo(tmp);
       //}
    }
    csocket tcp_echo = cgetsocket(&myinfo);
    creuseaddr(tcp_echo);
    cbind(tcp_echo, &myinfo);
    clisten(tcp_echo, DEFAULT_BACKLOG);
    
    while (ccanrun(id)){
        struct csockaddr clientinfo;
        csocket clientfd = caccept(tcp_echo, &clientinfo);
        if (clientfd == CSOCKET_INVALID) {
            continue;
        }
        
        char buf[1024];
        int numbytes = crecv(clientfd, buf, 1023);
        if (numbytes > 0){
            buf[numbytes] = '\0';
            printf("TCP RECVD: %s\n", buf);
            csend(clientfd, buf, numbytes);
        }
        cclose(clientfd);
    }
    
    cclose(tcp_echo);
    cstop(id);
    return NULL;
}

int main() {
	puts("1");
    csetup();
	puts("2");
    crun_in_thread(udpserver_logic, NULL);
	puts("3");
    crun_in_thread(tcpserver_logic, NULL);
	puts("4");
	cwait_shutdown();
	puts("5");
    cfree();
	puts("6");
    return 0;
}