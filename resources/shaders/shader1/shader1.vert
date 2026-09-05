#version 450

// Positions of vertecies
vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

// Actually set the positions of the vertecies by index
void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0); // the one is there just to make it a 4d vector
}