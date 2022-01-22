//
// Created by ocean on 1/22/22.
//

#include <vulkan/vulkan.hpp>

namespace VulkanPlayground
{


constexpr vk::AttachmentDescription dfltColorAttachment = {
	{},
	vk::Format::eB8G8R8A8Unorm,
	vk::SampleCountFlagBits::e1,
	vk::AttachmentLoadOp::eClear,
	vk::AttachmentStoreOp::eStore,
	vk::AttachmentLoadOp::eDontCare,
	vk::AttachmentStoreOp::eDontCare,
	vk::ImageLayout::eUndefined,
	vk::ImageLayout::eColorAttachmentOptimal
};

constexpr vk::AttachmentReference dfltColorAttachRef = {
	0,
	vk::ImageLayout::eColorAttachmentOptimal
};

constexpr vk::SubpassDescription dfltSubpass = {
	{},
	vk::PipelineBindPoint::eGraphics,
	0, nullptr,
	1, &dfltColorAttachRef
};

}