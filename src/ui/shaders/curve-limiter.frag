/* Libre Audio Suite - https://libreaudio.org
 * Copyright (C) 2026 Klaus Scheuermann <klaus@libreaudio.org>
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of the Libre Audio Suite. It comes with ABSOLUTELY NO
 * WARRANTY; see the LICENSE file in the project root for the full terms.
 *
 * For some parts, a large language model was involved as a coding assistant.
 * The ideas, the design decisions and the listening behind it are purely human.
*/

/* ===========================================================================
   LAS Limiter - gain-reduction HISTOGRAM curve, Shadertoy port
   ---------------------------------------------------------------------------
   What the limiter has been doing for the last WIN seconds, scrolling right to
   left: the right edge is now, the left edge is WIN seconds ago.

   Standalone, the top of the display is 0 dB and the bottom is -DBRANGE. Hosted
   in the limiter UI it is an overlay on the waveform: it hangs from the 0 dB
   line of the display's level scale (uDbMin .. uDbMax, shared with the waveform
   and the grid; with uDbMax at 0 dB that is the top edge) and uses that scale's
   dB per pixel, so 6 dB of reduction reaches the -6 dB line. Each column of the history is filled from the top down to its
   own reduction, and that fill is the whole drawing -- there is no stroke along
   its lower edge. Depth of reduction
   reads as area, which is what a histogram is for and what a single bar meter
   cannot show; it also means a reduction arriving in one column stands as its
   own block rather than being joined to its neighbour by a line up the riser.

   Two things the drawing does on purpose.

   The fill is transparent at 0 dB and gains opacity going down the scale, on a
   curve set by GRAD, reaching FILL at -DBRANGE. The gradient belongs to the dB
   axis, not to the column: a given depth always has the same opacity, whatever
   the reduction around it happens to be doing. So the 0 dB edge has no hard line
   of its own, and depth reads twice over -- as area and as brightness -- because
   a shallow reduction only ever reaches the faint end of the gradient. A wall of
   limiting and a column or two of it separate at a glance.

   The palette travels with the audio instead of sitting still on the screen.
   Colour is a function of a column's own timestamp, not of where it currently
   is, so a hit keeps the colour it arrived in and the whole sweep rolls left at
   the scroll speed. The seven stops are closed into a loop for this - the
   open-ended sweep the other curve shaders use would show a seam rolling
   across every CSEC seconds. Hosted, the timestamp is the column's position in
   the history ring, so the palette turns once per ring.

   Curve only: the background stays black and fully transparent (alpha 0), so it
   drops onto the analyser like the other curve shaders.

   The history itself cannot be computed here -- a fragment program keeps no
   state between frames -- so the host keeps it and hands it over as the
   widget's data texture: a ring the same size as the waveform's, holding the
   reduction in dB (positive). The plugin sends it with each waveform point --
   the deepest reduction within that point's slice of audio -- and the UI pushes
   it to the same position as the point. uHead is the waveform's own smoothed head, and the ring is read
   through the same window and bins as waveform-scroll.frag (MARGIN, power-of-two
   bins, coverage per pixel), so the reduction and the waveform scroll as one and
   a hit sits exactly over the peak that caused it. Standalone (the Shadertoy
   editor, which has neither custom uniforms nor that texture) the same seam
   invents a limiter's worth of movement instead, so the file pastes straight
   into a new shader and still looks like the thing it draws.

   Two things worth knowing about what is being drawn. The reduction arrives
   already peak-held with a 0.3 s decay (grHold in limiter.dsp), because the UI
   reads the parameter once per block and would otherwise show whichever sample
   the block happened to end on -- so the tails here are the meter's decay, not
   the limiter's release, and only the attacks are the limiter's own. And each
   column is the deepest reduction within its slice of time, not an average: a
   transient narrower than a column still reaches its full depth rather than
   being diluted by the quiet either side of it. Hosted, a column is one bin of
   the ring, at least a pixel wide, and keeps the deepest reduction in it.
   =========================================================================== */

#define DBRANGE 24.0  /* depth of the fill's gradient, dB of reduction - matches MAXGR in limiter.dsp */
#define WIN     8.0   /* time window shown standalone, seconds */
#define CSEC    WIN   /* seconds per turn of the palette; WIN = one sweep per width */
#define FADEDB  0.25  /* reduction under which the fill fades away entirely, dB */

