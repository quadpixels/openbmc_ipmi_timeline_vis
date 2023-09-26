#version 300 es

in vec2 aPos;
uniform vec3 color;

out vec3 vertexColor;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    vertexColor = color;
}