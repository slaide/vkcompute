#include "application/vulkan_context.h"
#include "application/vulkan_error.h"
#include "application/window.h"
#include "vk_video/vulkan_video_codec_h265std.h"
#include "vulkan/vulkan_metal.h"
#include <chrono>
#include <ios>
#include <stdexcept>
#include <string>
#include <thread>
#include <iostream>
#include <cstring>
#include <fstream>

#include <application.h>
#include <vector>
#include <vulkan/vulkan_core.h>

VkShaderModule create_shader_module(
    std::shared_ptr<VulkanContext> vulkan,
    std::string filepath
){
    std::ifstream shader_module_file{filepath};
    if (!shader_module_file) {
        throw  std::runtime_error("vertex shader file does not exist");
    }
    shader_module_file.seekg(0,std::ios::end);
    auto shader_module_file_size=shader_module_file.tellg();
    shader_module_file.seekg(0,std::ios::beg);

    std::vector<uint32_t> shader_module_bytes(shader_module_file_size);
    shader_module_file.read((char*)(shader_module_bytes.data()), shader_module_file_size);

    shader_module_file.close();

    VkShaderModuleCreateInfo shader_module_create_info{
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        nullptr,
        0,
        static_cast<uint32_t>(shader_module_file_size),
        shader_module_bytes.data()
    };
    VkShaderModule shader_module;
    auto shader_module_create_res=vkCreateShaderModule(
        vulkan->device, 
        &shader_module_create_info, 
        vulkan->allocator, 
        &shader_module
    );
    if (shader_module_create_res!=VK_SUCCESS) {
        throw  std::runtime_error("vertex_shader_module_create_res failed");
    }

    return  shader_module;
}

GraphicsPipeline::GraphicsPipeline(
    std::shared_ptr<VulkanContext> vulkan,
    VkRenderPass vk_render_pass
){
    std::vector<VkDescriptorSetLayout> graphics_pipeline_set_layouts{};
    std::vector<VkPushConstantRange> graphics_pipeline_push_constant_ranges{};
    VkPipelineLayoutCreateInfo graphics_pipeline_layout_create_info{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        nullptr,
        0,
        static_cast<uint32_t>(graphics_pipeline_set_layouts.size()),
        graphics_pipeline_set_layouts.data(),
        static_cast<uint32_t>(graphics_pipeline_push_constant_ranges.size()),
        graphics_pipeline_push_constant_ranges.data()
    };
    VkPipelineLayout graphics_pipeline_layout;
    vkCreatePipelineLayout(
        vulkan->device,
        &graphics_pipeline_layout_create_info,
        vulkan->allocator,
        &graphics_pipeline_layout
    );

    VkShaderModule vertex_shader_module=create_shader_module( vulkan, "vertex_shader.spv" );
    VkShaderModule fragment_shader_module=create_shader_module( vulkan, "fragment_shader.spv" );

    std::vector<VkPipelineShaderStageCreateInfo> pipeline_stages{
        VkPipelineShaderStageCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            fragment_shader_module,
            "main",
            nullptr
        },
        VkPipelineShaderStageCreateInfo{
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            vertex_shader_module,
            "main",
            nullptr
        }
    };
    std::vector<VkVertexInputBindingDescription> vertex_input_binding_descriptions{};
    std::vector<VkVertexInputAttributeDescription> vertex_input_attribute_descriptions{};
    VkPipelineVertexInputStateCreateInfo vertex_input_state{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr,
        0,
        static_cast<uint32_t>(vertex_input_binding_descriptions.size()),
        vertex_input_binding_descriptions.data(),
        static_cast<uint32_t>(vertex_input_attribute_descriptions.size()),
        vertex_input_attribute_descriptions.data()
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        VK_FALSE
    };
    VkViewport pipeline_viewport{
        0.0,0.0,
        500,500,
        0.0,1.0
    };
    VkRect2D pipeline_scissor{
        {0,0},
        {500,500}
    };
    VkPipelineViewportStateCreateInfo pipeline_viewport_state{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        nullptr,
        0,
        1,
        &pipeline_viewport,
        1,
        &pipeline_scissor
    };
    VkPipelineRasterizationStateCreateInfo rasterization_state{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_FALSE,
        VK_FALSE,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_BACK_BIT,
        VK_FRONT_FACE_CLOCKWISE,
        VK_FALSE,
        0.0,
        0.0,
        0.0,
        1.0
    };
    std::vector<VkPipelineColorBlendAttachmentState> graphics_pipeline_color_blend_attachment_states{
        VkPipelineColorBlendAttachmentState{
            VK_FALSE,
            VK_BLEND_FACTOR_ONE,
            VK_BLEND_FACTOR_ONE,
            VK_BLEND_OP_ADD,
            VK_BLEND_FACTOR_ONE,
            VK_BLEND_FACTOR_ONE,
            VK_BLEND_OP_ADD,
            VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT
        }
    };
    VkPipelineColorBlendStateCreateInfo color_blend_state{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr,
        0,
        VK_FALSE,
        VK_LOGIC_OP_AND,
        static_cast<uint32_t>(graphics_pipeline_color_blend_attachment_states.size()),
        graphics_pipeline_color_blend_attachment_states.data(),
        {1.0,1.0,1.0,1.0}
    };
    std::vector<VkGraphicsPipelineCreateInfo> graphics_pipeline_create_infos{
        VkGraphicsPipelineCreateInfo{
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            nullptr,
            0,
            static_cast<uint32_t>(pipeline_stages.size()),
            pipeline_stages.data(),
            &vertex_input_state,
            &input_assembly_state,
            nullptr,
            &pipeline_viewport_state,
            &rasterization_state,
            nullptr,
            nullptr,
            &color_blend_state,
            nullptr,
            graphics_pipeline_layout,
            vk_render_pass,
            0,
            VK_NULL_HANDLE,
            0
        }
    };
    VkPipeline graphics_pipeline_handle;
    auto graphics_pipeline_create_res=vkCreateGraphicsPipelines(
        vulkan->device,
        VK_NULL_HANDLE,
        static_cast<uint32_t>(graphics_pipeline_create_infos.size()),
        graphics_pipeline_create_infos.data(),
        vulkan->allocator,
        &graphics_pipeline_handle
    );
    VulkanError::check(VulkanErrorContext::CreateGraphicsPipelines,graphics_pipeline_create_res);
}

