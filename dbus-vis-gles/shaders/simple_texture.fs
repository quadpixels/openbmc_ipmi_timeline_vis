#version 300 es

precision mediump float;

in vec2 UV;
out vec4 color;
uniform sampler2D tex;

void main() {
  color = texture(tex, UV);
}