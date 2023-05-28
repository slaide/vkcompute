#include "application/vulkan_error.h"
#include "application/window.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cstring>

#include <application.h>
#include <vulkan/vulkan_core.h>

Application::Application(){
    xcb_connection=xcb_connect(
        nullptr,
        nullptr
    );

    std::cout<<"supported instance layers:"<<std::endl;
    for(auto instance_layer_property:Application::enumerateInstanceLayerProperties()){
        std::cout<<"  "<<instance_layer_property.layerName<<std::endl;

        auto layerExtensions=Application::enumerateInstanceExtensionProperties(instance_layer_property.layerName);
        if(layerExtensions.size()>0){
            std::cout<<"    layer extensions:"<<std::endl;
            for(auto layerExtension:layerExtensions){
                std::cout<<"      "<<layerExtension.extensionName<<std::endl;
            }
        }
    }

    std::cout<<"supported instance extensions:"<<std::endl;
    for(auto instance_layer_property:Application::enumerateInstanceExtensionProperties(nullptr)){
        std::cout<<"  "<<instance_layer_property.extensionName<<std::endl;
    }

    auto application_info=VkApplicationInfo{
        VK_STRUCTURE_TYPE_APPLICATION_INFO,
        nullptr,
        "vkcompute-app",
        VK_MAKE_API_VERSION(0, 0, 1, 0),
        "vkcompute-engine",
        VK_MAKE_API_VERSION(0, 0, 1, 0),
        VK_MAKE_API_VERSION(0, 1, 0, 0)
    };
    std::vector<const char*>instance_layers{
        "VK_LAYER_KHRONOS_validation"
    };
    std::vector<const char*>instance_extensions{
        "VK_KHR_surface",
        "VK_KHR_xcb_surface"
    };
    auto instance_create_info=VkInstanceCreateInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        &application_info,
        static_cast<uint32_t>(instance_layers.size()),
        instance_layers.data(),
        static_cast<uint32_t>(instance_extensions.size()),
        instance_extensions.data()
    };

    VkAllocationCallbacks *vk_allocator=nullptr;
    VkInstance vk_instance;
    auto res=vkCreateInstance(
        &instance_create_info,
        vk_allocator,
        &vk_instance
    );
    if(res!=VK_SUCCESS){
        throw new VulkanError(VulkanErrorContext::InstanceCreation,res);
    }

    std::vector<const char*> device_layers{
        "VK_LAYER_KHRONOS_validation"
    };
    std::vector<const char*> device_extensions{
        "VK_KHR_swapchain"
    };

    uint32_t num_physical_devices;
    vkEnumeratePhysicalDevices(vk_instance,&num_physical_devices,nullptr);
    std::vector<VkPhysicalDevice> physical_devices(num_physical_devices);
    vkEnumeratePhysicalDevices(vk_instance,&num_physical_devices,physical_devices.data());

    this->vulkan=std::make_shared<VulkanContext>(vk_allocator,vk_instance,VK_NULL_HANDLE,VK_NULL_HANDLE);
    auto test_window=create_window(100,100);

    VkPhysicalDevice vk_physical_device=VK_NULL_HANDLE;
    for(auto physical_device:physical_devices){
        bool device_is_usable=false;

        auto _=defer([&]()->void{
            if(device_is_usable){
                std::cout<<"-- viable device found"<<std::endl;
            }else{
                std::cout<<"-- device is not viable"<<std::endl;
            }
        });

        uint32_t num_device_layer_properties=0;
        vkEnumerateDeviceLayerProperties(physical_device,&num_device_layer_properties,nullptr);
        std::vector<VkLayerProperties> device_layer_properties(num_device_layer_properties);
        vkEnumerateDeviceLayerProperties(physical_device,&num_device_layer_properties,device_layer_properties.data());
        std::cout<<"device layers:"<<std::endl;
        for(auto device_layer_property:device_layer_properties){
            std::cout<<"  "<<device_layer_property.layerName<<std::endl;
        }

        uint32_t num_device_extension_properties=0;
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &num_device_extension_properties, nullptr);
        std::vector<VkExtensionProperties> device_extension_properties(num_device_extension_properties);
        vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &num_device_extension_properties, device_extension_properties.data());
        std::cout<<"device extensions:"<<std::endl;
        for(auto device_extension_property:device_extension_properties){
            std::cout<<"  "<<device_extension_property.extensionName<<std::endl;
        }

        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
        std::cout << physical_device_properties.deviceName << std::endl;

        // we want some gpu
        switch(physical_device_properties.deviceType){
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                std::cout<<"VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU"<<std::endl;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                std::cout<<"VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU"<<std::endl;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                std::cout<<"VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU"<<std::endl;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                std::cout<<"VK_PHYSICAL_DEVICE_TYPE_CPU"<<std::endl;
                continue;
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                std::cout<<"VK_PHYSICAL_DEVICE_TYPE_OTHER"<<std::endl;
                continue;
            default:
                std::cout<<"warning - found invalid vulkan device"<<std::endl;
                continue;
        }

        uint32_t physical_device_num_queue_family_properties;
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, 
            &physical_device_num_queue_family_properties, 
            nullptr
        );
        std::vector<VkQueueFamilyProperties> physical_device_queue_family_properties(physical_device_num_queue_family_properties);
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, 
            &physical_device_num_queue_family_properties, 
            physical_device_queue_family_properties.data()
        );

        int queue_family_index=0;
        for(auto queue_family:physical_device_queue_family_properties){
            std::cout<<queue_family.queueCount<<" queues allowed of family "<<queue_family_index<<std::endl;

            bool supports_transfer=(queue_family.queueFlags&VK_QUEUE_TRANSFER_BIT)>0;
            bool supports_graphics=(queue_family.queueFlags&VK_QUEUE_GRAPHICS_BIT)>0;

            VkBool32 supports_presentation=VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                physical_device, 
                queue_family_index, 
                test_window->vk_surface,
                &supports_presentation
            );

            if(supports_presentation){
                vk_present_queue_family_index=queue_family_index;
            }
            if(supports_graphics && supports_transfer){
                vk_graphics_queue_family_index=queue_family_index;
            }
            
            queue_family_index++;
        }

        device_is_usable=true;
        vk_physical_device=physical_device;
    }

    if(vk_physical_device==VK_NULL_HANDLE){
        throw VulkanError(VulkanErrorContext::NoViablePhysicalDeviceFound);
    }

    std::vector<float> queue_priorities{
        1.0,
        1.0
    };
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos{
        VkDeviceQueueCreateInfo{
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr,
            0,
            vk_present_queue_family_index,
            1,
            &queue_priorities[0]
        },
        VkDeviceQueueCreateInfo{
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            nullptr,
            0,
            vk_graphics_queue_family_index,
            1,
            &queue_priorities[1]
        },
    };
    auto device_features_enabled=VkPhysicalDeviceFeatures{};
    memset(&device_features_enabled,0,sizeof(device_features_enabled));
    auto device_create_info=VkDeviceCreateInfo{
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        nullptr,
        0,
        static_cast<uint32_t>(queue_create_infos.size()),
        queue_create_infos.data(),
        static_cast<uint32_t>(device_layers.size()),
        device_layers.data(),
        static_cast<uint32_t>(device_extensions.size()),
        device_extensions.data(),
        &device_features_enabled
    };

    VkDevice vk_device;
    res=vkCreateDevice(
        vk_physical_device,
        &device_create_info,
        vk_allocator,
        &vk_device
    );
    if(res!=VK_SUCCESS){
        throw VulkanError(VulkanErrorContext::InstanceCreation,res);
    }

    this->vulkan=std::make_shared<VulkanContext>(vk_allocator,vk_instance,vk_physical_device,vk_device);
    vulkan->deviceWaitIdle();
    std::cout<<"device is alive"<<std::endl;

    // get queues
    vkGetDeviceQueue(
        vk_device,
        vk_present_queue_family_index,
        0,
        &vk_present_queue
    );
    vkGetDeviceQueue(
        vk_device,
        vk_graphics_queue_family_index,
        0,
        &vk_graphics_queue
    );

    this->window=create_window(500,500);

    std::vector<VkAttachmentDescription> render_pass_attachments{
        VkAttachmentDescription{
            0,
            window->vk_swapchain_surface_format.format,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        }
    };

    std::vector<std::vector<VkAttachmentReference>> subpass_input_attachments{{}};
    std::vector<std::vector<VkAttachmentReference>> subpass_color_attachments{{
        VkAttachmentReference{
            0,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        }
    }};
    std::vector<VkSubpassDescription> render_pass_subpasses{
        VkSubpassDescription{
            0,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            static_cast<uint32_t>(subpass_input_attachments[0].size()),
            subpass_input_attachments[0].data(),
            static_cast<uint32_t>(subpass_color_attachments[0].size()),
            subpass_color_attachments[0].data(),
            nullptr,
            nullptr,
            0,
            nullptr
        }
    };
    std::vector<VkSubpassDependency> render_pass_subpass_dependencies{
        VkSubpassDependency{
            VK_SUBPASS_EXTERNAL,
            0,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_DEPENDENCY_BY_REGION_BIT
        },
        VkSubpassDependency{
            0,
            VK_SUBPASS_EXTERNAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_MEMORY_READ_BIT,
            VK_DEPENDENCY_BY_REGION_BIT
        },
    };
    auto render_pass_create_info=VkRenderPassCreateInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        nullptr,
        0,
        static_cast<uint32_t>(render_pass_attachments.size()),
        render_pass_attachments.data(),
        static_cast<uint32_t>(render_pass_subpasses.size()),
        render_pass_subpasses.data(),
        static_cast<uint32_t>(render_pass_subpass_dependencies.size()),
        render_pass_subpass_dependencies.data()
    };

    res=vkCreateRenderPass(
        vk_device,
        &render_pass_create_info,
        vk_allocator,
        &vk_render_pass
    );
    if(res!=VK_SUCCESS){
        throw VulkanError(VulkanErrorContext::CreateRenderPass,res);
    }

    window->create_framebuffers(vk_render_pass);
}

