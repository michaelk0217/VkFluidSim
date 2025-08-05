#pragma once


#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <array>
#include <chrono>

#include "VulkanInstance.h"
#include "VulkanSurface.h"
#include "VulkanDevices.h"
#include "VulkanSwapChain.h"
#include "VulkanDescriptorPool.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanUniformBuffer.h"
#include "VulkanPipelineLayout.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanCommandPool.h"
#include "VulkanDepthResources.h"
#include "VulkanDescriptorSets.h"
#include "VulkanCommandBuffers.h"
#include "VulkanSyncPrimitives.h"

#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"

#include "VulkanStructures.h"
#include "VulkanDebug.h"
#include "VulkanTools.h"

#include "Window.h"
#include "Camera.hpp"
#include "ImGuiManager.h"
#include "FluidSimulator.h"

//#define VK_USE_PLATFORM_WIN32_KHR

#define MAX_CONCURRENT_FRAMES 2

class VulkanApp
{
private:
    std::unique_ptr<Window> window;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<VulkanInstance> instance;
    std::unique_ptr<VulkanSurface> surface;
    std::unique_ptr<VulkanDevices> devices;
    std::unique_ptr<VulkanSwapChain> swapChain;
    std::unique_ptr<VulkanDescriptorPool> descriptorPool;
    

    std::unique_ptr<VulkanCommandPool> commandPool;
    std::unique_ptr<VulkanDepthResources> depthResourcesObj;

    std::unique_ptr<VulkanUniformBuffers> frameUBO;

    std::unique_ptr<VulkanCommandBuffers> commandBuffers;
    std::unique_ptr<VulkanSyncPrimitives> syncObj;


    std::unique_ptr<ImGuiManager> imguiManager;

    std::unique_ptr<FluidSimulator> fluidSimulator;




    std::vector<float> frame_history;
    void update_frame_history(float framerate)
    {
        if (!frame_history.empty())
        {
            frame_history.erase(frame_history.begin());
        }
        frame_history.push_back(framerate);
    }

    // simulation run reset
    bool runSim = false;
    bool resetSim = true;


    bool resized = false;
    uint32_t width = 2560;
    uint32_t height = 1440;
    const std::string windowTitle = "Fluid Simulation";

    

    uint32_t currentFrame = 0;

    struct Settings {
        bool validation = true;
        bool fullscreen = false;
        bool vsync = false;
        bool overlay = true;
    } settings;

    const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_dynamic_rendering",
        "VK_KHR_synchronization2"
    };


    void initGlfwWindow();
    void initVulkan();
    void mainLoop();
    void drawFrame(float deltaTime);
    void windowResize();
    void cleanUp();
    
    //void initializeParticles(uint32_t num);
    //void updateParticles(float deltaTime);
    //void updateContainer();

protected:
   
public:
    void run()
    {
        initGlfwWindow();
        initVulkan();
        mainLoop();
        cleanUp();
    }
};