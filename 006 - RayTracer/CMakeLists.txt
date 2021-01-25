cmake_minimum_required (VERSION 3.8)

find_package(Stb REQUIRED)
find_package(tinyobjloader REQUIRED)

include("../CmakeMacros.txt")

set(CMAKE_CXX_STANDARD 17)

set(
   target_name
   "006RayTracer"
)

set(
   src_files
   "src/Box.h"
   "src/Box.cpp"
   "src/Instance.h"
   "src/Instance.cpp"
   "src/Material.h"
   "src/Model.h"
   "src/Model.cpp"
   "src/Offset.h"
   "src/RayTracer.h"
   "src/RayTracer.cpp"
   "src/Rectangle2D.cpp"
   "src/Scene.h"
   "src/Scene.cpp"
   "src/Sphere.h"
   "src/Sphere.cpp"
   "src/Texture.h"
   "src/Vertex.h"
)

set(
   shader_header_files
   "Assets/Shaders/Bindings.glsl"
   "Assets/Shaders/Constants.glsl"
   "Assets/Shaders/Material.glsl"
   "Assets/Shaders/Offset.glsl"
   "Assets/Shaders/Random.glsl"
   "Assets/Shaders/RayPayload.glsl"
   "Assets/Shaders/Scatter.glsl"
   "Assets/Shaders/Texture.glsl"
   "Assets/Shaders/UniformBufferObject.glsl"
   "Assets/Shaders/Vertex.glsl"
)

set(
   shader_src_files
   "Assets/Shaders/Box.rchit"
   "Assets/Shaders/Box.rint"
   "Assets/Shaders/Equirectangular2Cubemap.comp"
   "Assets/Shaders/RayTrace.rgen"
   "Assets/Shaders/RayTrace.rmiss"
   "Assets/Shaders/Sphere.rchit"
   "Assets/Shaders/Sphere.rint"
   "Assets/Shaders/Triangles.rchit"
)

set(
   font_files
)

set(
   model_files
   "Assets/Models/Backdrop.obj"
   "Assets/Models/Box.obj"
   "Assets/Models/ShaderBall.obj"
   "Assets/Models/Rectangle2D.obj"
   "Assets/Models/Sphere.obj"
   "Assets/Models/WineGlass.obj"
)

set(
   texture_files
   "Assets/Textures/Backdrop.png"
   "Assets/Textures/birchwood_4k.hdr"
   "Assets/Textures/earthmap.jpg"
)

set_source_files_properties(${shader_header_files} PROPERTIES HEADER_FILE_ONLY TRUE)

compile_shaders(shader_src_files shader_header_files Assets/Shaders compiled_shaders)
copy_assets(font_files Assets/Fonts copied_fonts)
copy_assets(model_files Assets/Models copied_models)
copy_assets(texture_files Assets/Textures copied_textures)

source_group("Assets/Fonts" FILES ${font_files})
source_group("Assets/Models" FILES ${model_files})
source_group("Assets/Textures" FILES ${texture_files})
source_group("src" FILES ${src_files})
source_group("Assets/Shaders" FILES ${shader_header_files} ${shader_src_files})

add_executable(${target_name} ${src_files} ${font_files} ${model_files} ${shader_header_files} ${shader_src_files} ${texture_files} )

target_include_directories(
   ${target_name} PRIVATE
   "Assets/Shaders"
)

# These arent really "source" files.
# This line is here to make target depend on the listed files (so that cmake will then build them)
# The "correct" way to do this is to add_custom_target() and then add_dependencies() on the custom target.
# I do not want to clutter up the project with a whole load of custom targets, however.
set_source_files_properties(${copied_fonts} ${copied_models} ${compiled_shaders} ${copied_textures} PROPERTIES GENERATED TRUE)
target_sources(${target_name} PRIVATE ${copied_fonts} ${copied_models} ${compiled_shaders} ${copied_textures})

target_link_libraries(
   ${target_name} PRIVATE
   tinyobjloader::tinyobjloader
   Vulkan
)