/* The fill's gradient down the dB scale: FILL is the opacity reached at
   -DBRANGE, GRAD how it gets there from transparent at 0 dB. 1 is a straight
   ramp; above 1 holds the top clearer and saves the colour for the last few dB;
   below 1 brings it up early, so shallow limiting is already bright and the
   gradient reads more as a softened top edge than as a depth scale.

   These are what the fill's brightness hangs on. Drawn with SRC_ALPHA /
   ONE_MINUS_SRC_ALPHA over a dark backdrop, what reaches the eye is the colour
   times the alpha, so anything short of full FILL is by definition darker than
   the palette it is drawn from -- which is the point here, but it does mean
   GRAD, not FILL, is the lever for how bright ordinary limiting looks.

   TINT scales the colour itself, which is a smaller lever: the scope palette is
   already pastel, most stops within a fifth of white, so above 1 the channels
   with headroom reach it first and the colour brightens by desaturating. Good
   for a little more presence, not for rescuing a low alpha. */
#define FILL 0.95
#define GRAD 0.10
#define TINT 1.0

#ifdef LIBREAUDIO_HOSTED
/* ring position of the newest point shown, and the display's level scale in dB
   (bottom and top edge) -- all shared with waveform-scroll.frag */
uniform float uHead;
uniform float uDbMin;
uniform float uDbMax;

/* ring points left out at the oldest end, and the widest bin -- must match
   waveform-scroll.frag so both read the ring through the same window */
#define MARGIN  256.0
#define MAX_BIN 64

/* reduction in dB at ring position i */
float grPoint(float i, float size){
    return texture2D(_dpf_texture_data, vec2((mod(i, size) + 0.5) / size, 0.5)).r;
}

/* deepest reduction in one bin of the ring */
float grBinMax(float bin, float span, float size){
    float m = 0.0;
    for (int j = 0; j < MAX_BIN; ++j){
        if (float(j) >= span) break;
        m = max(m, grPoint(bin * span + float(j), size));
    }
    return m;
}
#endif

/* The 7-stop scope palette, lerped in sRGB like the SVG gradient, but closed
   into a loop: the last stop runs back into the first, so x can roll for ever
   without a seam. The other curve shaders map their sweep across the width once
   and so use the open version. */
vec3 rainbowLoop(float x){
    x = fract(x) * 7.0;                  // 7 stops in a ring -> 7 segments
    int i = int(floor(x));
    float f = fract(x);
    vec3 c0 = vec3(1.000, 0.749, 0.796); // #ffbfcb
    vec3 c1 = vec3(1.000, 0.875, 0.678); // #ffdfad
    vec3 c2 = vec3(0.824, 0.992, 0.827); // #d2fdd3
    vec3 c3 = vec3(0.745, 0.945, 1.000); // #bef1ff
    vec3 c4 = vec3(0.765, 0.851, 1.000); // #c3d9ff
    vec3 c5 = vec3(0.855, 0.757, 0.953); // #dac1f3
    vec3 c6 = vec3(1.000, 0.863, 0.961); // #ffdcf5
    vec3 a = c0, b = c1;
    if      (i == 1){ a = c1; b = c2; }
    else if (i == 2){ a = c2; b = c3; }
    else if (i == 3){ a = c3; b = c4; }
    else if (i == 4){ a = c4; b = c5; }
    else if (i == 5){ a = c5; b = c6; }
    else if (i >= 6){ a = c6; b = c0; }
    return mix(a, b, f);
}

#ifndef LIBREAUDIO_HOSTED
/* One train of hits: a burst every `per` seconds taking `amp` dB, with an
   instant attack and an exponential release of time constant `rel`. Three of
   these under a slow swell stand in for programme material. */
float train(float t, float per, float amp, float rel, float ph){
    float since = fract(t / per + ph) * per;
    return amp * exp(-since / rel);
}

