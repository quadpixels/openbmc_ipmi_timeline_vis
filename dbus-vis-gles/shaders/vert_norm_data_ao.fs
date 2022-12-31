#version 300 es

precision mediump float;

in vec3 vert_color;
in vec3 normal;
//in vec4 frag_pos_lightspace;

out     vec4 color;
uniform vec3 dir_light;

void main()
{
  //float shadow   = ShadowCalc(frag_pos_lightspace);
  //float strength = dot(-dir_light, normal);
  //strength = strength * 0.2f + 0.8f - 0.2f * shadow;
  //color = vec4(vert_color * strength, 1.0f);
  float strength = dot(-dir_light, normal);
  color = vec4(vert_color * strength, 1.0f);
}
