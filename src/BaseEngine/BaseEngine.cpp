//
// Created by ocean on 1/18/22.
//

#include "BaseEngine.hpp"

#include <array>
#include <chrono>
#include <thread>
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
	device_.destroy(renderPass_);
	device_.destroy(graphicsCmdPool_);
	vmaDestroyAllocator(vma_);
	device_.destroy();
	instance_.destroy(surface_);
	instance_.destroy(debugMsg_);
	instance_.destroy();

	if (window_)
		SDL_DestroyWindow(window_);
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
	auto targettime = 16.667ms;

	while (true) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				return;
			case SDL_WINDOWEVENT:
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						winSize_ = {event.window.data1, event.window.data2};
						initPresenter();
						auto time = std::chrono::steady_clock::now();
						std::this_thread::sleep_for(targettime - (time - lastframe));
						lastframe = std::chrono::steady_clock::now();
						continue;
				}
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_DOWN:
						arrowKey_[0] = true;
						break;
					case SDLK_UP:
						arrowKey_[1] = true;
						break;
					case SDLK_LEFT:
						arrowKey_[2] = true;
						break;
					case SDLK_RIGHT:
						arrowKey_[3] = true;
						break;
				}
				break;
			case SDL_KEYUP:
				switch (event.key.keysym.sym) {
					case SDLK_DOWN:
						arrowKey_[0] = false;
						break;
					case SDLK_UP:
						arrowKey_[1] = false;
						break;
					case SDLK_LEFT:
						arrowKey_[2] = false;
						break;
					case SDLK_RIGHT:
						arrowKey_[3] = false;
						break;
				}
				break;
			}
		}

		if (arrowKey_[0]) viewCenter_[1] -= 5;
		if (arrowKey_[1]) viewCenter_[1] += 5;
		if (arrowKey_[2]) viewCenter_[0] += 5;
		if (arrowKey_[3]) viewCenter_[0] -= 5;
		viewCenter_[0] = std::clamp(viewCenter_[0], -winSize_[0], winSize_[0]);
		viewCenter_[1] = std::clamp(viewCenter_[1], -winSize_[1], winSize_[1]);

		auto time = std::chrono::steady_clock::now();
		std::this_thread::sleep_for(targettime - (time - lastframe));
		lastframe = std::chrono::steady_clock::now();

		if (presenter_->Run()){
			auto windowflags = SDL_GetWindowFlags(window_);
			if (windowflags & SDL_WINDOW_MINIMIZED) continue;
			initPresenter();
		}
	}
}

}
