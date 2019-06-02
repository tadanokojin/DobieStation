//#ifndef TEXTURE_HPP
//#define TEXTURE_HPP
//
//#include <cstdint>
//
//#include "gsthread.hpp"
//#include "video/vulkancontext.hpp"
//
//enum PSM
//{
//	CT32 = 0x0,
//	CT24 = 0x1,
//	CT16 = 0x2,
//	CT16S = 0xa,
//	T8 = 0x13,
//	T4 = 0x14,
//	T8H = 0x1b,
//	T4HL = 0x24,
//	T4HH = 0x2c,
//	Z32 = 0x30,
//	Z24 = 0x31,
//	Z16 = 0x32,
//	Z16S = 0x3a
//};
//
//class Texture
//{
//private:
//	TEX0 texture_TEX0;
//	TEX1 texture_TEX1;
//
//	PSM format;
//	uint32_t* data;
//
//	VkImage texture;
//	VkDeviceMemory memory;
//
//	int width, height;
//	bool bilinear;
//public:
//	Texture(VkDevice device, const TEX0 tex0, const TEX1 tex1);
//
//	uint16_t get_width() const { return width; };
//	uint16_t get_height() const { return height; };
//
//	VkImage get_texture() const { return texture; };
//};
//
//#endif