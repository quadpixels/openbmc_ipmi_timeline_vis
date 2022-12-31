#version 300 es

in vec3 position;
out vec2 UV;

void main() {
  gl_Position = vec4(position, 1);
  UV = (position.xy + vec2(1,1)) / 2.0;
}