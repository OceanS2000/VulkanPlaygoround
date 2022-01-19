//
// Created by ocean on 1/18/22.
//

#ifndef VULKANPLAYGROUND_SRC_BASEENGINE_DEBUG_HPP
#define VULKANPLAYGROUND_SRC_BASEENGINE_DEBUG_HPP

#include <vulkan/vulkan.hpp>

namespace Debug
{
	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
		const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		const VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* const pCallbackData,
		void* const pUserData);
}

#endif //VULKANPLAYGROUND_SRC_BASEENGINE_DEBUG_HPP
