#version 330 core

in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vLocalPosition;

out vec4 FragColor;

uniform vec3 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;
uniform float uEmission;
uniform float uAlpha;
uniform int uMaterialKind;
uniform vec3 uCameraPosition;
uniform vec3 uCameraForward;
uniform bool uHeadlamp;
uniform bool uWorkLights;
uniform bool uLightingFailed;
uniform bool uSelected;
uniform bool uFogEnabled;
uniform float uFogDensity;
uniform vec3 uFogColor;
uniform float uTime;
uniform vec3 uObjectScale;
uniform float uObjectSeed;

float hash31(vec3 p) {
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float valueNoise(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(hash31(i + vec3(0,0,0)), hash31(i + vec3(1,0,0)), f.x),
                   mix(hash31(i + vec3(0,1,0)), hash31(i + vec3(1,1,0)), f.x), f.y),
               mix(mix(hash31(i + vec3(0,0,1)), hash31(i + vec3(1,0,1)), f.x),
                   mix(hash31(i + vec3(0,1,1)), hash31(i + vec3(1,1,1)), f.x), f.y), f.z);
}

vec3 evaluateLight(vec3 l, vec3 radiance, vec3 n, vec3 viewDir, vec3 albedo,
                   float roughness, float metallic) {
    float nDotL = max(dot(n, l), 0.0);
    if (nDotL <= 0.0) return vec3(0.0);
    vec3 halfway = normalize(l + viewDir);
    float shininess = mix(7.0, 112.0, 1.0 - roughness);
    float specular = pow(max(dot(n, halfway), 0.0), shininess);
    vec3 f0 = mix(vec3(0.035), albedo, metallic);
    vec3 diffuseColor = albedo * (1.0 - metallic);
    return radiance * nDotL * (diffuseColor + f0 * specular * mix(0.35, 1.2, 1.0 - roughness));
}

vec3 pointLight(vec3 position, vec3 color, float intensity, vec3 n, vec3 viewDir,
                vec3 albedo, float roughness, float metallic) {
    vec3 toLight = position - vWorldPosition;
    float distance = length(toLight);
    vec3 l = toLight / max(distance, 0.001);
    float attenuation = 1.0 / (1.0 + 0.08 * distance + 0.035 * distance * distance);
    return evaluateLight(l, color * intensity * attenuation, n, viewDir,
                         albedo, roughness, metallic);
}

