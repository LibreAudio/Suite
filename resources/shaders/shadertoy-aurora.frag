#define HUE 0.0
#define EVOLUTION_SPEED 0.050
#define EVOLUTION_AMOUNT 0.65
#define RAY_FLOW_SPEED 2.55

mat2 rotate2(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return mat2(c, s, -s, c);
}

const mat2 FIELD_TURN = mat2(0.95534, 0.29552, -0.29552, 0.95534);

float triangleWave(float x)
{
    return clamp(abs(fract(x) - 0.5), 0.01, 0.49);
}

vec2 trianglePair(vec2 p)
{
    return vec2(
        triangleWave(p.x) + triangleWave(p.y),
        triangleWave(p.y + triangleWave(p.x))
    );
}

float hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float auroraNoise(vec2 p, float phase)
{
    float scale = 1.8;
    float warpScale = 1.5;
    float ridgeSum = 0.0;
    p = p * rotate2(p.x * 0.1);
    vec2 base = p;

    for (int octave = 0; octave < 4; ++octave)
    {
        vec2 gradient = trianglePair(base * 1.85) * 0.75;
        gradient = gradient * rotate2(phase * 1.82);
        p -= gradient / warpScale;
        base *= 1.30;
        warpScale *= 0.45;
        scale *= 0.42;
        p *= 1.21 + (ridgeSum - 1.0) * 0.02;
        ridgeSum += triangleWave(p.x + triangleWave(p.y)) * scale;
        p = p * -FIELD_TURN;
    }

    float denominator = pow(max(ridgeSum * 29.0, 0.001), 1.30);
    return clamp(1.0 / denominator, 0.0, 0.62);
}

float valueNoise1(float x)
{
    float cell = floor(x);
    float local = fract(x);
    local = local * local * (3.0 - 2.0 * local);
    float a = hash21(vec2(cell, cell + 17.0));
    float b = hash21(vec2(cell + 1.0, cell + 18.0));
    return mix(a, b, local);
}

float rayFlow(float coordinate, float time)
{
    float broad = valueNoise1(coordinate * 2.7 - time * 0.16);
    float fine = valueNoise1(coordinate * 8.4 - time * 0.31);
    return 0.05 + 0.95 * pow(mix(broad, fine, 0.52), 5.0);
}

vec4 marchAurora(vec3 rayOrigin, vec3 rayDirection, vec2 screenUv, vec2 pixel)
{
    vec4 accumulated = vec4(0.0);
    vec4 runningAverage = vec4(0.0);
    float evolution = iTime * EVOLUTION_SPEED;
    float phaseA = evolution;
    float phaseB = evolution * 0.72 + 1.70;
    float evolutionPulse = 0.5 + 0.5
        * sin(evolution * 0.28 + 0.80);
    float jitterSeed = hash21(pixel) - 0.5;

    for (int sampleIndex = 0; sampleIndex < 26; ++sampleIndex)
    {
        float fi = float(sampleIndex);
        float h = fi / 13.0;
        float jitter = 0.01 * jitterSeed * smoothstep(0.0, 12.0, fi);
        float sheetHeight = 0.72 + pow(fi, 1.3) * 0.0025;
        sheetHeight += (0.001 + h * 0.001)
            * sin(phaseB * 1.15 + fi * 0.047 + screenUv.x * 1.30);
        float travel = (sheetHeight - rayOrigin.y)
            / (rayDirection.y * 2.0 + 0.40);
        travel -= jitter;

        vec3 position = rayOrigin + travel * rayDirection;
        vec2 fieldPosition = position.zx;

        float mainWarp = mix(0.43, 0.55, evolutionPulse) * EVOLUTION_AMOUNT;
        float detailWarp = mix(0.17, 0.12, evolutionPulse)
            * EVOLUTION_AMOUNT;
        fieldPosition.y += mainWarp
            * sin(fieldPosition.x * 0.34 + phaseA * 1.45);
        fieldPosition.y += detailWarp
            * sin(fieldPosition.x * 0.91 - phaseB * 1.25);
        fieldPosition.x += 0.075 * EVOLUTION_AMOUNT
            * sin(fieldPosition.y * 0.56 - phaseB * 0.92);

        vec2 curlCenter = vec2(
            0.50 + 0.08 * sin(phaseB * 0.72),
            -0.25 + 0.06 * cos(phaseA * 0.56)
        );
        vec2 curl = fieldPosition - curlCenter;
        float curlInfluence = exp(-dot(curl, curl) * 0.22);
        float curlStrength = (0.90
            + 0.18 * sin(phaseA * 0.48 + phaseB * 0.31))
            * EVOLUTION_AMOUNT;
        fieldPosition = curl * rotate2(
            curlStrength * curlInfluence + 0.1 * sin(phaseB)
        ) + curlCenter;

        float density = auroraNoise(fieldPosition, phaseA * 1.35);
        float rayCoordinate = screenUv.x * 4.2
            + 0.18 * sin(phaseB + screenUv.x * 60.0);
        float striation = rayFlow(rayCoordinate, iTime * RAY_FLOW_SPEED);
        density *= 0.72 + 0.18 * striation;

        vec3 colorWeights;
        colorWeights.r = 1.0 - smoothstep(0.20, 0.58, h);
        colorWeights.g = smoothstep(0.04, 0.34, h)
            * (1.0 - smoothstep(0.66, 0.94, h));
        colorWeights.b = smoothstep(0.48, 0.98, h);

        float colorFlow = 0.5 + 0.5
            * sin(position.z * 0.17 + phaseA * 1.40);
        colorWeights.r *= mix(1.15, 0.82, colorFlow);
        colorWeights.b *= mix(0.78, 1.18, colorFlow);
        float palettePosition = smoothstep(0.22, 0.90, screenUv.x);
        colorWeights.r *= mix(1.28, 0.55, palettePosition);
        colorWeights.g *= 1.08;
        colorWeights.b *= mix(0.78, 1.72, palettePosition);
        colorWeights /= max(dot(colorWeights, vec3(1.0)), 0.001);

        vec4 layer = vec4(colorWeights * density, density);
        runningAverage = mix(runningAverage, layer, 0.50);
        float depthWeight = exp2(-fi * 0.065 - 2.30)
            * smoothstep(0.0, 5.0, fi);
        accumulated += runningAverage * depthWeight;
    }

    float horizonFade = clamp(rayDirection.y * 12.0 + 0.30, 0.0, 1.0);
    return accumulated * horizonFade * 1.80;
}

