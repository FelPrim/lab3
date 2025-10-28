#include "crossplatform_networking.h"
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Простая тестовая функция для потока
void* simple_test_function(void *arg) {
    struct croutine_args *args = (struct croutine_args*) arg;
    int id = args->id;
    
    printf("Thread %d: started\n", id);
    
    // Простая работа - просто ждем немного
    for (int i = 0; i < 2; i++) {
        if (!ccanrun(id)) {
            printf("Thread %d: stopping early\n", id);
            cstop(id);
            return NULL;
        }
        printf("Thread %d: step %d\n", id, i + 1);
        
        #ifdef _WIN32
            Sleep(500);
        #else
            usleep(500000);
        #endif
    }
    
    printf("Thread %d: finished\n", id);
    cstop(id);
    return NULL;
}

int main() {
    printf("=== Simple Threading Test ===\n");
    
    // Инициализация
    csetup();
    printf("Initialization completed\n");
    
    // Простой тест - один поток
    printf("Starting one thread...\n");
    crun_in_thread(simple_test_function, NULL);
    
    printf("Main: Waiting for thread to finish...\n");
    cwait_shutdown();
    printf("Main: Thread finished\n");
    
    // Завершение
    cfree();
    printf("=== Test Completed ===\n");
    
    return 0;
}