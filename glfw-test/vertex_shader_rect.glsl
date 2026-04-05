#version 330 core

// VERTEX SHADER RECT

layout (location = 0) in vec2 dst_p0;
layout (location = 1) in vec2 dst_p1;
layout (location = 2) in vec2 src_p0;
layout (location = 3) in vec2 src_p1;

layout (location = 4) in vec4 color_bottom_left;
layout (location = 5) in vec4 color_bottom_right;
layout (location = 6) in vec4 color_top_left;
layout (location = 7) in vec4 color_top_right;

layout (location = 8) in float corner_radius;
layout (location = 9) in float edge_softness;
layout (location = 10) in float border_thickness;
layout (location = 11) in float texture_id;

out vec4 frag_color;
out vec2 frag_src_pos;
out float frag_texture_id;

out vec2 frag_dest_pos;
out vec2 frag_dest_center;
out vec2 frag_dest_half_size;

out float frag_corner_radius;
out float frag_edge_softness;
out float frag_border_thickness;

uniform vec2 u_resolution;

void main()
{
  vec2 vertices[] = vec2[](vec2(-1, -1),
                           vec2(-1, +1),
                           vec2(+1, -1),
                           vec2(+1, +1));
  
  vec4 corner_colors[] = vec4[](color_bottom_left,
                                color_top_left,
                                color_bottom_right,
                                color_top_right);
  
  vec2 dst_half_size = (dst_p1 - dst_p0) / 2.0;
  vec2 dst_center = (dst_p1 + dst_p0) / 2.0;
  vec2 dst_pos = (vertices[gl_VertexID] * dst_half_size + dst_center);
  
  vec2 src_half_size = (src_p1 - src_p0) / 2.0;
  vec2 src_center = (src_p1 + src_p0) / 2.0;
  vec2 src_pos = (vertices[gl_VertexID] * src_half_size + src_center);
  
  gl_Position = vec4(2 * dst_pos.x / u_resolution.x - 1,
                     2 * dst_pos.y / u_resolution.y - 1,
                     0, 
                     1);
  
  frag_color = corner_colors[gl_VertexID];
  frag_src_pos = src_pos;
  frag_texture_id = texture_id;
  
  frag_dest_pos = dst_pos;
  frag_dest_center = dst_center;
  frag_dest_half_size = dst_half_size;
  frag_corner_radius = corner_radius;
  frag_edge_softness = edge_softness;
  frag_border_thickness = border_thickness;
}