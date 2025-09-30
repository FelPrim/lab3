
// sdl3_simple.c
#include <stdio.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> // В SDL3 надо включать этот хедер в файле с main

int main(int argc, char **argv)
{
    // Инициализация (в SDL3 SDL_Init возвращает bool: true = успех)
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("SDL3: простейший пример", 800, 600, 0);
    if (!win) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        // SDL_PollEvent в SDL3 возвращает true, если событие доступно
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        // Простая пауза, чтобы не жрать CPU
        SDL_Delay(16);
    }

    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
