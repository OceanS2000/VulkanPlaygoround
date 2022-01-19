//
// Created by ocean on 1/18/22.
//

#ifndef VULKANPLAYGROUND_SRC_BASEENGINE_PRESENTER_HPP
#define VULKANPLAYGROUND_SRC_BASEENGINE_PRESENTER_HPP

#include <cstdint>

#include <vulkan/vulkan.hpp>

namespace VulkanPlayground
{
	class BaseEngine;

	class Presenter
	{
	public:
		Presenter(const BaseEngine& engine, Presenter* oldPresenter);
		~Presenter();

		Presenter(const Presenter&) = delete;
		Presenter(Presenter&&) = delete;
		Presenter& operator=(const Presenter&) = delete;
		Presenter& operator=(Presenter&&) = delete;

	private:
		const BaseEngine& engine_;
		const vk::Device& device_;

		vk::SwapchainKHR swapchain_;
		std::vector<vk::Image> images_;
		std::vector<vk::ImageView> imageViews_;

		vk::Extent2D extent_;
		vk::Format format_;

		friend BaseEngine;
	};
}

#endif //VULKANPLAYGROUND_SRC_BASEENGINE_PRESENTER_HPP
