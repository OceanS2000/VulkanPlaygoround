//
// Created by ocean on 1/18/22.
//

#include "Presenter.hpp"

#include <algorithm>
#include <array>

#include <SDL_vulkan.h>
#include <spdlog/spdlog.h>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "BaseEngine.hpp"
#include "ShaderModule.hpp"

namespace VulkanPlayground
{

constexpr static std::array<Vertex, 4> defaultVertices {
	{
		{{-0.5f, -0.5f}, {2.0f, 2.0f, 1.0f}},
		{{-0.5f,  0.5f}, {2.0f, 0.0f, 1.0f}},
		{{ 0.5f, -0.5f}, {0.0f, 2.0f, 1.0f}},
		{{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
	}
};

constexpr static std::array<uint32_t, 6> defaultIndexes {
	0, 1, 2, 2, 1, 3
};

template<typename T, glm::qualifier Q>
constexpr inline glm::mat<4, 4, T, Q> lookat(glm::vec<3, T, Q> const& eye, glm::vec<3, T, Q> const& center, glm::vec<3, T, Q> const& up)
{
	using namespace glm;
	vec<3, T, Q> const f(normalize(center - eye));
	vec<3, T, Q> const s(normalize(cross(f, up)));
	vec<3, T, Q> const u(cross(s, f));

	mat<4, 4, T, Q> Result(1);
	Result[0][0] = s.x;
	Result[1][0] = s.y;
	Result[2][0] = s.z;
	Result[0][1] =-u.x;
	Result[1][1] =-u.y;
	Result[2][1] =-u.z;
	Result[0][2] = f.x;
	Result[1][2] = f.y;
	Result[2][2] = f.z;
	Result[3][0] =-dot(s, eye);
	Result[3][1] = dot(u, eye);
	Result[3][2] =-dot(f, eye);
	return Result;
}

template<typename T>
constexpr inline glm::mat<4, 4, T, glm::defaultp> perspect(T fovy, T aspect, T zNear, T zFar)
{
	using namespace glm;
	assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

	T const tanHalfFovy = tan(radians(fovy) / static_cast<T>(2));

	mat<4, 4, T, defaultp> Result(static_cast<T>(0));
	Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
	Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
	Result[2][2] = zFar / (zFar - zNear);
	Result[2][3] = static_cast<T>(1);
	Result[3][2] = -(zFar * zNear) / (zFar - zNear);
	return Result;
}

static glm::vec2 norCenter;

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
			rasterization.setCullMode(vk::CullModeFlagBits::eBack);
			rasterization.setFrontFace(vk::FrontFace::eCounterClockwise);
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
				.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			};

			auto bufferCreate = (VkBufferCreateInfo)vk::BufferCreateInfo {
				{},
				sizeof(Vertex) * defaultVertices.size(),
				vk::BufferUsageFlagBits::eVertexBuffer,
				vk::SharingMode::eExclusive
			};

			VkBuffer buffer;
			VmaAllocationInfo bufferAllocInfo;
			auto result = vmaCreateBuffer(
				engine_.vma_,
				&bufferCreate,
				&vmalloc,
				&buffer,
				&vertexBufferAlloc_,
				&bufferAllocInfo
				);

			if (result != VK_SUCCESS) {
				spdlog::error("Falied to allocate buffer");
				std::terminate();
			}

			vertexBuffer_ = buffer;

			memcpy(bufferAllocInfo.pMappedData, &defaultVertices, sizeof(Vertex) * defaultVertices.size());
			vmaFlushAllocation(engine_.vma_, vertexBufferAlloc_, bufferAllocInfo.offset, bufferAllocInfo.size);
		}

		// Allocate Index buffer
		{
			VkBuffer staging;
			VmaAllocation stagingAlloc;
			VmaAllocationInfo stagingAllocInfo;

			const auto stagingCreate = VkBufferCreateInfo {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = defaultIndexes.size() * sizeof(defaultIndexes[0]),
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE
			};
			const VmaAllocationCreateInfo stagingAllocCreate = {
				.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
				.usage = VMA_MEMORY_USAGE_CPU_ONLY,
				.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			};

			auto result = vmaCreateBuffer(
				engine_.vma_,
				&stagingCreate,
				&stagingAllocCreate,
				&staging,
				&stagingAlloc,
				&stagingAllocInfo
			);
			if (result != VK_SUCCESS) {
				spdlog::error("Falied to allocate buffer");
				std::terminate();
			}
			const size_t indexsize = sizeof(defaultIndexes[0]) * defaultIndexes.size();
			memcpy(stagingAllocInfo.pMappedData, &defaultIndexes, indexsize);


			VkBuffer index;

			const auto indexCreate = VkBufferCreateInfo {
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = defaultIndexes.size() * sizeof(defaultIndexes[0]),
				.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE
			};
			const VmaAllocationCreateInfo indexAllocCreate = {
				.usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			};

			result = vmaCreateBuffer(
				engine_.vma_,
				&indexCreate,
				&indexAllocCreate,
				&index,
				&indexBufferAlloc_,
				nullptr
			);
			if (result != VK_SUCCESS) {
				vmaDestroyBuffer(engine_.vma_, staging, stagingAlloc);
				spdlog::error("Falied to allocate buffer");
				std::terminate();
			}

			const auto& cmdbufAlloc = device_.allocateCommandBuffers(
				{
					engine_.graphicsCmdPool_,
					vk::CommandBufferLevel::ePrimary,
					1
				}
				);
			const auto& cmdbuf = cmdbufAlloc.front();
			cmdbuf.begin(vk::CommandBufferBeginInfo {});
			cmdbuf.copyBuffer(staging, index, { { 0, 0, indexsize } });
			cmdbuf.end();
			engine_.graphicsQ_.submit({ {{}, {}, cmdbufAlloc, {}} });
			engine_.graphicsQ_.waitIdle();

			indexBuffer_ = index;
			vmaDestroyBuffer(engine_.vma_, staging, stagingAlloc);
			device_.freeCommandBuffers(engine_.graphicsCmdPool_, cmdbufAlloc);
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

		// Update View Uniform
		{
			glm::mat4 view2 = lookat(
				glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1)
			);
			float aspect = (float)extent_.width/(float)extent_.height;
			glm::mat4 proj2 = perspect(
				90.0f, aspect, 0.5f, 10.0f
			);

			void* uniform;
			vmaMapMemory(engine_.vma_, engine_.viewAlloc_, &uniform);
			*(glm::mat4 *)uniform = proj2 * view2;
			vmaUnmapMemory(engine_.vma_, engine_.viewAlloc_);
		}
	}

	Presenter::~Presenter()
	{
		device_.waitIdle();
		vmaDestroyBuffer(engine_.vma_, (VkBuffer)indexBuffer_, indexBufferAlloc_);
		vmaDestroyBuffer(engine_.vma_, (VkBuffer)vertexBuffer_, vertexBufferAlloc_);
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

		const auto & viewCenter = engine_.modelCenter_;
		norCenter[0] = static_cast<float>(viewCenter[0]) / 100.0f;
		norCenter[1] = static_cast<float>(viewCenter[1]) / 100.0f;

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
		cmdbuf.bindIndexBuffer(indexBuffer_, 0u, vk::IndexType::eUint32);
		cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout_, 0, 1, &engine_.globalDescriptors_[curimg%2], 0, nullptr);
		cmdbuf.pushConstants(pipelineLayout_, vk::ShaderStageFlagBits::eVertex, 0, sizeof(norCenter), &norCenter);
		cmdbuf.drawIndexed(6, 1, 0, 0, 0);
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
