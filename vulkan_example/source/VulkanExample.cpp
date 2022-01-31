#include "VulkanExample.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <chrono>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm> // Necessary for std::clamp
#include <fstream>
#include <array>
#include <thread>
#include <math.h>  

#define VK_API_VERSION_VARIANT(version) ((uint32_t)(version) >> 29)
#define VK_API_VERSION_MAJOR(version) (((uint32_t)(version) >> 22) & 0x7FU)
#define VK_API_VERSION_MINOR(version) (((uint32_t)(version) >> 12) & 0x3FFU)

VulkanApp::VulkanApp(){
	
	glfwExtensionCount = 0;
	deviceCount = 0;
	physicalDevice = VK_NULL_HANDLE;
	frameID = 0;	
	blocked = false;
	
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanApp::run() {
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

std::vector<char> VulkanApp::readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary); // open file at end character
        if (!file.is_open()) {
            throw std::runtime_error("failed to open file");
        }

        size_t fileSize = (size_t)file.tellg(); // set buffer size based on last character pos
        std::vector<char> buffer(fileSize);

        file.seekg(0); // return to first char pos
        file.read(buffer.data(), fileSize); // copy to buffer

        file.close();

        return buffer;

}

VkVertexInputBindingDescription VulkanApp::Vertex::getBindingDescription() {
	// define how to pass vertex data to shader
		// if using separate array per attribute - need binding per attribute
	// for faster memory access, best to store all verticex attributes sequentenly, in a single array
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(VulkanApp::Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // per vertex or per instance data
	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> VulkanApp::Vertex::getAttributeDescriptions() {
	// define how to extract attributes from vertex data
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
	attributeDescriptions[0].binding = 0; // VkVertexInputBindingDescription.binding
	attributeDescriptions[0].location = 0; // location index in shader
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // byte size
	/*
		float: VK_FORMAT_R32_SFLOAT
		vec2: VK_FORMAT_R32G32_SFLOAT
		vec3: VK_FORMAT_R32G32B32_SFLOAT
		vec4: VK_FORMAT_R32G32B32A32_SFLOAT
		ivec2: VK_FORMAT_R32G32_SINT
		uvec4: VK_FORMAT_R32G32B32A32_UINT
		double: VK_FORMAT_R64_SFLOAT        */
	attributeDescriptions[0].offset = offsetof(Vertex, pos);  // attribute's offset

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, uv);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, normal);

	return attributeDescriptions;
}

void VulkanApp::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Do not create OpenGL context
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Disable window resize
        window = glfwCreateWindow(WIDTH, HEIGHT, "VulkanTest", nullptr, nullptr);
        // (Width, height, name, display, opengl-relevant)
    }

void VulkanApp::checkExtensionLayersSupport() { // check if requetsted validaiton layers are available. return false if not.
        std::cout << "requested layers:\n";
        for (int i = 0; i < layers.size(); i++)
        {
            std::cout << layers[i] << '\n';
        }
        std::cout << '\n';

        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount); // vector to store available validation layers (and their properties)
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); // store available layers in vector

        std::cout << "available layers:\n";
        for (int i = 0; i < layerCount; i++)
        {
            std::cout << availableLayers[i].layerName << '\n';
        }
        std::cout << '\n';

        // iterate through requested layers
        for (int i = 0; i < layers.size(); i++) {
            bool layerFound = false;

            // iterate through available layers
            for (int j = 0; j < layerCount; j++)
            {
                if (strcmp(layers[i], availableLayers[j].layerName) == 0) // if requested layer == available layer, return 0
                {
                    layerFound = true;
                    std::cout << "requested validation layer is supported: " << layers[i] << '\n';
                    break;
                }
            }

            if (!layerFound)
            {
                throw std::runtime_error("requested validation layer is not supported: " + std::string(layers[i]));
            }

        }
        std::cout << '\n';
}

std::vector<const char*> VulkanApp::getRequiredInstanceExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwRequiredExtensions;
        glfwRequiredExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); // get list of glfw-required extensions

        std::vector<const char*> instanceExtensions(glfwRequiredExtensions, glfwRequiredExtensions + glfwExtensionCount);

        std::cout << "extensions required by GLFW:\n";
        for (int i = 0; i < glfwExtensionCount; i++) {
            std::cout << glfwRequiredExtensions[i] << '\n';
        }
        std::cout << '\n';

        if (enableExtensionLayers) {
            instanceExtensions.push_back("VK_EXT_debug_utils"); // add debug-utils extension
        }

        instanceExtensions.push_back("VK_KHR_get_physical_device_properties2");

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr); // get number of supported vulkan extensions

        std::vector<VkExtensionProperties> availableExtensions(extensionCount); // create vector to hold data on supported extensions
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());  // retrieve data on supported extensions

        std::cout << "available extensions: " << '\n';
        for (int i = 0; i < availableExtensions.size(); i++)
        {
            std::cout << availableExtensions[i].extensionName << '\n';
        }
        std::cout << '\n';

        // iterate through requested layers
        for (int i = 0; i < instanceExtensions.size(); i++) {

            bool extensionFound = false;
            // iterate through available layers
            for (int j = 0; j < availableExtensions.size(); j++)
            {
                if (strcmp(instanceExtensions[i], availableExtensions[j].extensionName) == 0) // if requested layer == available layer, return 0
                {
                    extensionFound = true;
                    std::cout << "requested extension is supported: " << instanceExtensions[i] << '\n';

                }
            }
            if (!extensionFound)
            {
                throw std::runtime_error("requested extension is not supported: " + std::string(instanceExtensions[i]));
            }
        }
        std::cout << '\n';

        return instanceExtensions;
}

void VulkanApp::createInstance() {

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Example - Subpasses";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (enableExtensionLayers) {
            checkExtensionLayersSupport();
            std::cout << "------------------------\n";
            createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
            createInfo.ppEnabledLayerNames = layers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }


        std::vector<const char*> instanceExtensions = getRequiredInstanceExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();


        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("vkCreateInstance() - failed to create vulkan instance");
        }
}

void VulkanApp::loadExtensionFunctions() {
    *(void**)&_vkCmdPipelineBarrier2KHR = (void*)vkGetInstanceProcAddr(instance, "vkCmdPipelineBarrier2KHR");
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanApp::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "<<validation layer>> " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
}

void VulkanApp::setupDebugMessenger() {
        if (!enableExtensionLayers) return;
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr;
        VkResult res = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }

}

bool VulkanApp::QueueFamilyIndices::isComplete() {
		return (graphicsFamilyIndex.has_value() && presentFamilyIndex.has_value() && computeFamilyIndex.has_value() && transferFamilyIndex.has_value()); // if value was asigned, will return true
}

VulkanApp::QueueFamilyIndices VulkanApp::findQueueFamilies(VkPhysicalDevice device) {

        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> deviceSupportQueueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, deviceSupportQueueFamilies.data());

        //VK_QUEUE_GRAPHICS_BIT == 0x00000001 == 00000001
        //VK_QUEUE_COMPUTE_BIT == 0x00000002 ==  00000010
        //VkQueueFlagBits queueFamilyBitflags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT; // == 00000011

        int i = 0;

        // iterate through supported queue families.
        // for each family, check if "graphics", "compute", "present" and "transfer" queues are supported.
        // if so, assign their index 
        for (int j = 0; j < queueFamilyCount; j++) {

            uint32_t bitflag = deviceSupportQueueFamilies[j].queueFlags;
            if ((bitflag & VK_QUEUE_GRAPHICS_BIT) == 1)  // 0000 0001
            {
                indices.graphicsFamilyIndex = i;
            }
            if ((bitflag & VK_QUEUE_COMPUTE_BIT) == 2)   // 0000 0010
            {
                indices.computeFamilyIndex = i;
            }
            if ((bitflag & VK_QUEUE_TRANSFER_BIT) == 4)  // 0000 0100
            {
                indices.transferFamilyIndex = i;
            }

            VkBool32 presentQueueFamilySupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentQueueFamilySupport);
            if (presentQueueFamilySupport) {
                indices.presentFamilyIndex = i;
            }

            if (indices.isComplete()) {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);
                std::cout << "device supports graphics, present and compute queue families: " << deviceProperties.deviceName << '\n';
                break;
            }

            i++;
        }
        return indices;
}

bool VulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, supportedExtensions.data());


        for (int i = 0; i < deviceExtensions.size(); i++) {
            bool extensionFound = false;

            // iterate through available layers
            for (int j = 0; j < extensionCount; j++)
            {
                if (strcmp(deviceExtensions[i], supportedExtensions[j].extensionName) == 0) // if requested layer == available layer, return 0
                {
                    extensionFound = true;
                    std::cout << "device extension is supported: " << deviceExtensions[i] << '\n';
                    break;
                }
            }

            if (!extensionFound)
            {
                throw std::runtime_error("device extension is not supported: " + std::string(deviceExtensions[i]));
            }

        }

        return true;

}

VulkanApp::SwapChainSupportDetails VulkanApp::querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        // get device surface capabilities (amount of images, width/height)
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.surfaceCapabilities);

        // get device supported surface formats (pixel format, color space)
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.surfaceFormats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.surfaceFormats.data());
        }

        // get device supported present modes (conditions for "swapping" images to the screen)
        // 
        //VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the screen right away, which may result in tearing.
        //VK_PRESENT_MODE_FIFO_KHR : The swap chain is a queue where the display takes an image from the front of the queue when the display is refreshedand the program inserts rendered images at the back of the queue.If the queue is full then the program has to wait.This is most similar to vertical sync as found in modern games.The moment that the display is refreshed is known as "vertical blank".
        //VK_PRESENT_MODE_FIFO_RELAXED_KHR : This mode only differs from the previous one if the application is lateand the queue was empty at the last vertical blank.Instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives.This may result in visible tearing.
        //VK_PRESENT_MODE_MAILBOX_KHR : This is another variation of the second mode.Instead of blocking the application when the queue is full, the images that are already queued are simply replaced with the newer ones.This mode can be used to render frames as fast as possible while still avoiding tearing, resulting in fewer latency issues than standard vertical sync.This is commonly known as "triple buffering", although the existence of three buffers alone does not necessarily mean that the framerate is unlocked.

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
}

int VulkanApp::rateDeviceSuitability(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        int score = 0;

        std::cout << "evaluating device: " << deviceProperties.deviceName << '\n';
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            std::cout << "device is a discreet GPU: " << deviceProperties.deviceName << '\n';
            score += 1500;
        }

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            std::cout << "device is an integrated GPU: " << deviceProperties.deviceName << '\n';
            score += 1000;
        }

        score += deviceProperties.limits.maxImageDimension2D;

        if (!deviceFeatures.geometryShader) {
            return 0;
        }
        std::cout << "device supports geometry shaders: " << deviceProperties.deviceName << '\n';

        if (!deviceFeatures.samplerAnisotropy) {
            return 0;
        }
        std::cout << "device supports anisotropic filtering: " << deviceProperties.deviceName << '\n';


        // check if required queues are supported by device (graphics queue)
        QueueFamilyIndices indices = findQueueFamilies(device);
        if (!indices.isComplete()) {
            return 0;
        }

        bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainSupported = false;
        if (!extensionsSupported) {
            std::cout << "required extensions not supported" << '\n';
            return 0;
        }
        else {
            SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(device);
            bool surfaceFormatsSupported = !swapChainSupportDetails.surfaceFormats.empty();
            bool presentModesSupported = !swapChainSupportDetails.presentModes.empty();
            if (surfaceFormatsSupported && presentModesSupported) {
                swapChainSupported = true;
            }
            else {
                std::cout << "required surface formats or present modes not supported" << '\n';
            }
        }
        if (!swapChainSupported) {
            std::cout << "swap chain not supported" << '\n';
            return 0;
        }

        return score;

}

void VulkanApp::pickPhysicalDevice() {
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to failed GPUs with vulkan support");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        std::multimap<int, VkPhysicalDevice> candidates;

        std::cout << '\n' << "rating available devices:" << '\n';
        for (int i = 0; i < devices.size(); i++) {
            int score = rateDeviceSuitability(devices[i]);
            candidates.insert(std::make_pair(score, devices[i]));
        }

        if (candidates.rbegin()->first > 0) {
            physicalDevice = candidates.rbegin()->second;
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
            std::cout << '\n' << "picked device: " << deviceProperties.deviceName << '\n';
            uint32_t variant = VK_API_VERSION_VARIANT(deviceProperties.apiVersion);
            uint32_t major = VK_API_VERSION_MAJOR(deviceProperties.apiVersion);
            uint32_t minor = VK_API_VERSION_MINOR(deviceProperties.apiVersion);
            std::cout << "device supports Vulkan API variant: " << std::to_string(variant) << '\n';
            std::cout << "device supports Vulkan API version: " << std::to_string(major) << "." << std::to_string(minor) << '\n';
        }
        else {
            throw std::runtime_error("failed to find a suitable gpu based on requirements");
        }

}

void VulkanApp::createLogicalDeviceAndQueues() {
        // setup info struct for required queues

        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
        std::set<uint32_t> uniqueQueueFamilies = {
            queueFamilyIndices.graphicsFamilyIndex.value(),
            queueFamilyIndices.presentFamilyIndex.value(),
            queueFamilyIndices.computeFamilyIndex.value(),
            queueFamilyIndices.transferFamilyIndex.value()
        };

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        float queuePriority = 1.0f;
        for (uint32_t queueFamilyIndex : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableExtensionLayers) {
            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
            deviceCreateInfo.ppEnabledLayerNames = layers.data();
        }
        else {
            deviceCreateInfo.enabledLayerCount = 0;
        }

        // create logical device
        VkResult deviceCreated = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
        if (deviceCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device");
        }
        std::cout << "logical device created" << '\n';

        // retrieve handle to newly created device queues
        vkGetDeviceQueue(device, queueFamilyIndices.graphicsFamilyIndex.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, queueFamilyIndices.presentFamilyIndex.value(), 0, &presentQueue);
        vkGetDeviceQueue(device, queueFamilyIndices.computeFamilyIndex.value(), 0, &computeQueue);
        vkGetDeviceQueue(device, queueFamilyIndices.transferFamilyIndex.value(), 0, &transferQueue);

        std::cout << "graphics, present and compute queues created" << '\n';

}

void VulkanApp::createSurface() {
        VkResult surfaceCreated = glfwCreateWindowSurface(instance, window, nullptr, &surface);
        if (surfaceCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface");
        }
}

