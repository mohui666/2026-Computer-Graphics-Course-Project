#version 330 core

in vec2 vUv;
out vec4 FragColor;

uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform sampler2D uOcclusion;
uniform float uExposureEv;
uniform float uBloomStrength;
uniform float uVignetteStrength;
uniform float uSsaoStrength;

vec3 acesFitted(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

float interleavedGradientNoise(vec2 pixel) {
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

void main() {
    vec3 hdr = texture(uScene, vUv).rgb;
    hdr += texture(uBloom, vUv).rgb * uBloomStrength;
    float occlusion = texture(uOcclusion, vUv).r;
    hdr *= mix(1.0, 0.58 + 0.42 * occlusion, uSsaoStrength);
    hdr *= exp2(uExposureEv);
    vec3 color = acesFitted(max(hdr, vec3(0.0)));

    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    color = mix(vec3(luminance), color, 1.055);
    float edge = clamp(length(vUv - 0.5) / 0.7071068, 0.0, 1.0);
    color *= 1.0 - uVignetteStrength * smoothstep(0.38, 1.0, edge);
    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
    color += (interleavedGradientNoise(gl_FragCoord.xy) - 0.5) / 255.0;
    FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
