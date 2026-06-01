#version 460

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inCornerRadius;
layout(location = 3) in vec4 inRectData;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    vec2 viewportSize;
} pc;

void main() {
    vec2 clip = (inPosition / pc.viewportSize) * 2.0 - 1.0;
    clip.y = -clip.y;
    gl_Position = vec4(clip, 0.0, 1.0);
    fragColor = inColor;
}
