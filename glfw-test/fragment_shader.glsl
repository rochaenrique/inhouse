#version 330 core
out vec4 FragColor;

// FRAGMENT SHADER SIMPLE

in vec3
fColor;

void main()
{
  FragColor = vec4(fColor, 1.0);
}