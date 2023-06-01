#include<application.h>

std::string WindowEvent::string()const{
    #define MATCHES(event_variant_type) \
        (const event_variant_type *ev=std::get_if<event_variant_type>(&event_variant))

    return std::visit([](const WindowEventVariant event_variant)->std::string{
        if MATCHES(PointerWindowEnterEvent){
            return "PointerWindowEnterEvent";
        }else if MATCHES(PointerWindowLeaveEvent){
            return "PointerWindowLeaveEvent";
        }else if MATCHES(ButtonPressed){
            return "ButtonPressed \{ button: "+std::to_string(ev->button)+" }";
        }else if MATCHES(ButtonReleased){
            return "ButtonReleased \{ button: "+std::to_string(ev->button)+" }";
        }else if MATCHES(WindowGainedFocus){
            return "WindowGainedFocus";
        }else if MATCHES(WindowLostFocus){
            return "WindowLostFocus";
        }else if MATCHES(KeyPressed){
            return "KeyPressed \{ key: "+std::to_string(ev->key)+" }";
        }else if MATCHES(KeyReleased){
            return "KeyReleased \{ key: "+std::to_string(ev->key)+" }";
        }else if MATCHES(ScrollEvent){
            return "ScrollEvent \{ scroll_x: "+std::to_string(ev->scroll_x)+" , scroll_y: "+std::to_string(ev->scroll_y)+" }";
        }else if MATCHES(PointerMoved){
            return "PointerMoved \{ x: "+std::to_string(ev->x)+" , y: "+std::to_string(ev->y)+" }";
    #ifdef VK_USE_PLATFORM_XCB_KHR
        }else if MATCHES(WindowCloseEvent){
            return "WindowCloseEvent \{ window: "+std::to_string(ev->window_handle)+" }";
        #endif
        }else if MATCHES(PointerEnteredWindow){
            return "PointerEnteredWindow";
        }else if MATCHES(PointerExitedWindow){
            return "PointerExitedWindow";
        }else if MATCHES(WindowResizeEvent){
            return "WindowResizeEvent \{ new_height: "+std::to_string(ev->new_height)+" , new_width: "+std::to_string(ev->new_width)+" }";
        }else if MATCHES(WindowMoveEvent){
            return "WindowMoveEvent \{ new_x: "+std::to_string(ev->new_x)+" , new_y: "+std::to_string(ev->new_y)+" }";
        }else{
            return "invalid";
        }
    },event_variant);
}

