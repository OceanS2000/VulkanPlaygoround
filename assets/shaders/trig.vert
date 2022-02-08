#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec2 uv;

layout(push_constant) uniform constants {
    vec2 viewCenter;
};

void main() {
    gl_Position = vec4(inPosition - viewCenter, 0.0, 1.0);
    uv = vec2(inColor);
}
