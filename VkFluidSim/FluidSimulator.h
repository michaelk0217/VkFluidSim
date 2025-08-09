#pragma once

#include "VulkanStructures.h"
#include "VulkanDevices.h"
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanDescriptorSets.h"
#include "VulkanPipelineLayout.h"
#include "VulkanDescriptorSetLayout.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
#include <algorithm>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>

class FluidSimulator
{
public:
	
	FluidSimulator(
		VulkanDevices& devices,
		//VulkanPipelineLayout& pipelineLayout,
		VkCommandPool commandPool,
		VkFormat colorFormat,
		VkDescriptorPool descriptorPool,
		std::vector<VkBuffer> uboBuffer
		//VulkanDescriptorSets& descriptorSets
	);

	~FluidSimulator();

	void update(float deltaTime, VkCommandBuffer commandBuffer, glm::vec2 mousePos);
	void draw(VkCommandBuffer commandBuffer, uint32_t currentFrameIndex, VkBuffer uboBuffer);
	
	FluidSimParameters& getParameters() { return m_params; };

private:
	
	VulkanDevices& m_devices;
	VkCommandPool m_commandPool;
	VkDescriptorPool m_descriptorPool;
	//VulkanPipelineLayout& m_pipelineLayout;

	std::vector<Particle> m_particles;
	//std::vector<Vertex> m_particleVertices;
	FluidSimParameters m_params;

	// particle resources
	std::unique_ptr<VulkanGraphicsPipeline> m_particlePipeline;
	//std::unique_ptr<VulkanVertexBuffer> m_particleVertexBuffer;
	const uint32_t MAX_PARTICLES = 100000;

	// container resources
	std::vector<Vertex> m_boxVertices;
	std::vector<uint32_t> m_boxIndices{
		0, 1,
		1, 2,
		2, 3,
		3, 0
	};
	std::unique_ptr<VulkanGraphicsPipeline> m_boxPipeline;
	std::unique_ptr<VulkanVertexBuffer> m_boxVertexBuffer;
	std::unique_ptr<VulkanIndexBuffer> m_boxIndexBuffer;

	std::unique_ptr<VulkanDescriptorSetLayout> m_boxDescriptorSetLayout;
	std::unique_ptr<VulkanPipelineLayout> m_boxPipelineLayout;
	std::unique_ptr<VulkanDescriptorSets> m_boxDescriptorSets;

	// graphics resources
	//VulkanDescriptorSets& m_graphicsDescriptorSets;
	std::unique_ptr<VulkanDescriptorSets> graphicsDescriptorSets;
	VkDescriptorSetLayout graphicsDescriptorSetLayout;
	VkPipelineLayout graphicsPipelineLayout;

	// compute resources
	std::vector<VkBuffer> particleBuffers;
	std::vector<VkDeviceMemory> particleBufferMemories;

	VkPipelineLayout m_fluidComputePipelineLayout;
	VkPipeline m_fluidComputePipeline;
	VkDescriptorSetLayout m_fluidComputeDescriptorSetLayout;
	std::vector<VkDescriptorSet> m_fluidComputeDescriptorSets;

	uint32_t currentBuffer = 0;

	// ---  Hashing pass resources ---
	const uint32_t HASH_TABLE_SIZE = 1000003;

	VkBuffer m_particleHashesBuffer;
	VkDeviceMemory m_particleHashesMemory;
	VkBuffer m_particleIndicesBuffer;
	VkDeviceMemory m_particleIndicesMemory;

	VkPipelineLayout m_hashPipelineLayout;
	VkPipeline m_hashPipeline;
	VkDescriptorSetLayout m_hashDescriptorSetLayout;
	std::vector<VkDescriptorSet> m_hashDescriptorSets;
	
	// --- Sorting Pass Resources ---
	VkPipelineLayout m_sortPipelineLayout;
	VkPipeline m_sortPipeline;
	VkDescriptorSetLayout m_sortDescriptorSetLayout;
	VkDescriptorSet m_sortDescriptorSet;

	// --- Indexing Pass Resources ---
	VkBuffer m_cellStartIndicesBuffer;
	VkDeviceMemory m_cellStartIndicesMemory;

	VkPipelineLayout m_indexingPipelineLayout;
	VkPipeline m_indexingPipeline;
	VkDescriptorSetLayout m_indexingDescriptorSetLayout;
	VkDescriptorSet m_indexingDescriptorSet;


	// hashing
	void createHashPassResources();
	void updateHashDescriptorSets();

	// sorting
	void createSortPassResources();
	void dispatchSort(VkCommandBuffer commandBuffer);

