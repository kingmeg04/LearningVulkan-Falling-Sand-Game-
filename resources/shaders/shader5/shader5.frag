#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    // Round, soft-edged points instead of squares
    vec2 coord = gl_PointCoord - vec2(0.5);
    outColor = vec4(fragColor.rgb, 0.5 - length(coord));
}