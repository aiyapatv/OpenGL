#version 330 core
out vec4 FragColor;

uniform vec3 lightColor;
uniform float intensity;

void main()
{
    vec3 col = lightColor * intensity;
    FragColor = vec4(col, 1.0);
}
