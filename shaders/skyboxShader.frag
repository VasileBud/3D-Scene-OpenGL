#version 410 core

in vec3 textureCoordinates;
out vec4 color;

uniform samplerCube skybox;

// controle (le poti hardcoda sau seta din C++)
uniform vec3 hazeColor;     // ex: vec3(0.02, 0.03, 0.08)
uniform float hazeStrength; // ex: 0.6
uniform float hazeHeight;   // ex: 0.15 (latimea benzii)

void main()
{
    vec3 dir = normalize(textureCoordinates);
    vec3 sky = texture(skybox, dir).rgb;

    // dir.y: +1 sus, 0 orizont, -1 jos
    float y = dir.y;

    // banda la orizont: maxima pe y=0, scade spre sus/jos
    float horizon = 1.0 - smoothstep(0.0, hazeHeight, abs(y));

    // intuneca sus putin (noapte)
    float topDark = smoothstep(0.0, 0.8, clamp(y, 0.0, 1.0));

    vec3 withHaze = mix(sky, hazeColor, horizon * hazeStrength);

    // optional: mai intunecat spre zenit
    withHaze *= (1.0 - 0.35 * topDark);

    color = vec4(withHaze, 1.0);
}
