#pragma once

#include <vulkan/vulkan.h>

class VulkanContext{
    public:
        VkAllocationCallbacks *allocator=nullptr;
        VkInstance instance;
        VkPhysicalDevice physical_device;
        VkDevice device;

        VulkanContext(
            VkAllocationCallbacks *vk_allocator,
            VkInstance vk_instance,
            VkPhysicalDevice vk_physical_device,
            VkDevice vk_device
        ):allocator{vk_allocator},instance{vk_instance},physical_device{vk_physical_device},device{vk_device}{
            
        }

        ~VulkanContext(){
            // context without device is used as temporary container for instance and allocator
            if(device!=VK_NULL_HANDLE){
                deviceWaitIdle();

                vkDestroyDevice(
                    device,
                    allocator
                );
            
                vkDestroyInstance(
                    instance,
                    allocator
                );
            }
        }

        void deviceWaitIdle()const{
            vkDeviceWaitIdle(device);
        }
};

class Semaphore{
    VkDevice device;
    VkAllocationCallbacks *allocator;
    public:
        VkSemaphore handle;
        Semaphore(
            VkDevice device,
            VkAllocationCallbacks *allocator,
            VkSemaphore handle
        ){
            this->device=device;
            this->allocator=allocator;
            this->handle=handle;
        }
        ~Semaphore(){
            vkDestroySemaphore(device,handle,allocator);
        }
};
