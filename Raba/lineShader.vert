#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 normal;

out vec2 norm;

uniform mat4 mvpMatrix;

void main()
{
	vec3 o_position = vec3(position, 0.0f);
	gl_Position = mvpMatrix * vec4(o_position, 1.0f);
	norm = normal;
}