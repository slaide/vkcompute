#pragma once

#include <variant>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include <xcb/xcb.h>
#include <vulkan/vulkan.h>

#include <application/vulkan_context.h>
#include <application/vulkan_error.h>

struct WindowMoveEvent{
    int new_x;
    int new_y;
};
struct WindowResizeEvent{
    int new_width;
    int new_height;
};
struct PointerEnteredWindow{};
struct PointerExitedWindow{};
struct WindowCloseEvent{
    xcb_window_t window_handle;
};
struct PointerMoved{
    float x;
    float y;
};
struct ScrollEvent{
    float scroll_y;
    float scroll_x;
};
struct KeyPressed{
    int key;
};
struct KeyReleased{
    int key;
};
struct WindowGainedFocus{};
struct WindowLostFocus{};
struct PointerWindowEnterEvent{};
struct PointerWindowLeaveEvent{};
struct ButtonPressed{
    int button;
};
struct ButtonReleased{
    int button;
};
typedef std::variant<
    PointerWindowEnterEvent,
    PointerWindowLeaveEvent,
    ButtonPressed,
    ButtonReleased,
    WindowGainedFocus,
    WindowLostFocus,
    KeyPressed,
    KeyReleased,
    ScrollEvent,
    PointerMoved,
    WindowCloseEvent,
    PointerEnteredWindow,
    PointerExitedWindow,
    WindowResizeEvent,
    WindowMoveEvent
> WindowEventVariant;
class WindowEvent{
    private:
    
    public:
        WindowEventVariant event_variant;

        WindowEvent(const WindowEventVariant &event_variant){
            this->event_variant=event_variant;
        }
        
        std::string string()const;
};


class Window{
    public:
        int width;
        int height;

        int screen_x=0;
        int screen_y=0;
    private:
        xcb_connection_t *xcb_connection;
    public:
        xcb_window_t window_handle;
        xcb_atom_t wm_delete_atom;

    private:
        std::shared_ptr<VulkanContext> vulkan;

    public:
        VkSwapchainKHR vk_swapchain=VK_NULL_HANDLE;
        std::vector<VkImage> swapchain_images;
        std::vector<VkFramebuffer> vk_swapchain_framebuffers;
        std::vector<VkImageView> vk_swapchain_image_views;

        VkSurfaceKHR vk_surface=VK_NULL_HANDLE;
        VkSurfaceFormatKHR vk_swapchain_surface_format;
    
    private:
        bool is_non_temp_window()const{
            return vulkan->device!=VK_NULL_HANDLE;
        }

        /// flush window system I/O
        void flush(){
            xcb_flush(xcb_connection);
        }
    
    public:
        /// creates a window with a surface, but without a swapchain
        Window(
            xcb_connection_t *xcb_connection,
            std::shared_ptr<VulkanContext> vulkan,
            int width,
            int height,
            int x=0,
            int y=0,
            int screen_index=0
        );

        xcb_atom_t get_intern_atom(
            std::string atom_name,
            bool only_if_exists=false
        )const;

        void create_framebuffers(
            VkRenderPass render_pass
        );
        void create_swapchain();

        void vulkan_resize(VkRenderPass render_pass){
            destroy_framebuffers();
            destroy_image_views();
            create_swapchain();
            create_framebuffers(render_pass);
            vulkan->deviceWaitIdle();
        }

        std::vector<WindowEvent> get_latest_events();

        void destroy_image_views(){
            for(auto image_view:vk_swapchain_image_views){
                vkDestroyImageView(vulkan->device,image_view,vulkan->allocator);
            }
            vk_swapchain_image_views.clear();
        }
        void destroy_framebuffers(){
            for(auto framebuffer:vk_swapchain_framebuffers){
                vkDestroyFramebuffer(vulkan->device,framebuffer,vulkan->allocator);
            }
            vk_swapchain_framebuffers.clear();
        }

        ~Window();
};
