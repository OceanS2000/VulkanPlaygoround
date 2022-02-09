//
// Created by ocean on 1/18/22.
//

#ifndef VULKANPLAYGROUND_SRC_BASEENGINE_BASEENGINE_HPP
#define VULKANPLAYGROUND_SRC_BASEENGINE_BASEENGINE_HPP

#include <array>
#include <bitset>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <SDL.h>
#include <vk_mem_alloc.h>

#include "ImgSyncer.hpp"
#include "TextureModule.hpp"

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
		vk::SurfaceFormatKHR surfaceFmt_;

		vk::PhysicalDevice chosenGPU_;
		vk::Device device_;
		uint32_t graphicsQF_ = badQF;
		vk::Queue graphicsQ_;
		vk::CommandPool graphicsCmdPool_;
		vk::RenderPass renderPass_;

		uint32_t imageCount_;
		std::vector<ImgSyncer> syncObjs_;

		VmaAllocator vma_;

		std::vector<TextureModule> texture_;
		vk::Sampler sampler_;

		vk::Buffer view_;
		VmaAllocation viewAlloc_;

		vk::DescriptorPool descriptorPool_;
		vk::DescriptorSetLayout globalDescriptorLayout_;
		std::vector<vk::DescriptorSet> globalDescriptors_;

		vk::PipelineLayout globalPipelineLayout_;

		std::unique_ptr<Presenter> presenter_;

		std::bitset<SDL_NUM_SCANCODES> arrowKey_;
		std::array<int, 2> winSize_ = {640, 480};
		std::array<int, 2> modelCenter_ = {0, 0};

		friend Presenter;
	};
}


#endif //VULKANPLAYGROUND_SRC_BASEENGINE_BASEENGINE_HPP
