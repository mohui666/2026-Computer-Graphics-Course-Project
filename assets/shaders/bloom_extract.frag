#version 330 core

in vec2 vUv;
out vec4 FragColor;

uniform sampler2D uScene;
uniform float uThreshold;
uniform float uSoftKnee;

void main() {
    vec3 color = texture(uScene, vUv).rgb;
    float brightness = max(max(color.r, color.g), color.b);
    float knee = max(uThreshold * uSoftKnee, 0.0001);
    float soft = clamp(brightness - uThreshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.0001);
    float contribution = max(brightness - uThreshold, soft) / max(brightness, 0.0001);
    FragColor = vec4(color * contribution, 1.0);
}
