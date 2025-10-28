#include "crossplatform_networking.h"
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

int main() {
    printf("Testing signal handling. Press Ctrl+C to test.\n");
    csetup();

    while (!cshutdown_requested) {
        printf("Working... Press Ctrl+C to exit\n");
#ifdef _WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
    }

    printf("Signal received. Shutting down gracefully...\n");
    cfree();
    return 0;
}