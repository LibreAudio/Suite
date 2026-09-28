// Libre Audio Suite
// Copyright (C) 2026 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: GPL-3.0-or-later

const float total = 512.;
const float rectSize = 1. / total;

const vec3 rectColor = vec3(0.01, 0.26, 0.57);

float rectangle(vec2 uv, float x1, float x2, float h1, float h2)
{
    vec2 pos = vec2(0., 0.);
    pos.x = rectSize / 2. - abs(uv.x - x1);
    pos.y = h1 - abs(uv.y);
    pos = smoothstep(0., 0., pos);
    return pos.x * pos.y;
}

float texpx(float x)
{
    vec4 c = texture2D(iChannel0, vec2(x, 0.));
    return clamp(c.r, 0., 1.);
//     * 255.0 + c.g * 255.0 + c.b * 255.0) / 65535.0;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy;

    vec3 color = vec3(0., 0., 0.);

    for (float i = 0.; i < total; ++i)
    {
        float x1 = i / (total - 1.) - rectSize * 0.5;
        float x2 = x1;
//         float x2 = (i + 1.) / total;
        float h1 = texpx(x1);
        float h2 = texpx(x2);

        float rect = rectangle(uv, x1, x2, h1, h2);
        color += rectColor * rect;
    }

    fragColor = vec4(color, 1.0);
}
