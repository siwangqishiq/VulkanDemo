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

/**
 * main
 * */
const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 800;

class HelloTriangleApplication{
public: 
    std::string appName = "Hi! Vulkan";

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

    void initWindow(){
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, appName.c_str(), nullptr, nullptr);
    }

    void initVulkan(){
        createInstance();
    }

    void createInstance(){
        std::cout << "create vulkan instance " << std::endl;

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

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        std::cout << "glfw extension :" << std::endl;
        for(int i = 0 ; i < glfwExtensionCount ; i++){
            std::cout << '\t' << glfwExtensions[i] << std::endl;
        }//end for i

        createInfo.enabledLayerCount = 0;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr , &extensionCount , nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr , &extensionCount , extensions.data());
        std::cout << "available extensions:\n";
        for(const auto &ex : extensions){
            std::cout << '\t' << ex.extensionName << '\t' << ex.specVersion << std::endl;
        }//end for each
    }

    void mainloop(){
        while(!glfwWindowShouldClose(window)){
            glfwPollEvents();
        }//end while
    }

    void cleanup(){
        vkDestroyInstance(instance , nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
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

