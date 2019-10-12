#include "application.hpp"
#include <SDL.h>
#include <memory>
#include <cstdio>


int main(int argc, char** argv)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
        return 1;

    auto app = std::make_unique<Application>();
    if (!app)
        return 1;

    int res = app->run();

    SDL_Quit();
    return res;
}
