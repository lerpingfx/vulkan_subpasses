#include "VulkanExample.h"
#include <iostream>

int main()
{
	VulkanApp vulkanApp;
	std::cout << "Launching Vulkan app" << std::endl;
	try {
        	vulkanApp.run();
    	}
    	catch (const std::exception& e) {
        	std::cerr << e.what() << std::endl;
        	return EXIT_FAILURE;
    	}
	
	return EXIT_SUCCESS;

 }
	
