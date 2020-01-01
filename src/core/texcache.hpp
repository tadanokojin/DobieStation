#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <sstream>
#include <stdio.h>
#include "gscontext.hpp"
#include "lodepng.h"

struct Texture;
struct TextureCache;

using lookup_t = std::pair<TEX0, TEXA_REG>;
using cache_map_t = std::unordered_map<lookup_t, Texture*>;

namespace std
{
template <>
struct hash<lookup_t>
{
    size_t operator()(const lookup_t& tex_info) const noexcept
    {
        size_t hash = 0;
        std::hash<uint64_t> hasher;

        TEX0 tex0 = tex_info.first;
        TEXA_REG texa = tex_info.second;

        // just a guess
        // might not be the best hashing function
        hash ^= hasher(tex0.texture_base) << 1;
        hash ^= hasher(tex0.tex_height)   << 2;
        hash ^= hasher(tex0.tex_width)    << 3;
        hash ^= hasher(tex0.format)       << 4;

        auto format = tex_info.first.format;

        // 24 and 16 bit
        if ((format & 0x3) == 1 || (format & 0x3) == 2 && !texa.trans_black)
            hash ^= hasher(texa.alpha0) << 6;

        // 16 bit
        if ((format & 0x3) == 2)
            hash ^= hasher(texa.alpha1) << 7;

        return hash;
    }
};

// should probably move to == operator of the TEX0 and TEXA registers
template <>
struct equal_to<lookup_t>
{
    bool operator()(const lookup_t& lhs, const lookup_t& rhs) const noexcept
    {
        bool wan = true;
        wan &= lhs.first.texture_base == rhs.first.texture_base;
        wan &= lhs.first.tex_height   == rhs.first.tex_height;
        wan &= lhs.first.tex_width    == rhs.first.tex_width;
        wan &= lhs.first.format       == rhs.first.format;

        if (!wan)
            return false;

        auto format = lhs.first.format;

        // 24 and 16 bit
        if ((format & 0x3) == 1 || (format & 0x3) == 2)
        {
            if (!lhs.second.trans_black && !rhs.second.trans_black)
                wan &= lhs.second.alpha0 == rhs.second.alpha0;
        }

        // 16 bit
        if ((format & 0x3) == 2)
            wan &= lhs.second.alpha1 == rhs.second.alpha1;

        return wan;
    }
};
}

struct TextureCache
{
    // TODO
    // perf implications of unordered map
    cache_map_t m_cached_tex{};
    uint64_t misses_this_frame{0};

    TextureCache();
    ~TextureCache();

    void add_texture(Texture* tex);

    // Attempts to find a texture in the cache
    // should it fail, it will perform a "miss"
    // and load the texture out of GS memory
    Texture* lookup(TEX0 tex0, TEXA_REG texa);

    // flushes the entire cache
    void flush();
};

// A representation of the GS texture in 32-bits
struct Texture
{
    // buffer for the cached data
    std::unique_ptr<uint32_t[]> m_data{ nullptr };

    // copies of the registers for later use
    TEX0 m_tex0;
    TEXA_REG m_texa;

    Texture() = delete;
    Texture(TEX0 tex0, TEXA_REG texa);

    bool map(uint32_t** buff);
    void unmap(); // not needed but common idiom

    bool save(std::string name);
};


// not awesome but will do for now
// just a small utility for loging tc
struct tclog
{
    static FILE* s_file;

    static void init();
    static void shutdown();

    static void log(char const* const format, ...);
};