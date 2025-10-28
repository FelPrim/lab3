#include "crossplatform_networking.h"
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
    #include <unistd.h>
#endif

volatile int CTHEREISRUNNINGPROCEDURE = 0;

// Менеджер потоков
typedef struct {
    int next_id;
    int active_threads;
} thread_manager_t;

static thread_manager_t thread_mgr = {0};

// Структура для передачи данных в поток
typedef struct {
    void* (*user_routine)(void*);
    void* user_arg;
    int thread_id;
} thread_data_t;

#ifdef _WIN32

static DWORD WINAPI thread_wrapper(LPVOID arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    // Создаем аргументы для пользовательской функции
    struct croutine_args args;
    args.id = data->thread_id;
    
    // Вызываем пользовательскую функцию
    data->user_routine(&args);
    
    // Уменьшаем счетчик потоков
    thread_mgr.active_threads--;
    if (thread_mgr.active_threads <= 0) {
        thread_mgr.active_threads = 0;
        CTHEREISRUNNINGPROCEDURE = 0;
    }
    
    free(data);
    return 0;
}

#else

static void* thread_wrapper(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    // Создаем аргументы для пользовательской функции
    struct croutine_args args;
    args.id = data->thread_id;
    
    // Вызываем пользовательскую функцию
    data->user_routine(&args);
    
    // Уменьшаем счетчик потоков
    thread_mgr.active_threads--;
    if (thread_mgr.active_threads <= 0) {
        thread_mgr.active_threads = 0;
        CTHEREISRUNNINGPROCEDURE = 0;
    }
    
    free(data);
    return NULL;
}

#endif

struct cprocedure_info crun_in_thread(void* (*routine)(void*), void* arg) {
    struct cprocedure_info info = {0};
    
    thread_data_t* data = malloc(sizeof(thread_data_t));
    if (!data) return info;
    
    data->user_routine = routine;
    data->user_arg = arg;
    data->thread_id = thread_mgr.next_id;
    
#ifdef _WIN32
    HANDLE thread = CreateThread(NULL, 0, thread_wrapper, data, 0, NULL);
    if (thread) {
        info.thread_handle = thread;
        info.id = thread_mgr.next_id++;
        CTHEREISRUNNINGPROCEDURE = 1;
        thread_mgr.active_threads++;
        
        // Не ждем завершения потока сразу
        CloseHandle(thread);
    } else {
        free(data);
    }
#else
    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_wrapper, data) == 0) {
        info.thread_id = thread;
        info.id = thread_mgr.next_id++;
        CTHEREISRUNNINGPROCEDURE = 1;
        thread_mgr.active_threads++;
        
        // Отсоединяем поток для автоматической очистки
        pthread_detach(thread);
    } else {
        free(data);
    }
#endif
    
    return info;
}

void cwait_shutdown(void) {
    while (CTHEREISRUNNINGPROCEDURE && !cshutdown_requested) {
#ifdef _WIN32
        Sleep(100);
#else
        usleep(100000);
#endif
    }
}

int ccanrun(int id) {
    (void)id;
    return !cshutdown_requested && CTHEREISRUNNINGPROCEDURE;
}

void cstop(int id) {
    (void)id;
    // Уменьшение счетчика теперь делается в thread_wrapper
    // Эта функция остается для совместимости
}