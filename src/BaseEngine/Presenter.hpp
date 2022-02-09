//
// Created by ocean on 1/18/22.
//

#ifndef VULKANPLAYGROUND_SRC_BASEENGINE_PRESENTER_HPP
#define VULKANPLAYGROUND_SRC_BASEENGINE_PRESENTER_HPP

#include <cstdint>

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include "Vertex.hpp"

namespace VulkanPlayground
{
	class BaseEngine;

	class Presenter
	{
	public:
		Presenter(const BaseEngine& engine, Presenter* oldPresenter);
		virtual ~Presenter();

		Presenter(const Presenter&) = delete;
		Presenter(Presenter&&) = delete;
		Presenter& operator=(const Presenter&) = delete;
		Presenter& operator=(Presenter&&) = delete;

		bool Run();

	private:
		const BaseEngine& engine_;
		const vk::Device& device_;

		vk::SwapchainKHR swapchain_;
		std::vector<vk::Image> images_;
		std::vector<vk::ImageView> imageViews_;
		std::vector<vk::Framebuffer> swapchainFramebuffer_;

		vk::RenderPass renderPass_;
		vk::PipelineLayout pipelineLayout_;
		vk::Pipeline pipeline_;

		vk::Buffer vertexBuffer_;
		VmaAllocation vertexBufferAlloc_;

		vk::Buffer indexBuffer_;
		VmaAllocation indexBufferAlloc_;

		std::array<vk::CommandBuffer, 2> cmdBuffer_;

		vk::Extent2D extent_;

		unsigned int frameCnt;

		friend BaseEngine;
	};
}

#endif //VULKANPLAYGROUND_SRC_BASEENGINE_PRESENTER_HPP
