import("stdfaust.lib");

// Roland Juno-60 Stereo Chorus
//
// Emulates the MN3009 BBD (256-stage bucket-brigade) chorus circuit.
//
// Chorus I:  single LFO at ~0.513 Hz — warm, subtle stereo widening
// Chorus II: two LFOs (~0.513 + 0.863 Hz) summed — richer, shimmering
//
// Stereo spread: L and R receive opposite-polarity LFO modulation so
// the pitch drifts up on one side while it drifts down on the other,
// matching the Juno-60 circuit topology.

// --- UI ---
mode = nentry("mode[style:radio{'I':0;'II':1}]", 0, 0, 1, 1) : int;
dry  = hslider("dry",  0.5, 0, 1, 0.01);
wet  = hslider("wet",  0.5, 0, 1, 0.01);

// --- Juno-60 circuit-derived constants ---
// MN3009: 256 stages, nominal clock ~21 kHz → center delay ≈ 6 ms
// LFO sweeps clock between ~15–45 kHz → delay range ~2.8–8.5 ms
rate1  = 0.513;    // primary LFO (Hz)
rate2  = 0.863;    // secondary LFO, Chorus II only (Hz)
dctr   = 0.006;    // center delay: 6 ms
ddepth = 0.003;    // modulation depth: ±3 ms

MAXN = 1 << 17;    // delay buffer size in samples

// --- BBD emulation ---
// MN3009 has a 2-pole anti-aliasing/reconstruction filter rolling off ~8 kHz
bbd = fi.lowpass(2, 8000);

// Delay time in samples, clamped to >= 1
samp(t) = max(1.0, t * float(ma.SR));

// --- LFOs ---
lfo1 = os.osc(rate1);
lfo2 = os.osc(rate2);

// Chorus I: single LFO, L/R polarity inverted for stereo spread
dtI_L = samp(dctr + lfo1 * ddepth);
dtI_R = samp(dctr - lfo1 * ddepth);

// Chorus II: two-LFO sum, cross-mixed between channels for shimmer
// Left gets  (+lfo1 + lfo2), right gets (-lfo1 + lfo2)
// The 0.5 factor keeps total depth the same as Chorus I
dtII_L = samp(dctr + ( lfo1 + lfo2) * ddepth * 0.5);
dtII_R = samp(dctr + (-lfo1 + lfo2) * ddepth * 0.5);

// Pure wet chorus: returns two modulated delay lines, no dry signal
chorus(x) = wetL, wetR
with {
    dtL  = select2(mode, dtI_L, dtII_L);
    dtR  = select2(mode, dtI_R, dtII_R);
    wetL = de.fdelay(MAXN, dtL, x) : bbd;
    wetR = de.fdelay(MAXN, dtR, x) : bbd;
};

process(x) = outL, outR
with {
    dtL  = select2(mode, dtI_L, dtII_L);
    dtR  = select2(mode, dtI_R, dtII_R);
    wL   = de.fdelay(MAXN, dtL, x) : bbd;
    wR   = de.fdelay(MAXN, dtR, x) : bbd;
    outL = x * dry + wL * wet;
    outR = x * dry + wR * wet;
};
