#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <random>
#include <algorithm>
#include "VulkanStructures.h"

namespace utils
{
	float randomFloat(float min, float max)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());

		std::uniform_real_distribution<float> dis(min, max);

		return dis(gen);
	}

	glm::vec3 lerpColor2(float t, glm::vec3 c1, glm::vec3 c2)
	{
		return c1 * (1.0f - t) + (c2 * t);
	}

	glm::vec3 lerpColorVector(float t, const std::vector<std::pair<float, glm::vec3>>& colorStops)
	{
		t = std::clamp(t, 0.0f, 1.0f);

		if (colorStops.empty()) {
			return glm::vec3(0.0f);
		}
		if (colorStops.size() == 1) {
			return colorStops[0].second;
		}

		float t0 = colorStops[0].first;
		float t1 = colorStops[colorStops.size() - 1].first;

		if (t <= t0) return colorStops[0].second;
		if (t >= t1) return colorStops[colorStops.size() - 1].second;

		for (size_t i = 0; i < colorStops.size() - 1; ++i) {
			if (t >= colorStops[i].first && t <= colorStops[i + 1].first) {
				float segmentT = (t - colorStops[i].first) /
					(colorStops[i + 1].first - colorStops[i].first);

				return colorStops[i].second * (1.0f - segmentT) +
					(colorStops[i + 1].second * segmentT);
			}
		}

		return colorStops[0].second;
	}

	float SmoothingKernal(float radius, float dst)
	{
		float volume = glm::pi<float>() * std::powf(radius, 8) / 4.0f;
		float value = std::max(0.0f, radius * radius - dst * dst);
		return value * value * value / volume;
	}

	float CaculateDensity(glm::vec3 samplePoint, float smoothingRadius, std::vector<Particle> particles)
	{
		float density = 0;
		const float mass = 1;

		for (auto p : particles)
		{
			float dst = glm::length(p.position - samplePoint);
			float influence = SmoothingKernal(smoothingRadius, dst);
			density += mass * influence;
		}

		return density;
	}

	/*float CalculateProperty(glm::vec3 samplePoint, float smoothingRadius, std::vector<Particle> particles)
	{
		float property;

		for (int i = 0; i < particles.size(); i++)
		{
			float dst = glm::length(particles[i].position - samplePoint);
			float influence = SmoothingKernal(smoothingRadius, dst);
			float density = CaculateDensity(particles[i].position, smoothingRadius, particles);
			property += particleProperties[i]
		}
	}*/

}