Application::Application(){
    #ifdef VK_USE_PLATFORM_XCB_KHR
    xcb_connection=xcb_connect(
        nullptr,
        nullptr
    );
    #endif

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
        "vkcomputeapp",
        VK_MAKE_API_VERSION(0, 0, 1, 0),
        "vkcomputeengine",
        VK_MAKE_API_VERSION(0, 0, 1, 0),
        VK_API_VERSION_1_0
    };
    std::vector<const char*>instance_layers{
        "VK_LAYER_KHRONOS_validation"
    };
    std::vector<const char*>instance_extensions{
        "VK_KHR_surface",
        #ifdef VK_USE_PLATFORM_XCB_KHR
            "VK_KHR_xcb_surface",
        #elif VK_USE_PLATFORM_METAL_EXT
            VK_EXT_METAL_SURFACE_EXTENSION_NAME,
            "VK_KHR_portability_enumeration",
            "VK_KHR_get_physical_device_properties2"
        #endif
    };

    VkInstanceCreateInfo instance_create_info{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        &application_info,
        static_cast<uint32_t>(instance_layers.size()),
        instance_layers.data(),
        static_cast<uint32_t>(instance_extensions.size()),
        instance_extensions.data()
    };
    #ifdef VK_USE_PLATFORM_METAL_EXT
    instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    #endif

    VkAllocationCallbacks *vk_allocator=nullptr;
    VkInstance vk_instance;
    auto res=vkCreateInstance(
        &instance_create_info,
        vk_allocator,
        &vk_instance
    );
    VulkanError::check(VulkanErrorContext::InstanceCreation,res);

    std::vector<const char*> device_layers{
        "VK_LAYER_KHRONOS_validation"
    };
    std::vector<const char*> device_extensions{
        "VK_KHR_swapchain",
        #ifdef  VK_USE_PLATFORM_METAL_EXT
        "VK_KHR_portability_subset",
        #endif
    };

    uint32_t num_physical_devices;
    vkEnumeratePhysicalDevices(vk_instance,&num_physical_devices,nullptr);
    std::vector<VkPhysicalDevice> physical_devices(num_physical_devices);
    vkEnumeratePhysicalDevices(vk_instance,&num_physical_devices,physical_devices.data());

    VkPhysicalDevice vk_physical_device=VK_NULL_HANDLE;
    {
        auto temp_vulkan=std::make_shared<VulkanContext>(vk_allocator,vk_instance,VK_NULL_HANDLE,VK_NULL_HANDLE);
        auto test_window=create_window(100,100,temp_vulkan);

        for(auto physical_device:physical_devices){
            bool device_is_usable=false;

            auto _=defer([&]()->void {
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
            std::cout << physical_device_properties.deviceName << " : ";

            // we want some gpu
            switch(physical_device_properties.deviceType){
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    std::cout<<"DISCRETE_GPU"<<std::endl;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    std::cout<<"INTEGRATED_GPU"<<std::endl;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    std::cout<<"VIRTUAL_GPU"<<std::endl;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    std::cout<<"CPU"<<std::endl;
                    continue;
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    std::cout<<"DEVICE_TYPE_OTHER"<<std::endl;
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

            vk_present_queue_family_index=-1;
            vk_graphics_queue_family_index=-1;

            int queue_family_index=0;
            for(auto queue_family:physical_device_queue_family_properties){
                std::cout<<"  "<<queue_family.queueCount<<" queues allowed of family "<<queue_family_index<<std::endl;

                bool supports_transfer=(queue_family.queueFlags&VK_QUEUE_TRANSFER_BIT)>0;
                bool supports_graphics=(queue_family.queueFlags&VK_QUEUE_GRAPHICS_BIT)>0;

                auto supports_presentation=VK_FALSE;
                auto res=vkGetPhysicalDeviceSurfaceSupportKHR(
                    physical_device, 
                    queue_family_index, 
                    test_window->vk_surface,
                    &supports_presentation
                );
                if(res!=VK_SUCCESS){
                    std::cout<<"failed vkGetPhysicalDeviceSurfaceSupportKHR with "<<res<<std::endl;
                    throw std::runtime_error("vkGetPhysicalDeviceSurfaceSupportKHR failed");
                }

                // use separate queue families for each queue to avoid the currently unhandled case of a queue family supporting both features, but not supporting 2 individual queues
                if(supports_presentation && vk_present_queue_family_index==-1){
                    vk_present_queue_family_index=queue_family_index;
                }
                if(supports_graphics && vk_graphics_queue_family_index==-1 && vk_present_queue_family_index!=queue_family_index){
                    vk_graphics_queue_family_index=queue_family_index;
                }
                
                queue_family_index++;
            }

            device_is_usable=true;
            vk_physical_device=physical_device;
        }
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
    VulkanError::check(VulkanErrorContext::InstanceCreation,res);

    this->vulkan=std::make_shared<VulkanContext>(vk_allocator,vk_instance,vk_physical_device,vk_device);
    vulkan->deviceWaitIdle();

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
    VulkanError::check(VulkanErrorContext::CreateRenderPass,res);

    this->window->create_framebuffers(vk_render_pass);

    auto create_semaphore_info=VkSemaphoreCreateInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        nullptr,
        0
    };
    res=vkCreateSemaphore(vulkan->device,&create_semaphore_info,vulkan->allocator,&vk_image_available_semaphore_handle);
    VulkanError::check(VulkanErrorContext::CreateSwapchain,res);

    vk_image_available_semaphore=std::make_shared<Semaphore>(vulkan->device,vulkan->allocator,vk_image_available_semaphore_handle);
    
    res=vkCreateSemaphore(vulkan->device,&create_semaphore_info,vulkan->allocator,&vk_rendering_finished_semaphore_handle);
    VulkanError::check(VulkanErrorContext::CreateSwapchain,res);

    vk_rendering_finished_semaphore=std::make_shared<Semaphore>(vulkan->device,vulkan->allocator,vk_rendering_finished_semaphore_handle);

    auto present_command_pool_create_info=VkCommandPoolCreateInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        vk_present_queue_family_index
    };
    res=vkCreateCommandPool(vulkan->device,&present_command_pool_create_info,vulkan->allocator,&present_vk_command_pool);
    VulkanError::check(VulkanErrorContext::CreateCommandPool,res);

    auto present_command_buffer_allocate_info=VkCommandBufferAllocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        present_vk_command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };
    present_command_buffers.resize(present_command_buffer_allocate_info.commandBufferCount);
    res=vkAllocateCommandBuffers(vulkan->device,&present_command_buffer_allocate_info,present_command_buffers.data());
    VulkanError::check(VulkanErrorContext::AllocateCommandBuffers,res);

    auto graphics_command_pool_create_info=VkCommandPoolCreateInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        nullptr,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        vk_graphics_queue_family_index
    };
    res=vkCreateCommandPool(vulkan->device,&graphics_command_pool_create_info,vulkan->allocator,&graphics_vk_command_pool);
    VulkanError::check(VulkanErrorContext::CreateCommandPool,res);

    auto graphics_command_buffer_allocate_info=VkCommandBufferAllocateInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        nullptr,
        graphics_vk_command_pool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        1
    };
    graphics_command_buffers.resize(graphics_command_buffer_allocate_info.commandBufferCount);
    res=vkAllocateCommandBuffers(vulkan->device,&graphics_command_buffer_allocate_info,graphics_command_buffers.data());
    VulkanError::check(VulkanErrorContext::AllocateCommandBuffers,res);

    auto graphics_pipeline_object=GraphicsPipeline(vulkan,vk_render_pass);
}

Application::~Application(){
    if(vulkan->device!=VK_NULL_HANDLE){
        vkDestroyCommandPool(vulkan->device,present_vk_command_pool,vulkan->allocator);
        vkFreeCommandBuffers(vulkan->device,present_vk_command_pool,present_command_buffers.size(),present_command_buffers.data());
        vkDestroyCommandPool(vulkan->device,graphics_vk_command_pool,vulkan->allocator);
        vkFreeCommandBuffers(vulkan->device,graphics_vk_command_pool,graphics_command_buffers.size(),graphics_command_buffers.data());

        vulkan->deviceWaitIdle();

        window.reset();

        vkDestroyRenderPass(
            vulkan->device,
            vk_render_pass,
            vulkan->allocator
        );

        vulkan.reset();
    }

    #ifdef VK_USE_PLATFORM_XCB_KHR
    xcb_disconnect(
        xcb_connection
    );
    #endif
}

void Application::run_forever(){
    vulkan->deviceWaitIdle();
    
    while(should_keep_running){
        std::this_thread::sleep_for(std::chrono::milliseconds(33));

        run_step();
    }
}

void Application::run_step(){
    auto graphics_vk_command_buffer=graphics_command_buffers[0];

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
}