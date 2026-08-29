#version 450

layout(location = 0) out vec4 outColor;

void main() {
    // Use screen-space position instead of interpolated vertex color
    vec2 uv = gl_FragCoord.xy / vec2(800.0, 600.0);  // normalize by your WIDTH/HEIGHT
    outColor = vec4(uv, 0.5, 1.0);
}