#include "texcache.hpp"
#include "emulator.hpp"

// Some potential edge cases
//
// Raw Danger, Grand Turismo games, Steambot Chronicles, etc
// will read a 32 bit texture as packed 8bit by gs page up to 4x per page
// and write it back to the same memory region. GS cache should mean
// that any writes are opaque to subsequent reads until the page break
//
// Haunting Ground, Matrix, Crash Twinsanity, Need for Speed U2, MC3, Transformers etc
// will read a 32 bit texture as 16 bit to split the rg and ba channels into
// 8 x height * 2 columns which they will texture map into a target. Transformers and
// MC3 do this by page. Twinsanity and Matrix do it in 2 parts (not always by exactly half).
// Others like HG do it on the entire framebuffer.
//
// Armored Core: Last Raven will local to local the framebuffer into an area of memory
// that will be used as the PCRCT read area
//
// Virtua Fighter 2, Burnout Games, Jak 3, etc
// will render directly into the clut which means the clut and any textures attached
// to it need to be invalidated
//
// Wallace and Gromit and Thrillville wrap around GS memory in FMVs
//
// Games by VIS and Superman Returns clear textures by writing a really tall rectangle
// with the FBW set 1
//
// Dark Cloud 2 will write to all of GS memory from the EE on init
//
// Many games will copy or read the alpha channel of 32/24 bit textures using the 8H format

void TextureCache::add_texture(Texture* tex)
{
    misses_this_frame++;

    CacheRegion region = {};
    region.base = tex->base();
    region.psm = tex->format();
    region.width = tex->buffer_width();
    region.rect = tex->rect();

    auto start_page = region.start_page();
    auto end_page = region.end_page();

    for (auto i = start_page; i < end_page; i++)
    {
        printf("[TEXCACHE] insert into page %d ($%x)\n", i, tex->m_tex0.texture_base);
        m_pages[i].push_back(tex);
    }
    m_cached_tex.insert({ lookup_t(tex->m_tex0, tex->m_texa), tex });
}

// Attempts to find a texture in the cache
// should it fail, it will perform a "miss"
// and load the texture out of GS memory
Texture* TextureCache::lookup(TEX0 tex0, TEXA_REG texa)
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

void TextureCache::invalidate(CacheRegion& region)
{
    printf("[TEXCACHE] invalidate!\n");
    printf("\tformat: $%x\n", region.psm);
    printf("\tbase:   $%x\n", region.base);
    printf("\twidth:  %d\n", region.width);
    printf("\t%d, %d -> %d, %d\n", region.rect.x, region.rect.y, region.rect.z, region.rect.w);
    printf("\tpages: %d\n", region.end_page() - region.start_page());

    for (auto i = region.start_page(); i < region.end_page(); i++)
    {
        printf("\tInvalidating page %d (%d textures)\n", i, m_pages[i].size());
        for (auto& tex : m_pages[i])
            tex->invalidate();
    }
}

// flushes the entire cache
void TextureCache::flush()
{
    // TODO
    // smart pointers
    for (const auto& entry : m_cached_tex)
        delete entry.second;

    for (auto& page_textures : m_pages)
        page_textures.clear();

    m_cached_tex.clear();
}

Texture::Texture(TEX0 tex0, TEXA_REG texa) :
    m_tex0(tex0), m_texa(texa)
{
    m_size = (size_t)m_tex0.tex_width * (size_t)m_tex0.tex_height;
    m_data = std::make_unique<uint32_t[]>(m_size);
}

bool Texture::map(uint32_t** buff)
{
    *buff = m_data.get();
    return true;
}

// not needed but common idiom
void Texture::unmap()
{

}

bool Texture::save(std::string name)
{
    const size_t width = m_tex0.tex_width;
    const size_t height = m_tex0.tex_height;

    std::vector<unsigned char> image(width * height * 4);
    std::vector<unsigned char> image_alpha(width * height * 4);

    for (auto y = 0; y < height; y++)
    for (auto x = 0; x < width; x++)
    {
        const size_t y_offset = width * 4 * y;
        const size_t x_offset = (size_t)x * 4;

        const size_t image_index = y_offset + x_offset;
        const size_t data_index = width * y + x;

        image[image_index + 0] = m_data[data_index];
        image[image_index + 1] = m_data[data_index] >> 8;
        image[image_index + 2] = m_data[data_index] >> 16;
        image[image_index + 3] = 255;

        image_alpha[image_index + 0] = m_data[data_index] >> 24;
        image_alpha[image_index + 1] = m_data[data_index] >> 24;
        image_alpha[image_index + 2] = m_data[data_index] >> 24;
        image_alpha[image_index + 3] = 255;
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
        printf("[TEXCACHE] Error saving color image %x: %s\n", err, lodepng_error_text(err));
        return false;
    }

    err = lodepng::encode(ss.str() + "_alpha.png", image_alpha, width, height);
    if (err)
    {
        printf("[TEXCACHE] Error saving alpha image %x: %s\n", err, lodepng_error_text(err));
        return false;
    }

    return true;
}