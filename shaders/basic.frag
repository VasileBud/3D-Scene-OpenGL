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

// directional light
uniform vec3 lightDir;
uniform vec3 lightColor;

// ship transform for lamp placement
uniform mat4 shipModelMatrix;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

// lighting parameters
const float ambientStrength = 0.2;
const float specularStrength = 0.5;
const float shininess = 32.0;

void computeDirLight(vec3 normalEye, vec3 viewDir, out vec3 ambient, out vec3 diffuse, out vec3 specular)
{
    vec3 lightDirEye = normalize(vec3(view * vec4(lightDir, 0.0)));

    ambient = ambientStrength * lightColor;
    diffuse = max(dot(normalEye, lightDirEye), 0.0) * lightColor;

    vec3 reflectDir = reflect(-lightDirEye, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    specular = specularStrength * specCoeff * lightColor;
}

vec3 computePointLight(vec3 lampPosWorld, vec3 lampColor, vec3 fPosEye, vec3 normalEye, vec3 viewDir)
{
    vec3 lampPosEye = vec3(view * vec4(lampPosWorld, 1.0));
    vec3 lightDirEye = normalize(lampPosEye - fPosEye);

    float diff = max(dot(normalEye, lightDirEye), 0.0);
    vec3 reflectDir = reflect(-lightDirEye, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    float dist = length(lampPosEye - fPosEye);
    float att = 1.0 / (1.0 + 0.025 * dist + 0.0015 * dist * dist);

    vec3 amb = 0.22 * lampColor;
    vec3 dif = diff * lampColor;
    vec3 spc = specularStrength * specCoeff * lampColor;

    return (amb + dif + spc) * att;
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
    float fogDensity = 0.00020f;
    float fogStart = 1200.0f;
    float fogMinFactor = 0.45f;
    float fragmentDistance = length(fPosEye);
    float fogDistance = max(fragmentDistance - fogStart, 0.0);
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
    vec3 color = (ambient + (1.0f - shadow) * diffuse) * baseColor +
    (1.0f - shadow) * specular * specMap;

    vec3 lampLocal1 = vec3(2.6f, 9.9f, -30.1f);
    vec3 lampLocal2 = vec3(-2.6f, 9.9f, -30.1f);
    vec3 lampPos1 = vec3(shipModelMatrix * vec4(lampLocal1, 1.0f));
    vec3 lampPos2 = vec3(shipModelMatrix * vec4(lampLocal2, 1.0f));
    vec3 lampColor1 = vec3(12.0f, 9.6f, 7.2f);
    vec3 lampColor2 = vec3(12.0f, 9.6f, 7.2f);
    color += computePointLight(lampPos1, lampColor1, fPosEye, normalEye, viewDir) * baseColor;
    color += computePointLight(lampPos2, lampColor2, fPosEye, normalEye, viewDir) * baseColor;

    float fogFactor = computeFog(fPosEye);
    vec3 fogColor = vec3(0.56f, 0.59f, 0.64f);
    vec3 finalColor = mix(fogColor, color, fogFactor);
    fColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
}
