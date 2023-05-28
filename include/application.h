#pragma once

#include <cstdlib>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>
#include <optional>

#include <vulkan/vulkan.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#define discard (void)
struct defer{
    private:
        std::function<void()> callback;

    public:
        defer(
            std::function<void()> callback
        ):callback(callback){}

        ~defer(){
            callback();
        }
};

#include <application/vulkan_context.h>
#include <application/vulkan_error.h>
#include <application/window.h>

class Application{
    private:
        xcb_connection_t *xcb_connection;

        std::shared_ptr<VulkanContext> vulkan;

        VkQueue vk_graphics_queue;
        uint32_t vk_graphics_queue_family_index;
        VkQueue vk_present_queue;
        uint32_t vk_present_queue_family_index;

        VkRenderPass vk_render_pass;

        std::shared_ptr<Window> window;

    public:
        static std::vector<VkLayerProperties> enumerateInstanceLayerProperties(){
            uint32_t supported_num_instance_layer_properties=0;
            vkEnumerateInstanceLayerProperties(
                &supported_num_instance_layer_properties, 
                nullptr
            );
            std::vector<VkLayerProperties> supported_instance_layer_properties(supported_num_instance_layer_properties);
            vkEnumerateInstanceLayerProperties(
                &supported_num_instance_layer_properties, 
                supported_instance_layer_properties.data()
            );
            return supported_instance_layer_properties;
        }
        static std::vector<VkExtensionProperties> enumerateInstanceExtensionProperties(
            const char* layer_name
        ){
            uint32_t supported_num_instance_extension_properties=0;
            vkEnumerateInstanceExtensionProperties(
                layer_name,
                &supported_num_instance_extension_properties, 
                nullptr
            );
            std::vector<VkExtensionProperties> supported_instance_extension_properties(supported_num_instance_extension_properties);
            vkEnumerateInstanceExtensionProperties(
                layer_name,
                &supported_num_instance_extension_properties, 
                supported_instance_extension_properties.data()
            );
            return supported_instance_extension_properties;
        }

        Application();

        void run();

        ~Application();

        std::shared_ptr<Window> create_window(
            int width,
            int height
        ){
            std::shared_ptr<Window> window=std::make_shared<Window>(
                xcb_connection,
                vulkan,
                width,
                height
            );
            return window;
        }

    public:
};
