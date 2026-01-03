#version 410 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoords;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

uniform float time;

// controale globale (din C++)
uniform float amplitude;  // 0.10 .. 0.35 (depinde de scale)
uniform float frequency;  // 0.6  .. 2.5
uniform float speed;      // 0.6  .. 3.0

// wave: sin( dot(dir, xz)*k + t*w + phase )
float wave(vec2 xz, vec2 dir, float k, float w, float phase)
{
    dir = normalize(dir);
    return sin(dot(xz, dir) * k + time * w + phase);
}
float heightAt(vec2 xz)
{
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
}

