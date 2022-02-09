//
// Created by ocean on 1/18/22.
//

#include "BaseEngine.hpp"

#include <array>
#include <chrono>
#ifdef __GLIBCXX__
#include <bits/this_thread_sleep.h>
#else
#include <thread>
#endif
#include <vector>
#include <stdexcept>

#include <spdlog/spdlog.h>
#include <SDL_vulkan.h>

#include "Presenter.hpp"
#include "Debug.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace VulkanPlayground
{

BaseEngine::BaseEngine()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
		spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
		std::terminate();
	}
	if (SDL_Vulkan_LoadLibrary(nullptr) < 0) {
		spdlog::error("Failed to load vulkan loader: {}", SDL_GetError());
	}

	auto vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)SDL_Vulkan_GetVkGetInstanceProcAddr();
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	window_ = SDL_CreateWindow("Hello?",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		winSize_[0], winSize_[1],
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

	if (!window_) {
		spdlog::error("Failed to create SDL window: {}", SDL_GetError());
		std::terminate();
	}

	unsigned count;
	if(!SDL_Vulkan_GetInstanceExtensions(window_, &count, nullptr)) {
		spdlog::error("Failed to query required vulkan instance extensions: {}", SDL_GetError());
		std::terminate();
	}
	std::vector<const char*> instanceExtensions(count);
	SDL_Vulkan_GetInstanceExtensions(window_, &count, instanceExtensions.data());

	// TODO: Make GPU debug optional
	instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	std::array<const char*, 1> explicitLayers { "VK_LAYER_KHRONOS_validation" };

	vk::ApplicationInfo appInfo(
		"Hello?", VK_MAKE_VERSION(0, 1, 0),
		"Hand-oping", VK_MAKE_VERSION(0, 1, 0),
		VK_API_VERSION_1_2
		);

	using enum vk::DebugUtilsMessageSeverityFlagBitsEXT;
	using enum vk::DebugUtilsMessageTypeFlagBitsEXT;

	vk::StructureChain instance {
		vk::InstanceCreateInfo {
			{},
			&appInfo,
			explicitLayers,
			instanceExtensions
		},
		vk::DebugUtilsMessengerCreateInfoEXT {
			{},
			eError | eWarning | eInfo,// | eVerbose,
			eGeneral | eValidation | ePerformance,
			Debug::VulkanDebugCallback
		}
	};

	instance_ = vk::createInstance(instance.get());
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_);

	// template type inference in C++ is awful
	debugMsg_ = instance_.createDebugUtilsMessengerEXT(instance.get<vk::DebugUtilsMessengerCreateInfoEXT>());

	VkSurfaceKHR SDLSurface; // Workaround that SDL only accept C vulkan construct
	if(!SDL_Vulkan_CreateSurface(window_, instance_, &SDLSurface)) {
		spdlog::error("Failed to create drawable surface: {}", SDL_GetError());
		std::terminate();
	}
	surface_ = SDLSurface;
}

BaseEngine::~BaseEngine()
{
	presenter_.reset(nullptr);
	for (auto& syncObj : syncObjs_) {
		auto [sem1, sem2, fence] = syncObj;
		device_.destroy(fence);
		device_.destroy(sem2);
		device_.destroy(sem1);
	}
	for (auto& t : texture_)
		t.destroy();
	vmaDestroyBuffer(vma_, view_, viewAlloc_);
	device_.destroy(renderPass_);
	device_.destroy(globalPipelineLayout_);
	device_.destroy(graphicsCmdPool_);
	device_.destroy(sampler_);
	device_.destroy(descriptorPool_);
	device_.destroy(globalDescriptorLayout_);
	vmaDestroyAllocator(vma_);
	device_.destroy();
	instance_.destroy(surface_);
	instance_.destroy(debugMsg_);
	instance_.destroy();

	if (window_)
		SDL_DestroyWindow(window_);
	SDL_Quit();
}

void BaseEngine::run()
{
	if (!presenter_) {
		spdlog::error("GPU to use is not set!");
		std::terminate();
	}
	SDL_Event event;

	using namespace std::chrono_literals;
	auto lastframe = std::chrono::steady_clock::now();
	constexpr auto targettime = 16.667ms;
	bool resized = false;

	while (true) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			case SDL_WINDOWEVENT:
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						winSize_ = {event.window.data1, event.window.data2};
						lastframe = std::chrono::steady_clock::now();
						resized = true;
				}
				break;
			case SDL_KEYDOWN:
				arrowKey_[event.key.keysym.scancode] = true;
				break;
			case SDL_KEYUP:
				arrowKey_[event.key.keysym.scancode] = false;
				break;
			}
		}

		auto windowflags = SDL_GetWindowFlags(window_);
		if (windowflags & SDL_WINDOW_MINIMIZED) continue;

		if (arrowKey_[SDL_SCANCODE_DOWN]) modelCenter_[1] -= 5;
		if (arrowKey_[SDL_SCANCODE_UP]) modelCenter_[1] += 5;
		if (arrowKey_[SDL_SCANCODE_LEFT]) modelCenter_[0] += 5;
		if (arrowKey_[SDL_SCANCODE_RIGHT]) modelCenter_[0] -= 5;
		modelCenter_[0] = std::clamp(modelCenter_[0], -100, 100);
		modelCenter_[1] = std::clamp(modelCenter_[1], -100, 100);

		auto now = std::chrono::steady_clock::now();
		if (resized) {
			if ((now - lastframe) < 100ms) {
				continue;
			} else {
				initPresenter();
			}
		}
		while ((now = std::chrono::steady_clock::now()) - lastframe < targettime);
		lastframe = now;

		resized = presenter_->Run();

		if (!resized) {
			std::this_thread::sleep_until(lastframe + 15.55ms);
		}
	}
}

}
