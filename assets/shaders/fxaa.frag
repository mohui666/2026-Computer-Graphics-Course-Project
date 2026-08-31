#version 330 core

in vec2 vUv;
out vec4 FragColor;

uniform sampler2D uImage;
uniform bool uEnabled;

float luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

void main() {
    if (!uEnabled) {
        FragColor = texture(uImage, vUv);
        return;
    }

    vec2 texel = 1.0 / vec2(textureSize(uImage, 0));
    vec3 rgbM  = texture(uImage, vUv).rgb;
    vec3 rgbNW = texture(uImage, vUv + vec2(-1.0, -1.0) * texel).rgb;
    vec3 rgbNE = texture(uImage, vUv + vec2( 1.0, -1.0) * texel).rgb;
    vec3 rgbSW = texture(uImage, vUv + vec2(-1.0,  1.0) * texel).rgb;
    vec3 rgbSE = texture(uImage, vUv + vec2( 1.0,  1.0) * texel).rgb;

    float lumaM = luma(rgbM);
    float lumaNW = luma(rgbNW);
    float lumaNE = luma(rgbNE);
    float lumaSW = luma(rgbSW);
    float lumaSE = luma(rgbSE);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 direction;
    direction.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    direction.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    float reduction = max((lumaNW + lumaNE + lumaSW + lumaSE) * 0.25 * 0.0312, 1.0 / 128.0);
    float reciprocalMinimum = 1.0 / (min(abs(direction.x), abs(direction.y)) + reduction);
    direction = clamp(direction * reciprocalMinimum, vec2(-8.0), vec2(8.0)) * texel;

    vec3 resultA = 0.5 * (
        texture(uImage, vUv + direction * (1.0 / 3.0 - 0.5)).rgb +
        texture(uImage, vUv + direction * (2.0 / 3.0 - 0.5)).rgb);
    vec3 resultB = resultA * 0.5 + 0.25 * (
        texture(uImage, vUv + direction * -0.5).rgb +
        texture(uImage, vUv + direction *  0.5).rgb);
    float lumaB = luma(resultB);
    FragColor = vec4((lumaB < lumaMin || lumaB > lumaMax) ? resultA : resultB, 1.0);
}