VkSurfaceFormatKHR VulkanApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        // iterate through available surface format and color spaces
        // pick "B8G8R8A8_SRGB" && "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR"
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        // if not found, pick first available format:
        return availableFormats[0];
}

VkPresentModeKHR VulkanApp::chooseSwapPresentMode(VkPhysicalDevice device, const std::vector<VkPresentModeKHR>& availablePresentModes) {
        // if device is embedded, use VK_PRESENT_MODE_FIFO_KHR
        // if device is discreet, use VK_PRESENT_MODE_MAILBOX_KHR
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // iterate through available present modes. If device is discreet, and "MAILBOX" is available, pick that.
        for (const auto& availablePresentMode : availablePresentModes) {
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        // otherwise, pick "FIFO" (vertical sync)
        return VK_PRESENT_MODE_FIFO_KHR;
}

   
VkExtent2D VulkanApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        // if device maximum supported resolution is not UINT32_MAX (4294967295U), use default resolution (matching window)
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        else { // some devices allow maximum resolution of UINT32_MAX. In such case, manually set to window size.
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
}

void VulkanApp::createSwapChain() {
        SwapChainSupportDetails swapChainSupportDetails = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupportDetails.surfaceFormats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(physicalDevice, swapChainSupportDetails.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupportDetails.surfaceCapabilities);

        // request +1 than minimum required images, to avoid waiting on driver
        uint32_t imageCount = swapChainSupportDetails.surfaceCapabilities.minImageCount + 1;

        // if we request more images than supported, set to maximum supported amount
        if (imageCount > swapChainSupportDetails.surfaceCapabilities.maxImageCount && swapChainSupportDetails.surfaceCapabilities.maxImageCount > 0) {
            imageCount = swapChainSupportDetails.surfaceCapabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = surface;
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = extent;
        swapchainCreateInfo.imageArrayLayers = 1; // layers per image. set 2 for stereoscopic
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // render directly to swapchain image's color attachment
        // can also set to VK_IMAGE_USAGE_TRANSFER_DST_BIT, for tranferring offscreen buffer data to swapchain images

        /*
        VK_SHARING_MODE_EXCLUSIVE: An image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family. This option offers the best performance.
        VK_SHARING_MODE_CONCURRENT: Images can be used across multiple queue families without explicit ownership transfers.
        */

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);


        // if graphics and present queues are separate, share images across graphics & present (+transfer) queues
        if (indices.graphicsFamilyIndex != indices.presentFamilyIndex) {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            uint32_t queueFamilyIndices[] = { indices.graphicsFamilyIndex.value(), indices.presentFamilyIndex.value() };
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            ;
        }

        // transform swapchain images
        swapchainCreateInfo.preTransform = swapChainSupportDetails.surfaceCapabilities.currentTransform;

        // specify alpha blending with other windows (in window system)
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        swapchainCreateInfo.presentMode = presentMode;

        // clip pixels that are occluded by other windows
        // disable if need to read back pixels
        swapchainCreateInfo.clipped = VK_TRUE;

        // need to recreate swapchain when resizing window/surface.
        // for this, need reference to old swapchain
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        VkResult swapchainCreated = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain);
        if (swapchainCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create swapchain");
        }

        std::cout << "swapchain created" << '\n';

        // retrieve handles for swapchain images
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        // retrieve swapchain images format and extent (resolution)
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

}

void VulkanApp::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memProperties, VkImage& image, VkDeviceMemory& imageMemory, bool generalLayout) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        if (generalLayout) {
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
        }
        else {
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        }
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // share resource across queues
        /*
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
        uint32_t queueIndices[] = { queueFamilyIndices.graphicsFamilyIndex.value(), queueFamilyIndices.computeFamilyIndex.value(), queueFamilyIndices.presentFamilyIndex.value(),  queueFamilyIndices.transferFamilyIndex.value() };
        imageInfo.queueFamilyIndexCount = 4;
        imageInfo.pQueueFamilyIndices = queueIndices;
        */
        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements imageMemRequirements;
        vkGetImageMemoryRequirements(device, image, &imageMemRequirements);
        uint32_t imageSupportedMemTypes_Bitflags = imageMemRequirements.memoryTypeBits;

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = imageMemRequirements.size;
        VkMemoryPropertyFlags requiredMemProperties = memProperties;
        allocInfo.memoryTypeIndex = findMemoryTypeIndex(imageSupportedMemTypes_Bitflags, requiredMemProperties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);

}

void VulkanApp::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView) {

        // create view for offscreen image
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = image;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = format;
        // can swizle view channels, or harcode a value (0/1)
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        // view properties :
        // in stereroscopic rendering, can set an image view per swapchain-image layer 
        viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        // create view from swapchain image
        VkResult viewCreated = vkCreateImageView(device, &viewCreateInfo, nullptr, &imageView);
        if (viewCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create view from offscreen image");
        }

}

void VulkanApp::createImageResources() {
        // create view for each swapchain image 
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, swapChainImageViews[i]);
        }
   
        // create image+view for offscreen image attachment
        VkImageUsageFlags usage0 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        createImage(WIDTH, HEIGHT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, usage0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreenImage, offscreenImageMemory, false);
        createImageView(offscreenImage, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, offscreenImageView);
        
}


void VulkanApp::createTextureImageResources() {
    // create image+view for texture
     // ------------------------------------
     // load texture to textureData
    stbi_uc* textureData = stbi_load(TEXTURE_PATH_0, &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
    if (!textureData) { throw std::runtime_error("failed to load texture data from image file " + std::string(TEXTURE_PATH_0)); }
    VkDeviceSize textureSize = textureWidth * textureHeight * 4;

    // copy image data to staging buffer, then copy to device-local image
    VkBuffer stagingBuffer0;
    VkDeviceMemory stagingBufferMemory0;
    VkBufferUsageFlags stagingBufferUsageBitflags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in transfer op
    VkMemoryPropertyFlags stagingBufferMemPropertiesBitflags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    createGraphicsBuffer(textureSize, stagingBufferUsageBitflags, stagingBufferMemPropertiesBitflags, stagingBuffer0, stagingBufferMemory0);
    void* data0;
    vkMapMemory(device, stagingBufferMemory0, 0, textureSize, 0, &data0); // (access region of specified GPU memory, at given offset, and size, 0, output pointer to memory)
    memcpy(data0, textureData, (size_t)textureSize); // copy data to GPU memory (happens in the background, before next vkSubmitQueue
    vkUnmapMemory(device, stagingBufferMemory0);

    stbi_image_free(textureData);

    // create image in device-local memory
    VkImageUsageFlags usage1 = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // will be used as destination for stagingbuffer copy, and for shader sampling
    createImage(textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, usage1, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, false);
    createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureImageView);

    // transition image layout to transfer layout
    transitionImageLayoutSync2(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    // copy stage buffer to device-local image
    copyBufferToImage(stagingBuffer0, textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
    // transition image layout to read-only layout
    transitionImageLayoutSync2(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer0, nullptr);
    vkFreeMemory(device, stagingBufferMemory0, nullptr);
}


VkFormat VulkanApp::findSupportedImageFormat(const std::vector<VkFormat> candidateFormats, VkImageTiling requiredTiling, VkFormatFeatureFlags requiredFeaturesBitflags) {
        for (int i = 0; i < candidateFormats.size(); i++) {
            VkFormat candidateFormat = candidateFormats[i];
            VkFormatProperties candidateFormat_deviceSupportedProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, candidateFormat, &candidateFormat_deviceSupportedProperties);

            bool linearTilingRequested = (requiredTiling == VK_IMAGE_TILING_LINEAR);
            bool optimalTilingRequested = (requiredTiling == VK_IMAGE_TILING_OPTIMAL);

            bool deviceSupportsRequestedFormatFeatures_linearTiling = (candidateFormat_deviceSupportedProperties.linearTilingFeatures & requiredFeaturesBitflags) == requiredFeaturesBitflags;
            bool deviceSupportsRequestedFormatFeatures_optimalTiling = (candidateFormat_deviceSupportedProperties.optimalTilingFeatures & requiredFeaturesBitflags) == requiredFeaturesBitflags;

            if (linearTilingRequested && deviceSupportsRequestedFormatFeatures_linearTiling) {
                std::cout << "device supports the requested format features, for provided candidate VkFormat (using linearTiling)" << '\n';
                return candidateFormat;
            }
            else if (optimalTilingRequested && deviceSupportsRequestedFormatFeatures_optimalTiling) {
                std::cout << "device supports the requested format features, for provided candidate VkFormat (using optimalTiling)" << '\n';
                return candidateFormat;
            }
        }

        throw std::runtime_error("none of the provided candidate formats were found, which support the requested tiling and features");

}

VkFormat VulkanApp::findSupportedDepthFormat() {
        const std::vector<VkFormat> candidateDepthSupportedFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
        VkFormatFeatureFlags requiredFormatFeatures = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        return findSupportedImageFormat(candidateDepthSupportedFormats, VK_IMAGE_TILING_OPTIMAL, requiredFormatFeatures);
}

bool VulkanApp::formatSupportsStencil(VkFormat format) {
        // supports (Depth-32bit signed float, Stencil-8bit unsigned integer) ||(Depth-24bit unsigned norm float, Stencil-8bit unsigned integer)
        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
            return true;
        }
}

/*
void VulkanApp::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {

        //VkCommandBuffer commandBuffer = createSingleUseCommandBuffer();


        VkImageMemoryBarrier transitionBarrier;
        transitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        transitionBarrier.image = image;
        transitionBarrier.oldLayout = oldLayout;
        transitionBarrier.newLayout = newLayout;
        // transition queue family ownership of the image (VK_QUEUE_FAMILY_IGNORED if not doing this)
        transitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        // specify the relevant aspect of the image (color/depth/stencil)
        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            if (formatSupportsStencil(format)) {
                transitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            else {
                transitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            }
        }
        else {
            transitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        // specify mip level
        transitionBarrier.subresourceRange.baseMipLevel = 0;
        transitionBarrier.subresourceRange.levelCount = 1;
        // specify part of image (if an array)
        transitionBarrier.subresourceRange.baseArrayLayer = 0;
        transitionBarrier.subresourceRange.layerCount = 1;

        VkAccessFlags blockingOperations;
        VkPipelineStageFlags blockingOperationsStage;
        VkAccessFlags operationsToBlock;
        VkPipelineStageFlags operationsToBlockStage;

        // on first subpass where image is used, transition image layout from "undefined" to "transfer-destination optimal":
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            blockingOperations = 0; // no need to block transition by any operation
            blockingOperationsStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // can transition in earliest pipeline stage

            operationsToBlock = VK_ACCESS_TRANSFER_WRITE_BIT; // block following copy operations, until transition is finished.
            operationsToBlockStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // relevant copy operations are in transfer stage of pipeline.
        }

        // on first subpass where image is used, transition image from "transfer-destination optimal" to "shader-read-only optimal"
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            blockingOperations = VK_ACCESS_TRANSFER_WRITE_BIT; // block layout transition, until previous copy-operations are finished.
            blockingOperationsStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // relevant copy operations are in transfer stage of pipeline.

            operationsToBlock = VK_ACCESS_SHADER_READ_BIT; // block upcoming shader-read operations, until transition is finished
            operationsToBlockStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // relevant shader-read operations are in fragment-shader stage of pipeline.
        }

        // on first subpass where image is used, transition image from "undefined" to "depth read/write optimal"
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            blockingOperations = 0;
            blockingOperationsStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // can transition in earliest stage of pipeline.

            operationsToBlock = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; // block upcoming depth-read/write operations, until layout transition is finished.
            operationsToBlockStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // revelant depth-read/write operations, are in early-fragment-test stage of pipeline.
                // depth testing: before fragment shading, the zbuffer's existing depth value will be read. If its less than the evaluated frag's depth, the frag will be discarded. new depth value wont be written.
        }
        else {
            throw std::invalid_argument("image-layout transition specified is not supported");
        }

        // Block transition, until the following operation is finished:
        transitionBarrier.srcAccessMask = blockingOperations;

        // Block the following operation, until transition is finished:
        transitionBarrier.dstAccessMask = operationsToBlock;

        vkCmdPipelineBarrier(transferCommandBuffer, blockingOperationsStage, operationsToBlockStage, 
        0, 
        0, nullptr, //pMemoryBarriers
        0, nullptr, //pBufferMemoryBarriers
        1, &transitionBarrier); // pImageMemoryBarriers
}
*/

void VulkanApp::transitionImageLayoutSync2(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {

    VkImageMemoryBarrier2KHR transitionBarrier;
    transitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    transitionBarrier.image = image;
    transitionBarrier.oldLayout = oldLayout;
    transitionBarrier.newLayout = newLayout;
    transitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    // specify the relevant aspect of the image (color/depth/stencil)
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        if (formatSupportsStencil(format)) {
            transitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        else {
            transitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
    }
    else {
        transitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    // specify mip level
    transitionBarrier.subresourceRange.baseMipLevel = 0;
    transitionBarrier.subresourceRange.levelCount = 1;
    // specify part of image (if an array)
    transitionBarrier.subresourceRange.baseArrayLayer = 0;
    transitionBarrier.subresourceRange.layerCount = 1;

    VkAccessFlags blockingOperations;
    VkPipelineStageFlags blockingOperationsStage;
    VkAccessFlags operationsToBlock;
    VkPipelineStageFlags operationsToBlockStage;

    // on first subpass where image is used, transition image layout from "undefined" to "transfer-destination optimal":
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        blockingOperations = 0; // no need to block transition by any operation
        blockingOperationsStage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR; // can transition in earliest pipeline stage

        operationsToBlock = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR; // block following copy operations, until transition is finished.
        operationsToBlockStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR; // relevant copy operations are in transfer stage of pipeline.
    }

    // on first subpass where image is used, transition image from "transfer-destination optimal" to "shader-read-only optimal"
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        blockingOperations = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR; // block layout transition, until previous copy-operations are finished.
        blockingOperationsStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR; // relevant copy operations are in transfer stage of pipeline.

        operationsToBlock = VK_ACCESS_2_SHADER_READ_BIT_KHR; // block upcoming shader-read operations, until transition is finished
        operationsToBlockStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR; // relevant shader-read operations are in fragment-shader stage of pipeline.
    }

    // on first subpass where image is used, transition image from "undefined" to "depth read/write optimal"
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        blockingOperations = 0;
        blockingOperationsStage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR; // can transition in earliest stage of pipeline.

        operationsToBlock = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR; // block upcoming depth-read/write operations, until layout transition is finished.
        operationsToBlockStage = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR; // revelant depth-read/write operations, are in early-fragment-test stage of pipeline.
            // depth testing: before fragment shading, the zbuffer's existing depth value will be read. If its less than the evaluated frag's depth, the frag will be discarded. new depth value wont be written.
    }
    else {
        throw std::invalid_argument("image-layout transition specified is not supported");
    }

    // Block transition, until the following operation is finished:
    transitionBarrier.srcAccessMask = blockingOperations;

    // Block the following operation, until transition is finished:
    transitionBarrier.dstAccessMask = operationsToBlock;


    VkDependencyInfoKHR dependencyInfo = {};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
    dependencyInfo.pNext = VK_NULL_HANDLE;
    dependencyInfo.dependencyFlags = 0;
    dependencyInfo.memoryBarrierCount = 0;
    dependencyInfo.pMemoryBarriers = nullptr;
    dependencyInfo.bufferMemoryBarrierCount = 0;
    dependencyInfo.pBufferMemoryBarriers = nullptr;
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &transitionBarrier;

    _vkCmdPipelineBarrier2KHR(transferCommandBuffer, &dependencyInfo);

}


