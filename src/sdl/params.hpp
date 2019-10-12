#ifndef __PARAMS_HPP__
#define __PARAMS_HPP__

struct Params
{
    const char* bios_path = nullptr;
    const char* rom_path  = nullptr;
    bool bios_boot = false;
    bool interpreter = false;
};

#endif//__PARAMS_HPP__
