//
// Created by ocean on 1/22/22.
//

#ifndef IMGSYNCER_HPP
#define IMGSYNCER_HPP

#include <vulkan/vulkan.hpp>

namespace VulkanPlayground {

struct ImgSyncer
{
	vk::Semaphore renderComplete;
	vk::Semaphore imageAvailable;
	vk::Fence imageDone;
};

}


#endif //IMGSYNCER_HPP
