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

/**
 * main
 * */
const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 800;

class HelloTriangleApplication{
public: 
    int run(){
        initWindow();

        initVulkan();
        mainloop();
        cleanup();
        return 0;
    }

private:
    GLFWwindow *window = nullptr;

    void initWindow(){
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "你好Vulkan", nullptr, nullptr);
    }

    void initVulkan(){

    }

    void mainloop(){
        while(!glfwWindowShouldClose(window)){
            glfwPollEvents();
        }//end while
    }

    void cleanup(){
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

