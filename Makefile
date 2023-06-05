# this script is best supplemented with the bear command to generate files usable by clangd

CXX = clang++
CXX_INCLUDES = -Iinclude
CXX_LINKS = -lvulkan
CXX_FLAGS_DEBUG = -std=c++20 -g
CXX_FLAGS_RELEASE = -std=c++20 -O3 -flto=full
CXX_DEFINES =

ifdef release
	CXX_FLAGS = $(CXX_FLAGS_RELEASE)
else
	CXX_FLAGS = $(CXX_FLAGS_DEBUG)
endif

ifeq ($(shell uname -s),Linux)
	CXX_DEFINES += -DVK_USE_PLATFORM_XCB_KHR
	CXX_LINKS += -lxcb
else ifeq ($(shell uname -s),Darwin)
	CXX_INCLUDES += -I/opt/vulkansdk/macOS/include
	CXX_DEFINES += -DVK_USE_PLATFORM_METAL_EXT	
	CXX_LINKS += -framework Appkit -framework Metal -framework MetalKit -framework QuartzCore
endif

COMP = $(CXX) $(CXX_FLAGS) $(CXX_INCLUDES) $(CXX_DEFINES)

.PHONY: default
default: run

.PHONY: clean
clean:
	$(RM) *.o application *.spv

build_shaders: vertex_shader.vert fragment_shader.frag
	glslangValidator vertex_shader.vert -V -o vertex_shader.spv
	glslangValidator fragment_shader.frag -V -o fragment_shader.spv

vulkan_error.o: src/application/vulkan_error.cpp
	$(COMP) -c -o vulkan_error.o src/application/vulkan_error.cpp
window.o: src/application/window.cpp
	$(COMP) -c -o window.o src/application/window.cpp
application.o: src/application.cpp
	$(COMP) -c -o application.o src/application.cpp


ifeq ($(shell uname -s),Linux)

PLATFORM_LINUX_SRC = src/platform_linux/platform_linux.cpp
platform.o: $(PLATFORM_LINUX_SRC)
	$(COMP) -c -o platform.o $(PLATFORM_LINUX_SRC)

else ifeq ($(shell uname -s),Darwin)

PLATFORM_MACOS_SRC = src/platform_macos/platform_macos.mm
platform.o: $(PLATFORM_MACOS_SRC)
	$(COMP) -c -o platform.o $(PLATFORM_MACOS_SRC)

endif

application: application.o window.o vulkan_error.o platform.o
	$(COMP) $(CXX_LINKS) -o application platform.o application.o window.o vulkan_error.o

.PHONY: build
build: application build_shaders

run: build
	./application