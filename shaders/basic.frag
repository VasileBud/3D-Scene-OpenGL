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

// moon directional light (existing)
uniform vec3 lightDir;
uniform vec3 lightColor;

// 2 lamp point lights (simple, lab-style)
uniform vec3 lampPos1;     // world space
uniform vec3 lampColor1;
uniform vec3 lampPos2;     // world space
uniform vec3 lampColor2;

// fog
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogStart;
uniform float fogMinFactor;

// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

// components (same style as yours)
vec3 ambient;
float ambientStrength = 0.2;

vec3 diffuse;
vec3 specular;
float specularStrength = 0.5;

void computeDirLight()
{
    // eye space coords
    vec4 fPosEye = view * model * vec4(fPosition, 1.0);
    vec3 normalEye = normalize(normalMatrix * fNormal);

    // light direction in eye space
    vec3 lightDirN = normalize(vec3(view * vec4(lightDir, 0.0)));

    // view direction (eye at origin in eye space)
    vec3 viewDir = normalize(-fPosEye.xyz);

    // ambient
    ambient = ambientStrength * lightColor;

    // diffuse
    diffuse = max(dot(normalEye, lightDirN), 0.0) * lightColor;

    // specular (Phong, same as your code)
    vec3 reflectDir = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    specular = specularStrength * specCoeff * lightColor;
}

vec3 computePointLight(vec3 lampPosWorld, vec3 lampColor, vec3 fPosEye, vec3 normalEye, vec3 viewDir)
{
    // lamp pos in eye space
    vec3 lampPosEye = vec3(view * vec4(lampPosWorld, 1.0));
    vec3 lightDirEye = normalize(lampPosEye - fPosEye);

    // diffuse
    float diff = max(dot(normalEye, lightDirEye), 0.0);

    // specular (Phong)
    vec3 reflectDir = reflect(-lightDirEye, normalEye);
    float specCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    // attenuation: con mic, dar fara sa dispara complet; distante scenei sunt mari
    float dist = length(lampPosEye - fPosEye);
    float att = 1.0 / (1.0 + 0.025 * dist +  0.0015 * dist * dist);

    vec3 amb = 0.22 * lampColor;
    vec3 dif = diff * lampColor;
    vec3 spc = specularStrength * specCoeff * lampColor;

    return (amb + dif + spc) * att;
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
    // compute moon dir light components
    computeDirLight();

    vec3 albedo  = texture(diffuseTexture,  fTexCoords).rgb;
    vec3 specMap = texture(specularTexture, fTexCoords).rgb;

    float shadow = computeShadow();
    vec3 color = (ambient + (1.0f - shadow) * diffuse) * albedo +
                 (1.0f - shadow) * specular * specMap;

    // compute eye-space data once
    vec3 fPosEye   = (view * model * vec4(fPosition, 1.0)).xyz;
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 viewDir   = normalize(-fPosEye);

    // add the 2 lamps as point lights
    color += computePointLight(lampPos1, lampColor1, fPosEye, normalEye, viewDir) * albedo;
    color += computePointLight(lampPos2, lampColor2, fPosEye, normalEye, viewDir) * albedo;

    // exponential squared fog in eye space
    float dist = length(fPosEye);
    float fogDist = max(dist - fogStart, 0.0);
    float fogFactor = exp(-fogDensity * fogDensity * fogDist * fogDist);
    fogFactor = clamp(fogFactor, fogMinFactor, 1.0);

    vec3 finalColor = mix(fogColor, color, fogFactor);
    finalColor = clamp(finalColor, 0.0, 1.0);
    fColor = vec4(finalColor, 1.0);
}
