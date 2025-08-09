#include "ImGuiManager.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <glm/glm.hpp>
#include <stdio.h>
#include <algorithm>

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

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.12f, 0.75f));
    ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Once);
    if (ImGui::Begin("Simulation")) {

        ImGui::Separator();
        ImGui::Text("Controls");
        if (uiPacket.parameters.runSimulation) {
            uiPacket.parameters.runSimulation = !ImGui::Button("Stop");
        }
        else {
            uiPacket.parameters.runSimulation = ImGui::Button("Run");
        }
        if (!uiPacket.parameters.runSimulation) {
            if (ImGui::Button("Reset"))
            {
                uiPacket.parameters.resetSimulation = true;
            }
        }

        ImGui::Separator();
        ImGui::Text("Particle Behavior");
        ImGui::DragFloat("Particle Size", &uiPacket.parameters.particleWorldRadius, 0.005f, 0.001f, 0.1f);
        ImGui::DragFloat("Collision Damping", &uiPacket.parameters.collisionDamping, 0.01f, 0.0f, 1.0f);
        if (!uiPacket.parameters.runSimulation)
        {
            if (ShowUint32Input("Particle Count", uiPacket.parameters.particleCount, 0, 50000))
            {
                uiPacket.parameters.resetSimulation = true;
            }
        }
        ImGui::DragFloat("Mass", &uiPacket.parameters.mass, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Gravity", &uiPacket.parameters.gravity, 0.01f, 0.0f, 9.0f);
        ImGui::DragFloat("Smoothing Radius", &uiPacket.parameters.smoothingRadius, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Target Density", &uiPacket.parameters.targetDensity, 0.01f,1.0f, 10.0f);
        ImGui::DragFloat("Pressure Multiplier", &uiPacket.parameters.pressureMultiplier, 0.01f, 0.0f, 500.0f);



        ImGui::Separator();
        ImGui::Text("Color Mapping");
        ImGui::DragFloat("Max Speed", &uiPacket.parameters.maxSpeedForColor, 0.1f, 0.1f, 50.0f);
        ImGui::Separator();
        ShowColorPickerRGB("Color 1 (min)", uiPacket.parameters.colorPoints.color1);
        ShowColorPickerRGB("Color 2", uiPacket.parameters.colorPoints.color2);
        ShowColorPickerRGB("Color 3 (max)", uiPacket.parameters.colorPoints.color3);
        //ShowColorPickerRGB("Color 4 (max)", uiPacket.parameters.colorPoints.color4);

        ImGui::Separator();
        ImGui::Text("Border Dimensions");
        ImGui::DragFloat("Width", &uiPacket.parameters.boxHalfWidth, 0.01f, 0.1f, 30.0f);
        ImGui::DragFloat("Height", &uiPacket.parameters.boxHalfHeight, 0.01f, 0.1f, 30.0f);
        ImGui::Separator();
        
           
    }
    ImGui::End();
    ImGui::PopStyleColor();
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

bool ImGuiManager::ShowUint32Input(const char* label, uint32_t& value, uint32_t min_value, uint32_t max_value)
{
    int temp_value = static_cast<int>(value);

    if (ImGui::InputInt(label, &temp_value, 1, 100)) {
        temp_value = std::clamp(temp_value, static_cast<int>(min_value), static_cast<int>(max_value));
        if (temp_value < 0) {
            temp_value = 0; 
        }
        value = static_cast<uint32_t>(temp_value); 
        
        return true;
    }
    return false;
}

bool ImGuiManager::ShowColorPickerRGB(const char* label, glm::vec3& color)
{
    float tempColor[3] = { color.r, color.g, color.b };
    if (ImGui::ColorEdit3(label, tempColor, ImGuiColorEditFlags_DisplayRGB))
    {
        color.r = tempColor[0];
        color.g = tempColor[1];
        color.b = tempColor[2];
        return true;
    }
    return false;
}

