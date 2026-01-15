#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

out vec4 fColor;

// matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

// lighting
uniform vec3 lightDir;
uniform vec3 lightColor;

// fog
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogMinFactor;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

// components
vec3 ambient;
float ambientStrength = 0.20;

vec3 diffuse;
vec3 specular;
float specularStrength = 0.60;
float shininess = 64.0;

void computeDirLight()
{
    // eye space coords
    vec4 fPosEye = view * model * vec4(fPosition, 1.0);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    // light direction in eye space
    vec3 lightDirN = normalize(vec3(view * vec4(lightDir, 0.0)));

    // view direction (eye is origin in eye space)
    vec3 viewDir = normalize(-fPosEye.xyz);

    // ambient
    ambient = ambientStrength * lightColor;

    // diffuse
    float diff = max(dot(normalEye, lightDirN), 0.0);
    diffuse = diff * lightColor;

    // specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDirN + viewDir);
    float spec = pow(max(dot(normalEye, halfDir), 0.0), shininess);
    specular = specularStrength * spec * lightColor;
}

void main()
{
    computeDirLight();

    vec3 albedo = texture(diffuseTexture,  fTexCoords).rgb;
    vec3 specMap = texture(specularTexture, fTexCoords).rgb;

    vec3 color = ambient * albedo + diffuse * albedo + specular * specMap;

    // eye-space dist for fog
    vec3 fPosEye = (view * model * vec4(fPosition, 1.0)).xyz;
    float dist = length(fPosEye);
    float fogDist = max(dist - fogStart, 0.0);
    float fogFactor = exp(-fogDensity * fogDensity * fogDist * fogDist);
    fogFactor = clamp(fogFactor, fogMinFactor, 1.0);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 finalColor = mix(fogColor, color, fogFactor);
    fColor = vec4(finalColor, 1.0);
}
