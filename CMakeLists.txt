cmake_minimum_required (VERSION 3.8)

project ("VulkanApps")

set(CMAKE_CXX_STANDARD 17)

add_subdirectory("Vulkan")
add_subdirectory("001 - Triangle")
add_subdirectory("002 - TexturedModel")
add_subdirectory("003 - Instancing")
add_subdirectory("004 - RasterSpheres")
add_subdirectory("005 - RayTraceSpheres")
add_subdirectory("006 - RayTracer")
