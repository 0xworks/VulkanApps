# VulkanApps
A personal project following along my adventures into the Vulkan graphics API.
From "hello triangle" up to ray tracing.

## Project structure
The "Vulkan" directory contains the "framework" for building an application that renders something using the Vulkan API.
The various 001, 002, etc, directories are applications that use the framework, in increasing order of complexity.

The framework, and example applications are based heavily on the following excellent resources:
* https://vulkan-tutorial.com/
* https://github.com/SaschaWillems/Vulkan
* https://github.com/GPSnoopy/RayTracingInVulkan
* https://raytracing.github.io/

## Building
I have tested this on Windows only.  Other platforms may work with some adaptation of the cmake files.

You first need to install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home).  I am currently using Vulkan SDK 1.2.141.2 - other versions may (or may not) work.

For other dependencies, I use [vcpkg](https://github.com/Microsoft/vcpkg), but other means of installing them should also work (so long as cmake can find them via find_package(...))

The dependencies are:
 * [glm](https://glm.g-truc.net/0.9.8/index.html)   (vckpg install glm)
 * [glfw3](https://www.glfw.org/)   (vcpkg install glfw3)
 * [spdlog](https://github.com/gabime/spdlog)   (vcpkg install spdlog)
 * [stb](https://github.com/nothings/stb)    (vcpkg install stb)
 * [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)   (vcpkg install tinyobjloader)

## Screenshots
<img src="https://github.com/freeman40/VulkanApps/blob/master/Screenshots/Balls.png" width="49%" /><img src="https://github.com/freeman40/VulkanApps/blob/master/Screenshots/RayTracingTheNextWeekFinal.png" width="49%" />
<img src="https://github.com/freeman40/VulkanApps/blob/master/Screenshots/WineGlass.png" />
<img src="https://github.com/freeman40/VulkanApps/blob/master/Screenshots/001%20-%20Triangle.png" width="49%" /><img src="https://github.com/freeman40/VulkanApps/blob/master/Screenshots/002%20-%20TexturedModel.png" width="49%" />
