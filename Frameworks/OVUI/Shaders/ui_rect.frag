#version 460

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    vec2 viewportSize;
} pc;

void main() {
    outColor = fragColor;
}
