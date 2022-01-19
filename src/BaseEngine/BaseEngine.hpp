//
// Created by ocean on 1/18/22.
//

#ifndef VULKANPLAYGROUND_SRC_BASEENGINE_BASEENGINE_HPP
#define VULKANPLAYGROUND_SRC_BASEENGINE_BASEENGINE_HPP

#include <vector>

#include <vulkan/vulkan.hpp>
#include <SDL.h>

namespace VulkanPlayground
{
	constexpr uint32_t badQF = ~(0u);
	class Presenter;

	class BaseEngine
	{
	public:
		BaseEngine();
		~BaseEngine();

		BaseEngine(const BaseEngine&) = delete;
		BaseEngine(BaseEngine&&) = delete;
		BaseEngine& operator=(const BaseEngine&) = delete;
		BaseEngine& operator=(BaseEngine&&) = delete;

		void run();

		void ChooseGPU(const std::function<int(const vk::PhysicalDevice&)>&);
		void initPresenter();

	private:
		SDL_Window * window_;
		vk::Instance instance_;
		vk::DebugUtilsMessengerEXT debugMsg_;
		vk::SurfaceKHR surface_;

		vk::PhysicalDevice chosenGPU_;
		vk::Device device_;
		uint32_t graphicsQF_ = badQF;
		vk::Queue graphicsQ_;
		vk::CommandPool cmdPool_;

		std::unique_ptr<Presenter> presenter_;

		friend Presenter;
	};
}


#endif //VULKANPLAYGROUND_SRC_BASEENGINE_BASEENGINE_HPP
