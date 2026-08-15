/* ===========================================================================
   LAS Chorus - modulation-scope CURVE, Shadertoy port
   ---------------------------------------------------------------------------
   Reproduces the two LFO traces riding on the filter response, plus the
   downward De-Ess high-shelf. Curve only: the background stays black and
   fully transparent (alpha 0), so it drops straight onto anything.

   Every parameter that shapes the curve is a `uniform float` below. Drive
   them from your host. For standalone testing in the Shadertoy editor (which
   can't set custom uniforms), leave USE_DEFAULTS at 1 to use the constants.
   =========================================================================== */

/* ---- adjustable parameters (drive these as uniform float from a host) ---- */
uniform float uRate1;     // LFO 1 rate, Hz          (0.05 .. 5)
uniform float uRate2;     // LFO 2 rate, Hz          (0.05 .. 5)
uniform float uAmp1;      // LFO 1 amplitude, fraction of height
uniform float uAmp2;      // LFO 2 amplitude, fraction of height
uniform float uShowB;     // draw the 2nd trace?     (0 or 1)
uniform float uLpHz;      // low-pass corner, Hz
uniform float uHpHz;      // high-pass corner, Hz
uniform float uLpRes;     // low-pass resonance bump  (0 .. 1)
uniform float uHpRes;     // high-pass resonance bump (0 .. 1)
uniform float uDeess;     // De-Ess amount           (0 .. 1)
uniform float uDeessFreq; // De-Ess shelf crossover, Hz
uniform float uDbMax;     // top of the dB window
uniform float uDbMin;     // bottom of the dB window
uniform float uWin;       // time window shown, seconds
uniform float uThick;     // line thickness, pixels
uniform float uGlow;      // glow strength (0 .. 1)
uniform float uGlowWidth; // glow radius, pixels
uniform float uFMin;      // frequency axis min, Hz (left edge)
uniform float uFMax;      // frequency axis max, Hz (right edge)

/* ---- standalone defaults (Shadertoy editor has no custom uniforms) ------- */
#define USE_DEFAULTS 1
#if USE_DEFAULTS
  #define R1   0.80
  #define R2   0.50
  #define AMP1 0.090
  #define AMP2 0.067
  #define SHOWB 1.0
  #define LPHZ 7000.0
  #define HPHZ 20.0
  #define LPRES 0.0
  #define HPRES 0.0
  #define DEESS 0.5
  #define DEESSFREQ 2000.0
  #define DBMAX 26.0
  #define DBMIN -42.0
  #define WIN  2.0
  #define THICK 2.0
  #define GLOW 0.55
  #define GLOWW 7.0
  #define FMIN 20.0
  #define FMAX 10000.0
#else
  #define R1   uRate1
  #define R2   uRate2
  #define AMP1 uAmp1
  #define AMP2 uAmp2
  #define SHOWB uShowB
  #define LPHZ uLpHz
  #define HPHZ uHpHz
  #define LPRES uLpRes
  #define HPRES uHpRes
  #define DEESS uDeess
  #define DEESSFREQ uDeessFreq
  #define DBMAX uDbMax
  #define DBMIN uDbMin
  #define WIN  uWin
  #define THICK uThick
  #define GLOW uGlow
  #define GLOWW uGlowWidth
  #define FMIN uFMin
  #define FMAX uFMax
#endif

#define TAU 6.28318530718

float log10_(float x){ return log(x) * 0.43429448190325176; }
float log2_ (float x){ return log(x) * 1.44269504088896341; }

/* resonance peak on the filter response (Gaussian in log-freq) */
float bump(float f, float fc, float res){
    if (res < 0.001) return 0.0;
    float x = log2_(f / fc);
    return res * 17.0 * exp(-(x * x) / (2.0 * 0.45 * 0.45));
}

/* combined HP + LP magnitude response, dB */
float fdb(float f){
    float lp = -10.0 * log10_(1.0 + pow(f / LPHZ, 16.0));
    float hp = -10.0 * log10_(1.0 + pow(HPHZ / f, 16.0));
    return lp + hp + bump(f, LPHZ, LPRES) + bump(f, HPHZ, HPRES);
}

/* exact 7-stop scope palette (evenly spaced), lerped in sRGB like the SVG gradient */
vec3 rainbow(float x){
    x = clamp(x, 0.0, 1.0) * 6.0;      // 7 stops -> 6 segments
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
    else if (i >= 5){ a = c5; b = c6; }
    return mix(a, b, f);
}