	// indexing
	void createIndexingPassResources();

	// graphics
	void createGraphicsPipelineLayout();

	// compute
	void createParticleBuffers();
	void createComputePipeline();
	void createComputeDescriptorSets();
	void updateComputeDescriptorSets();

	void initializeParticles();
	void populateParticlesRandom();
	void populateParticlesSquare();
	void updateContainerBuffer();


	

	/*void ResolveContainerCollisions(int particleIndex)
	{
		float effectiveBoxWidth = m_params.boxHalfWidth - m_params.particleWorldRadius;
		float effectiveBoxHeight = m_params.boxHalfHeight - m_params.particleWorldRadius;

		if (abs(m_particles[particleIndex].position.x) > effectiveBoxWidth)
		{
			m_particles[particleIndex].position.x = effectiveBoxWidth * glm::sign(m_particles[particleIndex].position.x);
			m_particles[particleIndex].velocity.x *= -1.0f * m_params.collisionDamping;
		}
		if (abs(m_particles[particleIndex].position.y) > effectiveBoxHeight)
		{
			m_particles[particleIndex].position.y = effectiveBoxHeight * glm::sign(m_particles[particleIndex].position.y);
			m_particles[particleIndex].velocity.y *= -1.0f * m_params.collisionDamping;
		}
	}*/

	//static float SmoothingKernal(float radius, float dst)
	//{
	//	/*float volume = glm::pi<float>() * std::powf(radius, 8) / 4.0f;
	//	float value = std::max(0.0f, radius * radius - dst * dst);
	//	return value * value * value / volume;*/

	//	if (dst >= radius) return 0;

	//	float volume = glm::pi<float>() * std::powf(radius, 4) / 6.0f;
	//	return (radius - dst) * (radius - dst) / volume;
	//}

	//static float SmoothingKernalDerivative(float dst, float radius)
	//{
	//	/*if (dst >= radius) return 0;
	//	float f = radius * radius - dst * dst;
	//	float scale = -24.0f / (glm::pi<float>() * std::powf(radius, 8));
	//	return scale * dst * f * f;*/

	//	if (dst >= radius) return 0;
	//	float scale = 12 / (powf(radius, 4) * glm::pi<float>());
	//	return (dst - radius) * scale;
	//}

	/*float CalculateDensity(glm::vec3 samplePoint)
	{
		float density = 0;
		const float mass = 1;

		for (auto& p : m_particles)
		{
			float dst = glm::length(p.position - samplePoint);
			float influence = SmoothingKernal(m_params.smoothingRadius, dst);
			density += mass * influence;
		}
		return density;
	}*/


	//void UpdateDensities()
	//{
	//	for (int i = 0; i < m_params.particleCount; i++)
	//	{
	//		//m_particles[i].density = CalculateDensity(m_particles[i].position);
	//		m_particles[i].density = CalculateDensityWithinRadius(m_particles[i].position);
	//	}
	//}

	/*glm::vec3 GetRamdomDirXY()
	{
		float angle = glm::linearRand(0.0f, glm::two_pi<float>());

		return glm::vec3(glm::cos(angle), glm::sin(angle), 0.0f);
	}*/

	/*float CalculateSharedPressure(float density1, float density2)
	{
		float pressure1 = ConvertDensityToPressure(density1);
		float pressure2 = ConvertDensityToPressure(density2);
		return (pressure1 + pressure2) / 2;
	}*/

	/*glm::vec3 CalculatePressureForce(int particleIndex)
	{
		glm::vec3 pressureForce = glm::vec3(0.0f);

		for (int i = 0; i < m_particles.size(); i++)
		{
			if (particleIndex == i) continue;

			glm::vec3 offset = m_particles[i].position - m_particles[particleIndex].position;
			float dst = glm::length(offset);
			glm::vec3 dir = dst == 0 ? GetRamdomDirXY() : offset / dst;

			float slope = SmoothingKernalDerivative(dst, m_params.smoothingRadius);
			float density = m_particles[i].density;
			float sharedPressure = CalculateSharedPressure(density, m_particles[particleIndex].density);
			pressureForce += sharedPressure * dir * slope * m_params.mass / density;
		}

		return pressureForce;
	}*/

	

	/*float ConvertDensityToPressure(float density)
	{
		float densityError = density - m_params.targetDensity;
		float pressure = densityError * m_params.pressureMultiplier;
		return pressure;
	}*/

	//std::pair<uint32_t, uint32_t> PositionToCellCoord2D(glm::vec3 particleLoc, float radius)
	//{

