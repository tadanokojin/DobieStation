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
struct LookupInfo;

// some typedefs
using cache_map_t = std::unordered_map<LookupInfo, Texture*>;

struct Rect
{
    uint32_t x, y, z, w;
};

struct LookupInfo
{
    uint32_t base;
    uint32_t format;
    uint32_t buffer_width;
    uint16_t width;
    uint16_t height;

    bool trans_black;
    uint8_t source_alpha0;
    uint8_t source_alpha1;
};

namespace std
{
template <>
struct hash<LookupInfo>
{
    size_t operator()(const LookupInfo& tex_info) const noexcept
    {
        size_t hash = 0;
        std::hash<uint64_t> hasher;

        // just a guess
        // might not be the best hashing function
        hash ^= hasher(tex_info.base)         << 1;
        hash ^= hasher(tex_info.height)       << 2;
        hash ^= hasher(tex_info.width)        << 3;
        hash ^= hasher(tex_info.format)       << 4;
        hash ^= hasher(tex_info.buffer_width) << 5;

        const uint32_t& format = tex_info.format;

        // 24 and 16 bit
        if ((format & 0x3) == 1 || (format & 0x3) == 2 && !tex_info.trans_black)
            hash ^= hasher(tex_info.source_alpha0) << 6;

        // 16 bit
        if ((format & 0x3) == 2)
            hash ^= hasher(tex_info.source_alpha1) << 7;

        return hash;
    }
};

template <>
struct equal_to<LookupInfo>
{
    bool operator()(const LookupInfo& lhs, const LookupInfo& rhs) const noexcept
    {
        bool wan = true;
        wan &= lhs.base   == rhs.base;
        wan &= lhs.height == rhs.height;
        wan &= lhs.width == rhs.width;
        wan &= lhs.buffer_width  == rhs.buffer_width;
        wan &= lhs.format == rhs.format;

        if (!wan)
            return false;

        const uint32_t& format = lhs.format;

        // 24 and 16 bit
        if ((format & 0x3) == 1 || (format & 0x3) == 2)
        {
            if (!lhs.trans_black && !rhs.trans_black)
                wan &= lhs.source_alpha0 == rhs.source_alpha0;
        }

        // 16 bit
        if ((format & 0x3) == 2)
            wan &= lhs.source_alpha1 == rhs.source_alpha1;

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

    Texture* lookup(LookupInfo& info);
    void add_texture(Texture* tex);

    void invalidate(CacheRegion& rect);

    void flush();
};

// A representation of the GS texture in 32-bits
struct Texture
{
    // buffer for the cached data
    std::unique_ptr<uint32_t[]> m_data{ nullptr };

    LookupInfo m_info;
    size_t m_size;

    bool m_valid{ false };

    Texture() = delete;
    Texture(LookupInfo info);

    bool map(uint32_t** buff);
    void unmap();

    void validate()   { m_valid = true;  };
    void invalidate() { m_valid = false; };

    bool save(std::string name);

    bool valid()   const noexcept { return m_valid;  };
    bool invalid() const noexcept { return !m_valid; };

    const uint32_t buffer_width() const noexcept { return m_info.buffer_width; };
    const uint32_t format()       const noexcept { return m_info.format; };
    const uint32_t base()         const noexcept { return m_info.base; };
    const LookupInfo info()       const noexcept { return m_info; };

    const size_t size() const noexcept { return m_size; };
    const Rect rect()   const noexcept { return {0, 0, m_info.height, m_info.width}; };
};