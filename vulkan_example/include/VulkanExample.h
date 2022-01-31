#ifndef VULKANEXAMPLE_H
#define VULKANEXAMPLE_H

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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



// Vulkan subpasses example

class VulkanApp {

private:
    bool blocked;
   
	GLFWwindow* window;
    const uint32_t WIDTH = 1920;
    const uint32_t HEIGHT = 1080;

    VkInstance instance;

    //extension functions:
    PFN_vkCmdPipelineBarrier2KHR _vkCmdPipelineBarrier2KHR;

    VkDebugUtilsMessengerEXT debugMessenger;

    const std::vector<const char*> layers{
        "VK_LAYER_KHRONOS_validation",
        "VK_LAYER_KHRONOS_synchronization2"
    };

	#ifdef NDEBUG
	const bool enableExtensionLayers = false;
	#else
    const bool enableExtensionLayers = true;
	#endif
	
    uint32_t glfwExtensionCount;

    uint32_t deviceCount;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkSurfaceKHR surface;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;
    VkQueue transferQueue;

    VkSwapchainKHR swapChain;

    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    VkImage offscreenImage;
    VkDeviceMemory offscreenImageMemory;
    VkFormat offscreenImageFormat;
    VkExtent2D offscreenImageExtent;
    VkImageView offscreenImageView;
    
    const char* TEXTURE_PATH_0 = "./assets/textures/hexagons.png";
    int textureWidth = 1024;
    int textureHeight = 1024;
    int textureChannels = 4;

    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkFormat textureImageFormat;
    VkExtent2D textureImageExtent;
    VkImageView textureImageView;
    VkSampler textureSampler;

    struct UniformBufferObjectScene {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };
    std::vector<VkBuffer> uniformBuffersScene;
    std::vector<VkDeviceMemory> uniformBuffersSceneMemory;

    struct UniformBufferObjectFX {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec2 res;
    };
    std::vector<VkBuffer> uniformBuffersFX;
    std::vector<VkDeviceMemory> uniformBuffersFXMemory;

    std::vector<VkBuffer> uniformBuffersDecal;
    std::vector<VkDeviceMemory> uniformBuffersDecalMemory;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;

    VkDescriptorPool descriptorPool;

    struct {
        //std::vector<VkDescriptorSet> shadows;
        std::vector<VkDescriptorSet> scene;
        std::vector<VkDescriptorSet> composition;
        std::vector<VkDescriptorSet> fx0;
        std::vector<VkDescriptorSet> fx1;
        std::vector<VkDescriptorSet> decal;
    } descriptorSets;

    struct {
        //VkDescriptorSetLayout shadows;
        VkDescriptorSetLayout scene;
        VkDescriptorSetLayout composition;
        VkDescriptorSetLayout fx0;
        VkDescriptorSetLayout fx1;
        VkDescriptorSetLayout decal;
    } descriptorSetLayouts;

    struct {
        VkPipelineLayout scene;
        VkPipelineLayout composition;
        VkPipelineLayout fx;
        VkPipelineLayout decal;
    } pipelineLayouts;

    struct {
        VkPipeline scene;
        VkPipeline composition;
        VkPipeline fx;
        VkPipeline decal;
    } pipelines;

    const std::string SHADER_VERT_PATH_0 = "./assets/shaders/vert0.spv";
    const std::string SHADER_FRAG_PATH_0 = "./assets/shaders/frag0.spv";
    const std::string SHADER_VERT_PATH_1 = "./assets/shaders/vertFX.spv";
    const std::string SHADER_FRAG_PATH_1 = "./assets/shaders/fragFX.spv";
    const std::string SHADER_VERT_PATH_2 = "./assets/shaders/vertDecal.spv";
    const std::string SHADER_FRAG_PATH_2 = "./assets/shaders/fragDecal.spv";
    const std::string SHADER_VERT_PATH_3 = "./assets/shaders/vertScreen.spv";
    const std::string SHADER_FRAG_PATH_3 = "./assets/shaders/fragScreen.spv";
    const std::string MODEL_PATH_0 = "./assets/models/scene.obj";
    const std::string MODEL_PATH_1 = "./assets/models/sphere_smooth.obj";
    const std::string MODEL_PATH_2 = "./assets/models/cube.obj";

    VkCommandPool graphicsCommandPool; // command pool is tied to specific queue.
    VkCommandPool transferCommandPool;
    std::vector<VkCommandBuffer> graphicsCommandBuffer; // command buffers allocate memory from specific command pool
    VkCommandBuffer transferCommandBuffer;

    const int MAX_FRAMES_IN_FLIGHT = 2;
    std::vector<VkSemaphore> imageAvailableSemaphore;
    std::vector<VkSemaphore> renderingFinishedSemaphore;
    std::vector<VkSemaphore> transferFinishedSemaphore;
    std::vector<VkFence> cbuffersExecutionFence;
    std::vector<VkFence> swapchainImageFence;
    size_t frameID;

    const std::vector<const char*> deviceExtensions = {
        "VK_KHR_swapchain"
    };