	//	//uint32_t numWidthCells = static_cast<uint32_t>(std::floor(m_params.boxHalfWidth / radius));
	//	//uint32_t numHeightCells = static_cast<uint32_t>(std::floor(m_params.boxHalfHeight / radius));

	//	uint32_t cellX = static_cast<uint32_t>(std::floor(particleLoc.x / (radius * 2)));
	//	uint32_t cellY = static_cast<uint32_t>(std::floor(particleLoc.y / (radius * 2)));

	//	//return (cellY * numWidthCells) + cellX;
	//	return std::pair(cellX, cellY);
	//}

	// ------ spacial lookup ---------
	std::vector<std::pair<uint32_t, uint32_t>> spacialLookup; // first: i, second: cellKey
	std::vector<uint32_t> startIndices;
	static const uint32_t hashK1 = 15823;
	static const uint32_t hashK2 = 9737333;

	/*void UpdateSpacialLookup()
	{
		spacialLookup.clear();
		spacialLookup.resize(m_params.particleCount);

		startIndices.clear();
		startIndices.resize(m_params.particleCount);

		for (uint32_t i = 0; i < m_params.particleCount; i++)
		{
			std::pair<uint32_t, uint32_t> cellCoord = PositionToCellCoord2D(m_particles[i].position, m_params.smoothingRadius);
			uint32_t cellKey = GetKeyFromHash(HashCell(cellCoord), static_cast<uint32_t>(spacialLookup.size()));
			spacialLookup[i] = std::pair<uint32_t, uint32_t>(i, cellKey);
			startIndices[i] = UINT32_MAX;
		}

		std::sort(spacialLookup.begin(), spacialLookup.end(),
			[](std::pair<uint32_t, uint32_t> a, std::pair<uint32_t, uint32_t> b) {
				return a.second < b.second;
			}
		);

		for (uint32_t i = 0; i < m_params.particleCount; i++)
		{
			uint32_t key = spacialLookup[i].second;
			uint32_t keyPrev = i == 0 ? UINT32_MAX : spacialLookup[i - 1].second;
			if (key != keyPrev)
			{
				startIndices[key] = i;
			}
		}
	}*/

	/*static uint32_t HashCell(std::pair<uint32_t, uint32_t> cell)
	{
		uint32_t a = cell.first * hashK1;
		uint32_t b = cell.second * hashK2;
		return (a + b);
	}*/

	/*static uint32_t GetKeyFromHash(uint32_t hash, uint32_t tableSize)
	{
		return hash % tableSize;

	}*/

	//float CalculateDensityWithinRadius(glm::vec3 samplePoint)
	//{
	//	float density = 0;

	//	//std::pair<uint32_t, uint32_t> cellCoord = PositionToCellCoord2D(m_particles[samplePointIndex].position, m_params.smoothingRadius);
	//	std::pair<uint32_t, uint32_t> cellCoord = PositionToCellCoord2D(samplePoint, m_params.smoothingRadius);

	//	float sqrRadius = m_params.smoothingRadius * m_params.smoothingRadius;
	//	
	//	std::vector<std::pair<int, int>> cellOffsets = {
	//		{-1, -1}, {0, -1}, {1, -1},
	//		{-1,  0}, {0,  0}, {1,  0},
	//		{-1,  1}, {0,  1}, {1,  1}
	//	};

	//	for (auto& offset: cellOffsets)
	//	{
	//		uint32_t key = GetKeyFromHash(HashCell(std::pair<uint32_t, uint32_t>(cellCoord.first + offset.first, cellCoord.second + offset.second)), spacialLookup.size());
	//		int cellStartIndex = startIndices[key];

	//		for (int i = cellStartIndex; i < spacialLookup.size(); i++)
	//		{
	//			if (spacialLookup[i].second != key) break;

	//			int particleIndex = spacialLookup[i].first;
	//			//float dst = glm::length(m_particles[particleIndex].position - m_particles[samplePointIndex].position);
	//			float dst = glm::length(m_particles[particleIndex].position - samplePoint);

	//			float sqrDst = dst * dst;

	//			// test if point is inside the radius
	//			if (sqrDst <= sqrRadius)
	//			{
	//				float influence = SmoothingKernal(m_params.smoothingRadius, dst);
	//				density += m_params.mass * influence;
	//			}
	//		}
	//	}
	//	return density;
	//}

	//glm::vec3 CalculatePressureForceWithinRadius(int particleIndex)
	//{
	//	glm::vec3 pressureForce = glm::vec3(0.0f);
	//	glm::vec3 samplePoint = m_particles[particleIndex].position;
	//	std::pair<uint32_t, uint32_t> centerCell = PositionToCellCoord2D(samplePoint, m_params.smoothingRadius);

