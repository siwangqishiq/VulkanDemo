/**
 * panyi
 * main.cpp
 * */
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include <string>
#include <vector>
#include <set>

#include "utils.hpp"

#define DEBUG

#ifdef DEBUG
const bool enableValidateLayers = true;
#else
const bool enableValidateLayers = false;
#endif

/**
 * main
 * */
const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 800;

//队列簇
struct QueueFamilyIndices{
    int graphicsIndex = -1;//图形队列
    int presentIndex = -1;//显示队列

    bool isComplete(){
        return graphicsIndex >= 0 && presentIndex >= 0;
    }
};

//交换链支持详情
struct SwapChainSupportDetail{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

//验证层名称
const std::vector<const char *> validateLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class HelloTriangleApplication{
public: 
    std::string appName = "Hello Vulkan";

    int run(){
        initWindow();
        initVulkan();

        mainloop();
        cleanup();
        return 0;
    }

private:
    GLFWwindow *window = nullptr;
    VkInstance instance;

    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    VkDevice device = VK_NULL_HANDLE;//逻辑设备

    VkQueue graphicsQueue;//图形队列
    VkQueue presentQueue;//显示队列

    VkSurfaceKHR surface; //窗口表面

    VkSwapchainKHR swapChain;//交换链
    std::vector<VkImage> swapChainImages;//交换链上的图像
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;//交换链图像分辨率

    std::vector<VkImageView> swapChainImageViews;//交换链imageView

    void initWindow(){
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, appName.c_str(), nullptr, nullptr);
    }

    void initVulkan(){
        createInstance();
        setupDebugMessenger();

        createSurface();

        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
    }

    //创建与image关联的imageview
    void createImageViews(){
        swapChainImageViews.resize(swapChainImages.size());

        for(int i = 0 ; i < swapChainImages.size(); i++){
            VkImageViewCreateInfo imageViewCreateInfo = {};
            imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCreateInfo.pNext = nullptr;
            imageViewCreateInfo.image = swapChainImages[i];
            imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCreateInfo.format = swapChainImageFormat;

            imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
            imageViewCreateInfo.subresourceRange.levelCount = 1;
            imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
            imageViewCreateInfo.subresourceRange.layerCount = 1;    

            if(vkCreateImageView(device , &imageViewCreateInfo , nullptr , &swapChainImageViews[i]) != VK_SUCCESS){
               throw std::runtime_error("failed to create imageview.");
            }
        }//end for i

        std::cout << "create image view " << swapChainImageViews.size() << " success" <<std::endl; 
    }

    //创建交换链 用于展示图像
    void createSwapChain(){
        SwapChainSupportDetail details = querySwapChainSupport(physicalDevice);

        //select 1. surface format  2. presentMode  3. set resolution 
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(details.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);
        VkExtent2D extent = chooseSwapExtent(details.capabilities);

        // std::cout << "Min Image Count " << details.capabilities.minImageCount << std::endl;
        // std::cout << "Max Image Count " << details.capabilities.maxImageCount << std::endl;

        //设置交换链 image 个数  需要介于 minImageCount ~ maxImageCount之间
        uint32_t imageCount = details.capabilities.minImageCount + 1;
        if(details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount){
            imageCount = details.capabilities.maxImageCount;
        }
        
        //create Swap chain
        VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
        swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapChainCreateInfo.pNext = nullptr;
        swapChainCreateInfo.surface = surface;

        swapChainCreateInfo.minImageCount = imageCount;
        swapChainCreateInfo.imageFormat = surfaceFormat.format;
        swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapChainCreateInfo.imageExtent = extent;
        swapChainCreateInfo.presentMode = presentMode;
        swapChainCreateInfo.imageArrayLayers = 1;
        swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
        if(queueFamilyIndices.graphicsIndex == queueFamilyIndices.presentIndex){//图形队列簇与呈现队列簇是同一个
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapChainCreateInfo.queueFamilyIndexCount = 0;
            swapChainCreateInfo.pQueueFamilyIndices = nullptr;
        }else{//图形 与 呈现队列簇不是同一个
            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapChainCreateInfo.queueFamilyIndexCount = 2;
            uint32_t queueFamilyIndicesArray[] = {static_cast<uint32_t>(queueFamilyIndices.graphicsIndex) , 
                    static_cast<uint32_t>(queueFamilyIndices.presentIndex)};
            swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndicesArray;
        }

        swapChainCreateInfo.preTransform = details.capabilities.currentTransform;
        swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        swapChainCreateInfo.clipped = VK_TRUE;
        swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        //create swap chain
        if(vkCreateSwapchainKHR(device , &swapChainCreateInfo , nullptr , &swapChain) != VK_SUCCESS){
            throw std::runtime_error("Failed to create swap chain!");
        }

        std::cout << "create swap chain success" << std::endl;

        swapChainImageFormat = swapChainCreateInfo.imageFormat;
        swapChainExtent = swapChainCreateInfo.imageExtent;

        //创建swapchain关联image
        uint32_t swapChainImageCount = 0;
        vkGetSwapchainImagesKHR(device ,swapChain , &swapChainImageCount ,nullptr);
        //std::cout << "swap chain image count : " << imageCount << std::endl;

        if(imageCount > 0){
            swapChainImages.resize(swapChainImageCount);
            vkGetSwapchainImagesKHR(device , swapChain, &swapChainImageCount , swapChainImages.data());
        }
    }

