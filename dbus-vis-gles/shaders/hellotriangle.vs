#version 300 es

in vec3 aPos;
in vec3 aColor;

out vec4 vertexColor;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    vertexColor = vec4(aColor.r, aColor.g, aColor.b, 1) ;
}