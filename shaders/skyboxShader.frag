#version 410 core

in vec3 textureCoordinates;
out vec4 color;

uniform samplerCube skybox;
uniform vec3 lightDir;

void main()
{
    vec3 dir = normalize(textureCoordinates);

    vec3 skyColor = texture(skybox, dir).rgb;
    vec3 hazeColor = vec3(0.56f, 0.59f, 0.64f);
    float hazeStrength = 0.55f;
    float hazeHeight = 0.65f;
    vec3 skyTint = vec3(0.003f, 0.006f, 0.015f);
    float skyTintStrength = 0.75f;

    skyColor = mix(skyColor, skyTint, clamp(skyTintStrength, 0.0, 1.0));

    float horizon = clamp(1.0 - abs(dir.y), 0.0, 1.0); // 0 sus, 1 orizont
    float haze = smoothstep(0.2, hazeHeight, horizon) * hazeStrength;

    float lightFactor = max(dot(dir, normalize(lightDir)), 0.0);
    haze *= mix(1.0, 0.7, lightFactor);

    skyColor = mix(skyColor, hazeColor, haze);

    float topDark = smoothstep(0.0, 0.8, clamp(dir.y, 0.0, 1.0));
    skyColor *= (1.0 - 0.25 * topDark);

    color = vec4(skyColor, 1.0);
}
