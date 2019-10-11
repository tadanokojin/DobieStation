#include <cstdio>
#include <SDL.h>

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
        return 1;

    const int winpos = SDL_WINDOWPOS_UNDEFINED;
    SDL_Window* window = SDL_CreateWindow("DobieSDL",
        winpos, winpos,
        640, 448,
        SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Quit();
        return 1;
    }

    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event) > 0)
        {
            if (event.type == SDL_QUIT)
                running = false;
        }

        SDL_Delay(10); // Don't saturate
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
