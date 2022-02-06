//
// Created by ocean on 1/18/22.
//

#include "BaseEngine.hpp"

#include <array>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include "Presenter.hpp"

namespace VulkanPlayground
{

void BaseEngine::ChooseGPU(const std::function<int(const vk::PhysicalDevice&)>& pref) {
	auto availGPUs = instance_.enumeratePhysicalDevices();
	if (availGPUs.empty()) {
		spdlog::error("No available PhysicalDevice");
		std::terminate();
	}

	unsigned best = 0; int bestScore = -1;
	for (unsigned i = 0; i < availGPUs.size(); i++) {
		int score = pref(availGPUs[i]);
		if (bestScore < score) {
			best = i;
		}
	}

	chosenGPU_ = vk::PhysicalDevice { availGPUs[best] };
	const auto& bestGPU = chosenGPU_;

	const auto& GPUProp = bestGPU.getProperties();
	spdlog::info("Chosen best GPU [{}]: {} with queue families available",
		GPUProp.deviceID, GPUProp.deviceName);

	const auto& queueFamilies = bestGPU.getQueueFamilyProperties();
	for (size_t i = 0; i < queueFamilies.size(); i++) {
		const auto& family = queueFamilies[i];
		spdlog::info("\t{}\t{}", family.queueCount, to_string(family.queueFlags));

		if (family.queueFlags & vk::QueueFlagBits::eGraphics)
			if (bestGPU.getSurfaceSupportKHR(i, surface_))
				graphicsQF_ = i;
	}

	if (graphicsQF_ == badQF) {
		spdlog::error("No presentable queue is found!");
		std::terminate();
	}

	float priority = 1.0f;

	// TODO: Make other GPU queue configurable (e.g. eCompute)
	std::array<vk::DeviceQueueCreateInfo, 1> queues {
		vk::DeviceQueueCreateInfo {{}, graphicsQF_, 1, &priority}
	};
	// TODO: Make GPU debug optional
	std::array<const char*, 1> explicitLayers { "VK_LAYER_KHRONOS_validation" };
	// TODO: Make enabled device extension configurable
	std::array<const char*, 1> deviceExtensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	device_ = bestGPU.createDevice({
		{},
		queues,
		explicitLayers,
		deviceExtensions
	});
	graphicsQ_ = device_.getQueue(graphicsQF_, 0);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(device_);

	graphicsCmdPool_ = device_.createCommandPool({
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		graphicsQF_
	});

	{
		VmaVulkanFunctions vulkanFunc = {
			.vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr,
			.vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr
		};

		VmaAllocatorCreateInfo vmaCreat = {
			.physicalDevice = chosenGPU_,
			.device = device_,
			.pVulkanFunctions = &vulkanFunc,
			.instance = instance_,
			.vulkanApiVersion = VK_API_VERSION_1_2
		};

		const auto res = vmaCreateAllocator(&vmaCreat, &vma_);
		if (res != VK_SUCCESS) {
			spdlog::error("Failed to create memory allocator");
			std::terminate();
		}
	}

	for (auto& syncObj : syncObjs_) {
		syncObj = {
			.renderComplete = device_.createSemaphore({}),
			.imageAvailable = device_.createSemaphore({}),
			.imageDone = device_.createFence({vk::FenceCreateFlagBits::eSignaled})
		};
	}


	// Determine Surface Format
	surfaceFmt_ = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	{
		using enum vk::Format;
		using enum vk::ColorSpaceKHR;
		const auto formatSups = chosenGPU_.getSurfaceFormatsKHR(surface_);
#if 0
		spdlog::info("Following format available:");
			for (const auto& formatSup : formatSups) {
				spdlog::info("\t[{} | {}]", to_string(formatSup.format), to_string(formatSup.colorSpace));
			}
#endif
		if (std::find(formatSups.begin(), formatSups.end(), surfaceFmt_) == formatSups.end()) {
			spdlog::error("The GPU does not support specified surface format");
		}
	}

	// Create Renderpass
	{
		std::array<vk::AttachmentDescription,1> attachment {
			{
				{
					{},
					surfaceFmt_.format,
					vk::SampleCountFlagBits::e1,
					vk::AttachmentLoadOp::eClear,
					vk::AttachmentStoreOp::eStore,
					vk::AttachmentLoadOp::eDontCare,
					vk::AttachmentStoreOp::eDontCare,
					vk::ImageLayout::eUndefined,
					vk::ImageLayout::ePresentSrcKHR}
			}};

		std::array<vk::AttachmentReference, 1> colorAttachRef = {
			{
				{
					0,
					vk::ImageLayout::eColorAttachmentOptimal
				}
			}};

		std::array<vk::SubpassDescription, 1> subpass = {
			{
				{
					{}, vk::PipelineBindPoint::eGraphics,
					nullptr,
					colorAttachRef
				}
			}};

		using enum vk::PipelineStageFlagBits;
		std::array<vk::SubpassDependency, 1> dependency = {
			{
				{
					VK_SUBPASS_EXTERNAL, 0,
					eColorAttachmentOutput, eColorAttachmentOutput,
					{}, vk::AccessFlagBits::eColorAttachmentWrite,
					{}
				}
			}};

		renderPass_ = device_.createRenderPass(
			{
				{}, attachment, subpass, dependency
			});
	}

	initPresenter();
}

void BaseEngine::initPresenter() {
	presenter_ = std::make_unique<Presenter>(*this, presenter_.get());
}

}
