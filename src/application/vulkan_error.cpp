#include "vulkan/vulkan_core.h"
#include <application/vulkan_error.h>

std::string VulkanError::info()const{
    std::string res;
    std::string context_string;
    #define VK_ERROR_CONTEXT_CASE(error_context) \
        case VulkanErrorContext::error_context: \
            context_string=#error_context; \
            break;

    switch(context){
        VK_ERROR_CONTEXT_CASE(InstanceCreation)
        VK_ERROR_CONTEXT_CASE(DeviceCreation)
        VK_ERROR_CONTEXT_CASE(NoViablePhysicalDeviceFound)
        VK_ERROR_CONTEXT_CASE(CreateSwapchain)
        VK_ERROR_CONTEXT_CASE(CreateRenderPass)
        VK_ERROR_CONTEXT_CASE(CreatePipelineLayout)
        VK_ERROR_CONTEXT_CASE(CreateGraphicsPipeline)
        VK_ERROR_CONTEXT_CASE(CreateShaderModule)
        VK_ERROR_CONTEXT_CASE(CreateDescriptorPool)
        VK_ERROR_CONTEXT_CASE(CreateDescriptorSetLayout)
        VK_ERROR_CONTEXT_CASE(AllocateDescriptorSets)
        VK_ERROR_CONTEXT_CASE(CreateBuffer)
        VK_ERROR_CONTEXT_CASE(CreateBufferView)
        VK_ERROR_CONTEXT_CASE(AllocateMemory)
        VK_ERROR_CONTEXT_CASE(BindBufferMemory)
        VK_ERROR_CONTEXT_CASE(CreateXCBSurface)
        VK_ERROR_CONTEXT_CASE(SwapchainAcquireNextImage)
        VK_ERROR_CONTEXT_CASE(CreateCommandPool)
        VK_ERROR_CONTEXT_CASE(AllocateCommandBuffers)
        VK_ERROR_CONTEXT_CASE(CreateGraphicsPipelines)
    }
    res+=context_string;
    res+=" failed";
    std::string vk_res_string=" with ";

    if(this->vk_res.has_value()){
        #define VK_ERROR_CASE(errorCode) \
            case errorCode: \
                vk_res_string += #errorCode; \
                break;

        switch(this->vk_res.value()){
            VK_ERROR_CASE(VK_SUCCESS)

            VK_ERROR_CASE(VK_ERROR_EXTENSION_NOT_PRESENT)
            VK_ERROR_CASE(VK_ERROR_LAYER_NOT_PRESENT)
            VK_ERROR_CASE(VK_ERROR_FEATURE_NOT_PRESENT)
            VK_ERROR_CASE(VK_ERROR_INCOMPATIBLE_DRIVER)

            VK_ERROR_CASE(VK_ERROR_DEVICE_LOST)

            VK_ERROR_CASE(VK_ERROR_OUT_OF_DATE_KHR)
            VK_ERROR_CASE(VK_ERROR_FRAGMENTATION)
            VK_ERROR_CASE(VK_ERROR_FRAGMENTED_POOL)
            
            VK_ERROR_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY)
            VK_ERROR_CASE(VK_ERROR_OUT_OF_HOST_MEMORY)
            VK_ERROR_CASE(VK_ERROR_OUT_OF_POOL_MEMORY)

            VK_ERROR_CASE(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
            VK_ERROR_CASE(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT)
            VK_ERROR_CASE(VK_ERROR_INVALID_EXTERNAL_HANDLE)

            VK_ERROR_CASE(VK_ERROR_MEMORY_MAP_FAILED)
            VK_ERROR_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED)
            VK_ERROR_CASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
            VK_ERROR_CASE(VK_ERROR_INITIALIZATION_FAILED)
            VK_ERROR_CASE(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
            VK_ERROR_CASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)

            default:
                vk_res_string="unknown";
        }
    }
    res+=vk_res_string;
    return res;
}

void VulkanError::check(
    VulkanErrorContext context,
    VkResult vk_res
){
    if (vk_res!=VK_SUCCESS) {
        throw VulkanError(context,vk_res);
    }    
}

