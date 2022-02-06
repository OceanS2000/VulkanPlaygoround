//
// Created by ocean on 2/5/22.
//

#ifndef SCENE_HPP
#define SCENE_HPP

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace VulkanPlayground
{

class BaseEngine;

class Scene
{
public:
	Scene(const BaseEngine& engine);
	virtual ~Scene();

	Scene(const Scene&) = delete;
	Scene(Scene&&) = delete;
	Scene& operator=(const Scene&) = delete;
	Scene& operator=(Scene&&) = delete;

private:
	vk::RenderPass renderPass_;
};

}


#endif //SCENE_HPP
