#include "application.hpp"
#include <SDL.h>
#include <memory>
#include <cstdio>


int main(int argc, char** argv)
{
    auto app = std::make_unique<Application>();
    if (!app)
        return 1;

    return app->run();
}
