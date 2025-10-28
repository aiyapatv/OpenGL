#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1; // Model loader binds diffuse maps as this name
uniform vec3 lightDir; // directional light direction (world space)
uniform vec3 viewPos;

void main()
{
    vec3 color = texture(texture_diffuse1, TexCoords).rgb;
    // fallback if model has no texture (optional)
    if (color == vec3(0.0)) color = vec3(0.8);

    // simple diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 light = normalize(-lightDir);
    float diff = max(dot(norm, light), 0.0);

    vec3 ambient = 0.25 * color;
    vec3 diffuse = diff * color;

    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0);
}
