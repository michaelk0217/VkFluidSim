#include "VulkanApp.h"


void VulkanApp::initGlfwWindow()
{
    try {
        window = std::make_unique<Window>(width, height, windowTitle);

        window->setAppFramebufferResizeCallback([this](int width, int height)
            {
                this->resized = true;
            }
        );

    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Window Initialization Error: " << e.what() << std::endl;
        throw;
    }
}

void VulkanApp::initVulkan()
{
    instance = std::make_unique<VulkanInstance>();
    instance->setupDebugging();
    
    surface = std::make_unique<VulkanSurface>(instance->getInstance(), window->getGlfwWindow());
    
    devices = std::make_unique<VulkanDevices>(instance->getInstance(), surface->getSurface(), deviceExtensions, validationLayers);
    
    swapChain = std::make_unique<VulkanSwapChain>(instance->getInstance(), surface->getSurface(), devices->getLogicalDevice(), devices->getPhysicalDevice(), window->getGlfwWindow());
    swapChain->create(width, height);
    
    descriptorPool = std::make_unique<VulkanDescriptorPool>(devices->getLogicalDevice());
    
   /* descriptorSetLayout = std::make_unique<VulkanDescriptorSetLayout>(devices->getLogicalDevice());
    
    pipelineLayout = std::make_unique<VulkanPipelineLayout>(devices->getLogicalDevice(), descriptorSetLayout->getDescriptorSetLayout());*/
    


    commandPool = std::make_unique<VulkanCommandPool>(devices->getLogicalDevice(), devices->getPhysicalDevice(), surface->getSurface());

    depthResourcesObj = std::make_unique<VulkanDepthResources>();
    depthResourcesObj->create(devices->getLogicalDevice(), devices->getPhysicalDevice(), devices->getGraphicsQueue(), commandPool->getCommandPool(), swapChain->getExtent());

    frameUBO = std::make_unique<VulkanUniformBuffers>();
    frameUBO->create(devices->getLogicalDevice(), devices->getPhysicalDevice(), MAX_CONCURRENT_FRAMES, sizeof(ShaderData));

   /* frameUboDescriptorSets = std::make_unique<VulkanDescriptorSets>();
    frameUboDescriptorSets->create(devices->getLogicalDevice(), descriptorPool->getDescriptorPool(), descriptorSetLayout->getDescriptorSetLayout(), MAX_CONCURRENT_FRAMES);*/
    
    commandBuffers = std::make_unique<VulkanCommandBuffers>();
    commandBuffers->create(devices->getLogicalDevice(), commandPool->getCommandPool(), MAX_CONCURRENT_FRAMES);

    syncObj = std::make_unique<VulkanSyncPrimitives>();
    syncObj->create(devices->getLogicalDevice(), MAX_CONCURRENT_FRAMES, static_cast<uint32_t>(swapChain->getImageViews().size()));

  
    camera = std::make_unique<Camera>();
    camera->type = Camera::CameraType::lookat;
    camera->setPosition(glm::vec3(0.0f, 0.0f, -15.0f));
    camera->setRotation(glm::vec3(0.0f));
    camera->setPerspective(60.0f, (float)width / (float)height, 1.0f, 500.0f);

    imguiManager = std::make_unique<ImGuiManager>(*window, *instance, *devices, *surface, *swapChain, *commandPool);

    fluidSimulator = std::make_unique<FluidSimulator>(
        *devices, 
        commandPool->getCommandPool(), 
        swapChain->getColorFormat(), 
        descriptorPool->getDescriptorPool(),
        frameUBO->getBuffers()
        //*frameUboDescriptorSets
    );
}

void VulkanApp::mainLoop()
{
    auto lastTime = std::chrono::high_resolution_clock::now();
    frame_history.resize(90, 0);
    //initializeParticles(numParticles);

    while (window && !window->shouldClose())
    {
        window->pollEvents();

        // delta time calculation
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
        lastTime = currentTime;

        update_frame_history(1.0f / deltaTime);

        imguiManager->newFrame();
        auto& simParams = fluidSimulator->getParameters();
        UiContextPacket uiPacket{ 
            deltaTime,
            frame_history,
            simParams
        };

        imguiManager->buildUI(uiPacket);
        drawFrame(deltaTime);
    }
}

