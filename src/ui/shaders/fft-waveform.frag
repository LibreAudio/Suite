// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// float total = textureSize(_dpf_texture_data, 0).x;

const vec3 color = vec3(0.01, 0.26, 0.57);

float data(float x)
{
    x = mod(1. + x - _dpf_texture_start, 1.);
    // x = (x * (total - 1.0) + 0.5) / total;
    float c = texture2D(_dpf_texture_data, vec2(x, 0.5)).r;
    return c;
}

#define FILL 0.95
#define GRAD 0.10

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy;

    float px = data(uv.x);
    float poly = smoothstep(-1.0, 1.0, (px - uv.y) * iResolution.y);
    float grad = FILL * pow(uv.y, GRAD);

    fragColor = vec4(color * poly, poly * grad);
}
