cmake_minimum_required (VERSION 3.8)

include("../CmakeMacros.txt")

set(CMAKE_CXX_STANDARD 17)

set(
	target_name
	"001Triangle"
)

set(
	src_files
	"src/Triangle.h"
	"src/Triangle.cpp"
)

set(
	shader_header_files
 )

set(
	shader_src_files
	"Assets/Shaders/Triangle.vert"
	"Assets/Shaders/Triangle.frag"
)

set(
	font_files
)

set(
	model_files
)

set(
	texture_files
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

if (WIN32)
	target_compile_definitions(
		${target_name} PRIVATE
		UNICODE
		_UNICODE
		WIN32_LEAN_AND_MEAN
		NOMINMAX
	)
endif()


# These arent really "source" files.
# This line is here to make target depend on the listed files (so that cmake will then build them)
# The "correct" way to do this is to add_custom_target() and then add_dependencies() on the custom target.
# I do not want to clutter up the project with a whole load of custom targets, however.
set_source_files_properties(${copied_fonts} ${copied_models} ${compiled_shaders} ${copied_textures} PROPERTIES GENERATED TRUE)
target_sources(${target_name} PRIVATE ${copied_fonts} ${copied_models} ${compiled_shaders} ${copied_textures})

target_link_libraries(
	${target_name} PRIVATE
	Vulkan
)
