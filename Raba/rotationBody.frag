#version 330 core

out vec4 color;

in vec3 Normal;  
in vec3 FragPosition;  

uniform vec3 viewPosition;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 lightPosition;

void main()
{
	vec3 norm = normalize(Normal);
	vec3 lightDirection = normalize(lightPosition - FragPosition);
	vec3 viewDirection = normalize(viewPosition - FragPosition);

	float diff = max(dot(norm, lightDirection), 0.0);
	vec3 diffuse = diff * lightColor;
    
	float specularStrength = 0.5f;
	vec3 reflectDirection = reflect(-lightDirection, norm);  
	float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), 16);
	if (dot(viewDirection, norm) < 0)
		spec = 0.0f;
	vec3 specular;

	specular = specularStrength * spec * lightColor;  
        
	vec3 result = (diffuse + specular) * objectColor;
	color = vec4(result, 1.0f);
} 