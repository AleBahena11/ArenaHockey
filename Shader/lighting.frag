#version 330 core
// Shader de iluminación con luz direccional (modelo de Phong) y textura.
// En la iteración 4 se le agregan las luces puntuales y el reflector de Iluminación II.

struct Material
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight
{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform Material material;
uniform int transparency;

// Prototipos
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Luz direccional
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    // El alfa sale de la textura (el cristal es semitransparente)
    color = vec4(result, texture(material.diffuse, TexCoords).a);
    if (color.a < 0.1 && transparency == 1)
        discard;   // huecos de la red
}

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // Difusa
    float diff = max(dot(normal, lightDir), 0.0);
    // Especular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // Resultado
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
    return (ambient + diffuse + specular);
}
