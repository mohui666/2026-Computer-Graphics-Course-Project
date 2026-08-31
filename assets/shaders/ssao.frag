#version 330 core

in vec2 vUv;
out float FragOcclusion;

uniform sampler2D uDepth;
uniform mat4 uProjection;
uniform mat4 uInverseProjection;
uniform float uRadius;
uniform float uBias;

vec3 reconstructViewPosition(vec2 uv) {
    float depth = texture(uDepth, uv).r;
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view = uInverseProjection * clip;
    return view.xyz / max(view.w, 0.00001);
}

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main() {
    float centerDepth = texture(uDepth, vUv).r;
    if (centerDepth >= 0.99999) {
        FragOcclusion = 1.0;
        return;
    }

    vec3 position = reconstructViewPosition(vUv);
    vec3 normal = normalize(cross(dFdx(position), dFdy(position)));
    if (normal.z < 0.0) normal = -normal;
    float angle = hash12(gl_FragCoord.xy) * 6.2831853;
    vec3 randomDirection = normalize(vec3(cos(angle), sin(angle), 0.37));
    vec3 tangent = normalize(randomDirection - normal * dot(randomDirection, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 basis = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    const int sampleCount = 16;
    for (int index = 0; index < sampleCount; ++index) {
        float fi = (float(index) + 0.5) / float(sampleCount);
        float phi = float(index) * 2.3999632 + angle;
        float hemisphereZ = mix(0.18, 0.94, fract(fi * 7.13));
        float radial = sqrt(max(0.0, 1.0 - hemisphereZ * hemisphereZ));
        vec3 sampleVector = vec3(cos(phi) * radial, sin(phi) * radial, hemisphereZ);
        float scale = mix(0.12, 1.0, fi * fi);
        vec3 samplePosition = position + basis * sampleVector * (uRadius * scale);

        vec4 offset = uProjection * vec4(samplePosition, 1.0);
        vec2 sampleUv = offset.xy / max(offset.w, 0.00001) * 0.5 + 0.5;
        if (sampleUv.x <= 0.001 || sampleUv.x >= 0.999 ||
            sampleUv.y <= 0.001 || sampleUv.y >= 0.999) continue;
        float sampledDepth = texture(uDepth, sampleUv).r;
        if (sampledDepth >= 0.99999) continue;
        float sampledViewZ = reconstructViewPosition(sampleUv).z;
        float rangeWeight = smoothstep(0.0, 1.0,
            uRadius / max(abs(position.z - sampledViewZ), 0.0001));
        occlusion += (sampledViewZ >= samplePosition.z + uBias ? 1.0 : 0.0) * rangeWeight;
    }
    FragOcclusion = clamp(pow(1.0 - occlusion / float(sampleCount), 1.35), 0.28, 1.0);
}
