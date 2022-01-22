/* Vulkan Helpers provided by vookoo library
 *  https://github.com/andy-thomason/Vookoo
 *
 *  The MIT License (MIT)
 *
 * Copyright (c) 2016 Vookoo contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
**/


#ifndef VULKANPLAYGROUND_SRC_ASSETSMANAGER_VOOKOO_HPP
#define VULKANPLAYGROUND_SRC_ASSETSMANAGER_VOOKOO_HPP

#include <vulkan/vulkan.hpp>


namespace vku
{

/// Factory for renderpasses.
/// example:
///     RenderpassMaker rpm;
///     rpm.subpassBegin(vk::PipelineBindPoint::eGraphics);
///     rpm.subpassColorAttachment(vk::ImageLayout::eColorAttachmentOptimal);
///
///     rpm.attachmentDescription(attachmentDesc);
///     rpm.subpassDependency(dependency);
///     s.renderPass_ = rpm.createUnique(device);
class RenderpassMaker
{
public:
	RenderpassMaker()
	{
	}

	/// Begin an attachment description.
	/// After this you can call attachment* many times
	RenderpassMaker &attachmentBegin(vk::Format format)
	{
		vk::AttachmentDescription desc{{}, format};
		s.attachmentDescriptions.push_back(desc);
		return *this;
	}

	RenderpassMaker &attachmentFlags(vk::AttachmentDescriptionFlags value)
	{
		s.attachmentDescriptions.back().flags = value;
		return *this;
	};
	RenderpassMaker &attachmentFormat(vk::Format value)
	{
		s.attachmentDescriptions.back().format = value;
		return *this;
	};
	RenderpassMaker &attachmentSamples(vk::SampleCountFlagBits value)
	{
		s.attachmentDescriptions.back().samples = value;
		return *this;
	};
	RenderpassMaker &attachmentLoadOp(vk::AttachmentLoadOp value)
	{
		s.attachmentDescriptions.back().loadOp = value;
		return *this;
	};
	RenderpassMaker &attachmentStoreOp(vk::AttachmentStoreOp value)
	{
		s.attachmentDescriptions.back().storeOp = value;
		return *this;
	};
	RenderpassMaker &attachmentStencilLoadOp(vk::AttachmentLoadOp value)
	{
		s.attachmentDescriptions.back().stencilLoadOp = value;
		return *this;
	};
	RenderpassMaker &attachmentStencilStoreOp(vk::AttachmentStoreOp value)
	{
		s.attachmentDescriptions.back().stencilStoreOp = value;
		return *this;
	};
	RenderpassMaker &attachmentInitialLayout(vk::ImageLayout value)
	{
		s.attachmentDescriptions.back().initialLayout = value;
		return *this;
	};
	RenderpassMaker &attachmentFinalLayout(vk::ImageLayout value)
	{
		s.attachmentDescriptions.back().finalLayout = value;
		return *this;
	};

	/// Start a subpass description.
	/// After this you can can call subpassColorAttachment many times
	/// and subpassDepthStencilAttachment once.
	RenderpassMaker &subpassBegin(vk::PipelineBindPoint bp)
	{
		vk::SubpassDescription desc{};
		desc.pipelineBindPoint = bp;
		s.subpassDescriptions.push_back(desc);
		return *this;
	}

	RenderpassMaker &subpassColorAttachment(vk::ImageLayout layout, uint32_t attachment)
	{
		vk::SubpassDescription &subpass = s.subpassDescriptions.back();
		auto *p = getAttachmentReference();
		p->layout = layout;
		p->attachment = attachment;
		if (subpass.colorAttachmentCount == 0) {
			subpass.pColorAttachments = p;
		}
		subpass.colorAttachmentCount++;
		return *this;
	}

	RenderpassMaker &subpassDepthStencilAttachment(vk::ImageLayout layout, uint32_t attachment)
	{
		vk::SubpassDescription &subpass = s.subpassDescriptions.back();
		auto *p = getAttachmentReference();
		p->layout = layout;
		p->attachment = attachment;
		subpass.pDepthStencilAttachment = p;
		return *this;
	}