#ifdef VK_USE_PLATFORM_XCB_KHR
Window::Window(
    xcb_connection_t *xcb_connection,
    std::shared_ptr<VulkanContext> vulkan,
    int width,
    int height,
    int x,
    int y,
    int screen_index
):width(width),height(height),xcb_connection(xcb_connection),vulkan{vulkan}{
    window_handle=xcb_generate_id(xcb_connection);

    auto setup=xcb_get_setup(xcb_connection);
    auto screen_iter=xcb_setup_roots_iterator(setup);
    for(int screen_iter_index=0;screen_iter_index<screen_index;screen_iter_index++)
        xcb_screen_next(&screen_iter);
    auto screen=screen_iter.data;

    int value_mask=XCB_CW_EVENT_MASK;
    std::vector<int> value_list{
        XCB_EVENT_MASK_EXPOSURE
        // | XCB_EVENT_MASK_RESIZE_REDIRECT
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
        | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
        | XCB_EVENT_MASK_BUTTON_PRESS
        | XCB_EVENT_MASK_BUTTON_RELEASE
        | XCB_EVENT_MASK_ENTER_WINDOW
        | XCB_EVENT_MASK_LEAVE_WINDOW
        | XCB_EVENT_MASK_KEY_PRESS
        | XCB_EVENT_MASK_KEY_RELEASE
        | XCB_EVENT_MASK_POINTER_MOTION
        | XCB_EVENT_MASK_VISIBILITY_CHANGE
        | XCB_EVENT_MASK_EXPOSURE
        | XCB_EVENT_MASK_FOCUS_CHANGE
    };

    auto create_window_cookie=xcb_create_window_checked(
        xcb_connection, 
        XCB_COPY_FROM_PARENT, 
        window_handle, 
        screen->root, 
        x, 
        y, 
        width, 
        height, 
        10, 
        XCB_WINDOW_CLASS_INPUT_OUTPUT, 
        screen->root_visual, 
        value_mask, 
        value_list.data()
    );
    auto xcb_error=xcb_request_check(xcb_connection,create_window_cookie);
    if(xcb_error!=nullptr){
        std::cout<<"got xcb error "<<xcb_error->error_code<<std::endl;
    }

    xcb_map_window(xcb_connection, window_handle);

    wm_delete_atom=get_intern_atom("WM_DELETE_WINDOW");

    xcb_change_property(
        xcb_connection,
        XCB_PROP_MODE_REPLACE,
        window_handle,
        get_intern_atom("WM_PROTOCOLS"),
        XCB_ATOM_ATOM,
        32,
        1,
        &wm_delete_atom
    );

    flush();

    auto surface_create_info=VkXcbSurfaceCreateInfoKHR{
        VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        nullptr,
        0,
        xcb_connection,
        window_handle
    };
    auto res=vkCreateXcbSurfaceKHR(
        vulkan->instance,
        &surface_create_info,
        vulkan->allocator,
        &vk_surface
    );
    if(res!=VK_SUCCESS){
        throw VulkanError(VulkanErrorContext::CreateXCBSurface,res);
    }

    flush();

    if(is_non_temp_window()){
        create_swapchain();
    }
}
#endif

void Window::create_swapchain(){
    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        vulkan->physical_device,
        vk_surface,
        &surface_capabilities
    );

    uint32_t num_surface_present_modes=0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vulkan->physical_device,
        vk_surface,
        &num_surface_present_modes,
        nullptr
    );
    std::vector<VkPresentModeKHR> surface_present_modes(num_surface_present_modes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vulkan->physical_device,
        vk_surface,
        &num_surface_present_modes,
        surface_present_modes.data()
    );

    uint32_t num_surface_formats=0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vulkan->physical_device,
        vk_surface,
        &num_surface_formats,
        nullptr
    );
    std::vector<VkSurfaceFormatKHR> surface_formats(num_surface_formats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vulkan->physical_device,
        vk_surface,
        &num_surface_formats,
        surface_formats.data()
    );
    vk_swapchain_surface_format=surface_formats[0];

    auto old_swapchain_handle=vk_swapchain;
    auto swapchain_create_info=VkSwapchainCreateInfoKHR{
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        0,
        vk_surface,
        surface_capabilities.minImageCount,
        vk_swapchain_surface_format.format,
        vk_swapchain_surface_format.colorSpace,
        surface_capabilities.currentExtent,
        1,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        surface_capabilities.currentTransform,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        surface_present_modes[0],
        VK_TRUE,
        old_swapchain_handle
    };

    auto res=vkCreateSwapchainKHR(
        vulkan->device, 
        &swapchain_create_info, 
        vulkan->allocator, 
        &vk_swapchain
    );
    if(res!=VK_SUCCESS){
        throw VulkanError(VulkanErrorContext::CreateSwapchain,res);
    }

    if(old_swapchain_handle!=VK_NULL_HANDLE){
        vkDestroySwapchainKHR(vulkan->device,old_swapchain_handle,vulkan->allocator);
    }

    uint32_t num_swapchain_images=0;
    vkGetSwapchainImagesKHR(vulkan->device,vk_swapchain,&num_swapchain_images,nullptr);
    swapchain_images.resize(num_swapchain_images);
    vkGetSwapchainImagesKHR(vulkan->device,vk_swapchain,&num_swapchain_images,swapchain_images.data());

    #ifdef VK_USE_PLATFORM_XCB_KHR
        flush();
    #endif
}

