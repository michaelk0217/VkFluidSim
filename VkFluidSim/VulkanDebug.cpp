#include "VulkanDebug.h"
#include <iostream>

namespace vks
{
	namespace debug
	{
		bool logToFile{ false };
		std::string logFileName{ "validation_out.txt" };

		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
		VkDebugUtilsMessengerEXT debugUtilsMessenger;

		
		VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessageCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
			VkDebugUtilsMessageTypeFlagsEXT messageType, 
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
			void* pUserData)
		{
			// Select prefix depending on flags passed to the callback
			std::string prefix;

			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			{
				prefix = "VERBOSE: ";
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			{
				prefix = "INFO: ";
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			{
				prefix = "WARNING: ";
			}
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			{
				prefix = "ERROR: ";
			}

			//Display message to defualt output (console/logcat)
			std::stringstream debugMessage;
			if (pCallbackData->pMessageIdName)
			{
				debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "][" << pCallbackData->pMessageIdName << "] : " << pCallbackData->pMessage;
			}
			else
			{
				debugMessage << prefix << "[" << pCallbackData->messageIdNumber << "] : " << pCallbackData->pMessage;
			}

			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			{
				std::cerr << debugMessage.str() << "\n\n";
			}
			else
			{
				std::cout << debugMessage.str() << "\n\n";
			}
			if (logToFile)
			{
				//log(debugMessage.str());
			}
			fflush(stdout);

			// The return value of this callback controls whether the Vulkan call that caused the validation message will be aborted or not
			// We return VK_FALSE as we DON'T want Vulkan calls that cause a validation message to abort
			// If you instead want to have calls abort, pass in VK_TRUE and the function will return VK_ERROR_VALIDATION_FAILED_EXT
			return VK_FALSE;
		}

		void setupDebugging(VkInstance instance)
		{
			vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
			vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

			VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
			setupDebugingMessengerCreateInfo(debugUtilsMessengerCI);
			VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCI, nullptr, &debugUtilsMessenger);
			assert(result == VK_SUCCESS);
		}

		void freeDebugCallback(VkInstance instance)
		{
			if (debugUtilsMessenger != VK_NULL_HANDLE)
			{
				vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
			}
		}

		void setupDebugingMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCI)
		{
			debugUtilsMessengerCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			debugUtilsMessengerCI.pfnUserCallback = debugUtilsMessageCallback;
		}

	}

	namespace debugutils
	{
		PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{ nullptr };
		PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{ nullptr };
		PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT{ nullptr };

		void setup(VkInstance instance)
		{
			vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
			vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
			vkCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
		}

		void cmdBeginLabel(VkCommandBuffer cmdbuffer, std::string caption, glm::vec4 color)
		{
			if (!vkCmdBeginDebugUtilsLabelEXT) {
				return;
			}
			VkDebugUtilsLabelEXT labelInfo{};
			labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			labelInfo.pLabelName = caption.c_str();
			memcpy(labelInfo.color, &color[0], sizeof(float) * 4);
			vkCmdBeginDebugUtilsLabelEXT(cmdbuffer, &labelInfo);
		}

		void cmdEndLabel(VkCommandBuffer cmdbuffer)
		{
			if (!vkCmdEndDebugUtilsLabelEXT) {
				return;
			}
			vkCmdEndDebugUtilsLabelEXT(cmdbuffer);
		}

	};
}