Application::~Application(){
    if(vulkan->device!=VK_NULL_HANDLE){
        vulkan->deviceWaitIdle();

        window.reset();

        vkDestroyRenderPass(
            vulkan->device,
            vk_render_pass,
            vulkan->allocator
        );

        vulkan.reset();
    }

    xcb_disconnect(
        xcb_connection
    );
}

void Application::run(){
    vulkan->deviceWaitIdle();

    auto create_semaphore_info=VkSemaphoreCreateInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        nullptr,
        0
    };
    VkSemaphore vk_image_available_semaphore_handle;
    vkCreateSemaphore(vulkan->device,&create_semaphore_info,vulkan->allocator,&vk_image_available_semaphore_handle);
    auto vk_image_available_semaphore=std::make_shared<Semaphore>(vulkan->device,vulkan->allocator,vk_image_available_semaphore_handle);
    
    VkSemaphore vk_rendering_finished_semaphore_handle;
    vkCreateSemaphore(vulkan->device,&create_semaphore_info,vulkan->allocator,&vk_rendering_finished_semaphore_handle);
    auto vk_rendering_finished_semaphore=std::make_shared<Semaphore>(vulkan->device,vulkan->allocator,vk_rendering_finished_semaphore_handle);

    auto present_command_pool_create_info=VkCommandPoolCreateInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        vk_present_queue_family_index
    };
    VkCommandPool present_vk_command_pool;
    vkCreateCommandPool(vulkan->device,&present_command_pool_create_info,vulkan->allocator,&present_vk_command_pool);
    auto defer_presentDestroyCommandPool=defer([&](){
        vkDestroyCommandPool(vulkan->device,present_vk_command_pool,vulkan->allocator);
    });

    auto present_command_buffer_allocate_info=VkCommandBufferAllocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        present_vk_command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };
    std::vector<VkCommandBuffer> present_command_buffers(present_command_buffer_allocate_info.commandBufferCount);
    vkAllocateCommandBuffers(vulkan->device,&present_command_buffer_allocate_info,present_command_buffers.data());
    auto defer_presentFreeCommandBuffers=defer([&](){
        vkFreeCommandBuffers(vulkan->device,present_vk_command_pool,present_command_buffer_allocate_info.commandBufferCount,present_command_buffers.data());
    });

    auto graphics_command_pool_create_info=VkCommandPoolCreateInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        vk_graphics_queue_family_index
    };
    VkCommandPool graphics_vk_command_pool;
    vkCreateCommandPool(vulkan->device,&graphics_command_pool_create_info,vulkan->allocator,&graphics_vk_command_pool);
    auto defer_graphicsDestroyCommandPool=defer([&](){
        vkDestroyCommandPool(vulkan->device,graphics_vk_command_pool,vulkan->allocator);
    });

    auto graphics_command_buffer_allocate_info=VkCommandBufferAllocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        graphics_vk_command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };
    std::vector<VkCommandBuffer> graphics_command_buffers(graphics_command_buffer_allocate_info.commandBufferCount);
    vkAllocateCommandBuffers(vulkan->device,&graphics_command_buffer_allocate_info,graphics_command_buffers.data());
    auto defer_graphicsFreeCommandBuffers=defer([&](){
        vkFreeCommandBuffers(vulkan->device,graphics_vk_command_pool,graphics_command_buffer_allocate_info.commandBufferCount,graphics_command_buffers.data());
    });

    //auto present_vk_command_buffer=present_command_buffers[0];
    auto graphics_vk_command_buffer=graphics_command_buffers[0];

    bool should_keep_running=true;
    bool should_resize_window=false;

    while(should_keep_running){
        std::this_thread::sleep_for(std::chrono::milliseconds(33));

        auto input_events=window->get_latest_events();
        for(auto event:input_events){
            if(const WindowCloseEvent* window_close_event=std::get_if<WindowCloseEvent>(&event.event_variant)){
                should_keep_running=false;
            }else if(const PointerMoved* pointer_moved_event=std::get_if<PointerMoved>(&event.event_variant)){
                ;
            }else if(const WindowResizeEvent* window_resize_event=std::get_if<WindowResizeEvent>(&event.event_variant)){
                should_resize_window=true;
            }
        }

        if(should_resize_window){
            window->vulkan_resize(vk_render_pass);

            should_resize_window=false;
        }

        uint32_t next_swapchain_image_index=0;
        auto res=vkAcquireNextImageKHR(vulkan->device,window->vk_swapchain,0,vk_image_available_semaphore->handle,VK_NULL_HANDLE,&next_swapchain_image_index);
        if(res!=VK_SUCCESS){
            switch(res){
                case VK_SUBOPTIMAL_KHR:
                    should_resize_window=true;
                    break;
                default:
                    throw VulkanError(VulkanErrorContext::SwapchainAcquireNextImage,res);
            }
        }
        VkImage current_swapchain_image=window->swapchain_images[next_swapchain_image_index];

        auto graphics_command_buffer_begin_info=VkCommandBufferBeginInfo{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            nullptr,
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            nullptr,
        };
        vkBeginCommandBuffer(graphics_vk_command_buffer,&graphics_command_buffer_begin_info);
        {
            VkClearValue clear_value;
            clear_value.color.float32[0]=1.0;
            clear_value.color.float32[1]=1.0;
            clear_value.color.float32[2]=1.0;
            clear_value.color.float32[3]=1.0;

            auto render_pass_begin_info=VkRenderPassBeginInfo{
                VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                nullptr,
                vk_render_pass,
                window->vk_swapchain_framebuffers[next_swapchain_image_index],
                VkRect2D{
                    VkOffset2D{
                        0,
                        0
                    },
                    VkExtent2D{
                        static_cast<uint32_t>(window->width),
                        static_cast<uint32_t>(window->height)
                    }
                },
                1,
                &clear_value
            };
            vkCmdBeginRenderPass(
                graphics_vk_command_buffer,
                &render_pass_begin_info,
                VK_SUBPASS_CONTENTS_INLINE
            );
            {
                // todo render logic
            }
            vkCmdEndRenderPass(graphics_vk_command_buffer);

            auto render_image_to_memory_barrier=VkImageMemoryBarrier{
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                nullptr,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_ACCESS_MEMORY_READ_BIT,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_QUEUE_FAMILY_IGNORED,
                VK_QUEUE_FAMILY_IGNORED,
                current_swapchain_image,
                VkImageSubresourceRange{
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    0,
                    1,
                    0,
                    1,
                }
            };
            vkCmdPipelineBarrier(
                graphics_vk_command_buffer,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &render_image_to_memory_barrier
            );
        }
        //discard vkEndCommandBuffer(present_vk_command_buffer);
        discard vkEndCommandBuffer(graphics_vk_command_buffer);

        const VkPipelineStageFlags wait_dst_stage_mask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        std::vector<VkCommandBuffer> submit_command_buffers{
            graphics_vk_command_buffer
        };
        std::vector<VkSubmitInfo> graphics_queue_submit_infos{
            VkSubmitInfo{
                VK_STRUCTURE_TYPE_SUBMIT_INFO,
                nullptr,
                1,
                &vk_image_available_semaphore->handle,
                &wait_dst_stage_mask,
                static_cast<uint32_t>(submit_command_buffers.size()),
                submit_command_buffers.data(),
                1,
                &vk_rendering_finished_semaphore->handle
            }
        };
        vkQueueSubmit(
            vk_graphics_queue,
            static_cast<uint32_t>(graphics_queue_submit_infos.size()),
            graphics_queue_submit_infos.data(),
            VK_NULL_HANDLE
        );

        std::vector<VkSemaphore> swapchain_present_await_semaphores{
            vk_rendering_finished_semaphore->handle
        };
        auto swapchain_present_info=VkPresentInfoKHR{
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            nullptr,
            static_cast<uint32_t>(swapchain_present_await_semaphores.size()),
            swapchain_present_await_semaphores.data(),
            1,
            &window->vk_swapchain,
            &next_swapchain_image_index,
            nullptr,
        };
        vkQueuePresentKHR(vk_present_queue,&swapchain_present_info);

        vulkan->deviceWaitIdle();

        if(should_resize_window){

        }

        vulkan->deviceWaitIdle();
    }
}