vec3 rgbToHsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)),
                d / (q.x + e), q.x);
}

vec3 hsvToRgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 shiftHue(vec3 color, float hue)
{
    vec3 hsv = rgbToHsv(max(color, vec3(0.0)));
    hsv.x = fract(hsv.x + clamp(hue, 0.0, 1.0));
    return hsvToRgb(hsv);
}

vec3 paintAurora(vec3 masks)
{
    const vec3 cyan = vec3(0.00, 0.65, 0.85);
    const vec3 electricBlue = vec3(0.08, 0.38, 1.00);
    const vec3 indigo = vec3(0.32, 0.18, 0.92);
    return masks.r * cyan + masks.g * electricBlue + masks.b * indigo;
}

float ellipseGlow(vec2 uv, vec2 center, vec2 scale)
{
    vec2 q = (uv - center) * scale;
    return exp(-dot(q, q));
}

float sparseStars(vec2 uv, float aspect)
{
    vec2 starUv = vec2(uv.x * aspect, uv.y) * 78.0;
    vec2 cell = floor(starUv);
    vec2 local = fract(starUv) - 0.5;
    float randomValue = hash21(cell);
    vec2 point = vec2(
        hash21(cell + vec2(1.7, 9.2)),
        hash21(cell + vec2(8.3, 2.8))
    ) - 0.5;
    float star = 1.0 - smoothstep(0.020, 0.054, length(local - point * 0.72));
    star *= step(0.9991, randomValue);
    star *= 0.84 + 0.16 * sin(iTime * 0.22 + randomValue * 24.0);
    return star;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy;
    float aspect = iResolution.x / iResolution.y;
    float landscape = smoothstep(1.0, 1.65, aspect);
    float horizontalScale = mix(0.76, aspect, landscape);

    vec2 cameraPlane = vec2(
        (uv.x - 0.5) * horizontalScale,
        mix(0.02, 0.54, uv.y)
    );

    vec3 rayOrigin = vec3(0.0, 0.0, -6.7);
    vec3 rayDirection = normalize(vec3(cameraPlane, 1.25));
    float cameraEvolution = iTime * EVOLUTION_SPEED;
    rayDirection.xz = rayDirection.xz
        * rotate2(0.045 * sin(cameraEvolution * 0.70));

    vec4 raw = marchAurora(rayOrigin, rayDirection, uv, fragCoord);

    // Three tonal layers approximate soft edges and bloom without a texture
    // buffer or repeated raymarches.
    vec3 structuredMasks = pow(max(raw.rgb, vec3(0.0)), vec3(1.48)) * 2.35;
    vec3 softMasks = pow(max(raw.rgb, vec3(0.0)), vec3(1.12)) * 0.18;
    vec3 glowMasks = pow(max(raw.rgb, vec3(0.0)), vec3(0.70)) * 0.085;
    vec3 aurora = paintAurora(structuredMasks + softMasks + glowMasks);
    aurora = shiftHue(aurora, HUE);

    const vec3 navy = vec3(0.002, 0.005, 0.018);
    const vec3 deepBlue = vec3(0.006, 0.030, 0.078);
    vec3 background = mix(navy, deepBlue, smoothstep(0.0, 1.0, uv.y));

    background += ellipseGlow(uv, vec2(0.04, 0.34), vec2(1.70, 1.20))
        * vec3(0.000, 0.030, 0.060);
    background += ellipseGlow(uv, vec2(0.82, 0.72), vec2(1.55, 1.28))
        * vec3(0.010, 0.008, 0.040);
    background += ellipseGlow(uv, vec2(0.76, 0.05), vec2(1.32, 1.72))
        * vec3(0.028, 0.012, 0.068);

    float densityHaze = pow(clamp(raw.a, 0.0, 1.0), 0.70) * 0.045;
    background += densityHaze * vec3(0.02, 0.07, 0.13);

    vec3 stars = sparseStars(uv, aspect) * vec3(0.30, 0.50, 0.78) * 0.42;
    vec3 color = background + stars + aurora;

    vec2 edge = uv * (1.0 - uv);
    float vignette = pow(clamp(16.0 * edge.x * edge.y, 0.0, 1.0), 0.16);
    color *= mix(0.68, 1.0, vignette);

    color = 1.0 - exp(-color);
    color = pow(max(color, vec3(0.0)), vec3(0.94));
    color += (hash21(fragCoord) - 0.5) / 255.0;
    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
