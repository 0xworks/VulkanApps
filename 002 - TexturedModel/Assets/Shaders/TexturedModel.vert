#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 MVP;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTextureCoordinate;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTextureCoordinate;

void main() {
    gl_Position = ubo.MVP * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTextureCoordinate = inTextureCoordinate;
}
