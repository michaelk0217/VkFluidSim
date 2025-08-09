// VulkanStructures.h

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.h>
#include <array>
#include <vector>
#include <string>
#include <memory>


struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	//glm::vec2 texCoord;
	//glm::vec3 inNormal;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		/*attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, inNormal);*/

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos/* && color == other.color && texCoord == other.texCoord && inNormal == other.inNormal*/;
	}
};

namespace std {
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			/*return  ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1)^
				(hash<glm::vec2>()(vertex.texCoord) << 1) ^
				(hash<glm::vec3>()(vertex.inNormal) << 2);*/
			return hash<glm::vec3>()(vertex.pos);
		}
	};
}

struct ShaderData {
	alignas(16) glm::mat4 modelMatrix;
	alignas(16) glm::mat4 viewMatrix;
	alignas(16) glm::mat4 projectionMatrix;
	float viewportHeight;
	float fovy;
	float particleWorldRadius;
	float _padding;
};

//struct Particle {
//	glm::vec3 position{ 0.0f, 0.0f, 0.0f };
//	glm::vec3 predictedPosition{ 0.0f, 0.0f, 0.0f };
//	glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };
//	float density = 0;
//};

struct Particle {
	alignas(16) glm::vec4 position;
	//alignas(16) glm::vec4 predictedPosition;
	alignas(16) glm::vec4 velocity;
	alignas(16) glm::vec4 color;
	float density;
	//float pressure;

	float _padding[3];
};


struct GradientColorsPoints
{
	glm::vec3 color1{ 0.0f, 0.0f, 1.0f };
	//float c1Point = 0.0f;
	glm::vec3 color2{ 0.0f, 1.0f, 0.0f };
	//float c2Point = 0.5f;
	glm::vec3 color3{ 1.0f, 1.0f, 0.0f };
	//float c3Point = 0.75f;
	//glm::vec3 color4{ 1.0f, 0.0f, 0.0f };
	//float c4Point = 1.0f;
};

struct FluidSimParameters
{
	uint32_t particleCount = 10000;
	float collisionDamping = 0.8f;
	float boxHalfWidth = 13.0f;
	float boxHalfHeight = 8.0f;
	float particleWorldRadius = 0.06f;
	bool runSimulation = false;
	bool resetSimulation = true;
	float maxSpeedForColor = 5.0f;
	/*glm::vec3 minSpeedColor{ 0.2f, 0.6f, 1.0f };
	glm::vec3 maxSpeedColor{ 1.0f, 0.0f, 0.0f };*/
	GradientColorsPoints colorPoints;
	float smoothingRadius = 0.1f;
	float mass = 1.0f;
	float targetDensity = 3.0f;
	float pressureMultiplier = 2.0f;
	float gravity = 0;

};

struct ComputeParameters
{
	uint32_t particleCount;
	float deltaTime;
	float gravity;
	float smoothingRadius;
	float targetDensity;
	float pressureMultiplier;
	float collisionDamping;
	float mass;
	glm::vec2 boxHalfSize;
	glm::vec2 mousePos;
	glm::vec4 color1;
	glm::vec4 color2;
	glm::vec4 color3;
	float maxSpeedForColor;
	float padding2[3];
};

struct SortPushConstants {
	uint32_t elementCount;
	uint32_t mergeSize;
	uint32_t compareStride;
};


struct UiContextPacket {
	float& deltaTime;
	std::vector<float>& frameHistory;
	FluidSimParameters& parameters;

	UiContextPacket(float& dt, std::vector<float>& history, FluidSimParameters& params)
		: deltaTime(dt), frameHistory(history), parameters(params)
	{
		// Constructor body can be empty
	}
};