#version 410 core

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;

out vec4 fColor;

uniform sampler2D diffuseTexture;

uniform vec3 moonColor = vec3(1.0);

void main()
{
    vec4 tex = texture(diffuseTexture, fTexCoords);

    // dacă textura are alpha și vrei să tai transparența:
    // if (tex.a < 0.1) discard;

    // self-lit simplu (luna rămâne vizibilă pe cer indiferent de lumină)
    vec3 color = tex.rgb * moonColor;

    fColor = vec4(color, tex.a);
}