#ifdef VK_USE_PLATFORM_XCB_KHR
xcb_atom_t Window::get_intern_atom(
    std::string atom_name,
    bool only_if_exists
)const{
    auto intern_cookie=xcb_intern_atom(
        /*c:*/ xcb_connection,
        /*only_if_exists:*/ only_if_exists,
        /*name_len:*/ atom_name.size(),
        /*name:*/ atom_name.c_str()
    );

    auto intern_reply=xcb_intern_atom_reply(
        /*c:*/ xcb_connection,
        /*cookie:*/ intern_cookie,
        /*e:*/ nullptr
    );

    auto intern_atom=intern_reply->atom;
    free(intern_reply);

    return intern_atom;
}
#endif

void Window::create_framebuffers(
    VkRenderPass render_pass
){
    destroy_image_views();
    destroy_framebuffers();

    vk_swapchain_image_views.resize(swapchain_images.size());
    vk_swapchain_framebuffers.resize(swapchain_images.size());

    for(int i=0;i<static_cast<int>(swapchain_images.size());i++){
        auto image_view_create_info=VkImageViewCreateInfo{
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            nullptr,
            0,
            swapchain_images[i],
            VK_IMAGE_VIEW_TYPE_2D,
            vk_swapchain_surface_format.format,
            VkComponentMapping{
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY
            },
            VkImageSubresourceRange{
                VK_IMAGE_ASPECT_COLOR_BIT,
                0,
                1,
                0,
                1
            }
        };
        auto res=vkCreateImageView(
            vulkan->device,
            &image_view_create_info,
            vulkan->allocator,
            &vk_swapchain_image_views[i]
        );
        if(res!=VK_SUCCESS){
            std::cout<<"failed to create image view"<<std::endl;
        }

        auto framebuffer_create_info=VkFramebufferCreateInfo{
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            nullptr,
            0,
            render_pass,
            1,
            &vk_swapchain_image_views[i],
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height),
            1
        };
        res=vkCreateFramebuffer(
            vulkan->device,
            &framebuffer_create_info,
            vulkan->allocator,
            &vk_swapchain_framebuffers[i]
        );
        if(res!=VK_SUCCESS){
            std::cout<<"failed to create image framebuffer"<<std::endl;
        }
    }

    #ifdef VK_USE_PLATFORM_XCB_KHR
        flush();
    #endif
}

