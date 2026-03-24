#version 330 core

// FRAGMENT SHADER RECT
in vec4 frag_color;
in vec2 frag_src_pos;
in float frag_texture_id;

in vec2 frag_dest_pos;
in vec2 frag_dest_center;
in vec2 frag_dest_half_size;
in float frag_corner_radius;
in float frag_edge_softness;

out vec4 out_color;

uniform sampler2D u_textures[16];

float rounded_rect_sdf(vec2 sample_pos, vec2 rect_center,
                       vec2 rect_half_size, float r)
{
  vec2 d2 = (abs(rect_center - sample_pos) - rect_half_size + vec2(r, r));
  float result = min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;
  return result;
}

void main()
{
  vec2 softness_padding = vec2(max(0, frag_edge_softness*2-1),
                               max(0, frag_edge_softness*2-1));
  
  float dist = rounded_rect_sdf(frag_dest_pos, 
                                frag_dest_center,
                                frag_dest_half_size - softness_padding,
                                frag_corner_radius);
  
  float sdf_factor = 1.f - smoothstep(0, 2*frag_edge_softness, dist);
  
  ivec2 texture_size = textureSize(u_textures[int(frag_texture_id)], 0);
  vec2 uv = vec2(frag_src_pos.x / texture_size.x, frag_src_pos.y / texture_size.y);
  vec4 sample = texture(u_textures[int(frag_texture_id)], uv);
  
  out_color = frag_color * sample * sdf_factor;
}