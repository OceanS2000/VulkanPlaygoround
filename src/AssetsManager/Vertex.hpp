//
// Created by ocean on 1/22/22.
//

#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <array>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace VulkanPlayground
{

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	constexpr static vk::VertexInputBindingDescription vertexInputBinding = {
		0, 5 * sizeof(float), vk::VertexInputRate::eVertex
	};

	constexpr static std::array<vk::VertexInputAttributeDescription,2> vertexInputAttribute = {
		vk::VertexInputAttributeDescription {
			0, 0, vk::Format::eR32G32Sfloat, 0
		},
		vk::VertexInputAttributeDescription {
			1, 0, vk::Format::eR32G32B32Sfloat, 2 * sizeof(float)
		}
	};
};

}



#endif //VERTEX_HPP
