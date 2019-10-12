#include "application.hpp"
#include <SDL.h>
#include <memory>
#include <getopt.h>
#include <cstdio>

static const struct option longopts[] =
{
    {"help", no_argument,       nullptr, 'h'},
    {"bios", required_argument, nullptr, 'b'},

    {nullptr, 0, nullptr, 0}
};

static const char* bios_path = nullptr;
static const char* rom_path  = nullptr;

void show_usage(const char* arg0)
{
    fprintf(stderr, "Usage: %s -b <bios path> <path to iso/cso/elf>\n", arg0);
}

void show_help(const char* arg0)
{
    show_usage(arg0);
    fprintf(stderr, "Options:\n");
    for (const struct option* opt = longopts; opt->name != nullptr; ++opt)
    {
        fprintf(stderr, "\t-%c --%s", opt->val, opt->name);
        if (opt->has_arg == required_argument)
            fprintf(stderr, " <>\n");
        else if (opt->has_arg == optional_argument)
            fprintf(stderr, " []\n");
        else
            fprintf(stderr, "\n");
    }

    fflush(stderr);
}

bool parse_options(int argc, char** argv)
{
    int opt;
    while ((opt = getopt_long(argc, argv, "hb:", longopts, nullptr)) > 0)
    {
        switch (opt)
        {
        case ('h'):
            show_help(argv[0]);
            exit(0);

        case ('?'):
            show_usage(argv[0]);
            return false;

        case ('b'):
            bios_path = optarg;
            break;

        default:
            break;
        }
    }

    return true;
}

bool parse_arguments(int argc, char** argv)
{
    if (!parse_options(argc, argv))
        return false;

    int argnum = argc - optind;
    if (argnum > 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        show_usage(argv[0]);
        return false;
    }
    else if (argnum == 1)
    {
        rom_path = argv[optind];
    }

    return true;
}

int main(int argc, char** argv)
{
    if (!parse_arguments(argc, argv))
        return 1;

    if (!rom_path)
    {
        fprintf(stderr, "No ROM path specified.\n");
        show_usage(argv[0]);
        return 1;
    }

    if (!bios_path)
    {
        fprintf(stderr, "A BIOS is required to use DobieStation.\n");
        show_usage(argv[0]);
        return 1;
    }

    printf("Params:\nrom_path:  %s\nbios_path: %s\n", rom_path, bios_path);
    return 0;

    auto app = std::make_unique<Application>();
    if (!app)
        return 1;

    return app->run();
}
