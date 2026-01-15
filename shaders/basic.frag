#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

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
    float att = 1.0 / (1.0 + 0.035 * dist + 0.0030 * dist * dist);

    vec3 amb = 0.22 * lampColor;
    vec3 dif = diff * lampColor;
    vec3 spc = specularStrength * specCoeff * lampColor;

    return (amb + dif + spc) * att;
}

void main()
{
    // compute moon dir light components
    computeDirLight();

    vec3 albedo  = texture(diffuseTexture,  fTexCoords).rgb;
    vec3 specMap = texture(specularTexture, fTexCoords).rgb;

    // base color from moon light + textures (your original formula)
    vec3 color = (ambient + diffuse) * albedo + specular * specMap;

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