void VulkanApp::createDepthResources() {
    VkFormat depthSupportedFormat = findSupportedDepthFormat();
    VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // allocate using device-local memory
    createImage(swapChainExtent.width, swapChainExtent.height, depthSupportedFormat, VK_IMAGE_TILING_OPTIMAL, usage, memoryProperties, depthImage, depthImageMemory, false);
    createImageView(depthImage, depthSupportedFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depthImageView);
    // on first subpass where depth0 is used, transition image layout to "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL".
        // block following depth-read/write operations (in early fragment-test stage of pipeline), until layout transition is finished.
    //transitionImageLayout(depthImage, depthSupportedFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}


void VulkanApp::createDescriptorSetLayouts() {

        // -------------------
        // Layout for DescriptorSets.scene
        VkDescriptorSetLayoutBinding sceneDescriptorSetLayoutBinding0{}; // sceneTransform
        sceneDescriptorSetLayoutBinding0.binding = 0;
        sceneDescriptorSetLayoutBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sceneDescriptorSetLayoutBinding0.descriptorCount = 1; //incase of descriptor array, set array length
        sceneDescriptorSetLayoutBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        sceneDescriptorSetLayoutBinding0.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 1> sceneSetBindings = { sceneDescriptorSetLayoutBinding0 };

        VkDescriptorSetLayoutCreateInfo sceneSetLayout{};
        sceneSetLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        sceneSetLayout.bindingCount = static_cast<uint32_t>(sceneSetBindings.size());
        sceneSetLayout.pBindings = sceneSetBindings.data();

        VkResult sceneDescriptorSetLayoutCreated = vkCreateDescriptorSetLayout(device, &sceneSetLayout, nullptr, &descriptorSetLayouts.scene);
        if (sceneDescriptorSetLayoutCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to created descriptor set layout");
        }

        // --------------
        // Layout for DescriptorSets.composition
        VkDescriptorSetLayoutBinding compositionDescriptorSetLayoutBinding0{};
        compositionDescriptorSetLayoutBinding0.binding = 0;
        compositionDescriptorSetLayoutBinding0.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        compositionDescriptorSetLayoutBinding0.descriptorCount = 1;
        compositionDescriptorSetLayoutBinding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        compositionDescriptorSetLayoutBinding0.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 1> compositionSetBindings = { compositionDescriptorSetLayoutBinding0 };

        VkDescriptorSetLayoutCreateInfo compositionSetLayout{};
        compositionSetLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        compositionSetLayout.bindingCount = static_cast<uint32_t>(compositionSetBindings.size());
        compositionSetLayout.pBindings = compositionSetBindings.data();

        VkResult compositionSetLayoutCreated = vkCreateDescriptorSetLayout(device, &compositionSetLayout, nullptr, &descriptorSetLayouts.composition);
        if (compositionSetLayoutCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to created descriptor set layout");
        }

        // --------------
        // Create 2 Descriptor Set Layouts for DescriptorSets.fx

        // Set 0 Layout 
        // (set = 0, binding 0)
        VkDescriptorSetLayoutBinding fxDescriptorSet0LayoutBinding0{};
        fxDescriptorSet0LayoutBinding0.binding = 0;
        fxDescriptorSet0LayoutBinding0.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        fxDescriptorSet0LayoutBinding0.descriptorCount = 1;
        fxDescriptorSet0LayoutBinding0.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        fxDescriptorSet0LayoutBinding0.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding fxSet0LayoutBindings[] = { fxDescriptorSet0LayoutBinding0};
        VkDescriptorSetLayoutCreateInfo fxSet0Layout{};
        fxSet0Layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        fxSet0Layout.bindingCount = 1;
        fxSet0Layout.pBindings = fxSet0LayoutBindings;

        VkResult fxDescriptorSet0LayoutCreated = vkCreateDescriptorSetLayout(device, &fxSet0Layout, nullptr, &descriptorSetLayouts.fx0);
        if (fxDescriptorSet0LayoutCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to created descriptor set layout");
        }

        // Set 1 Layout 
        // (set = 1, binding 0)
        VkDescriptorSetLayoutBinding fxDescriptorSet1LayoutBinding0{};
        fxDescriptorSet1LayoutBinding0.binding = 0;
        fxDescriptorSet1LayoutBinding0.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        fxDescriptorSet1LayoutBinding0.descriptorCount = 1;
        fxDescriptorSet1LayoutBinding0.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        fxDescriptorSet1LayoutBinding0.pImmutableSamplers = nullptr;
        // (set = 1, binding 1)
        VkDescriptorSetLayoutBinding fxDescriptorSet1LayoutBinding1{};
        fxDescriptorSet1LayoutBinding1.binding = 1;
        fxDescriptorSet1LayoutBinding1.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        fxDescriptorSet1LayoutBinding1.descriptorCount = 1;
        fxDescriptorSet1LayoutBinding1.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        fxDescriptorSet1LayoutBinding1.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding fxSet1LayoutBindings[] = { fxDescriptorSet1LayoutBinding0, fxDescriptorSet1LayoutBinding1 };
        VkDescriptorSetLayoutCreateInfo fxSet1Layout{};
        fxSet1Layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        fxSet1Layout.bindingCount = 2;
        fxSet1Layout.pBindings = fxSet1LayoutBindings;

        VkResult fxDescriptorSet1LayoutCreated = vkCreateDescriptorSetLayout(device, &fxSet1Layout, nullptr, &descriptorSetLayouts.fx1);
        if (fxDescriptorSet1LayoutCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to created descriptor set layout");
        }
}

void VulkanApp::createDescriptorPool() {
        std::vector<VkDescriptorPoolSize> poolSizes{};

        // uboScene 
        VkDescriptorPoolSize poolsize0;
        poolsize0.descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolsize0.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes.push_back(poolsize0);

        // uboFX 
        VkDescriptorPoolSize poolsize1;
        poolsize1.descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolsize1.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes.push_back(poolsize1);

        // inputCol
        VkDescriptorPoolSize poolsize2;
        poolsize2.descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolsize2.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        poolSizes.push_back(poolsize2);

        // inputDepth
        VkDescriptorPoolSize poolsize3;
        poolsize3.descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolsize3.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        poolSizes.push_back(poolsize3);

        /*
        // textureImage
        VkDescriptorPoolSize poolsize4;
        poolsize4.descriptorCount = static_cast<uint32_t>(swapChainImages.size());
        poolsize4.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes.push_back(poolsize4);
        */
        // ---------------

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        // createDescriptorSets() is creating 5 descriptor-sets (scene, comp, fx0, fx1), per swapchain image 
        poolInfo.maxSets = swapChainImages.size() * 4;
        poolInfo.flags = 0;

        VkResult descriptorPoolCreated = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
        if (descriptorPoolCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to created descriptor pool");
        }
}

void VulkanApp::createDescriptorSets() {

        std::vector<VkDescriptorSetLayout> sceneDescriptorSetLayout(swapChainImages.size(), descriptorSetLayouts.scene);
        std::vector<VkDescriptorSetLayout> compositionDescriptorSetLayout(swapChainImages.size(), descriptorSetLayouts.composition);
        std::vector<VkDescriptorSetLayout> fxDescriptorSet0Layout(swapChainImages.size(), descriptorSetLayouts.fx0);
        std::vector<VkDescriptorSetLayout> fxDescriptorSet1Layout(swapChainImages.size(), descriptorSetLayouts.fx1);
        

        // allocate scene descriptor sets from descriptor-pool
        VkDescriptorSetAllocateInfo sceneDescriptorSetInfo{};
        sceneDescriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        sceneDescriptorSetInfo.descriptorPool = descriptorPool;
        sceneDescriptorSetInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        sceneDescriptorSetInfo.pSetLayouts = sceneDescriptorSetLayout.data();

        descriptorSets.scene.resize(swapChainImages.size());
        VkResult sceneDescriptorSetAllocated = vkAllocateDescriptorSets(device, &sceneDescriptorSetInfo, descriptorSets.scene.data());

        if (sceneDescriptorSetAllocated != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets");
        }

        // allocate composition descriptor set from descriptor-pool
        VkDescriptorSetAllocateInfo compositionDescriptorSetInfo{};
        compositionDescriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        compositionDescriptorSetInfo.descriptorPool = descriptorPool;
        compositionDescriptorSetInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        compositionDescriptorSetInfo.pSetLayouts = compositionDescriptorSetLayout.data();

        descriptorSets.composition.resize(swapChainImages.size());
        VkResult compositionDescriptorSetAllocated = vkAllocateDescriptorSets(device, &compositionDescriptorSetInfo, descriptorSets.composition.data());
        if (compositionDescriptorSetAllocated != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets");
        }

        // allocate fx descriptor set 0  from descriptor-pool
        VkDescriptorSetAllocateInfo fxDescriptorSet0Info{};
        fxDescriptorSet0Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        fxDescriptorSet0Info.descriptorPool = descriptorPool;
        fxDescriptorSet0Info.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        fxDescriptorSet0Info.pSetLayouts = fxDescriptorSet0Layout.data();

        descriptorSets.fx0.resize(swapChainImages.size());
        VkResult fxDescriptorSet0Allocated = vkAllocateDescriptorSets(device, &fxDescriptorSet0Info, descriptorSets.fx0.data());
        if (fxDescriptorSet0Allocated != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets");
        }

        // allocate fx descriptor set 1 from descriptor-pool
        VkDescriptorSetAllocateInfo fxDescriptorSet1Info{};
        fxDescriptorSet1Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        fxDescriptorSet1Info.descriptorPool = descriptorPool;
        fxDescriptorSet1Info.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        fxDescriptorSet1Info.pSetLayouts = fxDescriptorSet1Layout.data();

        descriptorSets.fx1.resize(swapChainImages.size());
        VkResult fxDescriptorSet1Allocated = vkAllocateDescriptorSets(device, &fxDescriptorSet1Info, descriptorSets.fx1.data());
        if (fxDescriptorSet1Allocated != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets");
        }

        // write to descriptor sets (per swapchain image):
        for (size_t i = 0; i < swapChainImages.size(); i++) {

            // Descriptors:
            VkDescriptorBufferInfo sceneDescriptor{};
            sceneDescriptor.buffer = uniformBuffersScene[i]; // buffer with UBO content
            sceneDescriptor.offset = 0; // offset within buffer, where UBO content exists
            sceneDescriptor.range = sizeof(UniformBufferObjectScene); // data size of UBO content

            VkDescriptorBufferInfo fxDescriptor{};
            fxDescriptor.buffer = uniformBuffersFX[i];
            fxDescriptor.offset = 0;
            fxDescriptor.range = sizeof(UniformBufferObjectFX); // data size of UBO content

            VkDescriptorImageInfo imageDescriptor{};
            imageDescriptor.imageView = offscreenImageView;
            imageDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageDescriptor.sampler = VK_NULL_HANDLE;


            VkDescriptorImageInfo depthDescriptor{};
            depthDescriptor.imageView = depthImageView;
            depthDescriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depthDescriptor.sampler = VK_NULL_HANDLE;

            /*
            VkDescriptorImageInfo textureDescriptor{};
            depthDescriptor.imageView = textureImageView;
            depthDescriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
            depthDescriptor.sampler = textureSampler;
            */

            // ----------------
            // Scene Descriptor Set
            // layout(set = 0, binding = 0) uniform uboScene
            std::array<VkWriteDescriptorSet, 1> sceneDescriptorSetWrite{};
            sceneDescriptorSetWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            sceneDescriptorSetWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            sceneDescriptorSetWrite[0].dstSet = descriptorSets.scene[i]; //descriptor set
            sceneDescriptorSetWrite[0].dstBinding = 0; // descriptor set's (shader) binding index
            sceneDescriptorSetWrite[0].descriptorCount = 1; // incase of array, specify amount of elements to update
            sceneDescriptorSetWrite[0].dstArrayElement = 0; // incase of array, specify starting-offset element
            sceneDescriptorSetWrite[0].pBufferInfo = &sceneDescriptor;

            // update descriptor sets:  device, descSet amount, descSetWriteInfo[], copyCount, copyDescSets)
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(sceneDescriptorSetWrite.size()), sceneDescriptorSetWrite.data(), 0, nullptr);

            // ---------------
            // Composition Descriptor Set
            // layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColorAttachment;
            std::array<VkWriteDescriptorSet, 1> compositionDescriptorSetWrite{};
            compositionDescriptorSetWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            compositionDescriptorSetWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            compositionDescriptorSetWrite[0].dstSet = descriptorSets.composition[i];
            compositionDescriptorSetWrite[0].dstBinding = 0;
            compositionDescriptorSetWrite[0].descriptorCount = 1;
            compositionDescriptorSetWrite[0].dstArrayElement = 0;
            compositionDescriptorSetWrite[0].pImageInfo = &imageDescriptor;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(compositionDescriptorSetWrite.size()), compositionDescriptorSetWrite.data(), 0, nullptr);


            // ---------------
            // FX Descriptor Set 0
            // layout(set = 0, binding = 0) uniform uboFX
            std::array<VkWriteDescriptorSet, 1> fxDescriptorSet0Write{};
            fxDescriptorSet0Write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            fxDescriptorSet0Write[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            fxDescriptorSet0Write[0].dstSet = descriptorSets.fx0[i];
            fxDescriptorSet0Write[0].dstBinding = 0;
            fxDescriptorSet0Write[0].descriptorCount = 1;
            fxDescriptorSet0Write[0].dstArrayElement = 0;
            fxDescriptorSet0Write[0].pBufferInfo = &fxDescriptor;
            
            /*
            fxDescriptorSet0Write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            fxDescriptorSet0Write[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            fxDescriptorSet0Write[1].dstSet = descriptorSets.fx0[i];
            fxDescriptorSet0Write[1].dstBinding = 1;
            fxDescriptorSet0Write[1].descriptorCount = 1;
            fxDescriptorSet0Write[1].dstArrayElement = 0;
            fxDescriptorSet0Write[1].pImageInfo = &textureDescriptor;
            */
            vkUpdateDescriptorSets(device, static_cast<uint32_t>(fxDescriptorSet0Write.size()), fxDescriptorSet0Write.data(), 0, nullptr);


            std::array<VkWriteDescriptorSet, 2> fxDescriptorSet1Write{};
            // FX Descriptor Set 1
            // layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputColorAttachment;
            fxDescriptorSet1Write[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            fxDescriptorSet1Write[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            fxDescriptorSet1Write[0].dstSet = descriptorSets.fx1[i];
            fxDescriptorSet1Write[0].dstBinding = 0;
            fxDescriptorSet1Write[0].descriptorCount = 1;
            fxDescriptorSet1Write[0].dstArrayElement = 0;
            fxDescriptorSet1Write[0].pImageInfo = &imageDescriptor;
            // layout(input_attachment_index = 1, set = 1 binding = 1) uniform subpassInput inputDepthAttachment;
            fxDescriptorSet1Write[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            fxDescriptorSet1Write[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            fxDescriptorSet1Write[1].dstSet = descriptorSets.fx1[i];
            fxDescriptorSet1Write[1].dstBinding = 1;
            fxDescriptorSet1Write[1].descriptorCount = 1;
            fxDescriptorSet1Write[1].dstArrayElement = 0;
            fxDescriptorSet1Write[1].pImageInfo = &depthDescriptor;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(fxDescriptorSet1Write.size()), fxDescriptorSet1Write.data(), 0, nullptr);

        }

}

uint32_t VulkanApp::findMemoryTypeIndex(uint32_t bufferSupportedMemTypes_Bitflags, VkMemoryPropertyFlags requiredMemProperties) {
        VkPhysicalDeviceMemoryProperties deviceSupportedMemProperties; // get device supported memory properties:
            // memoryTypeCount: amount of memory types supported
            // memoryTypes[]: array of supported VkMemoryType's that can be allocated from heaps
            // memoryHeapCount: amount of available memory heaps
            // memoryHeaps[]: array of available memory heaps
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceSupportedMemProperties);

        for (uint32_t i = 0; i < deviceSupportedMemProperties.memoryTypeCount; i++) { // for every device-supported memory type

            // create iterating 32bit flag, by left-shifting bits of (.... 0000 0001) by i amount
            uint32_t bitflag = (1 << i);
            // find the first device-supported mem type, which matches one of the buffer-supported mem types
            bool memTypeFound = (bitflag & bufferSupportedMemTypes_Bitflags);
            // check if the mem type supports the required mem properties
            uint32_t memTypeSupportedProperties = deviceSupportedMemProperties.memoryTypes[i].propertyFlags;
            bool requiredMemPropertiesSupported = (memTypeSupportedProperties & requiredMemProperties);
            if ((memTypeFound) && (requiredMemPropertiesSupported)) {
                // if so, return index of the memory type
                return i;
            }
        }
}

void VulkanApp::createGraphicsBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage_Bitflags, VkMemoryPropertyFlags memProperties_Bitflags, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
 {
        // create buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = bufferUsage_Bitflags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        /*
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
        const uint32_t queueIndices[] = { queueFamilyIndices.graphicsFamilyIndex.value()) };
        bufferInfo.queueFamilyIndexCount = 3;
        bufferInfo.pQueueFamilyIndices = queueIndices;
        */

        VkResult bufferCreated = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
        if (bufferCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer");
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

        VkMemoryRequirements bufferMemRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &bufferMemRequirements);
        // get buffer's memory requirements:
            // size = memory size
            // aligment = byte offset
            // memoryTypeBits = 32bit flag for supported memory types
        uint32_t bufferSupportedMemTypes_Bitflags = bufferMemRequirements.memoryTypeBits;
        // find #id of memory type, based on buffer-supported mem types and required mem properties.
        uint32_t memoryTypeIndex = findMemoryTypeIndex(bufferSupportedMemTypes_Bitflags, memProperties_Bitflags); //get index of memory type, based of required properties and buffer's memory type
        allocInfo.memoryTypeIndex = memoryTypeIndex;

        allocInfo.allocationSize = bufferMemRequirements.size; // GPU memory size to allocate in bytes. can differ from buffers size

        VkResult memoryAllocated = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
        if (memoryAllocated != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate GPU memory for buffer");
        }

        // bind buffer to gpu memory
        vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VulkanApp::createUniformBuffers() {
        uniformBuffersScene.resize(swapChainImages.size());
        uniformBuffersSceneMemory.resize(swapChainImages.size());

        VkDeviceSize bufferSize0 = sizeof(UniformBufferObjectScene);

        uniformBuffersFX.resize(swapChainImages.size());
        uniformBuffersFXMemory.resize(swapChainImages.size());

        uniformBuffersDecal.resize(swapChainImages.size());
        uniformBuffersDecalMemory.resize(swapChainImages.size());

        VkDeviceSize bufferSize1 = sizeof(UniformBufferObjectFX);
        VkDeviceSize bufferSize2 = sizeof(UniformBufferObjectFX);

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkBufferUsageFlags bufferUsageBitflags0 = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            VkMemoryPropertyFlags memPropertiesBitflags0 = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            //mem properties bitflag : 
            // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: can be written by CPU
            // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: content of mapped CPU memory and GPU memory, gurranted to be the same

            createGraphicsBuffer(bufferSize0, bufferUsageBitflags0, memPropertiesBitflags0, uniformBuffersScene[i], uniformBuffersSceneMemory[i]);

            VkBufferUsageFlags bufferUsageBitflags1 = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            VkMemoryPropertyFlags memPropertiesBitflags1 = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            createGraphicsBuffer(bufferSize1, bufferUsageBitflags1, memPropertiesBitflags1, uniformBuffersFX[i], uniformBuffersFXMemory[i]);

            VkBufferUsageFlags bufferUsageBitflags2 = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            VkMemoryPropertyFlags memPropertiesBitflags2 = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            createGraphicsBuffer(bufferSize2, bufferUsageBitflags2, memPropertiesBitflags2, uniformBuffersDecal[i], uniformBuffersDecalMemory[i]);

        }
}

void VulkanApp::createTextureSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        // over/undersample filtering
        samplerInfo.magFilter = VK_FILTER_LINEAR; //oversample filtering
        samplerInfo.minFilter = VK_FILTER_LINEAR; //undersample filtering
        // addressing mode
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        // anisotropic filtering
        samplerInfo.anisotropyEnable = VK_TRUE;
        VkPhysicalDeviceProperties deviceProperties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        samplerInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;
        // border color (if clamp)
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        // coordinate space
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        // comparison value for precentange-close filtering
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        // mipmapping
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkResult samplerCreated = vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler);
        if (samplerCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler");
        }
        std::cout << "texture sampler created" << '\n';

    }