    //选择合适的格式
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR> &formatList){
        for(const auto &availableFormat : formatList){
            if(availableFormat.format == VK_FORMAT_B8G8R8_SRGB && 
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
                return availableFormat;
            }
        }//end for each

        return formatList[0];
    }

    //选择合适的展示方式
    VkPresentModeKHR chooseSwapPresentMode(std::vector<VkPresentModeKHR> &presentModeList){
        for(const auto &presentMode : presentModeList){
            if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR){
                return presentMode;
            }
        }//end for each
        
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    //选择合适的宽高 分辨率
    VkExtent2D chooseSwapExtent(VkSurfaceCapabilitiesKHR &capabilities){
        VkExtent2D resolution;

        uint32_t curWidth = capabilities.currentExtent.width;
        uint32_t curHeight = capabilities.currentExtent.height;

        // std::cout << "current extent = " << curWidth << " x " << curHeight << std::endl;

        // std::cout << "maxImageExtent = " << capabilities.maxImageExtent.width 
        //         << " x " << capabilities.maxImageExtent.height << std::endl;

        // std::cout << "minImageExtent = " << capabilities.minImageExtent.width 
        //         << " x " << capabilities.minImageExtent.height << std::endl;

        resolution.width = curWidth;
        resolution.height = curHeight;
        return resolution;
    }

    //查询交换链 属性
    SwapChainSupportDetail querySwapChainSupport(VkPhysicalDevice phDevice) {
        SwapChainSupportDetail details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phDevice , surface , &details.capabilities);
        // std::cout << "maxImageCount " << details.capabilities.maxImageCount << std::endl;  
        // std::cout << "minImageCount " << details.capabilities.minImageCount << std::endl; 

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(phDevice , surface , &formatCount , nullptr);
        details.formats.resize(formatCount); 
        vkGetPhysicalDeviceSurfaceFormatsKHR(phDevice , surface , &formatCount , details.formats.data());
        // std::cout << "format 格式 : " << formatCount << std::endl;
        // for(VkSurfaceFormatKHR format : details.formats){
        //     std::cout << "\t colorSpace " << format.colorSpace << " format " << format.format << std::endl;
        // }//end for

        uint32_t presentModesCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(phDevice , surface , 
                            &presentModesCount , nullptr);
        details.presentModes.resize(presentModesCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(phDevice , surface , 
                            &presentModesCount , details.presentModes.data());
        // std::cout << "presentModes 格式 : " << presentModesCount << std::endl;
        // for(VkPresentModeKHR presentMode : details.presentModes){
        //     std::cout << "\t presentMode " << presentMode << std::endl;
        // }//end for

        return details;
    }

    //创建窗口表面
    void createSurface(){
        if(glfwCreateWindowSurface(instance , window , nullptr , &surface) != VK_SUCCESS){
            throw std::runtime_error("failed to create window surface!");
        }
    }

    //create vulkan instance
    void createInstance(){
        std::cout << "create vulkan instance " << std::endl;
        //std::cout << "checkValidationLayerSupport " << checkValidationLayerSupport() << std::endl;
        if(enableValidateLayers && !checkValidationLayerSupport()){
            throw std::runtime_error("validate layer request but not available!");
        }

        getRequiredExtensions();

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "HelloTriangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
        appInfo.pEngineName = "NoEngine";
        appInfo.apiVersion = VK_API_VERSION_1_0;
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0 , 0);

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        //注入扩展层配置
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        //注入验证层
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        if(enableValidateLayers){
            createInfo.enabledLayerCount = static_cast<uint32_t>(validateLayers.size());
            createInfo.ppEnabledLayerNames = validateLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        }else{
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
        std::cout << "create instance success" << std::endl;
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &debugCreateInfo){
        debugCreateInfo = {};

        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
            |VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            |VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 
            |VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            |VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        
        debugCreateInfo.pfnUserCallback = debugCallback;
    }

    std::vector<const char*> getRequiredExtensions(){
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        std::vector<const char*> extensions(glfwExtensions , glfwExtensions + glfwExtensionCount);

        if(enableValidateLayers){
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return extensions;
    }
    

    //检查验证层的支持
    bool checkValidationLayerSupport(){
        uint32_t layerCount = 0;

        vkEnumerateInstanceLayerProperties(&layerCount , nullptr);
        //std::cout << "validate layer count = " << layerCount << std::endl;

        std::vector<VkLayerProperties> availableLayers(layerCount);
        VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        //std::cout << "vkEnumerateInstanceLayerProperties result = " << result << std::endl;
        
        // std::cout << "layer properties:" << std::endl;
        // for(int i = 0 ; i < layerCount ;i++){
        //     std::cout << '\t' << availableLayers[i].layerName << std::endl;
        // }//end for i

        for(const std::string &layer : validateLayers){
            bool layoutFound = false;

            for(VkLayerProperties &vkLayout : availableLayers){
                std::string vkLayerName = vkLayout.layerName;
                //std::cout << "compare = " << layer << " with " << vkLayerName << std::endl;
                if(layer == vkLayerName){
                    layoutFound = true;
                    break;
                }
            }

            if(!layoutFound){
                std::cout << "Not found validate " << layer << std::endl;
                return false;
            }
        }//end for

        return true;
    }

    //game loop
    void mainloop(){
        while(!glfwWindowShouldClose(window)){
            glfwPollEvents();
        }//end while
    }

    //清理资源
    void cleanup(){
        for(VkImageView &imageView : swapChainImageViews){
            vkDestroyImageView(device , imageView , nullptr);
        }//end for each
        vkDestroySwapchainKHR(device , swapChain , nullptr);

        vkDestroyDevice(device , nullptr);
        if(enableValidateLayers){
            destoryDebugUtilsMessengerEXT(instance ,debugMessenger , nullptr);
        }

        vkDestroySurfaceKHR(instance , surface ,nullptr);
        vkDestroyInstance(instance , nullptr);
        
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void setupDebugMessenger(){
        if(!enableValidateLayers){
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        populateDebugMessengerCreateInfo(createInfo);

        if(createDebugUtilsMessengerEXT(instance , &createInfo , nullptr , &debugMessenger) != VK_SUCCESS){
            throw std::runtime_error("failed to set up debug callback");
        }
    }

    VkResult createDebugUtilsMessengerEXT(VkInstance instance , const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
            const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pCallback){
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance , "vkCreateDebugUtilsMessengerEXT");
        if(func != nullptr){
            return func(instance , pCreateInfo , pAllocator , pCallback);
        }else{
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void destoryDebugUtilsMessengerEXT(VkInstance instance , VkDebugUtilsMessengerEXT callback , 
            const VkAllocationCallbacks *pAllocator){
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance , "vkDestroyDebugUtilsMessengerEXT");
        if(func != nullptr){
            return func(instance ,callback , pAllocator);
        }        
    }

    //debug 回调 输入日志
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity ,VkDebugUtilsMessageTypeFlagsEXT messageType
        ,const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData 
        ,void *pUserData){
        if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT){
                std::cerr << "validation layer : " << pCallbackData->pMessage << std::endl;
        }
        return VK_FALSE;
    }

    //选择物理设备GPU
    void pickPhysicalDevice(){
        uint32_t gpuCount = 0;
        vkEnumeratePhysicalDevices(instance , &gpuCount , nullptr);
        std::cout << "gpu count : " << gpuCount << std::endl;

        std::vector<VkPhysicalDevice> gpus(gpuCount);
        vkEnumeratePhysicalDevices(instance , &gpuCount , gpus.data());

        for(VkPhysicalDevice &device : gpus){
            if(isDeviceSuitable(device)){
                physicalDevice = device;
                break;
            }
        }//end for each

        if(physicalDevice == VK_NULL_HANDLE){
            throw std::runtime_error("no found suitable physical device!");
        }
    }

    //依据设备特性进行选择  
    bool isDeviceSuitable(VkPhysicalDevice phDevice){
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;

        vkGetPhysicalDeviceProperties(phDevice , &properties);
        vkGetPhysicalDeviceFeatures(phDevice , &features);

        // std::cout << "device name : " << properties.deviceName << 
        //     " " << properties.deviceType << 
        //     " deviceID : " << properties.deviceID <<
        //     " driverVersion : " << properties.driverVersion << 
        //     " geometryShader : " << features.geometryShader <<
        //     " tessellationShader : " << features.tessellationShader << std::endl;

        QueueFamilyIndices indices = findQueueFamilies(phDevice);
        bool extensionsSupported = checkDeviceExtensionSupport(phDevice);

        bool swapChainSupportAvailable = false;
        if(extensionsSupported){
            SwapChainSupportDetail swapSupportDetail = querySwapChainSupport(phDevice);
            swapChainSupportAvailable = (!swapSupportDetail.formats.empty()) && (!swapSupportDetail.presentModes.empty());
        }

        return indices.isComplete() && extensionsSupported && swapChainSupportAvailable;
    }

    //检测设备扩展是否支持
    bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice){
        uint32_t extensionCount = 0;

        vkEnumerateDeviceExtensionProperties(physicalDevice , nullptr , &extensionCount , nullptr);
        // std::cout << "设备扩展数量 = " << extensionCount << std::endl;
        
        if(extensionCount == 0)
            return false;
        
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice , nullptr , &extensionCount , availableExtensions.data());

        // std::cout << "设备扩展:" << std::endl;
        // for(VkExtensionProperties prop : availableExtensions){
        //     std::cout << "\t" << prop.extensionName << std::endl;
        // }//end for each

        std::set<std::string> requiredExtensions(deviceExtensions.begin() , deviceExtensions.end());
        for(VkExtensionProperties prop : availableExtensions){
            requiredExtensions.erase(prop.extensionName);
        }//end for each

        return requiredExtensions.empty();
    }

    //从物理设备上查询队列簇
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device){
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device , &queueFamilyCount , nullptr);
        // std::cout << "queueFamilyCount = " << queueFamilyCount << std::endl;

        std::vector<VkQueueFamilyProperties> familyProperies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device , &queueFamilyCount , familyProperies.data());

        uint32_t index = 0;
        for(VkQueueFamilyProperties &prop : familyProperies){
            // std::cout << "VkQueueFamilyProperty : queueCount " 
            //         << prop.queueCount << " flag " << prop.queueFlags << std::endl;
            if(prop.queueFlags & VK_QUEUE_GRAPHICS_BIT){
                indices.graphicsIndex = index;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device , index , surface , &presentSupport);

            if(presentSupport){
                indices.presentIndex = index;
            }

            if(indices.isComplete()){
                break;
            }

            index++;
        }//end for each


        return indices;
    }

    //创建逻辑设备
    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfoList;

        std::set<int> uniqueQueueFamilies = {indices.graphicsIndex , indices.presentIndex};
        for(int queueFamiliesIndex : uniqueQueueFamilies){
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueCount = 1;
            float queueProperties = 1.0f;
            queueCreateInfo.pQueuePriorities = &queueProperties;
            
            queueCreateInfoList.push_back(queueCreateInfo);
        }//end for each

        VkPhysicalDeviceFeatures deviceFeatures = {};
        
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfoList.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfoList.data();

        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

        //set device extension
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        //validate layer
        if(enableValidateLayers){
            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validateLayers.size());
            deviceCreateInfo.ppEnabledLayerNames = validateLayers.data();
        }else{
            deviceCreateInfo.enabledLayerCount = 0;
        }
        

        if(vkCreateDevice(physicalDevice , &deviceCreateInfo , nullptr , &device) != VK_SUCCESS){
            throw std::runtime_error("failed to create logical device !");
        }

        //创建队列  grapics + present queue
        vkGetDeviceQueue(device , indices.graphicsIndex , 0 , &graphicsQueue);
        vkGetDeviceQueue(device , indices.presentIndex , 0 , &presentQueue);
    }
};

int main(int argc , char *argv[]){
    HelloTriangleApplication app;
    try{
        app.run();
    }catch(const std::exception &e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

