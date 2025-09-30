#version 330 core
out vec4 FragColor;

struct DirLight { vec3 direction; vec3 ambient; vec3 diffuse; vec3 specular; };
struct PointLight {
    vec3 position;
    float constant; float linear; float quadratic;
    vec3 ambient; vec3 diffuse; vec3 specular;
};
struct SpotLight {
    vec3 position; vec3 direction; float cutOff; float outerCutOff;
    float constant; float linear; float quadratic;
    vec3 ambient; vec3 diffuse; vec3 specular;
};

#define NR_POINT_LIGHTS 4

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
flat in vec3 InstOffset;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform float material_shininess;

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor);

vec3 hsv2rgb(vec3 c)
{
    vec3 rgb = clamp( abs(mod(c.x*6.0 + vec3(0.0,4.0,2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0 );
    rgb = rgb*rgb*(3.0 - 2.0*rgb);
    return c.z * mix(vec3(1.0), rgb, c.y);
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    float height = InstOffset.y;
    float hue = fract(0.12 * InstOffset.x + 0.08 * InstOffset.z + 0.07 * height + 0.35);
    float sat = clamp(0.5 + 0.6 * height, 0.15, 1.0);
    float val = clamp(0.6 + 0.5 * sin(2.1 * height + InstOffset.x * 0.2), 0.2, 1.0);
    vec3 baseColor = hsv2rgb(vec3(hue, sat, val));

    vec3 result = vec3(0.0);
    result += CalcDirLight(dirLight, norm, viewDir, baseColor);

    for (int i = 0; i < NR_POINT_LIGHTS; ++i)
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, baseColor);

    result += CalcSpotLight(spotLight, norm, FragPos, viewDir, baseColor);

    result = clamp(result + 0.04 * baseColor, 0.0, 1.0);
    FragColor = vec4(result, 1.0);
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
    vec3 ambient = light.ambient * baseColor;
    vec3 diffuse = light.diffuse * diff * baseColor;
    vec3 specularColor = 0.18 * baseColor + 0.22; // small tint toward baseColor, keep some neutral
    vec3 specular = light.specular * spec * specularColor;
    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 ambient = light.ambient * baseColor;
    vec3 diffuse = light.diffuse * diff * baseColor;
    vec3 specularColor = 0.18 * baseColor + 0.22;
    vec3 specular = light.specular * spec * specularColor;
    ambient *= attenuation; diffuse *= attenuation; specular *= attenuation;
    return ambient + diffuse + specular;
}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    vec3 ambient = light.ambient * baseColor;
    vec3 diffuse = light.diffuse * diff * baseColor;
    vec3 specularColor = 0.18 * baseColor + 0.22;
    vec3 specular = light.specular * spec * specularColor;
    ambient *= attenuation * intensity; diffuse *= attenuation * intensity; specular *= attenuation * intensity;
    return ambient + diffuse + specular;
}
