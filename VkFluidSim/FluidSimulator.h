#pragma once

#include "VulkanStructures.h"
#include "VulkanDevices.h"
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanGraphicsPipeline.h"

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
	
	FluidSimulator(VulkanDevices& devices, VkPipelineLayout pipelineLayout, VkCommandPool commandPool, VkFormat colorFormat);
	~FluidSimulator();

	void update(float deltaTime);
	void draw(VkCommandBuffer commandBuffer);
	
	FluidSimParameters& getParameters() { return m_params; };

private:
	
	VulkanDevices& m_devices;

	std::vector<Particle> m_particles;
	std::vector<Vertex> m_particleVertices;
	FluidSimParameters m_params;

	// particle resources
	std::unique_ptr<VulkanGraphicsPipeline> m_particlePipeline;
	std::unique_ptr<VulkanVertexBuffer> m_particleVertexBuffer;
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

	void initializeParticles();
	void updateContainerBuffer();


	// calculation functions

	//float smoothingRadius = 1.0f;
	//float mass = 1.0f;
	//float targetDensity;
	//float pressureMultiplier;
	//float gravity = 0;

	void ResolveContainerCollisions(int particleIndex)
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
	}

	static float SmoothingKernal(float radius, float dst)
	{
		/*float volume = glm::pi<float>() * std::powf(radius, 8) / 4.0f;
		float value = std::max(0.0f, radius * radius - dst * dst);
		return value * value * value / volume;*/

		if (dst >= radius) return 0;

		float volume = glm::pi<float>() * std::powf(radius, 4) / 6.0f;
		return (radius - dst) * (radius - dst) / volume;
	}

	static float SmoothingKernalDerivative(float dst, float radius)
	{
		/*if (dst >= radius) return 0;
		float f = radius * radius - dst * dst;
		float scale = -24.0f / (glm::pi<float>() * std::powf(radius, 8));
		return scale * dst * f * f;*/

		if (dst >= radius) return 0;
		float scale = 12 / (powf(radius, 4) * glm::pi<float>());
		return (dst - radius) * scale;
	}

	float CalculateDensity(glm::vec3 samplePoint)
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
	}


	void UpdateDensities()
	{
		for (auto& p : m_particles)
		{
			p.density = CalculateDensity(p.position);
		}
	}

	glm::vec3 GetRamdomDirXY()
	{
		float angle = glm::linearRand(0.0f, glm::two_pi<float>());

		return glm::vec3(glm::cos(angle), glm::sin(angle), 0.0f);
	}

	float CalculateSharedPressure(float density1, float density2)
	{
		float pressure1 = ConvertDensityToPressure(density1);
		float pressure2 = ConvertDensityToPressure(density2);
		return (pressure1 + pressure2) / 2;
	}

	glm::vec3 CalculatePressureForce(int particleIndex)
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
			float sharedPressure = CalculateSharedPressure(density, m_particles[i].density);
			pressureForce += sharedPressure * dir * slope * m_params.mass / density;
		}

		return pressureForce;
	}

	

	float ConvertDensityToPressure(float density)
	{
		float densityError = density - m_params.targetDensity;
		float pressure = densityError * m_params.pressureMultiplier;
		return pressure;
	}

	void SimulationStep(float deltaTime)
	{
		UpdateDensities();

		// Second, calculate forces and update velocities
		for (int i = 0; i < m_particles.size(); i++)
		{
			m_particles[i].velocity += glm::vec3(0.0f, 1.0f, 0.0f) * m_params.gravity * deltaTime;

			glm::vec3 pressureForce = CalculatePressureForce(i);

			if (m_particles[i].density > 0)
			{
				glm::vec3 pressureAcceleration = pressureForce / m_particles[i].density;
				m_particles[i].velocity = pressureAcceleration * deltaTime;
			}
		}

		// Third, update positions and handle collisions
		for (int i = 0; i < m_particles.size(); i++)
		{
			m_particles[i].position += m_particles[i].velocity * deltaTime;
			ResolveContainerCollisions(i);
		}
	}
};

