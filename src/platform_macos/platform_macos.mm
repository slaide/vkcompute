#include <cstdbool>
#include <cstdio>
#include <iostream>
#include <memory>
#include <objc/objc.h>
#include <ostream>
#include <string>
#include <optional>

#include <vulkan/vulkan.h>

#import <QuartzCore/QuartzCore.h>
#import <AppKit/AppKit.h>

#include <application.h>
#include <application/window.h>
#include <application/vulkan_context.h>

@interface MyView: NSView
    - (instancetype)init;
@end

@implementation MyView
    - (instancetype)init{
        self=[super init];

        self.wantsLayer=YES;

        return self;
    }
    - (CALayer *)makeBackingLayer{
        return [CAMetalLayer layer];
    }
    - (void)drawLayer:(CALayer *)layer inContext:(CGContextRef)ctx {
        NSRect rect = self.bounds;
        rect.origin.x = 0;
        rect.origin.y = 0;
        [self drawRect:rect];
    }
    - (void)drawRect:(NSRect)dirtyRect {
        printf("hello from within drawRect\n");
        [[NSColor redColor] setFill];
        NSRectFill(dirtyRect);
    }
@end

@interface MyWindow: NSWindow
    - (instancetype)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)style;
@end

@implementation MyWindow
    - (instancetype)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)style{
        self=[
            super
            initWithContentRect:contentRect
            styleMask:style
            backing:NSBackingStoreBuffered
            defer:NO
        ];
    
        self.contentView=[[MyView alloc] init];

        return self;
    }
@end

Window::Window(
    std::shared_ptr<VulkanContext> vulkan,
    int width,
    int height,
    int x,
    int y,
    int screen_index
):width(width),height(height),vulkan{vulkan}{
    MyWindow *window=[
        [MyWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, static_cast<CGFloat>(width), static_cast<CGFloat>(height))
        styleMask:NSWindowStyleMaskClosable|NSWindowStyleMaskTitled
    ];

    [window center];
    [window setTitle:@"my window title"];
    [window orderFrontRegardless];
    // from docs: Moves the window to the front of the screen list, within its level, and makes it the key window; that is, it shows the window.
    [window makeKeyAndOrderFront:nil];

    auto metal_surface_create_info=VkMetalSurfaceCreateInfoEXT{
        VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
        nullptr,
        0,
        static_cast<CAMetalLayer*>(window.contentView.layer)
    };
    auto res=vkCreateMetalSurfaceEXT(vulkan->instance,&metal_surface_create_info,vulkan->allocator,&this->vk_surface);
    if(res!=VK_SUCCESS){
        std::cout<<"failed vkCreateMetalSurfaceEXT with "<<res<<std::endl;
    }

    this->window_handle=static_cast<id>(window);

    if(is_non_temp_window()){
        create_swapchain();
    }
}
void Window::platform_destroy(){
    [static_cast<MyWindow*>(window_handle) close];
}

@interface MyAppDelegate : NSObject <NSApplicationDelegate> 

    @property(assign, nonatomic) std::shared_ptr<Application> app;
    @property(nonatomic) CVDisplayLinkRef display_link;

@end

CVReturn display_link_callback(
    CVDisplayLinkRef displayLink,
    const CVTimeStamp *inNow,
    const CVTimeStamp *inOutputTime,
    CVOptionFlags flagsIn,
    CVOptionFlags *flagsOut,
    void *displayLinkContext
){
    MyAppDelegate *app_delegate=static_cast<MyAppDelegate*>(displayLinkContext);
    app_delegate.app->run_step();
    return kCVReturnSuccess;
}

@implementation MyAppDelegate

    - (void)applicationDidFinishLaunching:(NSNotification *)notification{
        printf("app finished launching\n");

        self.app=std::make_shared<Application>();

        CVDisplayLinkRef display_link;
        CVDisplayLinkCreateWithActiveCGDisplays(&display_link);
        CVDisplayLinkSetOutputCallback(display_link, display_link_callback, self);
        CVDisplayLinkStart(display_link);

        self.display_link=display_link;
    }
    - (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
        return YES;
    }

@end

int main(int argc, char *argv[]){
    NSApplication *app=[NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyRegular];
    [app activateIgnoringOtherApps:YES];

    MyAppDelegate *myDelegate=[[MyAppDelegate alloc] init];
    [app setDelegate:myDelegate];

    [app run];
}