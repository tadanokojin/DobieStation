//#include "texture.hpp"
//#include "video/vulkanutils.hpp"
//
//Texture::Texture(VkDevice device, TEX0 tex0, TEX1 tex1)
//:	texture_TEX0(tex0),
//	texture_TEX1(tex1)
//{
//	uint16_t w = tex0.tex_width;
//	uint16_t h = tex0.tex_height;
//
//	VkImageCreateInfo info = {};
//	info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//	info.imageType = VK_IMAGE_TYPE_2D;
//	info.mipLevels = 1;
//	info.arrayLayers = 1;
//	info.format = VK_FORMAT_R8G8B8A8_UINT;
//	info.tiling = VK_IMAGE_TILING_OPTIMAL;
//	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//
//	info.extent.width = static_cast<uint32_t>(w);
//	info.extent.height = static_cast<uint32_t>(h);
//	info.extent.depth = 1;
//
//	VkResult res = vkCreateImage(device, &info, nullptr, &texture);
//	if (res != VK_SUCCESS)
//		VK_PANIC(res, "vkCreateImage");
//
//	VkMemoryRequirements memory_requirements;
//	vkGetImageMemoryRequirements(device, texture, &memory_requirements);
//
//	VkMemoryAllocateInfo mem_info = {};
//	mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//	mem_info.allocationSize = memory_requirements.size;
//	//mem_info.memoryTypeIndex = 
//
//	res = vkAllocateMemory(device, &mem_info, nullptr, &memory);
//	if (res != VK_SUCCESS)
//		VK_PANIC(res, "vkAllocateMemory");
//
//	vkBindImageMemory(device, texture, memory, 0);
//}