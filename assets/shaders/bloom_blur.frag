#version 330 core

in vec2 vUv;
out vec4 FragColor;

uniform sampler2D uImage;
uniform bool uHorizontal;

void main() {
    vec2 texel = 1.0 / vec2(textureSize(uImage, 0));
    vec2 axis = uHorizontal ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);
    vec3 result = texture(uImage, vUv).rgb * 0.227027;
    result += texture(uImage, vUv + axis * 1.384615).rgb * 0.316216;
    result += texture(uImage, vUv - axis * 1.384615).rgb * 0.316216;
    result += texture(uImage, vUv + axis * 3.230769).rgb * 0.070270;
    result += texture(uImage, vUv - axis * 3.230769).rgb * 0.070270;
    FragColor = vec4(result, 1.0);
}
