//
// Created by ocean on 2/8/22.
//

#include "TextureModule.hpp"

#include <spdlog/spdlog.h>

#include "stb_image.h"

#include "OneTimeCommand.hpp"

namespace VulkanPlayground
{

namespace {

template<typename T, typename... U>
static inline constexpr auto VkInit(U&&... u)
{
	return static_cast<typename T::NativeType>(T {std::forward<U>(u)...});
}

}

TextureModule TextureModule::uploadTexture(const char *filename,
										   VmaAllocator allocator,
										   vk::Device device,
										   vk::Queue transferQueue,
										   vk::CommandPool commandPool)
{
	int w, h, n;
	const auto img = stbi_load(filename, &w, &h, &n, STBI_rgb_alpha);
	if (!img) {
		spdlog::error("Failed to load image {}: {}", filename, stbi_failure_reason());
		std::terminate();
	}

	VkBuffer staging;
	const auto textureSize = static_cast<vk::DeviceSize>(w * h * 4);
	const auto stagingBufferCreate = VkInit<vk::BufferCreateInfo>(
		vk::BufferCreateFlagBits {},
		textureSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::SharingMode::eExclusive
	);
	const VmaAllocationCreateInfo stagingAllocCreate = {
		.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = VMA_MEMORY_USAGE_CPU_ONLY,
		.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	VmaAllocation stagingAlloc;
	VmaAllocationInfo stagingAllocInfo;
	if (const auto result = vmaCreateBuffer(allocator,
					&stagingBufferCreate,
					&stagingAllocCreate,
					&staging,
					&stagingAlloc,
					&stagingAllocInfo) != VK_SUCCESS) {
		spdlog::error("Failed to create staging buffer");
		std::terminate();
	}
	memcpy(stagingAllocInfo.pMappedData, img, w * h * 4);
	stbi_image_free(img);

	VkImage texture;
	const auto textureCreate = VkInit<vk::ImageCreateInfo>(
		vk::ImageCreateFlagBits {},
		vk::ImageType::e2D,
		vk::Format::eR8G8B8A8Srgb,
		vk::Extent3D { (uint32_t) w, (uint32_t) h, 1u },
		1u,
		1u,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
		);
	const VmaAllocationCreateInfo textureAllocCreate = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	};
	VmaAllocation textureAlloc;
	VmaAllocationInfo textureAllocInfo;
	if (const auto result = vmaCreateImage(
		allocator,
		&textureCreate,
		&textureAllocCreate,
		&texture,
		&textureAlloc,
		&textureAllocInfo) != VK_SUCCESS) {
		spdlog::error("Failed to create texture buffer");
		vmaFreeMemory(allocator, stagingAlloc);
		std::terminate();
	}

	std::array<vk::BufferImageCopy, 1> region;
	region.front().setBufferOffset(0).setBufferRowLength(0).setBufferImageHeight(0)
		.setImageSubresource({
			vk::ImageAspectFlagBits::eColor,
			0, 0, 1
		}).setImageExtent({(uint32_t) w, (uint32_t) h, 1u})
		.setImageOffset({0, 0, 0});

	std::array<vk::ImageMemoryBarrier, 1> barrier = {
		{
			{
				vk::AccessFlags {}, vk::AccessFlagBits::eTransferWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				texture, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
			}}};
	std::array<vk::ImageMemoryBarrier, 1> barrier2 = {
		{
			{
				vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
				VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
				texture, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
			}
		}
	};

	vk::CommandBufferAllocateInfo allocateInfo {
		commandPool,
		vk::CommandBufferLevel::ePrimary,
		1
	};
	const auto & allocated = device.allocateCommandBuffers(allocateInfo);
	const auto & cmdbuf = allocated.front();
	cmdbuf.begin(vk::CommandBufferBeginInfo {});
	cmdbuf.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags {},
		{}, {}, barrier
	);
	cmdbuf.copyBufferToImage(staging, texture, vk::ImageLayout::eTransferDstOptimal, region);
	cmdbuf.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags {},
		{}, {}, barrier2
	);
	cmdbuf.end();
	transferQueue.submit({ {{}, {}, allocated, {}} });
	transferQueue.waitIdle();

	vk::ImageViewCreateInfo viewInfo {
		{},
		texture,
		vk::ImageViewType::e2D,
		vk::Format::eR8G8B8A8Srgb,
		vk::ComponentMapping {},
		{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
	};
	auto textureView = device.createImageView(viewInfo);

	vmaDestroyBuffer(allocator, staging, stagingAlloc);
	return {.device = device, .texture = texture, .textureView = textureView, .allocator = allocator, .allocation = textureAlloc};
}

void TextureModule::destroy()
{
	device.destroy(textureView);
	vmaDestroyImage(allocator, texture, allocation);
}

}