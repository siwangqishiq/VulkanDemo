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

const int MAX_FRAMES_IN_FLIGHT = 2;//可同时并行处理的帧数

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

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;//物理设备
    VkDevice device = VK_NULL_HANDLE;//逻辑设备

    VkQueue graphicsQueue;//图形队列
    VkQueue presentQueue;//显示队列

    VkSurfaceKHR surface; //窗口表面

    VkSwapchainKHR swapChain;//交换链
    std::vector<VkImage> swapChainImages;//交换链上的图像
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;//交换链图像分辨率

    std::vector<VkImageView> swapChainImageViews;//交换链imageView

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;//图形管线

    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkCommandPool cmdPool;//指令池
    std::vector<VkCommandBuffer> cmdBuffers;//指令缓存

    //同步信号量
    // VkSemaphore imageAvailableSemaphore;
    // VkSemaphore renderFinishedSemaphore;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    int currentFrame = 0;//指示当前渲染的是哪一帧

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
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    //创建信号量
    void createSyncObjects(){
        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for(int i = 0 ; i < MAX_FRAMES_IN_FLIGHT ;i++){
            if(vkCreateSemaphore(device , &createInfo , nullptr , &imageAvailableSemaphores[i]) != VK_SUCCESS
                || vkCreateSemaphore(device , &createInfo , nullptr , &renderFinishedSemaphores[i]) != VK_SUCCESS
                || vkCreateFence(device , &fenceCreateInfo , nullptr , &inFlightFences[i]) != VK_SUCCESS){
                throw std::runtime_error("failed create semaphore");
            }
        }//end for i

        std::cout << "create semaphores success " << std::endl;
    }

    //创建指令缓存
    void createCommandBuffers(){
        cmdBuffers.resize(swapChainFramebuffers.size());

        //创建与帧缓存个数相等的 指令缓冲区
        VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
        cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufAllocateInfo.commandPool = cmdPool;
        cmdBufAllocateInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
        cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        // cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;

        if(vkAllocateCommandBuffers(device , &cmdBufAllocateInfo , cmdBuffers.data()) != VK_SUCCESS){
            throw std::runtime_error("failed create command buffers");
        }

        std::cout << "create command buffers success." << std::endl;

        //start record command buffer
        for(int i = 0 ; i < cmdBuffers.size() ;i++){
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            if(vkBeginCommandBuffer(cmdBuffers[i] , &beginInfo) != VK_SUCCESS){
                throw std::runtime_error("failed to begin command buffer");
            }

            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[i];

            renderPassInfo.renderArea.offset = {0 , 0};
            renderPassInfo.renderArea.extent = swapChainExtent;

            VkClearValue clearColor = {1.0f , 1.0f, 1.0 , 1.0f};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(cmdBuffers[i] , &renderPassInfo , VK_SUBPASS_CONTENTS_INLINE);

            //bind graphic pipeline
            vkCmdBindPipeline(cmdBuffers[i] , VK_PIPELINE_BIND_POINT_GRAPHICS ,graphicsPipeline);
            vkCmdDraw(cmdBuffers[i] , 3 , 1 , 0 , 0);

            vkCmdEndRenderPass(cmdBuffers[i]);

            if(vkEndCommandBuffer(cmdBuffers[i]) != VK_SUCCESS){
                throw std::runtime_error("failed to recoder render pass !");
            }
        }//end for i

    }

    //创建指令池  池的目的是为了以后分配指令
    void createCommandPool(){
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
        cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsIndex;
        cmdPoolCreateInfo.flags = 0;

        if(vkCreateCommandPool(device , &cmdPoolCreateInfo , nullptr , &cmdPool) != VK_SUCCESS){
            throw std::runtime_error("failed create command pool !");
        }

        std::cout << "create command pool success" << std::endl;
    }

    //创建与swapchain 关联的framebuffer
    void createFramebuffers(){
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for(int i = 0; i < swapChainFramebuffers.size(); i++){
            const VkImageView attachments[] = {swapChainImageViews[i]};

            VkFramebufferCreateInfo framebufferCreateInfo = {};
            framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCreateInfo.renderPass = renderPass;
            framebufferCreateInfo.attachmentCount = 1;
            framebufferCreateInfo.pAttachments = attachments;
            framebufferCreateInfo.width = swapChainExtent.width;
            framebufferCreateInfo.height = swapChainExtent.height;
            framebufferCreateInfo.layers = 1;

            if(vkCreateFramebuffer(device , &framebufferCreateInfo , nullptr , 
                &swapChainFramebuffers[i]) != VK_SUCCESS){
                throw std::runtime_error("failed create framebuffer!");
            }
        }//end for i

        std::cout << "create frame buffer success count = " <<swapChainFramebuffers.size() << std::endl;
    }
    
    //创建渲染帧缓冲附着对象
    void createRenderPass(){
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = 1;
        renderPassCreateInfo.pAttachments = &colorAttachment;
        renderPassCreateInfo.subpassCount = 1;
        renderPassCreateInfo.pSubpasses = &subpass;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        renderPassCreateInfo.dependencyCount = 1;
        renderPassCreateInfo.pDependencies = &dependency;

        if(vkCreateRenderPass(device , &renderPassCreateInfo , nullptr , &renderPass) != VK_SUCCESS){
            throw std::runtime_error("failed to create render pass");
        }

        std::cout << "create render pass success." << std::endl;
    }

    //创建图形管线
    void createGraphicsPipeline(){
        auto vertShaderCode = readFile("shaders/vert.spv");
        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);

        auto fragShaderCode = readFile("shaders/frag.spv");
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertCreateInfo = {};
        vertCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertCreateInfo.module = vertShaderModule;
        vertCreateInfo.pName = "main";
        vertCreateInfo.pSpecializationInfo = nullptr;//importent 重要优化点

        VkPipelineShaderStageCreateInfo fragCreateInfo = {};
        fragCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragCreateInfo.module = fragShaderModule;
        fragCreateInfo.pName = "main";
        fragCreateInfo.pSpecializationInfo = nullptr;//importent 重要优化点

        //着色器阶段
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertCreateInfo , fragCreateInfo};

        //pipline fixed function
        //vertex input
        VkPipelineVertexInputStateCreateInfo vertexStateCreateInfo = {};
        vertexStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexStateCreateInfo.vertexAttributeDescriptionCount = 0;
        vertexStateCreateInfo.pVertexAttributeDescriptions = nullptr;
        vertexStateCreateInfo.vertexBindingDescriptionCount = 0;
        vertexStateCreateInfo.pVertexBindingDescriptions = nullptr;

        //Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
        inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
        inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        //Viewports and scissors
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        // viewport.height = 0.0f;
        // viewport.height = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0 , 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
        viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportCreateInfo.scissorCount = 1;
        viewportCreateInfo.pScissors = &scissor;
        viewportCreateInfo.viewportCount = 1;
        viewportCreateInfo.pViewports = &viewport;

        //Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {};
        rasterizationCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationCreateInfo.lineWidth = 1.0f;
        rasterizationCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizationCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizationCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationCreateInfo.depthBiasClamp = 0.0f;
        rasterizationCreateInfo.depthBiasSlopeFactor = 0.0f;

        //Multisampling
        VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
        multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
        multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisamplingCreateInfo.minSampleShading = 1.0f;
        multisamplingCreateInfo.pSampleMask = nullptr;
        multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

        //Depth and stencil testing
        //todo

        //Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttach = {};
        colorBlendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT 
                                        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttach.blendEnable = VK_FALSE;
        colorBlendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttach.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttach.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo blendCreateInfo = {};
        blendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blendCreateInfo.logicOpEnable = VK_FALSE;
        blendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        blendCreateInfo.attachmentCount = 1;
        blendCreateInfo.pAttachments = &colorBlendAttach;
        blendCreateInfo.blendConstants[0] = 0.0f;
        blendCreateInfo.blendConstants[1] = 0.0f;
        blendCreateInfo.blendConstants[2] = 0.0f;
        blendCreateInfo.blendConstants[3] = 0.0f;

        //Dynamic state
        VkDynamicState dynamicState[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        // dynamicStateCreateInfo.dynamicStateCount = 2;
        // dynamicStateCreateInfo.pDynamicStates = dynamicState;
        dynamicStateCreateInfo.dynamicStateCount = 0;
        dynamicStateCreateInfo.pDynamicStates = nullptr;

        //Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = 0;
        pipelineLayoutCreateInfo.pSetLayouts = nullptr;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        if(vkCreatePipelineLayout(device , &pipelineLayoutCreateInfo , nullptr , &pipelineLayout) != VK_SUCCESS){
            throw std::runtime_error("create pipeline layout failed.");
        }

        //create graphics pipeline
        VkGraphicsPipelineCreateInfo graphicPipelineCreateInfo = {};
        graphicPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphicPipelineCreateInfo.stageCount = 2;
        graphicPipelineCreateInfo.pStages = shaderStages;

        graphicPipelineCreateInfo.pVertexInputState = &vertexStateCreateInfo;
        graphicPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
        graphicPipelineCreateInfo.pViewportState = &viewportCreateInfo;
        graphicPipelineCreateInfo.pRasterizationState = &rasterizationCreateInfo;
        graphicPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
        graphicPipelineCreateInfo.pDepthStencilState = nullptr;
        graphicPipelineCreateInfo.pColorBlendState = &blendCreateInfo;
        graphicPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

        graphicPipelineCreateInfo.layout = pipelineLayout;

        graphicPipelineCreateInfo.renderPass = renderPass;
        graphicPipelineCreateInfo.subpass = 0;

        graphicPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        graphicPipelineCreateInfo.basePipelineIndex = -1;

        if(vkCreateGraphicsPipelines(device , VK_NULL_HANDLE , 1 , 
            &graphicPipelineCreateInfo, nullptr , &graphicsPipeline) != VK_SUCCESS){
            throw std::runtime_error("failed to create graphic pipeline.");
        }
        std::cout << "create graphics pipeline success." << std::endl;

        //destory shader modules
        vkDestroyShaderModule(device , vertShaderModule ,nullptr);
        vkDestroyShaderModule(device , fragShaderModule ,nullptr);
    }

    //从spir-v文件 构造出shaderModule
    VkShaderModule createShaderModule(std::vector<char> &code){
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        createInfo.pNext = nullptr;

        VkShaderModule shaderModule;

        if(vkCreateShaderModule(device , &createInfo , nullptr , &shaderModule) != VK_SUCCESS){
            throw std::runtime_error("create shader module error.");
        }
        return shaderModule;
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
            drawFrame();
        }//end while

        vkDeviceWaitIdle(device);
    }

    //渲染一帧图像
    void drawFrame(){
        //wait fence
        vkWaitForFences(device , 1 , &inFlightFences[currentFrame] , VK_TRUE , INT32_MAX);

        vkResetFences(device , 1 , &inFlightFences[currentFrame]);

        uint32_t imageIndex;

        vkAcquireNextImageKHR(device , swapChain , UINT64_MAX , 
            imageAvailableSemaphores[currentFrame] , VK_NULL_HANDLE , &imageIndex);

        //std::cout << "imageIndex = " << imageIndex << std::endl;

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if(vkQueueSubmit(graphicsQueue , 1 , &submitInfo , inFlightFences[currentFrame]) != VK_SUCCESS){
            throw std::runtime_error("fail to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        presentInfo.pResults = nullptr;

        vkQueuePresentKHR(presentQueue , &presentInfo);

        //效率较低 会使GPU长期处于闲置状态
        //vkQueueWaitIdle(presentQueue);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        // std::cout << "currentFrame = " << currentFrame << std::endl;
    }

    //清理资源
    void cleanup(){
        for(int i = 0 ; i < MAX_FRAMES_IN_FLIGHT  ;i++){
            vkDestroySemaphore(device , imageAvailableSemaphores[i] , nullptr);
            vkDestroySemaphore(device , renderFinishedSemaphores[i] , nullptr);

            vkDestroyFence(device , inFlightFences[i] , nullptr);
        }

        vkDestroyCommandPool(device , cmdPool , nullptr);

        for(VkFramebuffer &framebuffer : swapChainFramebuffers){
            vkDestroyFramebuffer(device ,framebuffer , nullptr);
        }//end for each

        vkDestroyPipeline(device , graphicsPipeline , nullptr);
        vkDestroyPipelineLayout(device , pipelineLayout , nullptr);
        vkDestroyRenderPass(device , renderPass , nullptr);

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
        if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT && enableValidateLayers){
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

