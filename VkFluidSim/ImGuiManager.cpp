#include "ImGuiManager.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <glm/glm.hpp>
#include <stdio.h>

#include "Window.h"
#include "VulkanDevices.h"
#include "VulkanSurface.h"
#include "VulkanSwapChain.h"
#include "VulkanCommandPool.h"

#include "VulkanTools.h"

ImGuiManager::ImGuiManager(
	Window& window,
	VulkanInstance& instance,
	VulkanDevices& devices,
	VulkanSurface& surface,
	VulkanSwapChain& swapChain,
	VulkanCommandPool& commandPool) :
	m_devices(devices), m_swapChain(swapChain)
{
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(m_devices.getLogicalDevice(), &pool_info, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool for ImGui!");
    }

    ImGui::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(window.getGlfwWindow(), true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = VK_API_VERSION_1_4;
    initInfo.Instance = instance.getInstance();
    initInfo.PhysicalDevice = devices.getPhysicalDevice();
    initInfo.Device = devices.getLogicalDevice();
    initInfo.QueueFamily = devices.getFamilyIndices().graphicsFamily.value();
    initInfo.Queue = devices.getGraphicsQueue();
    initInfo.DescriptorPool = m_descriptorPool;
    initInfo.MinImageCount = swapChain.getImageViews().size();
    initInfo.ImageCount = swapChain.getImageViews().size();
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;

    initInfo.UseDynamicRendering = true;
    //initInfo.PipelineRenderingCreateInfo
    //VkFormat depthFormat;
    VkFormat colorAttachmentFormat = swapChain.getColorFormat();
    //vks::tools::getSupportedDepthFormat(devices.getPhysicalDevice(), &depthFormat);
    VkPipelineRenderingCreateInfoKHR pipelineRenderingCI{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
    pipelineRenderingCI.colorAttachmentCount = 1;
    pipelineRenderingCI.pColorAttachmentFormats = &colorAttachmentFormat;
    pipelineRenderingCI.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipelineRenderingCI.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    initInfo.PipelineRenderingCreateInfo = pipelineRenderingCI;

    ImGui_ImplVulkan_Init(&initInfo);
}

ImGuiManager::~ImGuiManager()
{
    vkDeviceWaitIdle(m_devices.getLogicalDevice());

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(m_devices.getLogicalDevice(), m_descriptorPool, nullptr);
}

void ImGuiManager::newFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::buildUI(UiContextPacket& uiPacket)
{
    // Assumes you have some variables to display, for example:
// (In a real app, you would update these values every frame)
    //static float frame_history[90] = { /* ... fps data ... */ };
    
    static bool show_physics_colliders = false;

    // --- UI Code Starts Here ---

    // Use a more transparent style for an overlay
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.12f, 0.75f));
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
    if (ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav)) {

        // --- Performance Metrics ---
        ImGui::Text("Performance Stats");
        ImGui::Separator();

        // Plot a simple graph of frame rate history
        char overlay_text[32];
        sprintf_s(overlay_text, "FPS: %.1f", 1.0f / uiPacket.deltaTime);
        ImGui::PlotLines("##framerate", uiPacket.frameHistory.data(), static_cast<int>(uiPacket.frameHistory.size()), 0, overlay_text, 0.0f, 3000.0f, ImVec2(0, 50));
        ImGui::Text("Frame Time: %.7f ms", uiPacket.deltaTime);

    }
    ImGui::End();
    ImGui::PopStyleColor();


    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
    if (ImGui::Begin("Simulation")) {

        ImGui::Text("Controls");
        ImGui::Separator();
        if (uiPacket.parameters.runSimulation) {
            uiPacket.parameters.runSimulation = !ImGui::Button("Stop");
        }
        else {
            uiPacket.parameters.runSimulation = ImGui::Button("Run");
        }
        if (!uiPacket.parameters.runSimulation) {
            uiPacket.parameters.resetSimulation = ImGui::Button("Reset");
        }


        ImGui::Text("Border Dimensions");
        ImGui::Separator();
        ImGui::DragFloat("Width", &uiPacket.parameters.boxHalfWidth, 0.01f, 0.1f, 4.0f);
        ImGui::DragFloat("Height", &uiPacket.parameters.boxHalfHeight, 0.01f, 0.1f, 4.0f);
        ImGui::Separator();
        ImGui::Text("Particle Behavior");
        ImGui::Separator();
        ImGui::DragFloat("Collision Damping", &uiPacket.parameters.collisionDamping, 0.01f, 0.0f, 1.0f);


        //if (ImGui::Button("Crash the App")) {
        //    // For testing purposes, of course
        //    *(int*)0 = 0;
        //}
    }
    ImGui::End();
}

void ImGuiManager::render(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t width, uint32_t height)
{
    ImGui::Render();

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = m_swapChain.getImageViews()[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Use LOAD to draw on top of your scene
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = { 0, 0, width, height };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    vkCmdEndRendering(commandBuffer);
}

