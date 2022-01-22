//
// Created by ocean on 1/20/22.
//

#include "ShaderModule.hpp"

#include <cstdio>
#include <spdlog/spdlog.h>

std::vector<uint32_t> VulkanPlayground::ShaderModule::readShader(const char* path) noexcept
{
	const auto file = std::fopen(path, "rb");
	if (!file) {
		spdlog::error("Failed to open file: {}", path);
		std::terminate();
	}

	std::fseek(file, 0, SEEK_END);
	const size_t size = std::ftell(file);

	assert(!(size & 0x3));

	std::vector<uint32_t> code(size / 4);
	std::fseek(file, 0, SEEK_SET);
	const size_t read_in = std::fread(code.data(), 1, size, file);

	assert(read_in == size);

	return code;
}