void VulkanApp::createGraphicsPipelineScene() {

        /*Summary:
        -> programmable shader stages
        -> fixed function stages
            - vertex input
            - input assembler
            - input assembler
            - rasterizer
            - viewport
            - color blending
        -> pipeline layout (uniform layout)
        -> render pass (attachments and subpasses)

        */
        //1. Shader Loading 
        static std::vector<char> vertShaderCode = readFile(SHADER_VERT_PATH_0);
        static std::vector<char> fragShaderCode = readFile(SHADER_FRAG_PATH_0);

        // SPIR-V shaders get compiled to machine code, using ShaderModules, when pipeline is created.
        // Thus, shader modules can be deleted when leaving pipeline-creation scope.

        //2. Shader Modules setup
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        // shader function to call (entry point)
        vertShaderStageInfo.pName = "main";
        vertShaderStageInfo.pSpecializationInfo = nullptr; // can set values for shader constants 

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        // shader function to call (entry point)
        fragShaderStageInfo.pName = "main";
        fragShaderStageInfo.pSpecializationInfo = nullptr; // can set values for shader constants 

        //3. Shader Stages 
        VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = { vertShaderStageInfo, fragShaderStageInfo };

        //4. Vertex Input
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // describing vertex data bindings
        vertexInputInfo.vertexAttributeDescriptionCount = 4;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // describing vertex data attributes

        //5. Input AssembLer
            // Topology:
            /*
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST : line from every 2 vertices without reuse
            VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : the end vertex of every line is used as start vertex for the next line
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : triangle from every 3 vertices without reuse
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : the second and third vertex of every triangle are used as first two vertices of the next triangle
            */

            // PrimitiveRestartEnable:
            //VK_TRUE: Can break up lines and triangles in the _STRIP topology method, using 0xFFFF
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        //6. Viewport
            // Transform
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor (Clipping)
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;

        //7. Rasterizer (Rasterize fragments, perform depth-testing, face-culling and scissor-test.)
            // polygonMode: can rasterize polygon fill, edges (wireframe), or points.
                // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
                // VK_POLYGON_MODE_LINE : polygon edges are drawn as lines
                // VK_POLYGON_MODE_POINT : polygon vertices are drawn as points

        VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
        rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizerCreateInfo.depthClampEnable = VK_FALSE; // VK_TRUE = clamp (instead of discard) frags outside near/far plane.
        rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; // VK_TRUE = dicard all geometry
        rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizerCreateInfo.lineWidth = 1.0f;
        rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // backface / frontface / both culling
        rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // vertex order to define front facing polygons

        rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizerCreateInfo.depthBiasConstantFactor = 0.0f; // add constant offset to depth
        rasterizerCreateInfo.depthBiasClamp = 0.0f;
        rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

        //8. Multisampling
        VkPipelineMultisampleStateCreateInfo multisamplingCreateinfo{};
        multisamplingCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingCreateinfo.sampleShadingEnable = VK_FALSE;
        multisamplingCreateinfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisamplingCreateinfo.minSampleShading = 1.0f;
        multisamplingCreateinfo.pSampleMask = nullptr;
        multisamplingCreateinfo.alphaToCoverageEnable = VK_FALSE;
        multisamplingCreateinfo.alphaToOneEnable = VK_FALSE;

        //9. Depth Stencil settings
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;

        // can discard fragments outside a pre-defined range
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.minDepthBounds = 0.0f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.front = {};
        depthStencilStateCreateInfo.back = {};

        //10. Color Blending 
        // src = output's frag,  dst = attachment's frag   
         
            // RGB = srcCol *  srcBlend <op> dstCol * dstBlend
            // A = srcAlpha * srcBlend <op> dstAlpha * dstBlend

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        // bit flag for masking specific channels of the framebuffer:
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        // color and alpha blend settings

        colorBlendAttachment.blendEnable = VK_FALSE; // disable color blending

        VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
        colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendingCreateInfo.logicOpEnable = VK_FALSE; // VK_FALSE: Enabled bitwise combination alpha blending
        colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;

        colorBlendingCreateInfo.attachmentCount = 1;
        colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;
        colorBlendingCreateInfo.blendConstants[0] = 0.0f;
        colorBlendingCreateInfo.blendConstants[1] = 0.0f;
        colorBlendingCreateInfo.blendConstants[2] = 0.0f;
        colorBlendingCreateInfo.blendConstants[3] = 0.0f;

        //11. Dynamic States 
            // Pipeline structs to be changed in runtime
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT, //viewport size
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = 2;
        dynamicStateCreateInfo.pDynamicStates = dynamicStates;

        //12. Pipeline Layout (descriptor set layouts)
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.scene;

        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        VkResult pipelineLayoutCreated = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.scene);
        if (pipelineLayoutCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout");
        }


        // Graphics Pipeline Struct
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        // programmable shader stages
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = 2; // amount of shader stages
        pipelineCreateInfo.pStages = shaderStagesCreateInfo;

        // fixed function stages
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisamplingCreateinfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
        pipelineCreateInfo.pDynamicState = nullptr;

        // uniform pipeline layout
        pipelineCreateInfo.layout = pipelineLayouts.scene;

        // render passes
        pipelineCreateInfo.renderPass = renderPass;
        pipelineCreateInfo.subpass = 0; // index of subpass

        // create pipeline deriving from existing pipeline
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        // create pipeline(s)
        // (device, VkPipelineCache, pipelineInfos, nullptr, graphicsPipeline)
        VkResult pipelineCreated = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelines.scene);
        if (pipelineCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline");
        }

        std::cout << "graphics pipeline created" << '\n';

        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);

}

