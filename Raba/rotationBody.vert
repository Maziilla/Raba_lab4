#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

uniform mat4 mMatrix;
uniform mat4 mvpMatrix;
uniform mat3 normalMatrix;

out vec3 Normal;
out vec3 FragPosition;

void main()
{
	gl_Position = mvpMatrix * vec4(position, 1.0f);

	FragPosition = vec3(mMatrix * vec4(position, 1.0f));
    Normal = normalMatrix * normal; 

	gl_Position = mvpMatrix * vec4(position, 1.0f);
}