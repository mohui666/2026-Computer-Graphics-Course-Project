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
uniform float uShearerX;
uniform float uWorkLightY;
uniform float uWorkLightZ;
uniform bool uShadowsEnabled;
uniform bool uDustEnabled;
uniform bool uCutaway;
uniform sampler2DArray uShadowMaps;
uniform mat4 uShadowMatrix[2];
uniform int uShadowIndex0;
uniform int uShadowIndex1;
const float PI = 3.14159265;

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

// Cook-Torrance microfacet BRDF: GGX NDF, Smith masking, Schlick Fresnel.
// Reference equations: LearnOpenGL / PBR / Lighting (independent implementation).
vec3 evaluateLight(vec3 l, vec3 radiance, vec3 n, vec3 viewDir, vec3 albedo,
                   float roughness, float metallic) {
    float nl = clamp(dot(n,l),0.0,1.0), nv = clamp(dot(n,viewDir),0.001,1.0);
    vec3 h = normalize(l+viewDir);
    float nh = clamp(dot(n,h),0.0,1.0), vh = clamp(dot(viewDir,h),0.0,1.0);
    float a = max(roughness*roughness,0.025), a2 = a*a;
    float denominator = nh*nh*(a2-1.0)+1.0;
    float distribution = a2 / (PI*denominator*denominator);
    float k = (roughness+1.0)*(roughness+1.0)/8.0;
    float geometry = (nv/(nv*(1.0-k)+k))*(nl/(nl*(1.0-k)+k));
    vec3 f0 = mix(vec3(0.04),albedo,metallic);
    vec3 fresnel = f0+(1.0-f0)*pow(1.0-vh,5.0);
    vec3 specular = distribution*geometry*fresnel/max(4.0*nv*nl,0.001);
    vec3 diffuse = (1.0-fresnel)*(1.0-metallic)*albedo/PI;
    return (diffuse+specular)*radiance*nl;
}

