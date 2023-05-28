#pragma once

#include<optional>
#include<string>

#include<vulkan/vulkan.h>

enum class VulkanErrorContext{
    InstanceCreation,
    DeviceCreation,
    NoViablePhysicalDeviceFound,
    CreateSwapchain,
    CreateRenderPass,
    CreatePipelineLayout,
    CreateGraphicsPipeline,
    CreateShaderModule,
    CreateDescriptorPool,
    CreateDescriptorSetLayout,
    AllocateDescriptorSets,
    CreateBuffer,
    CreateBufferView,
    AllocateMemory,
    BindBufferMemory,
    CreateXCBSurface,
    SwapchainAcquireNextImage,
};
class VulkanError{
    private:
        VulkanErrorContext context;
        std::optional<VkResult> vk_res;

    public:
        VulkanError(
            VulkanErrorContext context,
            VkResult vk_res
        ):context(context),vk_res(vk_res){}

        VulkanError(
            VulkanErrorContext context
        ):context(context){}

        std::string info()const;
};