	//	std::vector<std::pair<int, int>> cellOffsets = {
	//		{-1, -1}, {0, -1}, {1, -1},
	//		{-1,  0}, {0,  0}, {1,  0},
	//		{-1,  1}, {0,  1}, {1,  1}
	//	};

	//	for (auto& offset : cellOffsets)
	//	{
	//		uint32_t key = GetKeyFromHash(HashCell({ centerCell.first + offset.first, centerCell.second + offset.second }), spacialLookup.size());
	//		int cellStartIndex = startIndices[key];

	//		if (cellStartIndex == UINT32_MAX) continue;

	//		for (int i = cellStartIndex; i < spacialLookup.size(); i++)
	//		{
	//			if (spacialLookup[i].second != key) break;

	//			int neighborIndex = spacialLookup[i].first;
	//			if (neighborIndex == particleIndex) continue;

	//			glm::vec3 offsetToNeighbor = m_particles[neighborIndex].position - samplePoint;
	//			float dst = glm::length(offsetToNeighbor);

	//			if (dst < m_params.smoothingRadius)
	//			{
	//				glm::vec3 dir = dst == 0 ? GetRamdomDirXY() : offsetToNeighbor / dst;
	//				float slope = SmoothingKernalDerivative(dst, m_params.smoothingRadius);
	//				float sharedPressure = CalculateSharedPressure(m_particles[particleIndex].density, m_particles[neighborIndex].density);

	//				if (m_particles[neighborIndex].density > 0)
	//				{
	//					pressureForce += sharedPressure * dir * slope * m_params.mass / m_particles[neighborIndex].density;
	//				}
	//			}
	//		}
	//	}
	//	return pressureForce;
	//}

	//void ForeachWithinRadius(int samplePointIndex)
	//{
	//	std::pair<uint32_t, uint32_t> cellCoord = PositionToCellCoord2D(m_particles[samplePointIndex].position, m_params.smoothingRadius);
	//	float sqrRadius = m_params.smoothingRadius * m_params.smoothingRadius;

	//	std::vector<std::pair<int, int>> cellOffsets = {
	//		{-1, -1}, {0, -1}, {1, -1},
	//		{-1,  0}, {0,  0}, {1,  0},
	//		{-1,  1}, {0,  1}, {1,  1}
	//	};

	//	for (auto& offset : cellOffsets)
	//	{
	//		uint32_t key = GetKeyFromHash(HashCell(std::pair<uint32_t, uint32_t>(cellCoord.first + offset.first, cellCoord.second + offset.second)), spacialLookup.size());
	//		int cellStartIndex = startIndices[key];

	//		for (int i = cellStartIndex; i < spacialLookup.size(); i++)
	//		{
	//			if (spacialLookup[i].second != key) break;

	//			int particleIndex = spacialLookup[i].first;
	//			float dst = glm::length(m_particles[particleIndex].position - m_particles[samplePointIndex].position);
	//			float sqrDst = dst * dst;

	//			// test if point is inside the radius
	//			if (sqrDst <= sqrRadius)
	//			{
	//				// do callback
	//			}
	//		}
	//	}
	//}



	//void SimulationStep(float deltaTime)
	//{
	//	// apply gravity and predit next positions
	//	for (int i = 0; i < m_params.particleCount; i++)
	//	{
	//		m_particles[i].velocity += glm::vec3(0.0f, 1.0f, 0.0f) * m_params.gravity * deltaTime;
	//		m_particles[i].predictedPosition = m_particles[i].position + m_particles[i].velocity * deltaTime;
	//	}

	//	UpdateSpacialLookup();
	//	
	//	// calculate densities
	//	for (int i = 0; i < m_params.particleCount; i++)
	//	{
	//		m_particles[i].density = CalculateDensityWithinRadius(m_particles[i].predictedPosition);
	//	}

	//	// calculate and apply pressure forces
	//	for (int i = 0; i < m_params.particleCount; i++)
	//	{
	//		glm::vec3 pressureForce = CalculatePressureForceWithinRadius(i);
	//		if (m_particles[i].density > 0)
	//		{
	//			glm::vec3 pressureAcceleration = pressureForce / m_particles[i].density;
	//			m_particles[i].velocity += pressureAcceleration * deltaTime;
	//		}
	//	}

	//	//  update positions and handle collisions
	//	for (int i = 0; i < m_particles.size(); i++)
	//	{
	//		m_particles[i].position += m_particles[i].velocity * deltaTime;
	//		ResolveContainerCollisions(i);
	//	}
	//}
};