void VulkanApp::createGraphicsPipelineFX() {

        //1. Shader Loading 
        static std::vector<char> vertShaderCode = readFile(SHADER_VERT_PATH_1);
        static std::vector<char> fragShaderCode = readFile(SHADER_FRAG_PATH_1);

        // SPIR-V shaders get compiled to machine code, using ShaderModules, when pipeline is created.
        // Thus, shader modules can be deleted when leaving pipeline-creation scope.

        //2. Shader Modules setup
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        // shader function to call (entry point)
        vertShaderStageInfo.pName = "main";
        vertShaderStageInfo.pSpecializationInfo = nullptr; // can set values for shader constants 

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        // shader function to call (entry point)
        fragShaderStageInfo.pName = "main";
        fragShaderStageInfo.pSpecializationInfo = nullptr; // can set values for shader constants 

        //3. Shader Stages 
        VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = { vertShaderStageInfo, fragShaderStageInfo };

        //4. Vertex Input
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // describing vertex data bindings
        vertexInputInfo.vertexAttributeDescriptionCount = 4;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // describing vertex data attributes

        //5. Input AssembLer
            // Topology:
            /*
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST : line from every 2 vertices without reuse
            VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : the end vertex of every line is used as start vertex for the next line
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : triangle from every 3 vertices without reuse
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : the second and third vertex of every triangle are used as first two vertices of the next triangle
            */

        // PrimitiveRestartEnable:
            //VK_TRUE: Can break up lines and triangles in the _STRIP topology method, using 0xFFFF
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        //6. Viewport
            // Transform
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor (Clipping)
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;

        //7. Rasterizer (Rasterize fragments, perform depth-testing, face-culling and scissor-test.)
            // polygonMode: can rasterize polygon fill, edges (wireframe), or points.
                // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
                // VK_POLYGON_MODE_LINE : polygon edges are drawn as lines
                // VK_POLYGON_MODE_POINT : polygon vertices are drawn as points

        VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
        rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizerCreateInfo.depthClampEnable = VK_FALSE; // VK_TRUE = clamp (instead of discard) frags outside near/far plane.
        rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; // VK_TRUE = dicard all geometry
        rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizerCreateInfo.lineWidth = 1.0f;
        rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // backface / frontface / both culling
        rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // vertex order to define front facing polygons

        rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizerCreateInfo.depthBiasConstantFactor = 0.0f; // add constant offset to depth
        rasterizerCreateInfo.depthBiasClamp = 0.0f;
        rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

        //8. Multisampling
        VkPipelineMultisampleStateCreateInfo multisamplingCreateinfo{};
        multisamplingCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingCreateinfo.sampleShadingEnable = VK_FALSE;
        multisamplingCreateinfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisamplingCreateinfo.minSampleShading = 1.0f;
        multisamplingCreateinfo.pSampleMask = nullptr;
        multisamplingCreateinfo.alphaToCoverageEnable = VK_FALSE;
        multisamplingCreateinfo.alphaToOneEnable = VK_FALSE;

        //9. Depth and Stencil write/test
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;

        // can discard fragments outside a pre-defined range
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.minDepthBounds = 0.0f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.front = {};
        depthStencilStateCreateInfo.back = {};

        //10. Color Blending
        // src = output's frag,  dst = attachment's frag   

            // RGB = srcCol *  srcBlend <op> dstCol * dstBlend
            // A = srcAlpha * srcBlend <op> dstAlpha * dstBlend

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        // bit flag for masking specific channels of the framebuffer:
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        
        // color and alpha blend settings
        colorBlendAttachment.blendEnable = VK_TRUE;
        // srcCol * srcAlpha + dstCol * 1-srcAlpha
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        // srcAlpha * 1 + dstAlpha * 0
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
        colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendingCreateInfo.logicOpEnable = VK_FALSE; // VK_FALSE: Enabled bitwise combination alpha blending
        colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;

        colorBlendingCreateInfo.attachmentCount = 1;
        colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;
        colorBlendingCreateInfo.blendConstants[0] = 0.0f;
        colorBlendingCreateInfo.blendConstants[1] = 0.0f;
        colorBlendingCreateInfo.blendConstants[2] = 0.0f;
        colorBlendingCreateInfo.blendConstants[3] = 0.0f;

        //11. Dynamic States 
            // Pipeline structs to be changed in runtime
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT, //viewport size
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = 2;
        dynamicStateCreateInfo.pDynamicStates = dynamicStates;

        //12. Pipeline Layout (descriptor set layouts)
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 2; // FX pipeline has two descriptor sets (fx0, fx1)
        VkDescriptorSetLayout layouts[] = { descriptorSetLayouts.fx0,  descriptorSetLayouts.fx1 };
        pipelineLayoutCreateInfo.pSetLayouts = layouts;

        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        VkResult pipelineLayoutCreated = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.fx);
        if (pipelineLayoutCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout");
        }


        // Graphics Pipeline Struct
        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        // programmable shader stages
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = 2; // amount of shader stages
        pipelineCreateInfo.pStages = shaderStagesCreateInfo;

        // fixed function stages
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisamplingCreateinfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
        pipelineCreateInfo.pDynamicState = nullptr;

        // uniform pipeline layout
        pipelineCreateInfo.layout = pipelineLayouts.fx;

        // render passes
        pipelineCreateInfo.renderPass = renderPass;
        pipelineCreateInfo.subpass = 1; // index of subpass

        // create pipeline deriving from existing pipeline
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        // create pipeline(s)
        // (device, VkPipelineCache, pipelineInfos, nullptr, graphicsPipeline)
        VkResult pipelineCreated = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelines.fx);
        if (pipelineCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline");
        }

        std::cout << "graphics pipeline created" << '\n';

        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);

}

void VulkanApp::createGraphicsPipelineDecal() {

    //1. Shader Loading 
    static std::vector<char> vertShaderCode = readFile(SHADER_VERT_PATH_2);
    static std::vector<char> fragShaderCode = readFile(SHADER_FRAG_PATH_2);

    // SPIR-V shaders get compiled to machine code, using ShaderModules, when pipeline is created.
    // Thus, shader modules can be deleted when leaving pipeline-creation scope.

    //2. Shader Modules setup
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    // shader function to call (entry point)
    vertShaderStageInfo.pName = "main";
    vertShaderStageInfo.pSpecializationInfo = nullptr; // can set values for shader constants 

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    // shader function to call (entry point)
    fragShaderStageInfo.pName = "main";
    fragShaderStageInfo.pSpecializationInfo = nullptr; // can set values for shader constants 

    //3. Shader Stages 
    VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = { vertShaderStageInfo, fragShaderStageInfo };

    //4. Vertex Input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // describing vertex data bindings
    vertexInputInfo.vertexAttributeDescriptionCount = 4;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // describing vertex data attributes

    //5. Input AssembLer
        // Topology:
        /*
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST : line from every 2 vertices without reuse
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : the end vertex of every line is used as start vertex for the next line
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : triangle from every 3 vertices without reuse
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : the second and third vertex of every triangle are used as first two vertices of the next triangle
        */

        // PrimitiveRestartEnable:
            //VK_TRUE: Can break up lines and triangles in the _STRIP topology method, using 0xFFFF
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    //6. Viewport
        // Transform
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor (Clipping)
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    //7. Rasterizer (Rasterize fragments, perform depth-testing, face-culling and scissor-test.)
        // polygonMode: can rasterize polygon fill, edges (wireframe), or points.
            // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
            // VK_POLYGON_MODE_LINE : polygon edges are drawn as lines
            // VK_POLYGON_MODE_POINT : polygon vertices are drawn as points

    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE; // VK_TRUE = clamp (instead of discard) frags outside near/far plane.
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; // VK_TRUE = dicard all geometry
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.lineWidth = 1.0f;
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT; // backface / frontface / both culling
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // vertex order to define front facing polygons

    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizerCreateInfo.depthBiasConstantFactor = 0.0f; // add constant offset to depth
    rasterizerCreateInfo.depthBiasClamp = 0.0f;
    rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

    //8. Multisampling
    VkPipelineMultisampleStateCreateInfo multisamplingCreateinfo{};
    multisamplingCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateinfo.sampleShadingEnable = VK_FALSE;
    multisamplingCreateinfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingCreateinfo.minSampleShading = 1.0f;
    multisamplingCreateinfo.pSampleMask = nullptr;
    multisamplingCreateinfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingCreateinfo.alphaToOneEnable = VK_FALSE;

    //9. Depth and Stencil write/test
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;

    // can discard fragments outside a pre-defined range
    depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    depthStencilStateCreateInfo.front = {};
    depthStencilStateCreateInfo.back = {};

    //10. Color Blending
    // src = output's frag,  dst = attachment's frag   

        // RGB = srcCol *  srcBlend <op> dstCol * dstBlend
        // A = srcAlpha * srcBlend <op> dstAlpha * dstBlend

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    // bit flag for masking specific channels of the framebuffer:
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // color and alpha blend settings
    colorBlendAttachment.blendEnable = VK_TRUE;
    // srcCol * srcAlpha + dstCol * 1-srcAlpha
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // srcAlpha * 1 + dstAlpha * 0
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
    colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingCreateInfo.logicOpEnable = VK_FALSE; // VK_FALSE: Enabled bitwise combination alpha blending
    colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;

    colorBlendingCreateInfo.attachmentCount = 1;
    colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;
    colorBlendingCreateInfo.blendConstants[0] = 0.0f;
    colorBlendingCreateInfo.blendConstants[1] = 0.0f;
    colorBlendingCreateInfo.blendConstants[2] = 0.0f;
    colorBlendingCreateInfo.blendConstants[3] = 0.0f;

    //11. Dynamic States 
        // Pipeline structs to be changed in runtime
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT, //viewport size
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = 2;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    //12. Pipeline Layout (descriptor set layouts)
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 2; // FX pipeline has two descriptor sets (fx0, fx1)
    VkDescriptorSetLayout layouts[] = { descriptorSetLayouts.fx0,  descriptorSetLayouts.fx1 };
    pipelineLayoutCreateInfo.pSetLayouts = layouts;

    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    VkResult pipelineLayoutCreated = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.decal);
    if (pipelineLayoutCreated != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }


    // Graphics Pipeline Struct
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    // programmable shader stages
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2; // amount of shader stages
    pipelineCreateInfo.pStages = shaderStagesCreateInfo;

    // fixed function stages
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingCreateinfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;

    // uniform pipeline layout
    pipelineCreateInfo.layout = pipelineLayouts.decal;

    // render passes
    pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.subpass = 1; // index of subpass

    // create pipeline deriving from existing pipeline
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    // create pipeline(s)
    // (device, VkPipelineCache, pipelineInfos, nullptr, graphicsPipeline)
    VkResult pipelineCreated = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelines.decal);
    if (pipelineCreated != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    std::cout << "graphics pipeline created" << '\n';

    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);

}

void VulkanApp::createGraphicsPipelineComposition() {

        /*Summary:
        -> programmable shader stages
        -> fixed function stages
            - vertex input
            - input assembler
            - rasterizer
            - viewport
            - color blending
        -> pipeline layout (uniform layout)
        -> render pass (attachments and subpasses)

        */
        //1. Shader Loading 
        static std::vector<char> vertShaderCode = readFile(SHADER_VERT_PATH_3);
        static std::vector<char> fragShaderCode = readFile(SHADER_FRAG_PATH_3);

        // SPIR-V shaders get compiled to machine code, using ShaderModules, when pipeline is created.
        // Thus, shader modules can be deleted when leaving pipeline-creation scope.

        //2. Shader Modules setup
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        // shader function to call (entry point)
        vertShaderStageInfo.pName = "main";
        vertShaderStageInfo.pSpecializationInfo = nullptr; // can set values for shader constants 

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        // shader function to call (entry point)
        fragShaderStageInfo.pName = "main";
        fragShaderStageInfo.pSpecializationInfo = nullptr; // can set values for shader constants 

        //3. Shader Stages 
        VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = { vertShaderStageInfo, fragShaderStageInfo };

        //4. Vertex Input
            // Bindings description: spacing between data and whether the data is per-vertex or per-instance
            // Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // describing vertex data bindings
        vertexInputInfo.vertexAttributeDescriptionCount = 4;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // describing vertex data attributes


        //5. Input AssembLer
            // Topology:
            /*
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST : line from every 2 vertices without reuse
            VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : the end vertex of every line is used as start vertex for the next line
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : triangle from every 3 vertices without reuse
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : the second and third vertex of every triangle are used as first two vertices of the next triangle
            */

            // PrimitiveRestartEnable:
            //VK_TRUE: Can break up lines and triangles in the _STRIP topology method, using 0xFFFF
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        //6. Viewport
            // Transform
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor (Clipping)
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;

        //7. Rasterizer (Rasterize fragments, perform depth-testing, face-culling and scissor-test.)
            // polygonMode: can rasterize polygon fill, edges (wireframe), or points.
                // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
                // VK_POLYGON_MODE_LINE : polygon edges are drawn as lines
                // VK_POLYGON_MODE_POINT : polygon vertices are drawn as points

        VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
        rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizerCreateInfo.depthClampEnable = VK_FALSE; // VK_TRUE = clamp (instead of discard) frags outside near/far plane.
        rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; // VK_TRUE = dicard all geometry
        rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizerCreateInfo.lineWidth = 1.0f;
        rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // backface / frontface / both culling
        rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // vertex order to define front facing polygons

        rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizerCreateInfo.depthBiasConstantFactor = 0.0f; // add constant offset to depth
        rasterizerCreateInfo.depthBiasClamp = 0.0f;
        rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

        //8. Multisampling
        VkPipelineMultisampleStateCreateInfo multisamplingCreateinfo{};
        multisamplingCreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingCreateinfo.sampleShadingEnable = VK_FALSE;
        multisamplingCreateinfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisamplingCreateinfo.minSampleShading = 1.0f;
        multisamplingCreateinfo.pSampleMask = nullptr;
        multisamplingCreateinfo.alphaToCoverageEnable = VK_FALSE;
        multisamplingCreateinfo.alphaToOneEnable = VK_FALSE;

        //9. Depth and Stencil Desting
        VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{};

        //10. Color Blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        // bit flag for masking specific channels of the framebuffer:
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        // color and alpha blend settings
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

        VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
        colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendingCreateInfo.logicOpEnable = VK_FALSE; // VK_FALSE: Enabled bitwise combination alpha blending
        colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;

        colorBlendingCreateInfo.attachmentCount = 1;
        colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;
        colorBlendingCreateInfo.blendConstants[0] = 0.0f;
        colorBlendingCreateInfo.blendConstants[1] = 0.0f;
        colorBlendingCreateInfo.blendConstants[2] = 0.0f;
        colorBlendingCreateInfo.blendConstants[3] = 0.0f;

        //11. Dynamic States 
            // Pipeline structs to be changed in runtime
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT, //viewport size
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = 2;
        dynamicStateCreateInfo.pDynamicStates = dynamicStates;

        //12. Pipeline Layout 
            // Global uniforms layout
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayouts.composition;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        VkResult pipelineLayoutCreated = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayouts.composition);
        if (pipelineLayoutCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout");
        }


        //13. Depth Stencil settings
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;

        // can discard fragments outside a pre-defined range
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.minDepthBounds = 0.0f;
        depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.front = {};
        depthStencilStateCreateInfo.back = {};



        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        // programmable shader stages
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stageCount = 2; // amount of shader stages
        pipelineCreateInfo.pStages = shaderStagesCreateInfo;

        // fixed function stages
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisamplingCreateinfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
        pipelineCreateInfo.pDynamicState = nullptr;

        // uniform pipeline layout
        pipelineCreateInfo.layout = pipelineLayouts.composition;

        // render passes
        pipelineCreateInfo.renderPass = renderPass;
        pipelineCreateInfo.subpass = 1; // index of subpass

        // create pipeline deriving from existing pipeline
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        // create pipeline(s)
        // (device, VkPipelineCache, pipelineInfos, nullptr, graphicsPipeline)
        VkResult pipelineCreated = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipelines.composition);
        if (pipelineCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline");
        }

        std::cout << "graphics pipeline created" << '\n';

        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);

}

