#include "crossplatform_networking.h"
#include <stdio.h>
#include <string.h>

#define ADDRESS "localhost"
#define UPORT "4321"
#define TPORT "5432"

void* udpclient_logic(void *arg){
    struct croutine_args *args = (struct croutine_args*) arg;
    int id = args->id;
    struct csockaddr servinfo;
    {
        struct csockaddr* tmp = cgetaddrinfo(ADDRESS, UPORT, CUDP | CCLIENT);
        //if (tmp) {
        //    memcpy(&servinfo, tmp, sizeof(servinfo));
        //    cfreeaddrinfo(tmp);
        //}
    }
    csocket udp_call = cgetsocket(&servinfo);
    
    struct csockaddr server;
    int numbytes = csendto(udp_call, "UDP from client!", strlen("UDP from client!"), &server);
    
    char buf[2048];
    numbytes = crecvfrom(udp_call, buf, 2047, &server);
    if (numbytes > 0) {
        buf[numbytes] = '\0';
        printf("UDP RECVD: %s\n", buf);
    }
    
    cclose(udp_call);
    cstop(id);
    return NULL;
}

void* tcpclient_logic(void *arg){
    struct croutine_args *args = (struct croutine_args*) arg;
    int id = args->id;
    struct csockaddr servinfo;
    {
        //struct csockaddr* tmp = cgetaddrinfo(ADDRESS, TPORT, CTCP | CCLIENT);
        //if (tmp) {
        //    memcpy(&servinfo, tmp, sizeof(servinfo));
        //    cfreeaddrinfo(tmp);
        //}
    }
    csocket tcp_call = cgetsocket(&servinfo);
    cconnect(tcp_call, &servinfo);
    csend(tcp_call, "TCP from client!", strlen("TCP from client!"));
    
    char buf[1024];
    int numbytes = crecv(tcp_call, buf, 1023);
    if (numbytes > 0) {
        buf[numbytes] = '\0';
        printf("TCP RECVD: %s\n", buf);
    }
    
    cclose(tcp_call);
    cstop(id);
    return NULL;
}

int main() {
    csetup();
    crun_in_thread(tcpclient_logic, NULL);
    crun_in_thread(udpclient_logic, NULL);
	puts("1");
    cwait_shutdown();
	puts("2");
    cfree();
	puts("3");
    return 0;
}