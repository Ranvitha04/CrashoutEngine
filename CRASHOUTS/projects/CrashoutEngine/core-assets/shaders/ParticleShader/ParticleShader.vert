#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

layout (location = 3) in vec3 aInstancePos;
layout (location = 4) in vec4 aInstanceColor;
layout (location = 5) in float aInstanceRot;
layout (location = 6) in float aInstanceScale;
layout (location = 7) in float aInstanceWidth;
layout (location = 8) in float aInstanceHeight;
layout (location = 9) in vec2  aInstanceOffset;

out vec4 color;
out vec2 texCoord;
out vec2 vWorldPos;
out float vWave;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec2 u_uvSize;

uniform float u_time;
uniform vec3 u_gravity;
uniform vec4 u_startColor;
uniform vec4 u_endColor;
uniform float u_sizeOverTime;
uniform float u_rotationOverTime;
uniform float u_flowStrength;
uniform float u_flowSpeed;
uniform float u_flowFrequency;
uniform int u_renderShape;
uniform float u_squareSizeMultiplier;
uniform float u_circleRadiusMultiplier;
uniform float u_triangleWidthMultiplier;
uniform float u_triangleHeightMultiplier;
uniform int u_useLocalSpace;
uniform vec3 u_parentPos;
uniform float u_parentRotDeg;
uniform vec2 u_parentScale;

void main() {
    float spawnTime = aInstanceScale;
    float maxLife = max(0.0001, aInstanceColor.a);
    float age = u_time - spawnTime;

    float alive = 1.0;
    if (age < 0.0 || age > maxLife) {
        alive = 0.0;
    }

    float t = clamp(age / maxLife, 0.0, 1.0);

    vec4 startC = vec4(u_startColor.rgb * aInstanceColor.rgb, u_startColor.a);
    vec4 endC = vec4(u_endColor.rgb * aInstanceColor.rgb, u_endColor.a);
    vec4 cNow = mix(startC, endC, t);
    cNow.a *= alive;

    vec3 basePos = aInstancePos;
    vec2 vel = aInstanceOffset;
    vec2 disp = vel * age + 0.5 * u_gravity.xy * (age * age);
    basePos.xy += disp;

    float surfaceIndex = float(gl_InstanceID);
    float wavePhase = u_time * max(0.0, u_flowSpeed) + surfaceIndex + basePos.x * u_flowFrequency;
    float waveSin = sin(wavePhase);
    basePos.y += waveSin * u_flowStrength;
    basePos.x += cos(wavePhase) * (0.35 * u_flowStrength);
    basePos.x += sin(age * 3.7 + surfaceIndex) * (0.18 * u_flowStrength);
    basePos.y += cos(age * 4.1 + surfaceIndex) * (0.10 * u_flowStrength);
    vWave = 0.5 + 0.5 * waveSin;

    if (u_useLocalSpace == 1) {
        float pr = radians(u_parentRotDeg);
        float ps = sin(pr);
        float pc = cos(pr);
        mat2 prot = mat2(pc, ps, -ps, pc);
        basePos.xy = prot * basePos.xy + u_parentPos.xy;
        basePos.z += u_parentPos.z;
    }

    vec2 baseSize = vec2(aInstanceWidth, aInstanceHeight);
    if (u_useLocalSpace == 1) {
        baseSize *= u_parentScale;
    }

    baseSize += vec2(u_sizeOverTime * age);
    baseSize = max(baseSize, vec2(0.0));

    vec2 scaleXY = baseSize;
    if (u_renderShape == 0) {
        scaleXY *= u_squareSizeMultiplier;
    }
    else if (u_renderShape == 2) {
        scaleXY.x *= u_triangleWidthMultiplier;
        scaleXY.y *= u_triangleHeightMultiplier;
    }
    else {
        float r = 0.5 * min(scaleXY.x, scaleXY.y) * u_circleRadiusMultiplier;
        scaleXY = vec2(r);
    }

    float rotDeg = aInstanceRot + u_rotationOverTime * age;
    float rads = radians(rotDeg);
    float s = sin(rads);
    float c = cos(rads);
    mat2 rot = mat2(c, s, -s, c);

    vec2 local = aPos.xy * scaleXY;
    local = rot * local;

    vec3 worldPos = vec3(local, aPos.z) + basePos;
    gl_Position = uProjection * uView * uModel * vec4(worldPos, 1.0);

    vec2 uvSize = (u_uvSize.x > 0.0 && u_uvSize.y > 0.0) ? u_uvSize : vec2(1.0);
    texCoord = aTexCoord * uvSize;
    color = cNow;
    vWorldPos = worldPos.xy;
}
