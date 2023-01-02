#version 300 es

in vec3 position;

// For this pass, M is an identity matrix.
uniform mat4 V;
uniform mat4 P;

void main() {
  gl_Position = P * V * vec4(position, 1.0f);
}