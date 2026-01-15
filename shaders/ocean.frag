#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

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
uniform sampler2D shadowMap;

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

float computeShadow()
{
    // perform perspective divide
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    normalizedCoords = normalizedCoords * 0.5 + 0.5;

    if (normalizedCoords.z > 1.0f)
    {
        return 0.0f;
    }

    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 lightDirEye = normalize(vec3(view * vec4(lightDir, 0.0)));
    float bias = max(0.05f * (1.0f - dot(normalEye, lightDirEye)), 0.005f);

    float currentDepth = normalizedCoords.z;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float closestDepth = texture(shadowMap, normalizedCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > closestDepth ? 1.0f : 0.0f;
        }
    }

    shadow /= 9.0f;
    return shadow;
}

void main()
{
    computeDirLight();

    vec3 albedo = texture(diffuseTexture,  fTexCoords).rgb;
    vec3 specMap = texture(specularTexture, fTexCoords).rgb;

    float shadow = computeShadow();
    vec3 color = ambient * albedo + (1.0f - shadow) * diffuse * albedo +
                 (1.0f - shadow) * specular * specMap;

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