    static std::vector<char> readFile(const std::string& filename);

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 uv;
        glm::vec3 normal;

        static VkVertexInputBindingDescription getBindingDescription();

        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
    };

    std::vector<Vertex> verticesScene;
    std::vector<uint32_t> indicesScene;

    std::vector<Vertex> verticesFX;
    std::vector<uint32_t> indicesFX;

    std::vector<Vertex> verticesDecal;
    std::vector<uint32_t> indicesDecal;

    const std::vector<Vertex> verticesScreenQuad = {
        // pos, color, uv, normal
        {{-1.00f, 1.00f, 0.00f}, {0.00f, 0.00f, 0.00f}, {0.00f, 0.00f}, {0.00f, 0.00f, -1.00f}},
        {{-1.00f,-1.00f, 0.00f}, {0.00f, 0.00f, 0.00f}, {0.00f, 1.00f}, {0.00f, 0.00f, -1.00f}},
        {{ 1.00f, 1.00f, 0.00f}, {0.00f, 0.00f, 0.00f}, {1.00f, 0.00f}, {0.00f, 0.00f, -1.00f}},
        {{ 1.00f, 1.00f, 0.00f}, {0.00f, 0.00f, 0.00f}, {1.00f, 0.00f}, {0.00f, 0.00f, -1.00f}},
        {{-1.00f,-1.00f, 0.00f}, {0.00f, 0.00f, 0.00f}, {0.00f, 1.00f}, {0.00f, 0.00f, -1.00f}},
        {{ 1.00f,-1.00f, 0.00f}, {0.00f, 0.00f, 0.00f}, {1.00f, 1.00f}, {0.00f, 0.00f, -1.00f}}
    };

    VkBuffer vertexBuffer0;
    VkDeviceMemory vertexBufferMemory0;
    VkBuffer indicesBuffer0;
    VkDeviceMemory indicesBufferMemory0;

    VkBuffer vertexBuffer1;
    VkDeviceMemory vertexBufferMemory1;
    VkBuffer indicesBuffer1;
    VkDeviceMemory indicesBufferMemory1;    
    
    VkBuffer vertexBuffer2;
    VkDeviceMemory vertexBufferMemory2;
    VkBuffer indicesBuffer2;
    VkDeviceMemory indicesBufferMemory2;

    VkBuffer vertexBuffer3;
    VkDeviceMemory vertexBufferMemory3;

    void initWindow();
	
    void checkExtensionLayersSupport();

    std::vector<const char*> getRequiredInstanceExtensions();

    void createInstance();

    void loadExtensionFunctions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    void setupDebugMessenger();

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamilyIndex; 
        std::optional<uint32_t> presentFamilyIndex;
        std::optional<uint32_t> computeFamilyIndex;
        std::optional<uint32_t> transferFamilyIndex;

        bool isComplete();
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    int rateDeviceSuitability(VkPhysicalDevice device);

    void pickPhysicalDevice();

    void createLogicalDeviceAndQueues();

    void createSurface();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(VkPhysicalDevice device, const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    void createSwapChain();

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memProperties, VkImage& image, VkDeviceMemory& imageMemory, bool generalLayout);
       
    void createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView);

    void createImageResources();

    void createTextureImageResources();
	
    VkFormat findSupportedImageFormat(const std::vector<VkFormat> candidateFormats, VkImageTiling requiredTiling, VkFormatFeatureFlags requiredFeaturesBitflags);

    VkFormat findSupportedDepthFormat();

    bool formatSupportsStencil(VkFormat format);

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    void transitionImageLayoutSync2(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    void createDepthResources();

    void createDescriptorSetLayouts();

    void createDescriptorPool();
	
    void createDescriptorSets();

    uint32_t findMemoryTypeIndex(uint32_t bufferSupportedMemTypes_Bitflags, VkMemoryPropertyFlags requiredMemProperties);

    void createGraphicsBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage_Bitflags, VkMemoryPropertyFlags memProperties_Bitflags, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void createUniformBuffers();

    void createTextureSampler();

    void createGraphicsPipelineScene();
  
    void createGraphicsPipelineFX();

    void createGraphicsPipelineDecal();

    void createGraphicsPipelineComposition();

    VkShaderModule createShaderModule(const std::vector<char>& code);

    void createRenderPass();
    
    void createFramebuffers();

    void createGraphicsCommandPool();
        
    void createTransferCommandPool();

    void recordCommandBuffers();
       
    void createSyncObjects();      

    void createTransferCommandBuffer();

    void submitTransferCommandBuffer();

    VkCommandBuffer createSingleUseCommandBuffer();

    void submitSingleUseCommandBuffer(VkCommandBuffer commandBuffer);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void copyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height);

    void createVertexBuffers();

    void createIndexBuffers();

    void loadObjs();
        
    void initVulkan();

    void updateUniformBuffers(uint32_t currentImage);

    void checkFenceStatus();

    void drawFrame();

    void mainLoop();

    void cleanup();


public:
	void run();
    VulkanApp();

};
#endif	