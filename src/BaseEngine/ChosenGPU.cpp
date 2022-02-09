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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
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
#pragma GCC diagnostic pop

		const auto res = vmaCreateAllocator(&vmaCreat, &vma_);
		if (res != VK_SUCCESS) {
			spdlog::error("Failed to create memory allocator");
			std::terminate();
		}
	}

	// Determine Image Count
	{
		const auto surfaceCap = chosenGPU_.getSurfaceCapabilitiesKHR(surface_);

		if (surfaceCap.maxImageCount < 2) {
			spdlog::error("The device does not support for more than two images!");
			std::terminate();
		}

		imageCount_ = std::max(2u, surfaceCap.minImageCount);
	}

	for (unsigned i = 0; i < imageCount_; i++) {
		syncObjs_.push_back(
			{
				.renderComplete = device_.createSemaphore({}),
				.imageAvailable = device_.createSemaphore({}),
				.imageDone = device_.createFence({vk::FenceCreateFlagBits::eSignaled})
		});
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

	{
		texture_.push_back(TextureModule::uploadTexture("../assets/textures/IMG_0800.JPG", vma_, device_, graphicsQ_, graphicsCmdPool_));
		vk::SamplerCreateInfo samplerInfo;
		samplerInfo.setMagFilter(vk::Filter::eLinear);
		sampler_ = device_.createSampler(samplerInfo);

		VkBuffer uniform;
		const auto uniformCreate = VkBufferCreateInfo {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(glm::mat4),
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
		const auto uniformAllocCreate = VmaAllocationCreateInfo {
			.usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
			.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		};
		auto result = vmaCreateBuffer(
			vma_,
			&uniformCreate,
			&uniformAllocCreate,
			&uniform,
			&viewAlloc_,
			nullptr
			);
		if (result != VK_SUCCESS) {
			spdlog::error("Falied to allocate buffer");
			std::terminate();
		}

		view_ = uniform;
	}

	{
		std::array sizes {
			vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer, 10u },
			vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, 10u }
		};
		descriptorPool_ = device_.createDescriptorPool({ vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 4, sizes });

		std::array bindings {
			vk::DescriptorSetLayoutBinding {
				0,
				vk::DescriptorType::eCombinedImageSampler,
				1,
				vk::ShaderStageFlagBits::eFragment,
				nullptr
			},
			vk::DescriptorSetLayoutBinding {
				1,
				vk::DescriptorType::eUniformBuffer,
				1,
				vk::ShaderStageFlagBits::eVertex,
				nullptr
			}
		};
		globalDescriptorLayout_ = device_.createDescriptorSetLayout({ vk::DescriptorSetLayoutCreateFlags {}, bindings});
	}

	// Create Pipelinelayout
	{
		std::array pushConstants {
			vk::PushConstantRange {
					vk::ShaderStageFlagBits::eVertex,
					0,
					sizeof(float) * 2
			}
		};
		globalPipelineLayout_ = device_.createPipelineLayout(
			{
				{}, globalDescriptorLayout_, pushConstants
			});
	}

	// Allocator Descriptor Sets
	{
		std::vector<vk::DescriptorSetLayout> layouts(imageCount_, globalDescriptorLayout_);
		globalDescriptors_ = device_.allocateDescriptorSets({descriptorPool_, layouts});

		std::array<vk::DescriptorImageInfo, 1> images {
			{
				{
					sampler_, texture_.front().textureView, vk::ImageLayout::eShaderReadOnlyOptimal
				}
			}
		};
		std::array<vk::DescriptorBufferInfo, 1> uniforms {
			{
				{
					view_, 0, sizeof(glm::mat4)
				}
			}
		};
		std::array<vk::WriteDescriptorSet, 2> updates;
		for (unsigned i = 0 ; i < imageCount_; i++) {
			auto& write = updates[i];
			write.setDstSet(globalDescriptors_[i])
				.setDstBinding(0).setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount(1)
				.setPImageInfo(&images[0]);
		}
		device_.updateDescriptorSets(updates, {});
		for (unsigned i = 0 ; i < imageCount_; i++) {
			auto& write = updates[i];
			write.setDstSet(globalDescriptors_[i])
				.setDstBinding(1).setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				.setPBufferInfo(&uniforms[0]);
		}
		device_.updateDescriptorSets(updates, {});
	}

	initPresenter();
}

void BaseEngine::initPresenter() {
	presenter_ = std::make_unique<Presenter>(*this, presenter_.get());
}

}
