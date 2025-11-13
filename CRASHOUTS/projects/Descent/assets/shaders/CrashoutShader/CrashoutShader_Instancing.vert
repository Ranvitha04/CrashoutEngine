void main() {
    
    bool useInstancing = (aInstanceScale > 0.0) || (aInstanceRot != 0.0) || (length(aInstancePos) > 0.0);

    if (useInstancing) {
        vec2 scaleXY = (aInstanceWidth > 0.0 && aInstanceHeight > 0.0)
            ? vec2(aInstanceWidth, aInstanceHeight)
            : vec2(aInstanceScale);

        float r = radians(aInstanceRot);
        float c = cos(r);
        float s = sin(r);
        mat2 rot = mat2(c, -s,
                        s,  c);

        vec2 local = rot * (aPos.xy * scaleXY);
        vec3 worldPos = vec3(local, aPos.z) + aInstancePos;

        gl_Position = uProjection * uView * uModel * vec4(worldPos, 1.0);

        color = aInstanceColor;
        vec2 uvSize = (u_uvSize.x > 0.0 && u_uvSize.y > 0.0) ? u_uvSize : vec2(1.0);
        texCoord = aTexCoord * uvSize;
    } else {
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
        color = aColor;
        texCoord = aTexCoord;
    }
    \
}