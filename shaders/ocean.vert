#version 410 core

// input attributes
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoords;

// outputs to fragment shader
out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
out vec4 fragPosLightSpace;

// matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform mat4 lightSpaceTrMatrix;

// time
uniform float time;

float heightAt(vec2 xz)
{
    float amplitude = 0.2f;
    float frequency = 0.7f;
    float speed = 2.6f;
    float swell = sin(dot(xz, normalize(vec2(1.0, 0.3))) * (frequency * 0.6) + time * (speed * 0.8));
    float ripples = sin(dot(xz, normalize(vec2(-0.2, 1.0))) * (frequency * 2.5) + time * (speed * 2.2));
    return amplitude * (0.85 * swell + 0.15 * ripples);
}

void main()
{
    vec3 pos = vPosition;
    vec2 xz = pos.xz;

    float height = heightAt(xz);
    vec3 baseN = normalize(vNormal);
    pos += baseN * height;

    float eps = 0.15;
    float h_dx = heightAt(xz + vec2(eps, 0.0));
    float h_dz = heightAt(xz + vec2(0.0, eps));

    float dhdx = (h_dx - height) / eps;
    float dhdz = (h_dz - height) / eps;
    vec3 n = normalize(vec3(-dhdx, 1.0, -dhdz));

    fPosition = pos;
    fNormal = n;
    fTexCoords = vTexCoords;

    gl_Position = projection * view * model * vec4(pos, 1.0);
    fragPosLightSpace = lightSpaceTrMatrix * model * vec4(pos, 1.0);
}

