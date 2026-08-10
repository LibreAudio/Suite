/* ===========================================================================
   LAS Vocal Doubler (v8) — modulation-scope CURVE, Shadertoy port
   ---------------------------------------------------------------------------
   Two rainbow voice traces riding on the wet-EQ response (HP + LP + presence
   bell), with the De-Ess dynamic downward shelf on top. Curve only: the
   background stays black and fully transparent (alpha 0), so it drops onto
   anything.

   Every parameter that shapes the curve is a `uniform float` below. Drive
   them from your host. For standalone testing in the Shadertoy editor (which
   can't set custom uniforms), leave USE_DEFAULTS at 1 to use the constants.
   =========================================================================== */

/* ---- adjustable parameters (drive these as uniform float from a host) ---- */
uniform float uRate1;     // voice 1 trace rate, Hz
uniform float uRate2;     // voice 2 trace rate, Hz
uniform float uDepth;     // trace amplitude / wet depth (0 .. 1)
uniform float uShowB;     // draw the 2nd voice? (0 or 1) — on in 1/3 Doubler & 2-voice ADT
uniform float uLpHz;      // wet-EQ low-pass corner, Hz
uniform float uHpHz;      // wet-EQ high-pass corner, Hz
uniform float uLpRes;     // low-pass resonance bump  (0 .. 1)
uniform float uHpRes;     // high-pass resonance bump (0 .. 1)
uniform float uPresence;  // presence bell (0 .. 1, 0.5 = flat), ±12 dB at 2 kHz
uniform float uDeess;     // De-Ess amount (0 .. 1)
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
  #define R1   0.55
  #define R2   0.85
  #define DEPTH 0.35
  #define SHOWB 1.0
  #define LPHZ 7000.0
  #define HPHZ 20.0
  #define LPRES 0.0
  #define HPRES 0.0
  #define PRESENCE 0.62
  #define DEESS 0.6
  #define DEESSFREQ 3000.0
  #define DBMAX 18.0
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
  #define DEPTH uDepth
  #define SHOWB uShowB
  #define LPHZ uLpHz
  #define HPHZ uHpHz
  #define LPRES uLpRes
  #define HPRES uHpRes
  #define PRESENCE uPresence
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
#define REF_H 210.0   /* reference scope height the plugin's px amplitudes assume */

float log10_(float x){ return log(x) * 0.43429448190325176; }
float log2_ (float x){ return log(x) * 1.44269504088896341; }

/* resonance peak on the filter response (Gaussian in log-freq) */
float bump(float f, float fc, float res){
    if (res < 0.001) return 0.0;
    float x = log2_(f / fc);
    return res * 17.0 * exp(-(x * x) / (2.0 * 0.45 * 0.45));
}

/* presence bell: ±12 dB, centred at 2 kHz, wide (sigma 0.9 octaves) */
float presBell(float f){
    float presDb = (PRESENCE - 0.5) * 24.0;
    if (abs(presDb) < 0.05) return 0.0;
    float x = log2_(f / 2000.0);
    return presDb * exp(-(x * x) / (2.0 * 0.9 * 0.9));
}

/* wet-EQ magnitude response, dB */
float fdb(float f){
    float lp = -10.0 * log10_(1.0 + pow(f / LPHZ, 16.0));
    float hp = -10.0 * log10_(1.0 + pow(HPHZ / f, 16.0));
    return lp + hp + bump(f, LPHZ, LPRES) + bump(f, HPHZ, HPRES) + presBell(f);
}

/* exact 7-stop scope palette (evenly spaced), lerped in sRGB like the SVG gradient */
vec3 rainbow(float x){
    x = clamp(x, 0.0, 1.0) * 6.0;      // 7 stops → 6 segments
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

void mainImage(out vec4 fragColor, in vec2 fragCoord){
    vec2 uv = fragCoord / iResolution.xy;
    float t  = uv.x;               // 0..1 left→right (time & frequency axis)
    float py = 1.0 - uv.y;         // 0 at top, 1 at bottom (matches the plugin)

    /* x → frequency, log axis */
    float freq = FMIN * pow(FMAX / FMIN, t);

    /* wet-EQ response baseline, normalised into the dB window */
    float db = min(DBMAX, fdb(freq));
    float baseNorm = (DBMAX - db) / (DBMAX - DBMIN);

    /* De-Ess: downward high-shelf above the crossover, amount animated per
       frame to read as dynamic (sibilant) gain reduction — downward only.
       Softer knee (^3) and faster pump than the chorus, matching v8. */
    float wsh = 1.0 / (1.0 + pow(DEESSFREQ / freq, 3.0));
    float env = 0.0;
    if (DEESS > 0.001){
        env = max(0.0, 0.6 * sin(TAU * iTime * 4.0) + 0.4 * sin(TAU * (iTime * 6.3) + 1.1));
    }
    float shelfNorm = (DEESS * 15.0 * env) * wsh / (DBMAX - DBMIN);

    float base = baseNorm + shelfNorm;   // shelf pulls the curve DOWN (bigger y)

    /* trace amplitudes, in height fractions (plugin: a1 = 4 + depth*10 px @210, a2 = 0.75*a1) */
    float a1 = (4.0 + DEPTH * 10.0) / REF_H;
    float a2 = a1 * 0.75;

    float y1 = base - a1 * sin(TAU * (t * R1 * WIN - R1 * iTime));
    float y2 = base - a2 * sin(TAU * (t * R2 * WIN - (R2 * iTime + 0.25)));

    /* antialiased line coverage */
    float hw  = THICK * 0.5 / iResolution.y;
    float aa  = 1.5 / iResolution.y;
    float c1 = 1.0 - smoothstep(hw, hw + aa, abs(py - y1));
    float c2 = (SHOWB > 0.5) ? 1.0 - smoothstep(hw, hw + aa, abs(py - y2)) : 0.0;
    float cov = max(c1, c2);

    /* soft glow bloom around each trace (a wide gaussian falloff) */
    float gw = max(GLOWW, 0.001) / iResolution.y;
    float g1 = exp(-(py - y1) * (py - y1) / (gw * gw));
    float g2 = (SHOWB > 0.5) ? exp(-(py - y2) * (py - y2) / (gw * gw)) : 0.0;
    float glow = max(g1, g2) * GLOW;

    vec3 col = rainbow(t);

    /* fill between the two voice traces (matches fillOpacity 0.16) */
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
