#include "BaseEngine.hpp"

#include <spdlog/spdlog.h>

int main()
{
	VulkanPlayground::BaseEngine baseEngine;

	baseEngine.ChooseGPU([](const vk::PhysicalDevice& device) {
		int score = 0;
		const auto & prop = device.getProperties2();
		if (prop.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			score += 10;

		const auto & extensions = device.enumerateDeviceExtensionProperties();
		for (const auto & ext : extensions) {
			if (std::strcmp(ext.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0)
				score += 20;
		}
		return score;
	});
	baseEngine.initPresenter();
	baseEngine.run();
	return 0;
}
