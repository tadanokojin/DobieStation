#include "application.hpp"
#include "params.hpp"

#include <SDL.h>
#include <memory>
#include <cstdio>


int main(int argc, char** argv)
{
    Params params;
    if (!parse_arguments(argc, argv, params))
        return 1;

    if (!params.rom_path)
    {
        fprintf(stderr, "No ROM path specified.\n");
        show_usage(argv[0]);
        return 1;
    }

    if (!params.bios_path)
    {
        fprintf(stderr, "A BIOS is required to use DobieStation.\n");
        show_usage(argv[0]);
        return 1;
    }

    auto app = std::make_unique<Application>();
    if (!app)
        return 1;

    return app->run(params);
}