VkShaderModule VulkanApp::createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo shaderCreateInfo{};
        shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderCreateInfo.codeSize = code.size();
        shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // pointer to shader code data

        VkShaderModule shaderModule;
        VkResult shaderCreated = vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule);
        if (shaderCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module");
        }

        return shaderModule;
}

void VulkanApp::createRenderPass() {
        //1. Attachments 
        // ------------------------
        VkAttachmentDescription col0{};
        col0.format = VK_FORMAT_R32G32B32A32_SFLOAT; // 32bit-per-channel floating point attachment (to store values higher than 1.0) for HDR rendering
        // (tonemapping applied before outputting to swapchain image)
        col0.samples = VK_SAMPLE_COUNT_1_BIT;
        // loadOp - attachment's content operation, at begining of subpass
        col0.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear the values to a constant  
        // storeOp - attachment's content operation, at end of subpass
        col0.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store for reading in subpass1
        col0.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        col0.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        col0.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // dont care about previous layout, will transition in subpasses
        col0.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentDescription depth0{};
        depth0.format = findSupportedDepthFormat();
        depth0.samples = VK_SAMPLE_COUNT_1_BIT;
        depth0.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear the values to a constant  
        depth0.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store for reading in subpass1 
        depth0.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth0.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;;
        depth0.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // dont care about previous layout, will transition in subpasses
        depth0.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; //VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription col1{};
        col1.format = swapChainImageFormat; 
        col1.samples = VK_SAMPLE_COUNT_1_BIT;
        col1.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear the values to a constant  
        col1.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store for presentation
        col1.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        col1.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        col1.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // dont care about previous layout, will transition in subpasses
        col1.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        std::array<VkAttachmentDescription, 3> attachments = { col0, depth0, col1 };

        //2. Subpasses
        // ------------
        // Define input/output attachments, and their target image layout for the subpass
        // 
        // Subpass 0
        // ---------
        VkAttachmentReference col0_out{};
        col0_out.attachment = 0;
        /// col0 -> transition to layout_color_attachment
        col0_out.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth0_out{};
        depth0_out.attachment = 1;
        // depth0 -> transition to layout_depth_stencil_attachment
        depth0_out.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass0{};
        subpass0.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // graphics or compute subpass
        subpass0.colorAttachmentCount = 1;
        subpass0.pInputAttachments = nullptr;
        subpass0.pColorAttachments = &col0_out;
        subpass0.pDepthStencilAttachment = &depth0_out;

        // Subpass 1
        // ---------
        VkAttachmentReference col0_in{};
        col0_in.attachment = 0;
        // col0 -> transition to layout_shader_read_only
        col0_in.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference depth0_in{};
        depth0_in.attachment = 1;
        // depth0 -> transition to layout_shader_read_only
        depth0_in.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference inputAttachments[] = { col0_in, depth0_in };

        VkAttachmentReference col1_out{};
        col1_out.attachment = 2;
        // col1 -> transition to layout_color_attachment
        col1_out.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass1{};
        subpass1.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // graphics or compute subpass
        subpass1.inputAttachmentCount = 2;
        subpass1.pInputAttachments = inputAttachments;
        subpass1.colorAttachmentCount = 1;
        subpass1.pColorAttachments = &col1_out;
        subpass1.pDepthStencilAttachment = &depth0_in;

        VkSubpassDescription subpasses[] = { subpass0, subpass1 };

        //Dependencies:
        // synchronization of attachment layout transitions + operations which access the attachments.
        // 
        // 1. block layout transition, until previous subpass operations (which access attachments) are finished.
        // 2. block subpass operations (which access attachments) until layout transition is finished.

        VkSubpassDependency dependency0{};

        dependency0.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency0.dstSubpass = 0;

        // block (out)col0 first layout transition (to "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL"), until begin's load/clear operation is finished.
        // block (out)depth0 first layout transition (to "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL"), until begin's load/clear operation is finished.
        dependency0.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependency0.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        // block subpass0 color read/write operation (in "output" stage of pipeline), until layout transition is finished.
        // block subpass0 depth read/write operation (in "early fragment-test" stage of pipeline), until layout transition is finished.
        dependency0.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency0.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        VkSubpassDependency dependency1{};
        dependency1.srcSubpass = 0;
        dependency1.dstSubpass = 1;

        // block (in)col0 final layout transition (to "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL"), until subpass0 output operation is finished. (color output stage)
        // block (in)depth0 final layout transition (to "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL"), until subpass0 depth-write operation is finished. (early test stage)
        // block (out)col1 first layout transition ("to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL"), until subpass0 output operation is finished.
        dependency1.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency1.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        // block supbass1 col0 read operation (in "fragment shader" stage of pipeline), until layout transition is finished.
        // block supbass1 depth0 read operation (in "fragment shader" stage of pipeline), until layout transition is finished.
        // block supbass1 col1 write operation (in "output" stage of pipeline), until layout transition is finished.
        dependency1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependency1.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        VkSubpassDependency dependency2{};
        dependency2.srcSubpass = 1;
        dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;

        // block col1 final layout transition (to "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR"), until subpass1 ouptut operation is finished.
        dependency2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency2.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        // block ending's col1 read operation, until layout transition is finished.
        dependency2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependency2.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        VkSubpassDependency dependencies[] = { dependency0, dependency1 };

        //3. Render Pass
        // -------------
        VkRenderPassCreateInfo renderPassCreateInfo{};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassCreateInfo.pAttachments = attachments.data();
        renderPassCreateInfo.subpassCount = 2;
        renderPassCreateInfo.pSubpasses = subpasses;
        renderPassCreateInfo.dependencyCount = 2;
        renderPassCreateInfo.pDependencies = dependencies;

        VkResult renderPassCreated = vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass);
        if (renderPassCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass");
        }

}

void VulkanApp::createFramebuffers() {
        // create framebuffer for each swapchain view
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                offscreenImageView, depthImageView, swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 3;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            VkResult framebufferCreated = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
            if (framebufferCreated != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer");
            }

            std::cout << "framebuffer created: framebuffer_" << std::to_string(i) << '\n';

        }
}

void VulkanApp::createGraphicsCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo graphicsPoolCreateInfo{};
        graphicsPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        graphicsPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamilyIndex.value();

        VkResult graphicsCommandPoolCreated = vkCreateCommandPool(device, &graphicsPoolCreateInfo, nullptr, &graphicsCommandPool);
        if (graphicsCommandPoolCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool");
        }
        std::cout << "graphics command pool created" << '\n';
}

void VulkanApp::createTransferCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo transferPoolCreateInfo{};
        transferPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        transferPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.transferFamilyIndex.value();

        VkResult transferCommandPoolCreated = vkCreateCommandPool(device, &transferPoolCreateInfo, nullptr, &transferCommandPool);
        if (transferCommandPoolCreated != VK_SUCCESS) {
            throw std::runtime_error("failed to create transfer command pool");
        }
        std::cout << "transfer command pool created" << '\n';
}

// create command buffers (per swap image), and record commands
void VulkanApp::recordCommandBuffers() {
        uint32_t swapchainImageCount = (uint32_t)swapChainFramebuffers.size();
        graphicsCommandBuffer.resize(swapchainImageCount); //create command buffer per swapchain framebuffer

        VkCommandBufferAllocateInfo gCommandBuffersAllocateInfo{};
        gCommandBuffersAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        gCommandBuffersAllocateInfo.commandPool = graphicsCommandPool;
        // level: primary / secondary command buffer
            // VK_COMMAND_BUFFER_LEVEL_PRIMARY: buffer can be executed by a queue directly
            // VK_COMMAND_BUFFER_LEVEL_SECONDARY: buffer can be executed by other command buffers
        gCommandBuffersAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        gCommandBuffersAllocateInfo.commandBufferCount = (uint32_t)graphicsCommandBuffer.size();

        VkResult graphicsCommandBuffersAllocated = vkAllocateCommandBuffers(device, &gCommandBuffersAllocateInfo, graphicsCommandBuffer.data());
        if (graphicsCommandBuffersAllocated != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate graphics command buffers from command pool");
        }
        std::cout << "graphics command buffers allocated from graphics pool. buffers count: " << std::to_string(graphicsCommandBuffer.size()) << '\n';

        for (uint32_t i = 0; i < swapchainImageCount; i++) {
            VkCommandBufferBeginInfo commandBufferBeginInfo{};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            commandBufferBeginInfo.flags = 0;
            commandBufferBeginInfo.pInheritanceInfo = nullptr;

            VkResult commandBufferBeganRecording = vkBeginCommandBuffer(graphicsCommandBuffer[i], &commandBufferBeginInfo);
            if (commandBufferBeganRecording != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer");
            }
            std::cout << "started recording command buffer " << std::to_string(i) << '\n';

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = swapChainExtent;

            // Define color of "VK_ATTACHMENT_LOAD_OP_CLEAR", used in attachment's load operation
            std::array<VkClearValue, 3> clearValues = {};
            clearValues[0] = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[1] = { 1.0f, 0 };
            clearValues[2] = { {0.0f, 0.0f, 0.0f, 1.0f} };

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            VkBuffer vertexBuffers[] = { vertexBuffer0, vertexBuffer1, vertexBuffer2, vertexBuffer3 };
            VkDeviceSize readOffset_Bytes[] = { 0, 0, 0, 0 };
            uint32_t bufferBinding[] = { 0, 0, 0, 0 };
            uint32_t bufferBindingCount[] = { 1, 1, 1, 1 };

            // record command: begin render pass (target command buffer, render pass info, primary/secondart buffer)
                // VK_SUBPASS_CONTENTS_INLINE: Render pass commands using primary command buffer
                // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: Render pass commands using secondary command buffer
            vkCmdBeginRenderPass(graphicsCommandBuffer[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


            // Subpass 0
            // ------------
            // scene draw to offscreen attachment
            // 
            // bind scene descriptor set
                // (set = 0, binding = 0)
            vkCmdBindDescriptorSets(graphicsCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.scene, 0, 1, &descriptorSets.scene[i], 0, nullptr);

            // record command: bind pipeline (target command buffer, pipline type (graphics/compute), pipeline)
            vkCmdBindPipeline(graphicsCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.scene);

            vkCmdBindVertexBuffers(graphicsCommandBuffer[i], bufferBinding[0], bufferBindingCount[0], &vertexBuffers[0], &readOffset_Bytes[0]);
            VkDeviceSize indexBufferByteOffset = 0;
            vkCmdBindIndexBuffer(graphicsCommandBuffer[i], indicesBuffer0, indexBufferByteOffset, VK_INDEX_TYPE_UINT32);
  
            uint32_t indexCount = static_cast<uint32_t>(indicesScene.size());
            uint32_t instanceCount = 1;
            uint32_t firstIndexOffset = 0;
            uint32_t vertexOffset = 0;
            uint32_t firstInstanceOffset = 0;
            vkCmdDrawIndexed(graphicsCommandBuffer[i], indexCount, instanceCount, firstIndexOffset, vertexOffset, firstInstanceOffset);


            //Subpass 1
            // -------------
            // increment active subpass index
            vkCmdNextSubpass(graphicsCommandBuffer[i], VK_SUBPASS_CONTENTS_INLINE);

            // fullscreen quad draw

            // bind composition descriptor set
                // (set = 0, binding = 0)
            vkCmdBindDescriptorSets(graphicsCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.composition, 0, 1, &descriptorSets.composition[i], 0, nullptr);
            vkCmdBindPipeline(graphicsCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.composition);
            vkCmdBindVertexBuffers(graphicsCommandBuffer[i], bufferBinding[3], bufferBindingCount[3], &vertexBuffers[3], &readOffset_Bytes[3]);
            const uint32_t vertexCount2 = static_cast<uint32_t>(verticesScreenQuad.size());
            vkCmdDraw(graphicsCommandBuffer[i], vertexCount2, 1, 0, 0);


            // fx draw
            // -------
            // binding fx descriptor set 0
                // layout (set = 0, binding = 0) uniform uboFX
            vkCmdBindDescriptorSets(graphicsCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.fx, 0, 1, &descriptorSets.fx0[i], 0, nullptr);
            // binding fx descriptor set 1
                // layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputColorAttachment;
                // layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputDepthAttachment;
            vkCmdBindDescriptorSets(graphicsCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.fx, 1, 1, &descriptorSets.fx1[i], 0, nullptr);
            vkCmdBindPipeline(graphicsCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.fx);
            vkCmdBindVertexBuffers(graphicsCommandBuffer[i], bufferBinding[1], bufferBindingCount[1], &vertexBuffers[1], &readOffset_Bytes[1]);
            vkCmdBindIndexBuffer(graphicsCommandBuffer[i], indicesBuffer1, indexBufferByteOffset, VK_INDEX_TYPE_UINT32);
            indexCount = static_cast<uint32_t>(indicesFX.size());
            instanceCount = 1;
            firstIndexOffset = 0;
            vertexOffset = 0;
            firstInstanceOffset = 0;
            vkCmdDrawIndexed(graphicsCommandBuffer[i], indexCount, instanceCount, firstIndexOffset, vertexOffset, firstInstanceOffset);

            // decal draw
            // ------------
            // binding fx descriptor set 0
                // layout (set = 0, binding = 0) uniform uboDecal
            vkCmdBindDescriptorSets(graphicsCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.decal, 0, 1, &descriptorSets.fx0[i], 0, nullptr);
            // binding fx descriptor set 1
                // layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputColorAttachment;
                // layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputDepthAttachment;
            vkCmdBindDescriptorSets(graphicsCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.decal, 1, 1, &descriptorSets.fx1[i], 0, nullptr);
            vkCmdBindPipeline(graphicsCommandBuffer[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.decal);
            vkCmdBindVertexBuffers(graphicsCommandBuffer[i], bufferBinding[2], bufferBindingCount[2], &vertexBuffers[2], &readOffset_Bytes[2]);
            vkCmdBindIndexBuffer(graphicsCommandBuffer[i], indicesBuffer2, indexBufferByteOffset, VK_INDEX_TYPE_UINT32);
            indexCount = static_cast<uint32_t>(indicesDecal.size());
            instanceCount = 1;
            firstIndexOffset = 0;
            vertexOffset = 0;
            firstInstanceOffset = 0;
            vkCmdDrawIndexed(graphicsCommandBuffer[i], indexCount, instanceCount, firstIndexOffset, vertexOffset, firstInstanceOffset);

            // end render pass
            vkCmdEndRenderPass(graphicsCommandBuffer[i]);

            // finish recording command buffer 
            VkResult commandBufferRecorded = vkEndCommandBuffer(graphicsCommandBuffer[i]);
            if (commandBufferRecorded != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer");
            }
            std::cout << "finished recording command buffer " << std::to_string(i) << '\n';

        }
}

void VulkanApp::createSyncObjects() {
        imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
        renderingFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
        transferFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
        VkSemaphoreCreateInfo semaphoreCreateInfo{}; // GPU-GPU syncrhonization (block GPU execution, using GPU signals)
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        cbuffersExecutionFence.resize(MAX_FRAMES_IN_FLIGHT);
        uint32_t swapchainImageCount = (uint32_t)swapChainFramebuffers.size();
        swapchainImageFence.resize(swapchainImageCount, VK_NULL_HANDLE);

        VkFenceCreateInfo fenceCreateInfo{}; // CPU-GPU synchronization (block CPU exection, using GPU signals)
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // create fence in already-signalled state

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkResult imageAvailableSemaphoreCreated = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore[i]);
            VkResult renderingFinishedSemaphoreCreated = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderingFinishedSemaphore[i]);
            VkResult transferFinishedSemaphoreCreated = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &transferFinishedSemaphore[i]);

            if (imageAvailableSemaphoreCreated != VK_SUCCESS) {
                throw std::runtime_error("failed creating imageFAvailable sempahore");
            }

            if (renderingFinishedSemaphoreCreated != VK_SUCCESS) {
                throw std::runtime_error("failed creating renderingFinished sempahore");
            }

            if (transferFinishedSemaphoreCreated != VK_SUCCESS) {
                throw std::runtime_error("failed creating transferFinished sempahore");
            }

            VkResult cbuffersExecutionFenceCreated = vkCreateFence(device, &fenceCreateInfo, nullptr, &cbuffersExecutionFence[i]);

            if (cbuffersExecutionFenceCreated != VK_SUCCESS) {
                throw std::runtime_error("failed creating cbuffersExecution fence");
            }
        }

}

