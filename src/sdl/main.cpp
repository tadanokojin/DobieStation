#include <SDL.h>
#include <emulator.hpp>

#include <cstdio>
#include <vector>
#include <memory>


constexpr int DEFAULT_WIDTH  = 640;
constexpr int DEFAULT_HEIGHT = 448;
constexpr int BIOS_SIZE = 1024 * 1024 * 4;

int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
        return 1;

    const int winpos = SDL_WINDOWPOS_UNDEFINED;
    SDL_Window* window = SDL_CreateWindow("DobieSDL",
        winpos, winpos, DEFAULT_WIDTH, DEFAULT_HEIGHT,
        SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
    {
    	SDL_Quit();
    	return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer,
    	SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
    	DEFAULT_WIDTH, DEFAULT_HEIGHT);
    if (!texture)
    {
    	SDL_Quit();
    	return 1;
    }

    auto emu = std::unique_ptr<Emulator>(new Emulator());
    emu->reset();
    //emu->load_BIOS(bios_data.data());

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
        //emu->run();

        //SDL_UpdateTexture(texture, rect, pixels, pitch);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
