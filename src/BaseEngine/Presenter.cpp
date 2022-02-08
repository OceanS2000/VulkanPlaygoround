//
// Created by ocean on 1/18/22.
//

#include "Presenter.hpp"

#include <algorithm>
#include <array>

#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>

#include "BaseEngine.hpp"
#include "ShaderModule.hpp"

namespace VulkanPlayground
{

constexpr static std::array<Vertex, 6> defaultVertices {
	{
		{{0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}},
		{{0.0f, 0.5f}, {0.0f, 2.0f, 0.0f}},
		{{0.5f, 0.0f}, {2.0f, 0.0f, 1.0f}},
		{{0.5f, 0.0f}, {2.0f, 0.0f, 1.0f}},
		{{0.0f, 0.5f}, {0.0f, 2.0f, 0.0f}},
		{{0.5f, 0.5f}, {2.0f, 2.0f, 0.0f}},
	}
};

static glm::vec2 norView;

Presenter::Presenter(const BaseEngine& engine, Presenter* oldPresenter)
	: engine_(engine), device_(engine_.device_)
	{
		const auto& phyDevice = engine_.chosenGPU_;
		const auto& surface = engine_.surface_;
		const auto& format = engine_.surfaceFmt_;

		// Determine Present mode
		auto presentMode = vk::PresentModeKHR::eImmediate;
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

		// Determine image extent
		int w, h;
		SDL_Vulkan_GetDrawableSize(engine_.window_, &w, &h);
		extent_ = vk::Extent2D {(uint32_t)w, (uint32_t)h};
		unsigned imgCnt = engine_.imageCount_;

		const auto surfaceCap = phyDevice.getSurfaceCapabilitiesKHR(surface);
		{
			const auto &maxExt = surfaceCap.maxImageExtent;
			const auto &minExt = surfaceCap.minImageExtent;
			extent_.width = std::clamp(extent_.width, minExt.width, maxExt.width);
			extent_.height = std::clamp(extent_.height, minExt.height, maxExt.height);
		}

		// Create swapchain
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

		renderPass_ = engine.renderPass_;

		images_ = device_.getSwapchainImagesKHR(swapchain_);

		// Per-image imageview and framebuffer from swapchain
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

			std::array<vk::ImageView, 1> framebufferAttachment = {
				{ imageViews_.back() }
			};

			swapchainFramebuffer_.emplace_back(
				device_.createFramebuffer(
					{
						{},
						renderPass_,
						framebufferAttachment,
						extent_.width, extent_.height,
						1
					}));
		}

		pipelineLayout_ = engine_.globalPipelineLayout_;

		// Create Graphics Pipeline
		{
			auto vertCode = ShaderModule::readShader("assets/trig.vert.spv");
			auto fragCode = ShaderModule::readShader("assets/trig.frag.spv");
			auto vert = device_.createShaderModuleUnique(
				{ {}, vertCode });
			auto frag = device_.createShaderModuleUnique(
				{ {}, fragCode });

			vk::PipelineShaderStageCreateInfo shaderStage[] = {
				{
					{},
					vk::ShaderStageFlagBits::eVertex,
					*vert,
					"main"
				},
				{
					{},
					vk::ShaderStageFlagBits::eFragment,
					*frag,
					"main"
				}
			};

			vk::PipelineVertexInputStateCreateInfo vertexInput = {
				{}, Vertex::vertexInputBinding, Vertex::vertexInputAttribute
			};

			vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {
				{}, vk::PrimitiveTopology::eTriangleList, VK_FALSE
			};

			vk::Viewport viewport = {
				0.0f, 0.0f,
				(float)extent_.width, (float)extent_.height,
				0.0f, 1.0f
			};

			vk::Rect2D sessior = {{0, 0}, extent_};

			vk::PipelineViewportStateCreateInfo viewportState = {
				{}, viewport, sessior
			};

			vk::PipelineRasterizationStateCreateInfo rasterization;
			rasterization.setLineWidth(1.0f);

			vk::PipelineMultisampleStateCreateInfo multisample;
			multisample.setRasterizationSamples(vk::SampleCountFlagBits::e1)
				.setSampleShadingEnable(VK_FALSE);

			vk::PipelineColorBlendAttachmentState colorBlendAttachment;
			colorBlendAttachment.setBlendEnable(VK_FALSE)
			.setColorWriteMask(
				vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA
				);

			vk::PipelineColorBlendStateCreateInfo colorBlend = {
				{},
				VK_FALSE, vk::LogicOp::eCopy,
				colorBlendAttachment,
				{1.0f, 1.0f, 1.0f, 1.0f}
			};

			auto result = device_.createGraphicsPipeline(nullptr,
				{
					{},
					2, shaderStage,
					&vertexInput,
					&inputAssembly,
					nullptr,
					&viewportState,
					&rasterization,
					&multisample,
					nullptr,
					&colorBlend,
					nullptr,
					pipelineLayout_,
					renderPass_,
					0,
					VK_NULL_HANDLE,
					0
			});

			if (result.result != vk::Result::eSuccess) {
				spdlog::error("Failed to create Graphics Pipeline!");
				std::terminate();
			}
			pipeline_ = result.value;
		}