void VulkanApp::createTransferCommandBuffer() {
    VkCommandBufferAllocateInfo commandBuffersAllocateInfo{};
    commandBuffersAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBuffersAllocateInfo.commandPool = graphicsCommandPool;
    commandBuffersAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBuffersAllocateInfo.commandBufferCount = 1;

    VkResult transferCommandBuffersAllocated = vkAllocateCommandBuffers(device, &commandBuffersAllocateInfo, &transferCommandBuffer);
    if (transferCommandBuffersAllocated != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate transfer command buffers from command pool");
    }
    std::cout << "transfer command buffer allocated " << '\n';

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;
    VkResult commandBufferBeganRecording = vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);
    if (commandBufferBeganRecording != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer");
    }
}

void VulkanApp::submitTransferCommandBuffer() {
    vkEndCommandBuffer(transferCommandBuffer);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    /*
    // GPU will block execution of cbuffer, until these semaphores are signalled:
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore[frameID] };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;

    // specify stage(s) in pipeline where waiting occurs
    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:
        // block before frag stage (attachment output)
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStages;
    */

    // specify command buffer, using correct swapchain image:
        // graphicsCommandBuffers[imageIndex]
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCommandBuffer;

    /*
    // specify semaphore(s) to signal when cbuffer finishes execution
    VkSemaphore signalSemaphores[] = { renderingFinishedSemaphore[frameID] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    */
    VkResult transferBufferSubmitted_GraphicsQueue = vkQueueSubmit(graphicsQueue, 1, &submitInfo, 0);         // (queue, command buffers count, submit info, optional fence to signal)
    if (transferBufferSubmitted_GraphicsQueue != VK_SUCCESS) {
        throw std::runtime_error("failed to submit command buffer to graphics queue");
    }
    std::cout << "submitted command buffer to graphics queue" << '\n';
    vkFreeCommandBuffers(device, graphicsCommandPool, 1, &transferCommandBuffer);
}

VkCommandBuffer VulkanApp::createSingleUseCommandBuffer() {
        VkCommandBufferAllocateInfo commandBuffersAllocateInfo{};
        commandBuffersAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBuffersAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBuffersAllocateInfo.commandPool = graphicsCommandPool;
        commandBuffersAllocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VkResult commandBuffersAllocated = vkAllocateCommandBuffers(device, &commandBuffersAllocateInfo, &commandBuffer);
        if (commandBuffersAllocated != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate transfer command buffers from command pool");
        }
        std::cout << "(single-use) transfer command buffers allocated from transfer command pool" << '\n';

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        // use command buffer once, to copy buffers.
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
}

void VulkanApp::submitSingleUseCommandBuffer(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue); // block until queue finishes executing commandbuffer 

        vkFreeCommandBuffers(device, graphicsCommandPool, 1, &commandBuffer);
}

void VulkanApp::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

        VkCommandBuffer commandBuffer = createSingleUseCommandBuffer();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // byte offset in buffer
        copyRegion.dstOffset = 0;
        copyRegion.size = size; // byte size to copy
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        submitSingleUseCommandBuffer(commandBuffer);
}

void VulkanApp::copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height) {

    VkCommandBuffer commandBuffer = createSingleUseCommandBuffer();
    VkBufferImageCopy region{};
    region.bufferOffset = 0; //byte offset for start of image data
    region.bufferRowLength = 0; 
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    // texel offset for relevant region of image
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };
    
    vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    submitSingleUseCommandBuffer(commandBuffer);
}

void VulkanApp::createVertexBuffers() {

        // Store vertex buffer in high-performacne GPU memory (local device memory)
        // -----------------------------------------------------------------------------
        // 1. Create CPU-accessible staging buffer
        // 2. Create GPU-local vertex buffer

        // 3. Upload data to CPU-writeable "staging" buffer
        // 4. Copy data from staging buffer to vertex buffer


        VkDeviceSize buffer0Size = sizeof(verticesScene[0]) * verticesScene.size(); // buffer's byte size

        VkBuffer stagingBuffer0;
        VkDeviceMemory stagingBufferMemory0;

        VkBufferUsageFlags stagingBufferUsageBitflags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in transfer op
        VkMemoryPropertyFlags stagingBufferMemPropertiesBitflags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        createGraphicsBuffer(buffer0Size, stagingBufferUsageBitflags, stagingBufferMemPropertiesBitflags, stagingBuffer0, stagingBufferMemory0);

        void* data0;
        vkMapMemory(device, stagingBufferMemory0, 0, buffer0Size, 0, &data0); // (access region of specified GPU memory, at given offset, and size, 0, output pointer to memory)
        memcpy(data0, verticesScene.data(), (size_t)buffer0Size); // copy data to GPU memory (happens in the background, before next vkSubmitQueue
        vkUnmapMemory(device, stagingBufferMemory0);

        VkBufferUsageFlags bufferUsageBitflags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkMemoryPropertyFlags memPropertiesBitflags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        createGraphicsBuffer(buffer0Size, bufferUsageBitflags, memPropertiesBitflags, vertexBuffer0, vertexBufferMemory0);
        copyBuffer(stagingBuffer0, vertexBuffer0, buffer0Size);
        vkDestroyBuffer(device, stagingBuffer0, nullptr);
        vkFreeMemory(device, stagingBufferMemory0, nullptr);

        // --------------------------------------------------------------------------------

        VkDeviceSize buffer1Size = sizeof(verticesFX[0]) * verticesFX.size();
        VkBuffer stagingBuffer1;
        VkDeviceMemory stagingBufferMemory1;
        createGraphicsBuffer(buffer1Size, stagingBufferUsageBitflags, stagingBufferMemPropertiesBitflags, stagingBuffer1, stagingBufferMemory1);

        void* data1;
        vkMapMemory(device, stagingBufferMemory1, 0, buffer1Size, 0, &data1);
        memcpy(data1, verticesFX.data(), (size_t)buffer1Size);
        vkUnmapMemory(device, stagingBufferMemory1);

        createGraphicsBuffer(buffer1Size, bufferUsageBitflags, memPropertiesBitflags, vertexBuffer1, vertexBufferMemory1);
        copyBuffer(stagingBuffer1, vertexBuffer1, buffer1Size);
        vkDestroyBuffer(device, stagingBuffer1, nullptr);
        vkFreeMemory(device, stagingBufferMemory1, nullptr);


        // --------------------------------------------------------------------------------------


        VkDeviceSize buffer2Size = sizeof(verticesDecal[0]) * verticesDecal.size();
        VkBuffer stagingBuffer2;
        VkDeviceMemory stagingBufferMemory2;
        createGraphicsBuffer(buffer2Size, stagingBufferUsageBitflags, stagingBufferMemPropertiesBitflags, stagingBuffer2, stagingBufferMemory2);

        void* data2;
        vkMapMemory(device, stagingBufferMemory2, 0, buffer2Size, 0, &data2);
        memcpy(data2, verticesDecal.data(), (size_t)buffer2Size);
        vkUnmapMemory(device, stagingBufferMemory2);

        createGraphicsBuffer(buffer2Size, bufferUsageBitflags, memPropertiesBitflags, vertexBuffer2, vertexBufferMemory2);
        copyBuffer(stagingBuffer2, vertexBuffer2, buffer2Size);
        vkDestroyBuffer(device, stagingBuffer2, nullptr);
        vkFreeMemory(device, stagingBufferMemory2, nullptr);

        // ---------------------

        VkDeviceSize buffer3Size = sizeof(verticesScreenQuad[0]) * verticesScreenQuad.size();
        VkBuffer stagingBuffer3;
        VkDeviceMemory stagingBufferMemory3;
        createGraphicsBuffer(buffer3Size, stagingBufferUsageBitflags, stagingBufferMemPropertiesBitflags, stagingBuffer3, stagingBufferMemory3);

        void* data3;
        vkMapMemory(device, stagingBufferMemory3, 0, buffer3Size, 0, &data3);
        memcpy(data3, verticesScreenQuad.data(), (size_t)buffer3Size);
        vkUnmapMemory(device, stagingBufferMemory3);

        createGraphicsBuffer(buffer3Size, bufferUsageBitflags, memPropertiesBitflags, vertexBuffer3, vertexBufferMemory3);
        copyBuffer(stagingBuffer3, vertexBuffer3, buffer3Size);
        vkDestroyBuffer(device, stagingBuffer3, nullptr);
        vkFreeMemory(device, stagingBufferMemory3, nullptr);

}

void VulkanApp::createIndexBuffers() {

        VkDeviceSize buffer0Size = sizeof(indicesScene[0]) * indicesScene.size(); // buffer's byte size

        VkBuffer stagingBuffer0;
        VkDeviceMemory stagingBufferMemory0;

        // VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in transfer op
        VkBufferUsageFlags stagingBufferUsageBitflags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkMemoryPropertyFlags stagingBufferMemPropertiesBitflags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        createGraphicsBuffer(buffer0Size, stagingBufferUsageBitflags, stagingBufferMemPropertiesBitflags, stagingBuffer0, stagingBufferMemory0);

        void* data0;
        vkMapMemory(device, stagingBufferMemory0, 0, buffer0Size, 0, &data0); // (access region of specified GPU memory, at given offset, and size, 0, output pointer to memory)
        memcpy(data0, indicesScene.data(), (size_t)buffer0Size); // copy data to GPU memory (happens in the background, before next vkSubmitQueue
        vkUnmapMemory(device, stagingBufferMemory0);

        VkBufferUsageFlags bufferUsageBitflags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        VkMemoryPropertyFlags memPropertiesBitflags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        createGraphicsBuffer(buffer0Size, bufferUsageBitflags, memPropertiesBitflags, indicesBuffer0, indicesBufferMemory0);
        copyBuffer(stagingBuffer0, indicesBuffer0, buffer0Size);
        vkDestroyBuffer(device, stagingBuffer0, nullptr);
        vkFreeMemory(device, stagingBufferMemory0, nullptr);

        // --------------------------------------------------------------------------------


        VkDeviceSize buffer1Size = sizeof(indicesFX[0]) * indicesFX.size();
        VkBuffer stagingBuffer1;
        VkDeviceMemory stagingBufferMemory1;
        createGraphicsBuffer(buffer1Size, stagingBufferUsageBitflags, stagingBufferMemPropertiesBitflags, stagingBuffer1, stagingBufferMemory1);

        void* data1;
        vkMapMemory(device, stagingBufferMemory1, 0, buffer1Size, 0, &data1);
        memcpy(data1, indicesFX.data(), (size_t)buffer1Size);
        vkUnmapMemory(device, stagingBufferMemory1);

        createGraphicsBuffer(buffer1Size, bufferUsageBitflags, memPropertiesBitflags, indicesBuffer1, indicesBufferMemory1);
        copyBuffer(stagingBuffer1, indicesBuffer1, buffer1Size);
        vkDestroyBuffer(device, stagingBuffer1, nullptr);
        vkFreeMemory(device, stagingBufferMemory1, nullptr);


        // --------------------------------------------------------------------------------


        VkDeviceSize buffer2Size = sizeof(indicesDecal[0]) * indicesDecal.size();
        VkBuffer stagingBuffer2;
        VkDeviceMemory stagingBufferMemory2;
        createGraphicsBuffer(buffer2Size, stagingBufferUsageBitflags, stagingBufferMemPropertiesBitflags, stagingBuffer2, stagingBufferMemory2);

        void* data2;
        vkMapMemory(device, stagingBufferMemory2, 0, buffer2Size, 0, &data2);
        memcpy(data2, indicesDecal.data(), (size_t)buffer2Size);
        vkUnmapMemory(device, stagingBufferMemory2);

        createGraphicsBuffer(buffer2Size, bufferUsageBitflags, memPropertiesBitflags, indicesBuffer2, indicesBufferMemory2);
        copyBuffer(stagingBuffer2, indicesBuffer2, buffer2Size);
        vkDestroyBuffer(device, stagingBuffer2, nullptr);
        vkFreeMemory(device, stagingBufferMemory2, nullptr);
}