void VulkanApp::drawFrame(float deltaTime)
{
    vkWaitForFences(devices->getLogicalDevice(), 1, &syncObj->waitFences[currentFrame], VK_TRUE, UINT64_MAX);
    VK_CHECK_RESULT(vkResetFences(devices->getLogicalDevice(), 1, &syncObj->waitFences[currentFrame]));

    uint32_t imageIndex{ 0 };
    VkResult result = swapChain->acquireNextImage(syncObj->presentCompleteSemaphores[currentFrame], imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // resize window
        resized = false;
        windowResize();
        return;
    }
    else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR))
    {
        throw "Could not acquire the next swap chain image";
    }

    ShaderData shaderData{};
    shaderData.projectionMatrix = camera->matrices.perspective;
    shaderData.viewMatrix = camera->matrices.view;
    shaderData.modelMatrix = glm::mat4(1.0f);
    shaderData.viewportHeight = (float)this->height;
    shaderData.fovy = glm::radians(camera->getFov());
    shaderData.particleWorldRadius = fluidSimulator->getParameters().particleWorldRadius;

    memcpy(frameUBO->getMappedMemory(currentFrame), &shaderData, sizeof(shaderData));

    vkResetCommandBuffer(commandBuffers->getCommandBuffer(currentFrame), 0);
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    const VkCommandBuffer commandBuffer = commandBuffers->getCommandBuffer(currentFrame);
    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

    
    if (fluidSimulator->getParameters().runSimulation)
    {
        vks::tools::insertMemoryBarrier2(
            commandBuffer,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
        );
    }
    fluidSimulator->update(deltaTime, commandBuffer);
    if (fluidSimulator->getParameters().runSimulation)
    {
        vks::tools::insertMemoryBarrier2(
            commandBuffer,
            VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT
        );
    }

    vks::tools::insertImageMemoryBarrier(
        commandBuffer,
        swapChain->getImages()[imageIndex],
        0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    );
    vks::tools::insertImageMemoryBarrier(
        commandBuffer,
        depthResourcesObj->getImage(),
        0,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 }
    );

    // dynamic rendering attachments
    // color attachment
    VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    colorAttachment.imageView = swapChain->getImageViews()[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
    // depth/stencil attachment
    VkRenderingAttachmentInfo depthStencilAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    depthStencilAttachment.imageView = depthResourcesObj->getDepthImageView();
    depthStencilAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthStencilAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthStencilAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthStencilAttachment.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO_KHR };
    renderingInfo.renderArea = { 0, 0, width, height };
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthStencilAttachment;
    renderingInfo.pStencilAttachment = &depthStencilAttachment;

    // start dynamic rendering
    vkCmdBeginRendering(commandBuffer, &renderingInfo);
    // update dynamic viewport state
    VkViewport viewport{ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    // update dynamic scissor state
    VkRect2D scissor{ 0, 0, width, height };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


    fluidSimulator->draw(commandBuffer, currentFrame, frameUBO->getBuffer(currentFrame));

    // finish the current dynamic rendering section
    vkCmdEndRendering(commandBuffer);

    imguiManager->render(commandBuffer, imageIndex, width, height);

    vks::tools::insertImageMemoryBarrier(
        commandBuffer,
        swapChain->getImages()[imageIndex],
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0,
        VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    );

    VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

    // submit command buffer to graphics queue

    // pipeline stage where the queue submission will wait (via waitSemaphores)
    VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo = vks::initializers::submitInfo();
    submitInfo.pWaitDstStageMask = &waitStageMask;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.commandBufferCount = 1;
    // Semaphore to wait upon before submitted command buffer starts executing
    submitInfo.pWaitSemaphores = &syncObj->presentCompleteSemaphores[currentFrame];
    submitInfo.waitSemaphoreCount = 1;
    // semaphore to ge signalled when command buffers have completed
    submitInfo.pSignalSemaphores = &syncObj->renderCompleteSemaphores[imageIndex];
    submitInfo.signalSemaphoreCount = 1;

    VK_CHECK_RESULT(vkQueueSubmit(devices->getGraphicsQueue(), 1, &submitInfo, syncObj->waitFences[currentFrame]));


    result = swapChain->queuePresent(devices->getPresentQueue(), imageIndex, syncObj->renderCompleteSemaphores[imageIndex]);
    if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
    {
        windowResize();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Could not present the image to the swap chain!");
    }

    currentFrame = (currentFrame + 1) % MAX_CONCURRENT_FRAMES;
}

void VulkanApp::windowResize()
{
    //window->waitForRestoredSize();
    int newWidth = 0, newHeight = 0;
    window->getFramebufferSize(newWidth, newHeight);
    while (newWidth == 0 || newHeight == 0)
    {
        window->getFramebufferSize(newWidth, newHeight);
        // todo: implement window->waitEvents() wait if window is minimized
    }
    vkDeviceWaitIdle(devices->getLogicalDevice());
    this->width = newWidth;
    this->height = newHeight;

    depthResourcesObj->cleanup();
    swapChain->cleanup();

    swapChain->create(width, height);
    depthResourcesObj->create(devices->getLogicalDevice(), devices->getPhysicalDevice(), devices->getGraphicsQueue(), commandPool->getCommandPool(), swapChain->getExtent());
}

void VulkanApp::cleanUp()
{
    vkDeviceWaitIdle(devices->getLogicalDevice());

    fluidSimulator.reset();
    imguiManager.reset();

    camera.reset();

    syncObj.reset();
    commandBuffers.reset();
    frameUBO.reset();
    depthResourcesObj.reset();
    commandPool.reset();



    //pipelineLayout.reset();
    //descriptorSetLayout.reset();
    descriptorPool.reset();
    swapChain.reset();
    devices.reset();
    surface.reset();
    instance.reset();
    window.reset();
}
