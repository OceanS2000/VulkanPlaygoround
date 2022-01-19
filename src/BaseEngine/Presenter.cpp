//
// Created by ocean on 1/18/22.
//

#include "Presenter.hpp"

#include <algorithm>
#include <array>

#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

#include "BaseEngine.hpp"

namespace VulkanPlayground
{
	Presenter::Presenter(const BaseEngine& engine, Presenter* oldPresenter)
	: engine_(engine), device_(engine_.device_)
	{
		const auto& phyDevice = engine_.chosenGPU_;
		const auto& surface = engine_.surface_;

		// Determine Present mode
		auto presentMode = vk::PresentModeKHR::eMailbox;
		{
			using enum vk::PresentModeKHR;
			const auto& presentSup = phyDevice.getSurfacePresentModesKHR(surface);
			if (std::find(presentSup.begin(), presentSup.end(), presentMode) == presentSup.end()) {
				presentMode = eFifoRelaxed;
				if (std::find(presentSup.begin(), presentSup.end(), presentMode) == presentSup.end()) {
					presentMode = eFifo;
				}
			}
			spdlog::debug("Using present mode {}", to_string(presentMode));
		}

		// Determine Surface Format
		vk::SurfaceFormatKHR format { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
		{
			using enum vk::Format;
			using enum vk::ColorSpaceKHR;
			const auto formatSups = phyDevice.getSurfaceFormatsKHR(surface);
#if 0
			spdlog::info("Following format available:");
			for (const auto& formatSup : formatSups) {
				spdlog::info("\t[{} | {}]", to_string(formatSup.format), to_string(formatSup.colorSpace));
			}
#endif
			if (std::find(formatSups.begin(), formatSups.end(), format) == formatSups.end()) {
				spdlog::error("The GPU does not support specified surface format");
			}

			format_ = eB8G8R8A8Unorm;
		}

		// Determine image extent and count
		const auto surfaceCap = phyDevice.getSurfaceCapabilitiesKHR(surface);

		int w, h;
		SDL_Vulkan_GetDrawableSize(engine_.window_, &w, &h);
		extent_ = vk::Extent2D {(uint32_t)w, (uint32_t)h};
		const auto& maxExt = surfaceCap.maxImageExtent;
		const auto& minExt = surfaceCap.minImageExtent;
		extent_.width = std::max(minExt.width, std::min(maxExt.width, extent_.width));
		extent_.height = std::max(minExt.height, std::min(maxExt.height, extent_.height));

		if (surfaceCap.maxImageCount < 2) {
			spdlog::error("The device does not support for more than two images!");
			std::terminate();
		}
		unsigned imgCnt = std::max(2u, surfaceCap.minImageCount);

		{
			using enum vk::ImageUsageFlagBits;
			using enum vk::SharingMode;
			using enum vk::CompositeAlphaFlagBitsKHR;

			std::array<uint32_t, 1> queueFamilies { engine_.graphicsQF_ };

			swapchain_ = device_.createSwapchainKHR({
				{},
				surface,
				imgCnt,
				format.format,
				format.colorSpace,
				extent_,
				1,
				eColorAttachment | eTransferDst,
				eExclusive,
				queueFamilies,
				surfaceCap.currentTransform,
				eOpaque,
				presentMode,
				VK_TRUE,
				(oldPresenter) ? (oldPresenter->swapchain_) : nullptr
			});
		}

		images_ = device_.getSwapchainImagesKHR(swapchain_);

		for (const auto& image :images_) {
			imageViews_.emplace_back(
				device_.createImageView({
					{},
					image,
					vk::ImageViewType::e2D,
					format.format,
					{
						vk::ComponentSwizzle::eIdentity,
						vk::ComponentSwizzle::eIdentity,
						vk::ComponentSwizzle::eIdentity,
						vk::ComponentSwizzle::eIdentity
					},
					{
						vk::ImageAspectFlagBits::eColor,
						0, 1,
						0, 1
					}
			}));
		}

		// Now all the ground has been laid down, we can use Vookoo now!
	}

	Presenter::~Presenter()
	{
		for (const auto& imageV: imageViews_) {
			device_.destroy(imageV);
		}
		device_.destroy(swapchain_);
	}
}
