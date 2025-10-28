#include "crossplatform_networking.h"
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <signal.h>
    #include <unistd.h>
#endif

volatile int cshutdown_requested = 0;

#ifdef _WIN32

static BOOL WINAPI console_handler(DWORD dwType) {
    switch (dwType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            cshutdown_requested = 1;
            return TRUE;
        default:
            return FALSE;
    }
}

void csignal_handle_init(void) {
    if (!SetConsoleCtrlHandler(console_handler, TRUE)) {
        exit(1);
    }
}

void csignal_handle_cleanup(void) {
    SetConsoleCtrlHandler(console_handler, FALSE);
}

#else

static void sigint_handler(int sig) {
    (void)sig;
    cshutdown_requested = 1;
}

static struct sigaction old_sa;

void csignal_handle_init(void) {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, &old_sa) == -1) {
        exit(1);
    }
    
    signal(SIGPIPE, SIG_IGN);
}

void csignal_handle_cleanup(void) {
    sigaction(SIGINT, &old_sa, NULL);
    signal(SIGPIPE, SIG_DFL);
}

#endif