		// Allocate Vertex Buffer
		{
			using enum vk::MemoryPropertyFlagBits;
			VmaAllocationCreateInfo vmalloc = {
				.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
				.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
				.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//				.preferredFlags = (VkMemoryPropertyFlags)(eDeviceLocal),
			};

			auto bufferCreate = (VkBufferCreateInfo)vk::BufferCreateInfo {
				{},
				sizeof(Vertex) * defaultVertices.size(),
				vk::BufferUsageFlagBits::eVertexBuffer,
				vk::SharingMode::eExclusive
			};

			VkBuffer buffer;
			auto result = vmaCreateBuffer(
				engine_.vma_,
				&bufferCreate,
				&vmalloc,
				&buffer,
				&bufferAlloc_,
				&bufferAllocInfo_
				);

			if (result != VK_SUCCESS) {
				spdlog::error("Falied to allocate buffer");
				std::terminate();
			}

			vertexBuffer_ = buffer;

			memcpy(bufferAllocInfo_.pMappedData, &defaultVertices, sizeof(Vertex) * defaultVertices.size());
			vmaFlushAllocation(engine_.vma_, bufferAlloc_, bufferAllocInfo_.offset, bufferAllocInfo_.size);
		}

		// Allocate command buffer
		{
			vk::CommandBufferAllocateInfo cmdBufferAllocInfo = {
				engine_.graphicsCmdPool_,
				vk::CommandBufferLevel::ePrimary,
				2
			};
			auto result = device_.allocateCommandBuffers(
				&cmdBufferAllocInfo,
				cmdBuffer_.data()
				);
			if (result != vk::Result::eSuccess) {
				spdlog::error("Failed to allocate command buffers");
				std::terminate();
			}
		}
	}

	Presenter::~Presenter()
	{
		device_.waitIdle();
		vmaDestroyBuffer(engine_.vma_, (VkBuffer)vertexBuffer_, bufferAlloc_);
		for (auto & fb : swapchainFramebuffer_) {
			device_.destroy(fb);
		}
		device_.destroy(pipeline_);
		for (const auto& imageV: imageViews_) {
			device_.destroy(imageV);
		}
		device_.destroy(swapchain_);
	}

	bool Presenter::Run()
	{
		unsigned int theFrame = frameCnt % 2;
		auto spin = glm::vec2{0.0f, 0.0f};

		const auto& [renderComplete, imageAvailable, imageDone] = engine_.syncObjs_[theFrame];
		auto result1 = device_.waitForFences(1, &imageDone, VK_TRUE, UINT64_MAX);
		if (result1 != vk::Result::eSuccess) {
			spdlog::error("fences not working!");
			return true;
		}

		auto result2 = device_.acquireNextImageKHR(swapchain_, UINT64_MAX, imageAvailable, VK_NULL_HANDLE);

		if (result2.result != vk::Result::eSuccess) {
			if (result2.result == vk::Result::eSuboptimalKHR) {
				spdlog::info("Get suboptimal framebuffer image");
			} else if (result2.result == vk::Result::eErrorOutOfDateKHR) {
				spdlog::warn("Image out of date");
				return true;
			} else {
				spdlog::error("Image acquire error: {}", to_string(result2.result));
				std::terminate();
			}
		}
		auto curimg = result2.value;
		if (curimg != 0 && curimg != 1) {
			spdlog::warn("Incorrect assumption about iamge index");
		}

		auto& cmdbuf = cmdBuffer_[curimg % 2];
		std::array<vk::ClearValue, 1> clearColor = {
			{
				vk::ClearColorValue {
					std::array<float, 4> {0.3f, 0.3f, 0.3f, 1.0f}
				}
			}};

		const auto & viewCenter = engine_.viewCenter_;
		const auto & windowSize = engine_.winSize_;
		norView[0] = static_cast<float>(viewCenter[0]) / static_cast<float>(windowSize[0]);
		norView[1] = static_cast<float>(viewCenter[1]) / static_cast<float>(windowSize[1]);

		cmdbuf.begin(vk::CommandBufferBeginInfo {});
		cmdbuf.beginRenderPass(
			{
				renderPass_,
				swapchainFramebuffer_[curimg],
				{{0, 0}, extent_},
				clearColor
			},
			vk::SubpassContents::eInline
		);
		cmdbuf.bindPipeline(
			vk::PipelineBindPoint::eGraphics,
			pipeline_
			);
		std::array<vk::Buffer, 1> vertexBuffers = {{ vertexBuffer_ }};
		std::array<vk::DeviceSize, 1> offsets = {{ 0 }};
		cmdbuf.bindVertexBuffers(0, vertexBuffers, offsets);
		cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout_, 0, 1, &engine_.globalDescriptors_[curimg%2], 0, nullptr);
		cmdbuf.pushConstants(pipelineLayout_, vk::ShaderStageFlagBits::eVertex, 0, sizeof(norView), &norView);
		cmdbuf.draw(6, 1, 0, 0);
		cmdbuf.endRenderPass();
		cmdbuf.end();

		std::array<vk::Semaphore, 1> waitSem = {{ imageAvailable }};
		std::array<vk::PipelineStageFlags, 1> waitStage = {{vk::PipelineStageFlagBits::eColorAttachmentOutput}};
		std::array<vk::Semaphore, 1> signalS = {{ renderComplete }};
		std::array<vk::CommandBuffer, 1> submitCmd = {{cmdbuf}};
		std::array<vk::SubmitInfo, 1> submit = {
			{{
				 waitSem,
				 waitStage,
				 submitCmd,
				 signalS
			 }}
		};
		std::array<vk::Fence, 1> fenceReset = {{imageDone}};
		device_.resetFences(fenceReset);
		engine_.graphicsQ_.submit(submit, imageDone);
		frameCnt++;
		try {
			result1 = engine_.graphicsQ_.presentKHR(
				{
					1, &renderComplete,
					1, &swapchain_,
					&curimg, nullptr
				});
			if (result1 != vk::Result::eSuccess) {
				spdlog::info("Get suboptimal result");
				return true;
			}
		} catch (vk::OutOfDateKHRError& e) {
			spdlog::info("Get out of date image");
			return true;
		}

		return false;
	}
}
