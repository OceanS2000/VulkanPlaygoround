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
	std::sort(availGPUs.begin(), availGPUs.end(),
		[=](const auto& lhs, const auto& rhs) {
		return pref(lhs) > pref(rhs);
	});

	chosenGPU_ = vk::PhysicalDevice { availGPUs.front() };
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

	cmdPool_ = device_.createCommandPool({
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		graphicsQF_
	});
}

void BaseEngine::initPresenter() {
	presenter_ = std::make_unique<Presenter>(*this, presenter_.get());
}

}
