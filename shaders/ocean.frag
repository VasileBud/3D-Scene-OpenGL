#version 410 core

// inputs from vertex shader
in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

// output color
out vec4 fColor;

// matrices
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

// lighting
uniform vec3 lightDir;
uniform vec3 lightColor;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

// lighting parameters
const float ambientStrength = 0.20;
const float specularStrength = 0.60;
const float shininess = 64.0;

void computeDirLight(vec3 normalEye, vec3 viewDir, out vec3 ambient, out vec3 diffuse, out vec3 specular)
{
    vec3 lightDirEye = normalize(vec3(view * vec4(lightDir, 0.0)));

    ambient = ambientStrength * lightColor;
    diffuse = max(dot(normalEye, lightDirEye), 0.0) * lightColor;

    vec3 halfDir = normalize(lightDirEye + viewDir);
    float spec = pow(max(dot(normalEye, halfDir), 0.0), shininess);
    specular = specularStrength * spec * lightColor;
}

float computeShadow(vec3 normalEye)
{
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    normalizedCoords = normalizedCoords * 0.5 + 0.5;

    if (normalizedCoords.z > 1.0f)
    {
        return 0.0f;
    }

    vec3 lightDirEye = normalize(vec3(view * vec4(lightDir, 0.0)));
    float bias = max(0.05f * (1.0f - dot(normalEye, lightDirEye)), 0.005f);

    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
    float currentDepth = normalizedCoords.z;
    float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

    return shadow;
}

float computeFog(vec3 fPosEye)
{
    float fragmentDistance = length(fPosEye);
    float fogDensity = 0.00020f;
    float fogMinFactor = 0.45f;
    float fogDistance = max(fragmentDistance, 0.0);
    float fogFactor = exp(-fogDensity * fogDensity * fogDistance * fogDistance);
    fogFactor = clamp(fogFactor, fogMinFactor, 1.0);
    return clamp(fogFactor, 0.0, 1.0);
}

void main()
{
    vec3 fPosEye = (view * model * vec4(fPosition, 1.0)).xyz;
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 viewDir = normalize(-fPosEye);

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    computeDirLight(normalEye, viewDir, ambient, diffuse, specular);

    vec3 baseColor = texture(diffuseTexture, fTexCoords).rgb;
    vec3 specMap = texture(specularTexture, fTexCoords).rgb;

    float shadow = computeShadow(normalEye);
    vec3 color = ambient * baseColor + (1.0f - shadow) * diffuse * baseColor +
                 (1.0f - shadow) * specular * specMap;

    float fogFactor = computeFog(fPosEye);
    vec3 fogColor = vec3(0.56f, 0.59f, 0.64f);
    vec3 finalColor = mix(fogColor, color, fogFactor);
    fColor = vec4(finalColor, 1.0);
}
