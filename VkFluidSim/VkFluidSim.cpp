// VkFluidSim.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "VulkanApp.h"

int main()
{
    VulkanApp* vulkanApp = new VulkanApp();
    vulkanApp->run();
    delete(vulkanApp);
    return 0;
}