void VulkanApp::loadObjs() {
        tinyobj::attrib_t attribScene; 
        std::vector<tinyobj::shape_t> shapesScene;
        std::vector<tinyobj::material_t> materialsScene;
        std::string warn0, err0;
        bool sceneObjLoaded = tinyobj::LoadObj(&attribScene, &shapesScene, &materialsScene, &warn0, &err0, MODEL_PATH_0.c_str());
        if (!sceneObjLoaded) {
            throw std::runtime_error(warn0 + err0);
        }
        std::cout << "scene.obj loaded" << '\n';
        // for every triangle
        for (const auto& shape : shapesScene) {
            int ind = 0;
            // for every unique vertex/index
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                // access attrib_t.vertices float array:
                float posx = attribScene.vertices[3 * index.vertex_index + 0];
                float posy = attribScene.vertices[3 * index.vertex_index + 1];
                float posz = attribScene.vertices[3 * index.vertex_index + 2];
                vertex.pos = glm::vec3(posx, posy, posz);

                float uvx = attribScene.texcoords[2 * index.texcoord_index + 0];
                float uvy = attribScene.texcoords[2 * index.texcoord_index + 1];
                vertex.uv = glm::vec2(uvx, uvy);


                float normalx = attribScene.normals[3 * index.normal_index + 0];
                float normaly = attribScene.normals[3 * index.normal_index + 1];
                float normalz = attribScene.normals[3 * index.normal_index + 2];
                vertex.normal = glm::vec3(normalx, normaly, normalz);

                vertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
                verticesScene.push_back(vertex);
                indicesScene.push_back(ind);
                ind += 1;
            }
        }

        tinyobj::attrib_t attribSphere;
        std::vector<tinyobj::shape_t> shapesSphere;
        std::vector<tinyobj::material_t> materialsSphere;
        std::string warn1, err1;
        bool sphereObjLoaded = tinyobj::LoadObj(&attribSphere, &shapesSphere, &materialsSphere, &warn1, &err1, MODEL_PATH_1.c_str());
        if (!sphereObjLoaded) {
            throw std::runtime_error(warn1 + err1);
        }
        std::cout << "sphere.obj loaded" << '\n';
        for (const auto& shape : shapesSphere) {
            int ind = 0;
            // for every unique vertex/index
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                // access attrib_t.vertices float array:
                float posx = attribSphere.vertices[3 * index.vertex_index + 0];
                float posy = attribSphere.vertices[3 * index.vertex_index + 1];
                float posz = attribSphere.vertices[3 * index.vertex_index + 2];
                vertex.pos = glm::vec3(posx, posy, posz);

                float uvx = attribSphere.texcoords[2 * index.texcoord_index + 0];
                float uvy = attribSphere.texcoords[2 * index.texcoord_index + 1];
                vertex.uv = glm::vec2(uvx, uvy);

                float normalx = attribSphere.normals[3 * index.normal_index + 0];
                float normaly = attribSphere.normals[3 * index.normal_index + 1];
                float normalz = attribSphere.normals[3 * index.normal_index + 2];
                vertex.normal = glm::vec3(normalx, normaly, normalz);

                vertex.color = glm::vec3(0.0f, 0.0f, 1.0f);
                verticesFX.push_back(vertex);
                indicesFX.push_back(ind);
                ind += 1;
            }
        }

        tinyobj::attrib_t attribCube;
        std::vector<tinyobj::shape_t> shapesCube;
        std::vector<tinyobj::material_t> materialsCube;
        std::string warn2, err2;
        bool cubeObjLoaded = tinyobj::LoadObj(&attribCube, &shapesCube, &materialsCube, &warn2, &err2, MODEL_PATH_2.c_str());
        if (!cubeObjLoaded) {
            throw std::runtime_error(warn2 + err2);
        }
        std::cout << "cube.obj loaded" << '\n';
        for (const auto& shape : shapesCube) {
            int ind = 0;
            // for every unique vertex/index
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                // access attrib_t.vertices float array:
                float posx = attribCube.vertices[3 * index.vertex_index + 0];
                float posy = attribCube.vertices[3 * index.vertex_index + 1];
                float posz = attribCube.vertices[3 * index.vertex_index + 2];
                vertex.pos = glm::vec3(posx, posy, posz);

                float uvx = attribCube.texcoords[2 * index.texcoord_index + 0];
                float uvy = attribCube.texcoords[2 * index.texcoord_index + 1];
                vertex.uv = glm::vec2(uvx, uvy);

                float normalx = attribCube.normals[3 * index.normal_index + 0];
                float normaly = attribCube.normals[3 * index.normal_index + 1];
                float normalz = attribCube.normals[3 * index.normal_index + 2];
                vertex.normal = glm::vec3(normalx, normaly, normalz);

                vertex.color = glm::vec3(0.0f, 0.0f, 1.0f);
                verticesDecal.push_back(vertex);
                indicesDecal.push_back(ind);
                ind += 1;
            }
        }
}

void VulkanApp::initVulkan() {
        createInstance();
        if (enableExtensionLayers) {
            loadExtensionFunctions();
        }
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDeviceAndQueues();
        createSwapChain();
        //createTransferCommandPool();
        createGraphicsCommandPool();
        createTransferCommandBuffer();
        createImageResources();
        createDepthResources();
        createTextureImageResources();
        submitTransferCommandBuffer();
        createRenderPass();
        createDescriptorSetLayouts();
        createGraphicsPipelineScene();
        createGraphicsPipelineFX();
        createGraphicsPipelineDecal();
        createGraphicsPipelineComposition();
        createFramebuffers();
        createTextureSampler();

        loadObjs();
        createVertexBuffers();
        createIndexBuffers();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        recordCommandBuffers();

        createSyncObjects();

}

void VulkanApp::updateUniformBuffers(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        float t = sin(time);
        t = t * 0.5 + 0.5; //[0,1]

        glm::mat4 cam0View = glm::lookAt(glm::vec3(0.0f, 1.0f, -4.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat cam0Proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.001f, 10.0f);

        UniformBufferObjectScene uboScene{};
        glm::mat4 trans = glm::mat4(1.0f);
        trans = glm::translate(trans, glm::vec3(0.0f, 0.0f, 0.0f));
        //trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
        trans = glm::scale(trans, glm::vec3(1.0f, 1.0f, 1.0f));
        uboScene.model = trans;
        uboScene.view = cam0View;
        uboScene.proj = cam0Proj;
        // flip y coordinates
        uboScene.proj[1][1] *= -1;

        // map gpu memory at address "data" (to store "ubo")
        void* data0;
        vkMapMemory(device, uniformBuffersSceneMemory[currentImage], 0, sizeof(uboScene), 0, &data0);
        // copy "ubo" data to gpu memory (at "data" address)
        memcpy(data0, &uboScene, sizeof(uboScene));
        vkUnmapMemory(device, uniformBuffersSceneMemory[currentImage]);

        UniformBufferObjectFX uboFX{};
        trans = glm::mat4(1.0f);
        float transOffset = 0.0f + 0.5f * t;
        trans = glm::translate(trans, glm::vec3(0.0f, 0.0f, 0.0f));
        //trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
  
        float scaleOffset = 1.0f + 0.25f * t;
        trans = glm::scale(trans, glm::vec3(scaleOffset, scaleOffset, scaleOffset));
        uboFX.model = trans;
        uboFX.view = cam0View;
        uboFX.proj = cam0Proj;
        uboFX.proj[1][1] *= -1;

        uboFX.res = glm::vec2(WIDTH, HEIGHT);

        void* data1;
        vkMapMemory(device, uniformBuffersFXMemory[currentImage], 0, sizeof(uboFX), 0, &data1);
        // copy "ubo" data to gpu memory (at "data" address)
        memcpy(data1, &uboFX, sizeof(uboFX));
        vkUnmapMemory(device, uniformBuffersFXMemory[currentImage]);


    }

void VulkanApp::checkFenceStatus() {
        while (blocked == true) {
            std::cout << "Frame " << std::to_string(frameID) << "'s command-buffer is stil executing....\n";
            std::cout << "inFlightFences " << std::to_string(frameID) << " is unsignalled \n";
        }
    }

void VulkanApp::drawFrame() {

        std::cout << "drawing frame..... " << '\n';
        std::cout << "target frame " << std::to_string(frameID) << '\n';

        // block further cbuffer submissions, until previously submitted cbuffers finished execution (for frameID)
            // note: fences created in already "signaled" state, to avoid initial block
        vkWaitForFences(device, 1, &cbuffersExecutionFence[frameID], VK_TRUE, UINT64_MAX);

        uint32_t swapImageID;
        // fetch available image from swapchain, then signal image's semaphore
        // --------------------------------------------------------------------
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore[frameID], VK_NULL_HANDLE, &swapImageID);
        std::cout << "target swapchain image " << std::to_string(swapImageID) << '\n';

        // block further cbuffer submissions, until previously submitted cbuffers finished execution (which render to swapchain image ID)
            // note: swapchain-image fences initialize as null handle, to avoid initial block
        if (swapchainImageFence[swapImageID] != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &swapchainImageFence[swapImageID], VK_TRUE, UINT64_MAX);
        }

        swapchainImageFence[swapImageID] = cbuffersExecutionFence[frameID];

        updateUniformBuffers(swapImageID);


        // Submit commandbuffers to queue:
        // -------------------------------
            // Wait until swapchain-image semaphore is signalled (image is available)
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // GPU will block execution of cbuffer, until these semaphores are signalled:
        VkSemaphore waitSemaphores[] = { imageAvailableSemaphore[frameID] };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;

        // specify stage(s) in pipeline where waiting occurs
        // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:
            // block before frag stage (attachment output)
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.pWaitDstStageMask = waitStages;

        // specify command buffer, using correct swapchain image:
            // graphicsCommandBuffers[imageIndex]
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &graphicsCommandBuffer[swapImageID];

        // specify semaphore(s) to signal when cbuffer finishes execution
        VkSemaphore signalSemaphores[] = { renderingFinishedSemaphore[frameID] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // unsignal frame's fence
        vkResetFences(device, 1, &cbuffersExecutionFence[frameID]);
        // frame1signal = false

        // submit command buffers to graphics queue
        // when execution finishes, signal the frame's fence. (cbuffersExecutionFence[currentFrame])
        VkResult commandBufferSubmitted_GraphicsQueue = vkQueueSubmit(graphicsQueue, 1, &submitInfo, cbuffersExecutionFence[frameID]);         // (queue, command buffers count, submit info, optional fence to signal)
        if (commandBufferSubmitted_GraphicsQueue != VK_SUCCESS) {
            throw std::runtime_error("failed to submit command buffer to graphics queue");
        }
        std::cout << "submitted command buffer to graphics queue" << '\n';
        std::cout << "fence: cbufferExecFence " << std::to_string(frameID) << " will signal when execution is done" << '\n';


        // Present swapchain image 
        // -------------------------
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        // wait for frame's cbuffer execution to finish, before presenting swapchain image
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        // specify target image in swapchain 
        presentInfo.pImageIndices = &swapImageID;
        // specify array of VkResults incase of multiple swapchains (multi windows)
        presentInfo.pResults = nullptr;

        // present swapchain image to window surface
        VkResult swapchainImagePresented = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (swapchainImagePresented != VK_SUCCESS) {
            throw std::runtime_error("failed to present swapchain image to window surface");
        }

        std::cout << "presented swapchain image to window sufface (swapchain-image " << std::to_string(swapImageID) << ")" << '\n';
        // loop current frame id (currrentFrame = 0, 1)
        frameID = (frameID + 1) % MAX_FRAMES_IN_FLIGHT;
    }

void VulkanApp::mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            drawFrame();
            glfwPollEvents();
        }

        // block until device finishes work (avoid exiting loop/application while device is executing work asynchronously)
        vkDeviceWaitIdle(device);
    }

void VulkanApp::cleanup() {

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, imageAvailableSemaphore[i], nullptr);
            vkDestroySemaphore(device, renderingFinishedSemaphore[i], nullptr);
            vkDestroySemaphore(device, transferFinishedSemaphore[i], nullptr);
            vkDestroyFence(device, cbuffersExecutionFence[i], nullptr);
        }

        vkDestroyCommandPool(device, graphicsCommandPool, nullptr);
        vkDestroyCommandPool(device, transferCommandPool, nullptr);

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        vkDestroyBuffer(device, vertexBuffer0, nullptr);
        vkDestroyBuffer(device, vertexBuffer1, nullptr);
        vkDestroyBuffer(device, vertexBuffer2, nullptr);
        vkDestroyBuffer(device, indicesBuffer0, nullptr);
        vkDestroyBuffer(device, indicesBuffer1, nullptr);

        vkFreeMemory(device, vertexBufferMemory0, nullptr);
        vkFreeMemory(device, vertexBufferMemory1, nullptr);
        vkFreeMemory(device, vertexBufferMemory2, nullptr);
        vkFreeMemory(device, indicesBufferMemory0, nullptr);
        vkFreeMemory(device, indicesBufferMemory1, nullptr);

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vkDestroyBuffer(device, uniformBuffersScene[i], nullptr);
            vkDestroyBuffer(device, uniformBuffersFX[i], nullptr);
            vkFreeMemory(device, uniformBuffersSceneMemory[i], nullptr);
            vkFreeMemory(device, uniformBuffersFXMemory[i], nullptr);
        }
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, offscreenImageView, nullptr);
        vkFreeMemory(device, offscreenImageMemory, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.scene, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.composition, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.fx0, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayouts.fx1, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroyPipeline(device, pipelines.scene, nullptr);
        vkDestroyPipeline(device, pipelines.composition, nullptr);
        vkDestroyPipeline(device, pipelines.fx, nullptr);
        vkDestroyPipeline(device, pipelines.decal, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayouts.scene, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayouts.composition, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayouts.fx, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayouts.decal, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyDevice(device, nullptr);
        if (enableExtensionLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
