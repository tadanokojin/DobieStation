#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <sstream>
#include "gscontext.hpp"
#include "lodepng.h"

struct Texture;
struct TextureCache;

using lookup_t = std::pair<TEX0, TEXA_REG>;

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
        TEXA_REG tex1 = tex_info.second;
        
        // just a guess
        // might not be the best hashing function
        hash ^= hasher(tex0.texture_base) << 1;
        hash ^= hasher(tex0.tex_height)   << 2;
        hash ^= hasher(tex0.tex_width)    << 3;
        hash ^= hasher(tex0.format)       << 4;

        return hash;
    }
};

// should probably move to == operator of the TEX0 and TEXA registers
template <>
struct equal_to<lookup_t>
{
    bool operator()(const lookup_t& lhs, const lookup_t& rhs) const noexcept
    {
        return lhs.first.texture_base == rhs.first.texture_base
               && lhs.first.tex_height == rhs.first.tex_height
               && lhs.first.tex_width == rhs.first.tex_width
               && lhs.first.format == rhs.first.format;
    }
};
}

using cache_map_t = std::unordered_map<lookup_t, Texture*>;

// A representation of the GS texture in 32-bits
struct Texture
{
    // buffer for the cached data
    std::unique_ptr<uint32_t[]> m_data{nullptr};
    TEX0 m_tex0;
    TEXA_REG m_texa;

    Texture() = delete;
    Texture(TEX0 tex0, TEXA_REG texa) :
        m_tex0(tex0), m_texa(texa)
    {
        m_data = std::make_unique<uint32_t[]>(m_tex0.tex_width * m_tex0.tex_height);
    }

    bool map(uint32_t** buff)
    {
        *buff = m_data.get();
        return true;
    }

    // not needed but common idiom
    void unmap(){}

    bool save(std::string name)
    {
        auto width = m_tex0.tex_width;
        auto height = m_tex0.tex_height;

        std::vector<unsigned char> image(width * height * 4);

        for (auto y = 0; y < height; y++)
        for (auto x = 0; x < width; x++)
        {
            image[4 * width * y + 4 * x + 0] = m_data[width * y + x];
            image[4 * width * y + 4 * x + 1] = m_data[width * y + x] >> 8;
            image[4 * width * y + 4 * x + 2] = m_data[width * y + x] >> 16;
            image[4 * width * y + 4 * x + 3] = 255;
        }

        std::stringstream ss;

        std::hash<lookup_t> hasher;
        auto hash = hasher(lookup_t(m_tex0, m_texa));

        ss << "dump/" << hash << "_" << name << "_";

        switch (m_tex0.format)
        {
        case 0x0:
            ss << "CT32";
            break;
        case 0x1:
            ss << "CT24";
            break;
        case 0x2:
            ss << "CT16";
            break;
        case 0xA:
            ss << "CT16S";
            break;
        case 0x13:
            ss << "T8";
            break;
        case 0x14:
            ss << "T4";
            break;
        case 0x1B:
            ss << "T8H";
            break;
        case 0x24:
            ss << "T4HL";
            break;
        case 0x2C:
            ss << "T4HH";
            break;
        case 0x30:
            ss << "Z32";
            break;
        case 0x31:
            ss << "Z24";
            break;
        case 0x32:
            ss << "Z16";
            break;
        case 0x3A:
            ss << "Z16S";
            break;
        default:
            ss << "unk";
        }

        ss << "_" << std::hex << m_tex0.texture_base << ".png";

        auto err = lodepng::encode(ss.str(), image, width, height);
        if (err)
        {
            printf("[TEXCACHE] Error saving image %x: %s\n", err, lodepng_error_text(err));
            return false;
        }

        return true;
    }
};

struct TextureCache
{
    // TODO
    // perf implications of unordered map
    cache_map_t m_cached_tex{};
    uint64_t misses_this_frame{0};

    void add_texture(Texture* tex)
    {
        m_cached_tex.insert({ lookup_t(tex->m_tex0, tex->m_texa), tex });
        misses_this_frame++;
    }

    // Attempts to find a texture in the cache
    // should it fail, it will perform a "miss"
    // and load the texture out of GS memory
    Texture* lookup(TEX0 tex0, TEXA_REG texa)
    {
        auto it = m_cached_tex.find(lookup_t(tex0, texa));

        // lookup hit
        if (it != m_cached_tex.end())
        {
            printf(
                "[TEXCACHE] hit! ($%x) ($%x) w: %d h: %d \n",
                tex0.texture_base, tex0.format,
                tex0.tex_width, tex0.tex_height
            );

            return it->second;
        }

        printf(
            "[TEXCACHE] miss! ($%x) ($%x) w: %d h: %d \n",
            tex0.texture_base, tex0.format,
            tex0.tex_width, tex0.tex_height
        );

        return nullptr;
    }

    // update frame information
    void update_frame()
    {
        printf("[TEXCACHE] total misses %d\n", misses_this_frame);
        misses_this_frame = 0;
    }

    // flushes the entire cache
    void flush()
    {
        // TODO
        // smart pointers
        for (const auto& entry : m_cached_tex)
            delete entry.second;

        m_cached_tex.clear();
    }
};