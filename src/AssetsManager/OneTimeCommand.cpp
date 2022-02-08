//
// Created by ocean on 2/8/22.
//

#include "OneTimeCommand.hpp"

#include <array>

namespace VulkanPlayground
{

OneTimeCommand::OneTimeCommand(vk::Device device, vk::Queue queue, vk::CommandPool commandPool) :
	device_(device), queue_(queue), pool_(commandPool)
{
	vk::CommandBufferAllocateInfo allocateInfo {
		commandPool,
		vk::CommandBufferLevel::ePrimary,
		1
	};

	const auto & allocated = device.allocateCommandBuffers(allocateInfo);
	cmdBuf_ = allocated.front();
	cmdBuf_.begin(vk::CommandBufferBeginInfo {});
}

void OneTimeCommand::flush()
{
	cmdBuf_.end();
	std::array<vk::SubmitInfo, 1> submitInfo;
	submitInfo[0].setCommandBufferCount(1)
		.setPCommandBuffers(&cmdBuf_);
	queue_.submit(submitInfo);
	queue_.waitIdle();
	device_.free(pool_, 1, &cmdBuf_);
	cmdBuf_ = nullptr;
}

OneTimeCommand::~OneTimeCommand()
{
	if (cmdBuf_) device_.free(pool_, 1, &cmdBuf_);
}

OneTimeCommandBuffer OneTimeCommandBuffer::begin(
	vk::Device device,
	vk::Queue queue,
	vk::CommandPool commandPool)
{
	vk::CommandBufferAllocateInfo allocateInfo {
		commandPool,
		vk::CommandBufferLevel::ePrimary,
		1
	};

	const auto & allocated = device.allocateCommandBuffers(allocateInfo);
	const auto & cmdbuf = allocated.front();
	cmdbuf.begin(vk::CommandBufferBeginInfo {});
	return {device, queue, commandPool, cmdbuf};
}

void OneTimeCommandBuffer::flush() const
{
	this->end();
	std::array<vk::SubmitInfo, 1> submitInfo;
	submitInfo[0].setCommandBufferCount(1)
		.setPCommandBuffers(this);
	queue_.submit(submitInfo);
	queue_.waitIdle();
	device_.free(pool_, 1, this);
}

OneTimeCommandBuffer::OneTimeCommandBuffer(vk::Device d, vk::Queue q, vk::CommandPool cp, vk::CommandBuffer cb)
	: vk::CommandBuffer(cb), device_(d), queue_(q), pool_(cp)
{}

}