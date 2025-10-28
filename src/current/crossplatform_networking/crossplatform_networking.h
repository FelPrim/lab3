#ifndef CROSSPLATFORM_NETWORKING_H
#define CROSSPLATFORM_NETWORKING_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
#elif defined(__linux__) || defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
#endif

#ifdef _WIN32
    typedef SOCKET csocket;
    #define cclose closesocket
	#define CSOCKET_INVALID INVALID_SOCKET
    #define CSOCKET_ERROR SOCKET_ERROR
#else
    typedef int csocket;
    #define cclose close
	#define CSOCKET_INVALID (-1)
    #define CSOCKET_ERROR (-1)
#endif

void csetup();
void cfree();

struct csockaddr {
    struct sockaddr_in addr;
};

struct cprocedure_info {
    int id;
    #ifdef _WIN32
        HANDLE thread_handle;
    #else
        pthread_t thread_id;
    #endif
};

struct croutine_args {
    int id;
};

struct cprocedure_info crun_in_thread(void* (*routine)(void*), void* arg);

void cwait_shutdown();
int ccanrun(int id);
void cstop(int id);

extern volatile int cshutdown_requested;
extern volatile int CTHEREISRUNNINGPROCEDURE;
void csignal_handle_init(void);
void csignal_handle_cleanup(void);

#define UNASSIGNED 0
#define CTCP 5
#define CUDP 1
#define CCLIENT 10
#define CSERVER 2

struct csockaddr* cgetaddrinfo(const char* node, const char* service, int params);
void cfreeaddrinfo(struct csockaddr* addr);

csocket cgetsocket(const struct csockaddr* info);
int cbind(csocket sockfd, const struct csockaddr* info);
int creuseaddr(csocket sockfd);
int cconnect(csocket sockfd, const struct csockaddr* info);

#define DEFAULT_BACKLOG 10
int clisten(csocket sockfd, int backlog);
csocket caccept(csocket sockfd, struct csockaddr* info);

#define DEFAULT_SENDFLAGS 0
int csend(csocket sockfd, const void* msg, int len);
#define DEFAULT_RECVFLAGS 0
int crecv(csocket sockfd, void* buf, int len);

#define DEFAULT_SENDTOFLAGS 0
int csendto(csocket sockfd, const void* msg, int len, const struct csockaddr* to);
#define DEFAULT_RECVFROMFLAGS 0
int crecvfrom(csocket sockfd, void* buf, int len, struct csockaddr* from);

#ifdef __cplusplus
}
#endif

#endif