#version 330 core

in vec2 vUv;
out float FragOcclusion;

uniform sampler2D uOcclusion;
uniform sampler2D uDepth;
uniform bool uHorizontal;

void main() {
    vec2 aoTexel = 1.0 / vec2(textureSize(uOcclusion, 0));
    vec2 axis = uHorizontal ? vec2(aoTexel.x, 0.0) : vec2(0.0, aoTexel.y);
    float centerDepth = texture(uDepth, vUv).r;
    float weightedOcclusion = 0.0;
    float totalWeight = 0.0;
    for (int offsetIndex = -3; offsetIndex <= 3; ++offsetIndex) {
        vec2 sampleUv = vUv + axis * float(offsetIndex);
        float sampleDepth = texture(uDepth, sampleUv).r;
        float spatialWeight = exp(-float(offsetIndex * offsetIndex) / 7.0);
        float depthWeight = exp(-abs(sampleDepth - centerDepth) * 1800.0);
        float weight = spatialWeight * depthWeight;
        weightedOcclusion += texture(uOcclusion, sampleUv).r * weight;
        totalWeight += weight;
    }
    FragOcclusion = weightedOcclusion / max(totalWeight, 0.0001);
}
