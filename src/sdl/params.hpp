#ifndef __PARAMS_HPP__
#define __PARAMS_HPP__

struct Params
{
    const char* bios_path = nullptr;
    const char* rom_path  = nullptr;
    bool bios_boot = false;
    bool interpreter = false;
};

void show_usage(const char* arg0);
bool parse_arguments(int argc, char** argv, Params& params);

#endif//__PARAMS_HPP__
