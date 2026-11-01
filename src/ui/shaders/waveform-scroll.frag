// Libre Audio Suite
// SPDX-License-Identifier: GPL-3.0-or-later

// Scrolling peak waveform, newest on the right, on a dB scale.
//
// The texture is a ring of peak values. uHead is the ring position of the newest point to show, fractional and
// advanced smoothly by the UI, so the waveform rolls at a steady speed between data bursts.
//
// Several ring points fall on each pixel. They are reduced to bins fixed in ring space, a power of two points wide so
// they tile the ring exactly and at least a pixel wide, so a peak stays in the same bin while it scrolls instead of
// being picked up or missed from frame to frame. Each bin is drawn as a box of its maximum, and a pixel is filled by
// how much of its width the boxes cover at its height: a narrow peak keeps its full height and only shares its
// brightness between neighbouring pixels as it moves.
//
// The fill uses the mbComp5 rainbow turned upright: colour and opacity are fixed to the display, the rainbow running
// from bottom to top while the opacity rises from ALPHA_BOTTOM to ALPHA_TOP, and the waveform shows whatever part of
// that gradient it covers. ALPHA_CURVE and COLOR_CURVE shape both by height: 1.0 is linear, below 1 the change
// happens early (towards the bottom), above 1 it happens late (towards the top).

uniform float uHead;

// level scale of the display in dB, bottom and top edge; peaks are drawn at their height on it
uniform float uDbMin;
uniform float uDbMax;

// mbComp5 rainbow, bottom to top
const vec3 c0 = vec3(255.0, 191.0, 203.0) / 255.0;
const vec3 c1 = vec3(255.0, 223.0, 173.0) / 255.0;
const vec3 c2 = vec3(210.0, 253.0, 211.0) / 255.0;
const vec3 c3 = vec3(190.0, 241.0, 255.0) / 255.0;
const vec3 c4 = vec3(218.0, 193.0, 243.0) / 255.0;

// fill opacity at the bottom and at the top of the display
#define ALPHA_BOTTOM 0.0
#define ALPHA_TOP 0.7

// how the opacity rises with height: 0.5 = square root, 1.0 = linear, 2.0 = late
#define ALPHA_CURVE 1.3

// where the rainbow colours sit by height: 1.0 = evenly, below 1 = blue and lavender take more, above 1 = pink and
// peach take more
#define COLOR_CURVE 1.0

// ring points left out at the oldest end, so points written ahead of uHead are never shown there
#define MARGIN 256.0
#define MAX_BIN 64

float point(float i, float size)
{
    return texture2D(_dpf_texture_data, vec2((mod(i, size) + 0.5) / size, 0.5)).r;
}

// peak value (linear, 0..1) to height on the display (0..1)
float heightOf(float peak)
{
    float db = 8.685889638 * log(max(peak, 1e-6)); // 20 * log10
    return clamp((db - uDbMin) / (uDbMax - uDbMin), 0.0, 1.0);
}

vec3 rainbow(float t)
{
    float p = clamp(t, 0.0, 1.0) * 4.0;
    if (p < 1.0) return mix(c0, c1, p);
    if (p < 2.0) return mix(c1, c2, p - 1.0);
    if (p < 3.0) return mix(c2, c3, p - 2.0);
    return mix(c3, c4, p - 3.0);
}

float binMax(float bin, float span, float size)
{
    float m = 0.0;
    for (int j = 0; j < MAX_BIN; ++j)
    {
        if (float(j) >= span)
            break;
        m = max(m, point(bin * span + float(j), size));
    }
    return m;
}


void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy;

    float size = float(_dpf_texture_size);
    float window = size - MARGIN;
    float perPixel = window / iResolution.x;
    float span = clamp(exp2(ceil(log2(perPixel))), 1.0, float(MAX_BIN));

    // the pixel's footprint in ring points; it is no wider than a bin, so it overlaps two bins at most
    float k = mod(uHead - (1.0 - uv.x) * window, size);
    float a = k - perPixel * 0.5;
    float b0 = floor(a / span);
    float edge = (b0 + 1.0) * span;
    float w0 = min(k + perPixel * 0.5, edge) - a;
    float w1 = perPixel - w0;

    float in0 = smoothstep(-1.0, 1.0, (heightOf(binMax(b0, span, size)) - uv.y) * iResolution.y);
    float in1 = w1 > 0.0 ? smoothstep(-1.0, 1.0, (heightOf(binMax(b0 + 1.0, span, size)) - uv.y) * iResolution.y) : 0.0;
    float cover = (w0 * in0 + w1 * in1) / perPixel;

    float h = max(uv.y, 0.0);
    float alpha = mix(ALPHA_BOTTOM, ALPHA_TOP, pow(h, ALPHA_CURVE));

    fragColor = vec4(rainbow(pow(h, COLOR_CURVE)), cover * alpha);
}
