//
// Created by ocean on 1/18/22.
//

#include "Debug.hpp"

#include <spdlog/spdlog.h>

VKAPI_ATTR VkBool32 VKAPI_CALL Debug::VulkanDebugCallback(
	const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	const VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* const pCallbackData,
	void* const pUserData) {

	// TODO: Probably come up with some use for pUserData
	(void)pUserData;
	auto severity = (vk::DebugUtilsMessageSeverityFlagBitsEXT)messageSeverity;
	auto type = (vk::DebugUtilsMessageTypeFlagsEXT)messageType;

	using enum spdlog::level::level_enum;
	using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;

	spdlog::level::level_enum level;
	switch(severity) {
	case eError:
		level = err;
		break;
	case eWarning:
		level = warn;
		break;
	case eInfo:
		level = info;
		break;
	case eVerbose:
		level = debug;
		break;
	}

	spdlog::log(level, "VALIDATION: {} {}", to_string(type), pCallbackData->pMessage);
	if (pCallbackData->objectCount > 0 && severity > eInfo) {
		spdlog::log(level, " Objects({}):", pCallbackData->objectCount);
		for (uint32_t i = 0; i < pCallbackData->objectCount; ++i) {
			const auto& obj = pCallbackData->pObjects[i];
			spdlog::log(level,
				"\t- Object[{}]: Type: {}, Handle: {}, Name: {}",
				i,
				to_string(static_cast<vk::ObjectType>(obj.objectType)),
				reinterpret_cast<void*>(obj.objectHandle),
				obj.pObjectName ? obj.pObjectName : "none");
		}
	}

	return VK_FALSE;
}
