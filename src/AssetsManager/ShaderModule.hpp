//
// Created by ocean on 1/20/22.
//

#ifndef VULKANPLAYGROUND_SRC_ASSETSMANAGER_SHADERMODULE_HPP
#define VULKANPLAYGROUND_SRC_ASSETSMANAGER_SHADERMODULE_HPP

#include <cstdint>
#include <vector>

namespace VulkanPlayground
{

class ShaderModule
{
public:
	static std::vector<uint32_t> readShader(const char* path) noexcept;
};

}

#endif //VULKANPLAYGROUND_SRC_ASSETSMANAGER_SHADERMODULE_HPP