/* Gain reduction `age` seconds ago, in dB (negative). */
float grDb(float age){
    float t   = iTime - age;
    float sus = max(0.0, 3.0 + 2.5 * sin(t * 0.23) + 1.5 * sin(t * 0.091 + 1.3));
    float hit = max(max(train(t, 0.53,  9.0, 0.09, 0.00),
                        train(t, 1.31, 14.0, 0.22, 0.37)),
                        train(t, 0.17,  5.0, 0.05, 0.61));
    /* a slow swell over the top, so the picture has quiet passages too */
    return -max(sus, hit) * (0.55 + 0.45 * sin(t * 0.061));
}
#endif

#ifndef LIBREAUDIO_HOSTED
/* Reduction at horizontal position x (0 = left edge / oldest, 1 = right edge /
   now), normalised: 0 = none, 1 = DBRANGE or deeper. */
float grNorm(float x){
    x = clamp(x, 0.0, 1.0);
    float db = grDb((1.0 - x) * WIN);
    return clamp(-db / DBRANGE, 0.0, 1.0);
}
#endif

void mainImage(out vec4 fragColor, in vec2 fragCoord){
    vec2 uv = fragCoord / iResolution.xy;

#ifndef LIBREAUDIO_HOSTED
    /* 0 = top of the display = 0 dB, 1 = bottom = -DBRANGE, for this pixel and
       for the column's own reduction */
    float py = 1.0 - uv.y;
    float n  = grNorm(uv.x);

    /* Inside the fill, with the lower edge antialiased. Nothing draws the upper
       edge: the gradient below takes it to zero there. */
    float band = smoothstep(-1.0, 1.0, (n - py) * iResolution.y);

    /* Per column, so the display is empty wherever the limiter was idle rather
       than carrying a sliver of tint along the top. The gradient alone very
       nearly does this already; this is what makes it exact, and keeps it true
       if GRAD is taken below 1. */
    float cover = band * smoothstep(0.0, FADEDB / DBRANGE, n);

    /* A column's own timestamp drives the colour, so the sweep travels left with
       the audio it belongs to instead of standing still on the screen. */
    float stamp = (iTime - (1.0 - uv.x) * WIN) / CSEC;
#else
    float size     = float(_dpf_texture_size);
    float window   = size - MARGIN;
    float perPixel = window / iResolution.x;
    float span     = clamp(exp2(ceil(log2(perPixel))), 1.0, float(MAX_BIN));
    float pxPerDb  = iResolution.y / (uDbMax - uDbMin);

    /* this pixel's depth below the 0 dB line, dB; nothing is drawn above it */
    float depth = -mix(uDbMin, uDbMax, uv.y);
    float py    = clamp(depth / DBRANGE, 0.0, 1.0);

    /* The pixel's footprint in ring points overlaps two bins at most. Each bin is
       a block of its deepest reduction, faded out where the limiter was idle,
       and the pixel takes how much of its width the blocks cover at its depth. */
    float k     = mod(uHead - (1.0 - uv.x) * window, size);
    float a     = k - perPixel * 0.5;
    float b0    = floor(a / span);
    float w0    = min(k + perPixel * 0.5, (b0 + 1.0) * span) - a;
    float w1    = perPixel - w0;
    float gr0   = grBinMax(b0, span, size);
    float gr1   = w1 > 0.0 ? grBinMax(b0 + 1.0, span, size) : 0.0;
    float in0   = smoothstep(-1.0, 1.0, (gr0 - depth) * pxPerDb) * smoothstep(0.0, FADEDB, gr0);
    float in1   = smoothstep(-1.0, 1.0, (gr1 - depth) * pxPerDb) * smoothstep(0.0, FADEDB, gr1);
    float cover = step(0.0, depth) * (w0 * in0 + w1 * in1) / perPixel;

    /* the column's place in the ring is its timestamp */
    float stamp = k / size;
#endif

    /* Transparent at 0 dB, FILL at -DBRANGE. Measured down the scale rather than
       down the column, so opacity is a function of depth alone and less
       reduction is dimmer as well as smaller. */
    float grad = FILL * pow(py, GRAD);

    vec3 col = clamp(rainbowLoop(stamp) * TINT, 0.0, 1.0);

    /* Straight (non-premultiplied) alpha: DPF draws with
       glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), so premultiplying here
       would darken the colour a second time -- the antialiased edge would fringe
       towards black instead of fading to transparent. */
    fragColor = vec4(col, cover * grad);
}
