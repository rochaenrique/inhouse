#version 330 core

// VERTEX SHADER SIMPLE

layout (location = 0) in vec2 apos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aOffset;

out vec3 fColor;

void main()
{
  fColor = aColor;
  gl_position = vec4(apos + aOffset, 0.0, 1.0);
}