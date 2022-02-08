//
// Created by ocean on 2/8/22.
//

#ifndef TEXTUREMODULE_HPP
#define TEXTUREMODULE_HPP

#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

namespace VulkanPlayground
{

struct TextureModule {
	static TextureModule uploadTexture(const char* filename, VmaAllocator allocator, vk::Device device, vk::Queue transferQueue, vk::CommandPool commandPool);
	void destroy();
	const vk::Device device;
	const vk::Image texture;
	const vk::ImageView textureView;
	const VmaAllocator allocator;
	const VmaAllocation allocation;
};

}

#endif //TEXTUREMODULE_HPP
