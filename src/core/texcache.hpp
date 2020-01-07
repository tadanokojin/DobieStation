#pragma once
#include <memory>
#include <unordered_map>
#include <array>
#include <string>
#include <sstream>
#include "gscontext.hpp"
#include "lodepng.h"

struct Texture;
struct TextureCache;
struct CacheRegion;
struct Rect;

// some typedefs
using lookup_t = std::pair<TEX0, TEXA_REG>;
using cache_map_t = std::unordered_map<lookup_t, Texture*>;

struct Rect
{
    uint32_t x, y, z, w;
};

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

//        <------------------- buffer width --------------------->
// base -> ------------------------------------------------------
//        |                                                      |
//        | (x, y) -> --------------------------                 |
//        |          |                          |                |
//        |          |                          |                |
//        |          |                          |                |
//        |          |                          |                |
//        |          |                          |                |
//        |          |                          |                |
//        |          |                          |                |
//        |           -------------------------- <- (z, w)       |
//        |                                                      |
struct CacheRegion
{
    uint32_t psm;
    uint32_t base;
    uint32_t width;
    Rect rect;

    const uint32_t start_page() const noexcept
    {
        return base / 8192;
    };

    const uint32_t end_page() const noexcept
    {
        uint32_t page_width;
        uint32_t page_height;

        switch (psm)
        {
        case 0x0:
        case 0x1:
        case 0x30:
        case 0x31:
            page_width = 64;
            page_height = 32;
            break;
        case 0x2:
        case 0xa:
        case 0x32:
        case 0x3a:
            page_width = 64;
            page_height = 64;
            break;
        default: // TODO
            return 1;
        }

        const uint32_t total_pages = ((rect.z - 1) / page_width) + (rect.w / page_height);

        // invalidate at lease 1 page
        return std::max(base / 8192 + total_pages, base / 8192 + 1);
    };
};

// optimization notes:
// 1. currently we invalidate entire textures instead of
//    invalidating the area within them. This means that
//    misses are technically more expensive than they need be
// 2. invalidations happen between the start and end page boundries
//    this doesn't account for the fact that writes don't need to be
//    the full width of the buffer so in some cases we are needlessly
//    invalidating pages
// 3. I have read that unordered map can be slow.

struct TextureCache
{
    // Max number of pages is 512 on a retail PS2
    // However, I believe some dev/arcade machines can do more
    // For now, assume retail as that is what the gsthread does.
    static constexpr int MAX_PAGES{512};

    cache_map_t m_cached_tex{};
    std::array<std::vector<Texture*>, MAX_PAGES> m_pages{};

    uint64_t misses_this_frame{0};

    Texture* lookup(TEX0 tex0, TEXA_REG texa);
    void add_texture(Texture* tex);

    void invalidate(CacheRegion& rect);

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

    size_t m_size;

    bool m_valid{ false };

    Texture() = delete;
    Texture(TEX0 tex0, TEXA_REG texa);

    bool map(uint32_t** buff);
    void unmap();

    void validate()   { m_valid = true;  };
    void invalidate() { m_valid = false; };

    bool save(std::string name);

    bool valid()   const noexcept { return m_valid;  };
    bool invalid() const noexcept { return !m_valid; };

    const uint32_t buffer_width() const noexcept { return m_tex0.width; };
    const uint32_t format()       const noexcept { return m_tex0.format; };
    const uint32_t base()         const noexcept { return m_tex0.texture_base; };

    const size_t size() const noexcept { return m_size; };
    const Rect rect()   const noexcept { return {0, 0, m_tex0.tex_height, m_tex0.tex_width}; };
};