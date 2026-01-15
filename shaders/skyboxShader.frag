#version 410 core

in vec3 textureCoordinates;
out vec4 color;

uniform samplerCube skybox;

// controle
uniform vec3 hazeColor;
uniform float hazeStrength;
uniform float hazeHeight;
uniform vec3 skyTint;
uniform float skyTintStrength;
uniform vec3 lightDir; // pentru variatie usoara spre luna

void main()
{
    vec3 dir = normalize(textureCoordinates);

    // culoarea originala din cubemap
    vec3 skyColor = texture(skybox, dir).rgb;

    // tint global aplicat inainte de haze
    skyColor = mix(skyColor, skyTint, clamp(skyTintStrength, 0.0, 1.0));

    // haze pe orizont bazat pe directia privirii
    float horizon = clamp(1.0 - abs(dir.y), 0.0, 1.0); // 0 sus, 1 orizont
    float haze = smoothstep(0.2, hazeHeight, horizon) * hazeStrength;

    // variatie dupa lumina (luna): mai “ars” spre lightDir
    float lightFactor = max(dot(dir, normalize(lightDir)), 0.0);
    haze *= mix(1.0, 0.7, lightFactor);

    skyColor = mix(skyColor, hazeColor, haze);

    // intunecare subtila spre zenit pentru adancime
    float topDark = smoothstep(0.0, 0.8, clamp(dir.y, 0.0, 1.0));
    skyColor *= (1.0 - 0.25 * topDark);

    color = vec4(skyColor, 1.0);
}
