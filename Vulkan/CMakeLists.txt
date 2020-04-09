cmake_minimum_required (VERSION 3.8)

find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(Vulkan REQUIRED)

message(STATUS "using Vulkan library: ${Vulkan_LIBRARY}")

set(
	src_files
	"Application.h"
	"Application.cpp"
	"Buffer.h"
	"Buffer.cpp"
	"Core.h"
	"GeometryInstance.h"
	"Image.h"
	"Image.cpp"
	"Log.h"
	"Log.cpp"
	"Main.cpp"
	"QueueFamilyIndices.h"
	"SwapChainSupportDetails.h"
	"Utility.h"
	"Utility.cpp"
)

add_library(
	Vulkan
	${src_files}
)

target_compile_definitions(
	Vulkan PUBLIC
	"$<$<CONFIG:DEBUG>:ENABLE_ASSERTS>"
	VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
	FMT_HEADER_ONLY
	GLM_FORCE_DEPTH_ZERO_TO_ONE
	GLM_FORCE_RADIANS
)


target_include_directories(
	Vulkan PUBLIC
	.
	${Vulkan_INCLUDE_DIR}
)

target_link_libraries(
	Vulkan PUBLIC
	glm
	glfw
	spdlog::spdlog_header_only
	${Vulkan_LIBRARY}
)