float fixtureVisibility(int index, vec3 normal, vec3 lightDirection) {
    int layer = index == uShadowIndex0 ? 0 : (index == uShadowIndex1 ? 1 : -1);
    if (!uShadowsEnabled || layer < 0) return 1.0;
    vec4 clip = uShadowMatrix[layer]*vec4(vWorldPosition,1.0);
    vec3 q = clip.xyz/clip.w*0.5+0.5;
    if (clip.w <= 0.0 || q.z >= 1.0 || q.z <= 0.0 || any(lessThan(q.xy,vec2(0))) || any(greaterThan(q.xy,vec2(1)))) return 1.0;
    float bias = max(0.00065*(1.0-dot(normal,lightDirection)),0.00016);
    float visible = 0.0;
    vec2 texel = 1.0/vec2(textureSize(uShadowMaps,0).xy);
    for (int x=-1; x<=1; ++x) for(int y=-1; y<=1; ++y)
        visible += q.z-bias <= texture(uShadowMaps,vec3(q.xy+vec2(x,y)*texel,layer)).r ? 1.0 : 0.0;
    return visible/9.0;
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
    if (uMaterialKind == 9 || uMaterialKind == 10) {
        if (!uDustEnabled) discard;
        vec2 d = vTexCoord*2.0-1.0;
        float radius2 = dot(d,d);
        if (radius2 > 1.0) discard;
        float soft = pow(max(1.0-radius2,0.0),2.0);
        float noise = valueNoise(vWorldPosition*4.0+uObjectSeed*11.0);
        float lit = (uWorkLights && !uLightingFailed ? 0.42 : 0.02) + (uHeadlamp ? 0.17 : 0.0);
        FragColor = vec4(uBaseColor*lit, uAlpha*soft*(0.65+0.35*noise));
        return;
    }
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
        float bedding = abs(sin(p.y*14.0 + valueNoise(p*vec3(2.0,0.8,2.0))*7.2));
        float fracture = smoothstep(0.035,0.12,bedding);
        albedo *= mix(0.32,1.0,fracture);
        float grain = valueNoise(p*26.0);
        float cleave = valueNoise(p*vec3(7.0,15.0,9.0));
        albedo *= mix(0.42,1.65,grain)*mix(0.48,1.24,smoothstep(0.25,0.63,cleave));
        float wet = smoothstep(0.76,0.96,coarse+fine*0.15);
        roughness = mix(0.89,0.24,wet);
        metallic = 0.03;
        float e = 0.025;
        vec3 grad = vec3(valueNoise((p + vec3(e,0,0)) * 3.2) - valueNoise((p - vec3(e,0,0)) * 3.2),
                         valueNoise((p + vec3(0,e,0)) * 3.2) - valueNoise((p - vec3(0,e,0)) * 3.2),
                         valueNoise((p + vec3(0,0,e)) * 3.2) - valueNoise((p - vec3(0,0,e)) * 3.2));
        vec3 micro = vec3(valueNoise((p+vec3(0.008,0,0))*26.0)-grain,
                          valueNoise((p+vec3(0,0.008,0))*26.0)-grain,
                          valueNoise((p+vec3(0,0,0.008))*26.0)-grain);
        n = normalize(n - grad*1.3 - micro*2.2);
    } else if (uMaterialKind == 2) {
        float strata = 0.5 + 0.5 * sin(p.y * 7.2 + p.x * 0.22 + coarse * 3.0);
        // Damp shale and mud remain charcoal rather than sandy brown.
        albedo = mix(albedo,vec3(0.055,0.061,0.060),0.62);
        roughness = mix(0.94,0.26,smoothstep(0.64,0.83,coarse)*max(n.y,0.0));
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
        float chip = smoothstep(0.78, 0.92, paintFine * 0.74 + paintMid * 0.38);
        float edgeWear = smoothstep(0.44,0.5,max(abs(vLocalPosition.x),abs(vLocalPosition.z))) * smoothstep(0.48,0.70,paintMid);
        chip = max(chip,edgeWear*0.30);
        float lowerGrime = 1.0 - smoothstep(-0.48, 0.20, vLocalPosition.y);
        float grime = valueNoise(localMeters * vec3(2.1, 3.8, 2.1) + seedOffset) * lowerGrime;
        albedo *= mix(0.82, 1.06, paintFine) * mix(1.0, 0.69, grime * 0.55);
        albedo = mix(albedo, mix(vec3(0.10,0.12,0.13),vec3(0.105,0.054,0.026),paintMid), chip);
        float coalDust = smoothstep(0.42,0.78,paintMid)*max(n.y,0.0)*0.52;
        albedo = mix(albedo,vec3(0.025,0.027,0.028),coalDust);
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
        roughness = mix(0.62, 0.43, machined * faceMask);
        metallic = 0.72;
    }

    float hemi = clamp(n.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambient = mix(vec3(0.018, 0.021, 0.023), vec3(0.115, 0.126, 0.138), hemi);
    vec3 color = albedo * ambient;

    if (uWorkLights && !uLightingFailed) {
        for(int i=0;i<6;++i) {
            vec3 position = vec3(-30.0+12.0*float(i),uWorkLightY,uWorkLightZ);
            vec3 l = normalize(position-p);
            float cone = smoothstep(0.38,0.58,dot(-l,normalize(vec3(0,-1,-0.38))));
            float visibility = fixtureVisibility(i,n,l);
            vec3 tint = i%2==0 ? vec3(0.82,0.89,1.0) : vec3(1.0,0.88,0.72);
            color += pointLight(position,tint,12.0*cone*visibility,n,viewDir,albedo,roughness,metallic);
        }
        color += pointLight(vec3(uShearerX,2.5,-3.6),vec3(0.78,0.86,1.0),1.8,n,viewDir,albedo,roughness,metallic);
    }
    if (uHeadlamp) {
        vec3 toFragment = normalize(vWorldPosition - uCameraPosition);
        float cone = smoothstep(0.72, 0.96, dot(toFragment, normalize(uCameraForward)));
        float distance = length(vWorldPosition - uCameraPosition);
        vec3 l = normalize(uCameraPosition - vWorldPosition);
        vec3 radiance = vec3(0.76, 0.86, 0.92) * cone * 6.5 /
                        (1.0 + 0.042 * distance * distance);
        color += evaluateLight(l, radiance, n, viewDir, albedo, roughness, metallic);
    }

    // Diagram/inspection illumination is only enabled with the roof cutaway.
    if (uCutaway) {
        color += albedo*0.38;
        color += evaluateLight(normalize(vec3(-0.35,1.0,0.45)),vec3(2.4,2.3,2.1),
                               n,viewDir,albedo,roughness,metallic);
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
