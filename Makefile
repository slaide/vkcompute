# this script is best supplemented with the bear command to generate files usable by clangd

CXX = clang++
CXX_INCLUDES = -Iinclude
CXX_LINKS = -lvulkan
CCFLAGS = -std=c++20 -g # -O2 -flto=thin
CXX_DEFINES =

ifeq ($(shell uname -s),Linux)
	CXX_DEFINES += -DVK_USE_PLATFORM_XCB_KHR
	CXX_LINKS += -lxcb
else ifeq ($(shell uname -s),Darwin)
	CXX_INCLUDES += -I/opt/vulkansdk/macOS/include
	CXX_DEFINES += -DVK_USE_PLATFORM_METAL_EXT	
	CXX_LINKS += -framework Appkit -framework Metal -framework MetalKit -framework QuartzCore
endif

COMP = $(CXX) $(CCFLAGS) $(CXX_INCLUDES) $(CXX_DEFINES)

.PHONY: default
default: application

.PHONY: clean
clean:
	rm ./*.o

vulkan_error.o: src/application/vulkan_error.cpp
	$(COMP) -c -o vulkan_error.o src/application/vulkan_error.cpp
window.o: src/application/window.cpp
	$(COMP) -c -o window.o src/application/window.cpp
application.o: src/application.cpp
	$(COMP) -c -o application.o src/application.cpp

PLATFORM_MACOS_SRC = src/platform_macos/platform_macos.mm
platform.o: $(PLATFORM_MACOS_SRC)
	$(COMP) -c -o platform.o $(PLATFORM_MACOS_SRC)

application: application.o window.o vulkan_error.o platform.o
	$(COMP) $(CXX_LINKS) -o application platform.o application.o window.o vulkan_error.o

run: application
	./application