#pragma once

#include <vulkan/vulkan.h>

#include <math.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <vector>
#include <sstream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vks
{
	namespace debug
	{
		extern bool logToFile;
		extern std::string logFileName;

		// Defualt debug callback
		VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		// Load debug function pointers and set debug callback
		void setupDebugging(VkInstance instance);
		// Clear debug callback
		void freeDebugCallback(VkInstance instance);
		// Used to populate a VkDebugUtilsMessengerCreateInfoEXT withour example messenger function and desired flags
		void setupDebugingMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCI);
		//void log(std::string message);
	}

	// Wrapper for the VK_EXT_debug_utils extension
	// These can be used to name Vulkan objects for debugging tools like RenderDoc
	namespace debugutils
	{
		void setup(VkInstance instance);
		void cmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color);
		void cmdEndLabel(VkCommandBuffer cmdbuffer);
	}
}