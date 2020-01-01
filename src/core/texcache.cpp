#include <stdarg.h>
#include "texcache.hpp"

TextureCache::TextureCache()
{
    tclog::init();
}

TextureCache::~TextureCache()
{
    tclog::shutdown();
}

void TextureCache::add_texture(Texture* tex)
{
    m_cached_tex.insert({ lookup_t(tex->m_tex0, tex->m_texa), tex });
    misses_this_frame++;
}

Texture* TextureCache::lookup(TEX0 tex0, TEXA_REG texa)
{
    auto it = m_cached_tex.find(lookup_t(tex0, texa));
    std::hash<lookup_t> hasher;

    // lookup hit
    if (it != m_cached_tex.end())
    {
        tclog::log("[TEXCACHE] hit! ($%x) ($%x) ($%x) w: %d h: %d \n",
                    hasher(lookup_t(tex0, texa)),
                    tex0.texture_base, tex0.format,
                    tex0.tex_width, tex0.tex_height);

        return it->second;
    }

    tclog::log("[TEXCACHE] miss!\n");
    tclog::log("\thash: %x\n", hasher(lookup_t(tex0, texa)));
    tclog::log("\tbase: $%x\n", tex0.texture_base);
    tclog::log("\tformat: $%x\n", tex0.format);
    tclog::log("\tbuffer width: $%x\n", tex0.width);
    tclog::log("\twidth: %d\n", tex0.tex_width);
    tclog::log("\theight: %d\n", tex0.tex_height);
    tclog::log("\tuse alpha: $%x\n", tex0.use_alpha);
    tclog::log("\t16/24 alpha (A=0): $%x\n", texa.alpha0);
    tclog::log("\tmethod of expansion: $%x\n", texa.trans_black);
    tclog::log("\t16 alpha (A=1): $%x\n", tex0.texture_base);

    return nullptr;
}

void TextureCache::flush()
{
    // TODO
    // smart pointers
    for (const auto& entry : m_cached_tex)
        delete entry.second;

    m_cached_tex.clear();
}

Texture::Texture(TEX0 tex0, TEXA_REG texa) :
    m_tex0(tex0), m_texa(texa)
{
    m_data = std::make_unique<uint32_t[]>(m_tex0.tex_width * m_tex0.tex_height);
}

bool Texture::map(uint32_t** buff)
{
    *buff = m_data.get();
    return true;
}

void Texture::unmap()
{

}

bool Texture::save(std::string name)
{
    auto width = m_tex0.tex_width;
    auto height = m_tex0.tex_height;

    std::vector<unsigned char> image(width * height * 4);
    std::vector<unsigned char> image_alpha(width * height * 4);

    for (auto y = 0; y < height; y++)
        for (auto x = 0; x < width; x++)
        {
            image[4 * width * y + 4 * x + 0] = m_data[width * y + x];
            image[4 * width * y + 4 * x + 1] = m_data[width * y + x] >> 8;
            image[4 * width * y + 4 * x + 2] = m_data[width * y + x] >> 16;
            image[4 * width * y + 4 * x + 3] = 255;

            image_alpha[4 * width * y + 4 * x + 0] = m_data[width * y + x] >> 24;
            image_alpha[4 * width * y + 4 * x + 1] = m_data[width * y + x] >> 24;
            image_alpha[4 * width * y + 4 * x + 2] = m_data[width * y + x] >> 24;
            image_alpha[4 * width * y + 4 * x + 3] = 255;
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

    ss << "_" << std::hex << m_tex0.texture_base;

    auto err = lodepng::encode(ss.str() + ".png", image, width, height);
    if (err)
    {
        tclog::log("[TEXCACHE] Error saving color image %x: %s\n", err, lodepng_error_text(err));
        return false;
    }

    err = lodepng::encode(ss.str() + "_alpha.png", image_alpha, width, height);
    if (err)
    {
        tclog::log("[TEXCACHE] Error saving alpha image %x: %s\n", err, lodepng_error_text(err));
        return false;
    }

    return true;
}

FILE* tclog::s_file;

void tclog::init()
{
    s_file = fopen("tc.log", "w");
}

void tclog::shutdown()
{
    if (s_file)
        fclose(s_file);
}

void tclog::log(char const* const format,  ...)
{
    if (s_file)
    {
        va_list arglist;
        va_start(arglist, format);
        vfprintf(s_file, format, arglist);
        va_end(arglist);
    }
}