/* baseline (filter response + De-Ess shelf) in height fractions, 0 = top */
float baseY(float t, float env){
    /* x -> frequency, log axis */
    float freq = FMIN * pow(FMAX / FMIN, t);

    /* filter response, normalised into the dB window */
    float db = min(DBMAX, fdb(freq));
    float baseNorm = (DBMAX - db) / (DBMAX - DBMIN);

    /* De-Ess: downward high-shelf above the crossover, pulls the curve DOWN */
    float wsh = 1.0 / (1.0 + pow(DEESSFREQ / freq, 6.0));
    float shelfNorm = (DEESS * 18.0 * env) * wsh / (DBMAX - DBMIN);

    return baseNorm + shelfNorm;
}

/* One LFO trace wiggling on the baseline. `phase` is the scroll position in
   cycles, already wrapped to [0,1) by the caller - only its fractional part
   matters, and keeping it small stops the sine argument from quantising. */
float traceY(float t, float env, float rate, float amp, float phase){
    return baseY(t, env) - amp * sin(TAU * (t * rate * WIN - phase));
}

void mainImage(out vec4 fragColor, in vec2 fragCoord){
    vec2 uv = fragCoord / iResolution.xy;
    float t  = uv.x;               // 0..1 left->right (time & frequency axis)
    float py = 1.0 - uv.y;         // 0 at top, 1 at bottom (matches the plugin)

    /* De-Ess amount animated per frame to read as dynamic (sibilant) gain
       reduction - downward only */
    float env = 0.0;
    if (DEESS > 0.001){
        float e1 = pow(max(0.0, sin(TAU * fract(iTime * 0.8))), 10.0);
        float e2 = pow(max(0.0, sin(TAU * fract(iTime * 1.9 + 1.1))), 14.0);
        env = min(1.0, 0.25 + e1 + 0.7 * e2);
    }

    /* Scroll positions, wrapped to [0,1). Wrapping here is what keeps the sine
       argument small: unwrapped, TAU * rate * iTime lands where a single float
       step is a large fraction of a radian and the trace visibly staircases. */
    float ph1 = fract(R1 * iTime);
    float ph2 = fract(R2 * iTime + 0.25);

    float y1 = traceY(t, env, R1, AMP1, ph1);
    float y2 = traceY(t, env, R2, AMP2, ph2);

    /* Curve slope in pixels-of-y per pixel-of-x, by central difference one
       pixel either side. The filter skirts are ~160 dB/decade, so squeezed
       into the dB window they run near-vertical: without this a vertical-only
       distance test pinches the stroke to a hairline and the line reads as
       broken. Central (not forward) difference so the DBMAX clamp kink stays
       symmetric. */
    float dtx = 1.0 / iResolution.x;
    float s1 = (traceY(t + dtx, env, R1, AMP1, ph1) - traceY(t - dtx, env, R1, AMP1, ph1))
             * 0.5 * iResolution.y;
    float s2 = (traceY(t + dtx, env, R2, AMP2, ph2) - traceY(t - dtx, env, R2, AMP2, ph2))
             * 0.5 * iResolution.y;

    /* perpendicular distance to each trace, in pixels */
    float d1 = abs(py - y1) * iResolution.y * inversesqrt(1.0 + s1 * s1);
    float d2 = abs(py - y2) * iResolution.y * inversesqrt(1.0 + s2 * s2);

    /* antialiased line coverage (constant width whatever the slope) */
    float hw  = THICK * 0.5;
    float aa  = 1.5 / iResolution.y;
    float c1 = 1.0 - smoothstep(hw, hw + 1.5, d1);
    float c2 = (SHOWB > 0.5) ? 1.0 - smoothstep(hw, hw + 1.5, d2) : 0.0;
    float cov = max(c1, c2);

    /* soft glow bloom around each trace (a wide gaussian falloff) */
    float gw = max(GLOWW, 0.001);
    float g1 = exp(-(d1 * d1) / (gw * gw));
    float g2 = (SHOWB > 0.5) ? exp(-(d2 * d2) / (gw * gw)) : 0.0;
    float glow = max(g1, g2) * GLOW;

    vec3 col = rainbow(t);

    /* Juno-II fill between the two traces (matches fillOpacity 0.16) */
    float fill = 0.0;
    if (SHOWB > 0.5){
        float lo = min(y1, y2), hi = max(y1, y2);
        float band = smoothstep(-aa, aa, py - lo) * smoothstep(-aa, aa, hi - py);
        fill = band * 0.16;
    }

    /* composite: fill, then glow, then the crisp lines on top */
    float a = fill;
    a = glow + a * (1.0 - glow);
    a = cov  + a * (1.0 - cov);
    /* premultiplied output: black background is alpha 0 and lifts away cleanly */
    fragColor = vec4(col * a, a);
}
