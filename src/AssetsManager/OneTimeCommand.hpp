//
// Created by ocean on 2/8/22.
//

#ifndef ONETIMECOMMAND_HPP
#define ONETIMECOMMAND_HPP

#include <vulkan/vulkan.hpp>

namespace VulkanPlayground
{

class OneTimeCommand
{
public:
	OneTimeCommand(vk::Device device, vk::Queue queue, vk::CommandPool commandPool);
	~OneTimeCommand();
	void flush();

	vk::CommandBuffer const & get() const {
		return cmdBuf_;
	}

private:
	vk::Device device_;
	vk::Queue queue_;
	vk::CommandPool pool_;
	vk::CommandBuffer cmdBuf_;
};

class OneTimeCommandBuffer : public vk::CommandBuffer {
public:
	static OneTimeCommandBuffer begin(vk::Device device, vk::Queue queue, vk::CommandPool commandPool);
	void flush() const;

private:
	OneTimeCommandBuffer(vk::Device d, vk::Queue q, vk::CommandPool cp, vk::CommandBuffer cb);

	const vk::Device device_;
	const vk::Queue queue_;
	const vk::CommandPool pool_;
};

}

#endif //ONETIMECOMMAND_HPP