#ifdef VK_USE_PLATFORM_XCB_KHR
std::vector<WindowEvent> Window::get_latest_events(){
    std::vector<WindowEventVariant> events;
    while(auto xcb_event=xcb_poll_for_event(xcb_connection)){
        auto event_type=xcb_event->response_type&0x7F;
        switch(event_type){
            case XCB_BUTTON_PRESS:{
                auto button_press_notify_event=*((xcb_button_press_event_t*)xcb_event);

                events.push_back(ButtonPressed{
                    button_press_notify_event.detail
                });
                break;
            }
            case XCB_BUTTON_RELEASE:{
                auto button_release_notify_event=*((xcb_button_release_event_t*)xcb_event);

                events.push_back(ButtonReleased{
                    button_release_notify_event.detail
                });
                break;
            }

            case XCB_FOCUS_IN:{
                events.push_back(WindowGainedFocus{});
                break;
            }
            case XCB_FOCUS_OUT:{
                events.push_back(WindowLostFocus{});
                break;
            }

            case XCB_KEY_PRESS:{
                auto key_release_notify_event=*((xcb_key_press_event_t*)xcb_event);
                events.push_back(KeyPressed{
                    key_release_notify_event.detail
                });
                break;
            }
            case XCB_KEY_RELEASE:{
                auto key_release_notify_event=*((xcb_key_release_event_t*)xcb_event);
                events.push_back(KeyReleased{
                    key_release_notify_event.detail
                });
                break;
            }

            case XCB_MOTION_NOTIFY:{
                auto motion_notify_event=*((xcb_motion_notify_event_t*)xcb_event);
                
                events.push_back(PointerMoved{
                    static_cast<float>(motion_notify_event.event_x),
                    static_cast<float>(motion_notify_event.event_y),
                });
                break;
            }

            case XCB_CLIENT_MESSAGE:{
                auto client_message_event=*((xcb_client_message_event_t*)xcb_event);
            
                if(client_message_event.data.data32[0]==wm_delete_atom){
                    events.push_back(WindowCloseEvent{
                        window_handle
                    });
                }
                break;
            }

            case XCB_ENTER_NOTIFY:{
                events.push_back(PointerEnteredWindow{});
                break;
            }
            case XCB_LEAVE_NOTIFY:{
                events.push_back(PointerExitedWindow{});
                break;
            }

            case XCB_CONFIGURE_NOTIFY:{
                auto configure_notify_event=*((xcb_configure_notify_event_t*)xcb_event);

                if(configure_notify_event.width!=width || configure_notify_event.height!=height){
                    events.push_back(WindowResizeEvent{
                        configure_notify_event.width,
                        configure_notify_event.height
                    });

                    width=configure_notify_event.width;
                    height=configure_notify_event.height;
                }

                if(configure_notify_event.x!=screen_x || configure_notify_event.y!=screen_y){
                    events.push_back(WindowMoveEvent{
                        configure_notify_event.x,
                        configure_notify_event.y
                    });

                    screen_x=configure_notify_event.x;
                    screen_y=configure_notify_event.y;
                }

                break;
            }
            
            case XCB_EXPOSE:
            case XCB_KEYMAP_NOTIFY:
            case XCB_GRAPHICS_EXPOSURE:
            case XCB_NO_EXPOSURE:
            case XCB_VISIBILITY_NOTIFY:
            case XCB_CREATE_NOTIFY:
            case XCB_DESTROY_NOTIFY:
            case XCB_UNMAP_NOTIFY:
            case XCB_MAP_NOTIFY:
            case XCB_MAP_REQUEST:
            case XCB_REPARENT_NOTIFY:
            case XCB_CONFIGURE_REQUEST:
            case XCB_GRAVITY_NOTIFY:
            // this is the resize request, notably distinct from the resize event
            case XCB_RESIZE_REQUEST:
            case XCB_CIRCULATE_NOTIFY:
            case XCB_CIRCULATE_REQUEST:
            case XCB_PROPERTY_NOTIFY:
            case XCB_SELECTION_CLEAR:
            case XCB_SELECTION_REQUEST:
            case XCB_SELECTION_NOTIFY:
            case XCB_COLORMAP_NOTIFY:
            case XCB_MAPPING_NOTIFY:
            case XCB_GE_GENERIC:
                break;

            default:
                std::cout<<"got unhandled event "<<event_type<<std::endl;
        }
    }

    std::vector<WindowEvent> window_events;
    for(auto event_variant:events){
        window_events.push_back(WindowEvent(event_variant));
    }
    return window_events;
}
#endif

#ifdef VK_USE_PLATFORM_METAL_EXT

#include<objc/objc.h>

std::vector<WindowEvent> Window::get_latest_events(){
    std::vector<WindowEvent> ret{};
    return ret;
}
#endif

Window::~Window(){
    if(is_non_temp_window()){
        destroy_image_views();
        destroy_framebuffers();

        vkDestroySwapchainKHR(
            vulkan->device,
            vk_swapchain,
            vulkan->allocator
        );
    }

    vkDestroySurfaceKHR(
        vulkan->instance, 
        vk_surface, 
        vulkan->allocator
    );

    platform_destroy();
}
#ifdef VK_USE_PLATFORM_XCB_KHR
void Window::platform_destroy(){
    xcb_unmap_window(xcb_connection, window_handle);
    xcb_destroy_window(xcb_connection, window_handle);

    flush();
}
#endif
