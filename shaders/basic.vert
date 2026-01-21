#version 410 core

// input attributes
layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

// outputs to fragment shader
out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
out vec4 fragPosLightSpace;

// matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceTrMatrix;

void main()
{
    vec4 worldPos = model * vec4(vPosition, 1.0f);

    fPosition = vPosition;
    fNormal = vNormal;
    fTexCoords = vTexCoords;

    gl_Position = projection * view * worldPos;
    fragPosLightSpace = lightSpaceTrMatrix * worldPos;
}