void main() {
    vec3 n = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPosition - vWorldPosition);
    vec3 p = vWorldPosition;
    float coarse = valueNoise(p * 0.72);
    float fine = valueNoise(p * 4.6);
    vec3 albedo = uBaseColor;
    float roughness = uRoughness;
    float metallic = uMetallic;

    if (uMaterialKind == 1) {
        float cleat = smoothstep(0.25, 0.82, valueNoise(p * vec3(0.45, 2.8, 2.2)));
        albedo *= mix(0.46, 1.65, cleat) * mix(0.72, 1.18, fine);
        roughness = mix(0.82, 0.42, fine);
        float e = 0.025;
        vec3 grad = vec3(valueNoise((p + vec3(e,0,0)) * 3.2) - valueNoise((p - vec3(e,0,0)) * 3.2),
                         valueNoise((p + vec3(0,e,0)) * 3.2) - valueNoise((p - vec3(0,e,0)) * 3.2),
                         valueNoise((p + vec3(0,0,e)) * 3.2) - valueNoise((p - vec3(0,0,e)) * 3.2));
        n = normalize(n - grad * 4.2);
    } else if (uMaterialKind == 2) {
        float strata = 0.5 + 0.5 * sin(p.y * 7.2 + p.x * 0.22 + coarse * 3.0);
        float mineralGrain = valueNoise(p * 11.5 + vec3(2.7, 7.1, 1.9));
        albedo *= mix(0.58, 1.38, strata) * mix(0.78, 1.18, fine) *
                  mix(0.84, 1.16, mineralGrain);
        float e = 0.035;
        vec3 grad = vec3(valueNoise((p + vec3(e,0,0)) * 1.8) - valueNoise((p - vec3(e,0,0)) * 1.8),
                         valueNoise((p + vec3(0,e,0)) * 1.8) - valueNoise((p - vec3(0,e,0)) * 1.8),
                         valueNoise((p + vec3(0,0,e)) * 1.8) - valueNoise((p - vec3(0,0,e)) * 1.8));
        n = normalize(n - grad * 3.0);
    } else if (uMaterialKind == 3) {
        float streak = 0.72 + 0.28 * sin((p.x + p.z) * 31.0 + fine * 5.0);
        float corrosion = smoothstep(0.74, 0.96, coarse + fine * 0.24);
        albedo *= mix(0.76, 1.12, streak);
        albedo = mix(albedo, vec3(0.19, 0.055, 0.018), corrosion * 0.48);
        roughness = mix(roughness, 0.75, corrosion);
    } else if (uMaterialKind == 4) {
        vec3 localMeters = vLocalPosition * max(uObjectScale, vec3(0.05));
        vec3 seedOffset = vec3(uObjectSeed * 17.0, uObjectSeed * 31.0, uObjectSeed * 47.0);
        float paintFine = valueNoise(localMeters * 16.0 + seedOffset);
        float paintMid = valueNoise(localMeters * 5.2 + seedOffset * 0.37);
        float chip = smoothstep(0.895, 0.965, paintFine * 0.74 + paintMid * 0.32);
        float lowerGrime = 1.0 - smoothstep(-0.48, 0.20, vLocalPosition.y);
        float grime = valueNoise(localMeters * vec3(2.1, 3.8, 2.1) + seedOffset) * lowerGrime;
        albedo *= mix(0.82, 1.06, paintFine) * mix(1.0, 0.69, grime * 0.55);
        albedo = mix(albedo, vec3(0.060, 0.070, 0.072), chip * 0.86);
        metallic = mix(metallic, 0.86, chip);
        roughness = mix(roughness, 0.39, chip);
        float paintBump = valueNoise(p * 18.0 + seedOffset);
        float bumpE = 0.012;
        vec3 paintGradient = vec3(
            valueNoise((p + vec3(bumpE,0,0)) * 18.0 + seedOffset) - paintBump,
            valueNoise((p + vec3(0,bumpE,0)) * 18.0 + seedOffset) - paintBump,
            valueNoise((p + vec3(0,0,bumpE)) * 18.0 + seedOffset) - paintBump);
        n = normalize(n - paintGradient * 0.18);
    } else if (uMaterialKind == 5) {
        albedo *= 0.72 + fine * 0.35;
        roughness = 0.92;
    } else if (uMaterialKind == 6) {
        float wear = smoothstep(0.78, 0.95, fine + coarse * 0.18);
        albedo = mix(albedo, vec3(0.12, 0.105, 0.075), wear * 0.56);
    } else if (uMaterialKind == 8) {
        float radial = length(vLocalPosition.xz) * 2.0;
        float angle = atan(vLocalPosition.z, vLocalPosition.x);
        float machined = 0.5 + 0.5 * sin(radial * 38.0 + angle * 5.0 + uObjectSeed * 9.0);
        float circularGroove = smoothstep(0.72, 0.94,
            0.5 + 0.5 * sin(radial * 54.0 + fine * 1.7));
        float faceMask = smoothstep(0.40, 0.485, abs(vLocalPosition.y));
        float axialWear = 0.5 + 0.5 * sin(vLocalPosition.y * uObjectScale.y * 22.0 + angle * 2.0);
        albedo *= mix(0.70, 1.30, mix(axialWear, machined, faceMask));
        albedo = mix(albedo, vec3(0.010,0.013,0.014), circularGroove * faceMask * 0.58);
        roughness = mix(0.34, 0.19, machined * faceMask);
        metallic = 0.94;
    }

    float hemi = clamp(n.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambient = mix(vec3(0.018, 0.021, 0.023), vec3(0.060, 0.067, 0.064), hemi);
    vec3 color = albedo * ambient;

    if (uWorkLights && !uLightingFailed) {
        color += pointLight(vec3(-30.0, 5.25, 4.1), vec3(1.0, 0.72, 0.46), 3.4, n, viewDir, albedo, roughness, metallic);
        color += pointLight(vec3(-18.0, 5.25, 4.1), vec3(1.0, 0.75, 0.50), 3.7, n, viewDir, albedo, roughness, metallic);
        color += pointLight(vec3( -6.0, 5.25, 4.1), vec3(1.0, 0.79, 0.56), 4.1, n, viewDir, albedo, roughness, metallic);
        color += pointLight(vec3(  6.0, 5.25, 4.1), vec3(1.0, 0.79, 0.56), 4.1, n, viewDir, albedo, roughness, metallic);
        color += pointLight(vec3( 18.0, 5.25, 4.1), vec3(1.0, 0.75, 0.50), 3.7, n, viewDir, albedo, roughness, metallic);
        color += pointLight(vec3( 30.0, 5.25, 4.1), vec3(1.0, 0.72, 0.46), 3.4, n, viewDir, albedo, roughness, metallic);
    }
    if (uHeadlamp) {
        vec3 toFragment = normalize(vWorldPosition - uCameraPosition);
        float cone = smoothstep(0.72, 0.96, dot(toFragment, normalize(uCameraForward)));
        float distance = length(vWorldPosition - uCameraPosition);
        vec3 l = normalize(uCameraPosition - vWorldPosition);
        vec3 radiance = vec3(0.76, 0.86, 0.92) * cone * 2.4 /
                        (1.0 + 0.042 * distance * distance);
        color += evaluateLight(l, radiance, n, viewDir, albedo, roughness, metallic);
    }

    float activeEmission = uEmission;
    if (uMaterialKind == 7 && (!uWorkLights || uLightingFailed)) activeEmission = 0.0;
    color += albedo * activeEmission;
    if (uSelected) {
        float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 2.0);
        color += vec3(1.0, 0.58, 0.04) * (0.45 + rim * 1.8);
    }

    float distanceToCamera = length(vWorldPosition - uCameraPosition);
    float fogFactor = uFogEnabled ? 1.0 - exp(-uFogDensity * uFogDensity * distanceToCamera * distanceToCamera) : 0.0;
    color = mix(color, uFogColor, clamp(fogFactor, 0.0, 0.92));
    FragColor = vec4(max(color, vec3(0.0)), uAlpha);
}
