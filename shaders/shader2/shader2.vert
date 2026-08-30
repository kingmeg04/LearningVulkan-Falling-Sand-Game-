#version 450

// Positions of vertices
const vec2 positions[4] = vec2[](
    vec2(-0.5, -0.5),  // top-left
    vec2( 0.5, -0.5),  // top-right
    vec2(-0.5,  0.5),  // bottom-left
    vec2( 0.5,  0.5)   // bottom-right
);

// Colors of vertices
const vec3 colors[4] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 1.0, 0.0)
);

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}