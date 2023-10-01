#version 300 es

precision mediump float;

layout (location=0) in vec3 vert_color;
layout (location=1) in vec3 normal;
layout (location=2) in vec4 frag_pos_lightspace;

out     vec4 color;
uniform vec3 dir_light;
uniform sampler2D shadow_map;

float ShadowCalc(vec4 frag) {
	vec3 xyz = frag.xyz / frag.w;
	xyz = xyz * 0.5 + 0.5;
	vec2 texel_size = vec2(textureSize(shadow_map, 0));
	
	int num_in_shadow = 0, num_samples = 0;
	
	for (int dy=-1; dy<=1; dy++) {
		for (int dx=-1; dx<=1; dx++) {
			vec2 sxy = xyz.xy;
			sxy += vec2(float(dx), float(dy))*1.5 / texel_size;		
			float closest_depth = 
				texture(shadow_map, sxy).r;
			float curr_depth    = xyz.z;
			
			const float bias = 0.006f;
			num_in_shadow += curr_depth - bias > closest_depth ? 1 : 0;
			num_samples += 1;
		}
	}
	
	float shadow = float(num_in_shadow) / float(num_samples);
	return shadow;
}

void main()
{
  float shadow   = ShadowCalc(frag_pos_lightspace);
  float strength = dot(-dir_light, normal);
  strength = strength * 0.2f + 0.8f - 0.2f * shadow;
  color = vec4(vert_color * strength, 1.0f);
}