	vk::UniqueRenderPass createUnique(const vk::Device &device) const
	{
		vk::RenderPassCreateInfo renderPassInfo{};
		renderPassInfo.attachmentCount = (uint32_t)s.attachmentDescriptions.size();
		renderPassInfo.pAttachments = s.attachmentDescriptions.data();
		renderPassInfo.subpassCount = (uint32_t)s.subpassDescriptions.size();
		renderPassInfo.pSubpasses = s.subpassDescriptions.data();
		renderPassInfo.dependencyCount = (uint32_t)s.subpassDependencies.size();
		renderPassInfo.pDependencies = s.subpassDependencies.data();
		return device.createRenderPassUnique(renderPassInfo);
	}

	vk::UniqueRenderPass createUnique(const vk::Device &device, const vk::RenderPassMultiviewCreateInfo &I) const
	{
		vk::RenderPassCreateInfo renderPassInfo{};
		renderPassInfo.attachmentCount = (uint32_t)s.attachmentDescriptions.size();
		renderPassInfo.pAttachments = s.attachmentDescriptions.data();
		renderPassInfo.subpassCount = (uint32_t)s.subpassDescriptions.size();
		renderPassInfo.pSubpasses = s.subpassDescriptions.data();
		renderPassInfo.dependencyCount = (uint32_t)s.subpassDependencies.size();
		renderPassInfo.pDependencies = s.subpassDependencies.data();
		renderPassInfo.pNext =
			&I; // identical to createUnique(const vk::Device &device) except set pNext to use multi-view &I
		return device.createRenderPassUnique(renderPassInfo);
	}

	RenderpassMaker &dependencyBegin(uint32_t srcSubpass, uint32_t dstSubpass)
	{
		vk::SubpassDependency desc{};
		desc.srcSubpass = srcSubpass;
		desc.dstSubpass = dstSubpass;
		s.subpassDependencies.push_back(desc);
		return *this;
	}

	RenderpassMaker &dependencySrcSubpass(uint32_t value)
	{
		s.subpassDependencies.back().srcSubpass = value;
		return *this;
	};
	RenderpassMaker &dependencyDstSubpass(uint32_t value)
	{
		s.subpassDependencies.back().dstSubpass = value;
		return *this;
	};
	RenderpassMaker &dependencySrcStageMask(vk::PipelineStageFlags value)
	{
		s.subpassDependencies.back().srcStageMask = value;
		return *this;
	};
	RenderpassMaker &dependencyDstStageMask(vk::PipelineStageFlags value)
	{
		s.subpassDependencies.back().dstStageMask = value;
		return *this;
	};
	RenderpassMaker &dependencySrcAccessMask(vk::AccessFlags value)
	{
		s.subpassDependencies.back().srcAccessMask = value;
		return *this;
	};
	RenderpassMaker &dependencyDstAccessMask(vk::AccessFlags value)
	{
		s.subpassDependencies.back().dstAccessMask = value;
		return *this;
	};
	RenderpassMaker &dependencyDependencyFlags(vk::DependencyFlags value)
	{
		s.subpassDependencies.back().dependencyFlags = value;
		return *this;
	};

private:
	constexpr static int max_refs = 64;

	vk::AttachmentReference *getAttachmentReference()
	{
		return (s.num_refs < max_refs) ? &s.attachmentReferences[s.num_refs++] : nullptr;
	}

	struct State
	{
		std::vector<vk::AttachmentDescription> attachmentDescriptions;
		std::vector<vk::SubpassDescription> subpassDescriptions;
		std::vector<vk::SubpassDependency> subpassDependencies;
		std::array<vk::AttachmentReference, max_refs> attachmentReferences;
		int num_refs = 0;
		bool ok_ = false;
	};

	State s;
};

}

#endif