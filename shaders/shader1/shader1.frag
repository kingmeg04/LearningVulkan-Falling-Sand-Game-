#version 450

// Location determines which interface "slot" the variable uses for other shaders to access
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}