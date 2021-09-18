/**
 * 
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

//验证层名称
const std::vector<const char *> validateLayers = {
    "VK_LAYER_KHRONOS_validation"
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

    void initWindow(){
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, appName.c_str(), nullptr, nullptr);
    }

    void initVulkan(){
        createInstance();
        setupDebugMessenger();
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

    void cleanup(){
        if(enableValidateLayers){
            destoryDebugUtilsMessengerEXT(instance ,debugMessenger , nullptr);
        }

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
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity 
        ,VkDebugUtilsMessageTypeFlagsEXT messageType
        ,const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData 
        ,void *pUserData){
        std::cerr << "validation layer : " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }
};

int main(int argc , char *argv[]){
    //std::cout << "Hello 你好 Vulkan" << std::endl;
    HelloTriangleApplication app;

    try{
        app.run();
    }catch(const std::exception &e){
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

