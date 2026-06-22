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
mode        = nentry("mode[style:radio{'I':0;'II':1}][symbol:mode]", 0, 0, 1, 1) : int;
true_stereo = checkbox("true stereo[symbol:true_stereo]");
dry         = hslider("dry [unit:dB][symbol:dry]", -6.0, -96.0, 0.0, 0.1) : ba.db2linear;
wet         = hslider("wet [unit:dB][symbol:wet]", -6.0, -96.0, 0.0, 0.1) : ba.db2linear;
drywet      = hslider("drywet [unit:%][symbol:drywet]",50,0,100,1) / 100;
rate1       = hslider("rate1[unit:Hz][symbol:rate1]",  0.513, 0.05, 5.0,    0.001);  // primary LFO (Hz)
rate2       = hslider("rate2[unit:Hz][symbol:rate2]",  0.863, 0.05, 5.0,    0.001);  // secondary LFO, mode II only (Hz)
dctr        = hslider("dctr [unit:ms][symbol:dctr]",       6.0,   1.0,  20.0,  0.1)  / 1000;
ddepth      = hslider("ddepth [unit:ms][symbol:ddepth]",     3.0,   0.0,  10.0,  0.01) / 1000; // LFO rate detune between L/R instances (true stereo)
detune      = hslider("detune [unit:%][symbol:detune]",      5.0,   0.0,  50.0,  0.1)  / 100;
hp_freq     = hslider("hp_freq [unit:Hz][symbol:hp_freq]",    20,    20,   2000,  1);
lp_freq     = hslider("lp_freq [unit:Hz][symbol:lp_freq]",    20000, 200,  20000, 1);

MAXN = 1 << 17;    // delay buffer size in samples

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

// True stereo: two detuned instances, one per channel
// Instance A processes L (rates detuned down), instance B processes R (rates detuned up)
// Their stereo outputs are summed back to a stereo pair
lfo1_a = os.osc(rate1 * (1 - detune));
lfo1_b = os.osc(rate1 * (1 + detune));
lfo2_a = os.osc(rate2 * (1 - detune));
lfo2_b = os.osc(rate2 * (1 + detune));

dtIII_LL = samp(dctr + lfo1_a * ddepth);
dtIII_LR = samp(dctr - lfo1_a * ddepth);
dtIII_RL = samp(dctr + lfo1_b * ddepth);
dtIII_RR = samp(dctr - lfo1_b * ddepth);

dtIV_LL = samp(dctr + ( lfo1_a + lfo2_a) * ddepth * 0.5);
dtIV_LR = samp(dctr + (-lfo1_a + lfo2_a) * ddepth * 0.5);
dtIV_RL = samp(dctr + ( lfo1_b + lfo2_b) * ddepth * 0.5);
dtIV_RR = samp(dctr + (-lfo1_b + lfo2_b) * ddepth * 0.5);

dryWetMixerUnity(dw, X) = _,_ <: (*(dG),*(dG)), (X : *(wG),*(wG)) :> _,_
with { dG = min(1.0, 2.0*(1.0-dw)); wG = min(1.0, 2.0*dw); };

// equal-power crossfade: both channels at -3 dB at mid position
dryWetMixer3dB(dw, X) = _,_ <: (*(dG),*(dG)), (X : *(wG),*(wG)) :> _,_
with { dG = cos(dw * ma.PI/2.0); wG = sin(dw * ma.PI/2.0); };

process = dryWetMixer3dB(drywet, chorus);

chorus(L, R) = outL, outR
with {
    // Modes I/II: sum to mono, single delay pair
    mono   = L + R;
    dtL_12 = select2(mode, dtI_L, dtII_L);
    dtR_12 = select2(mode, dtI_R, dtII_R);
    wL_12  = de.fdelay(MAXN, dtL_12, mono);
    wR_12  = de.fdelay(MAXN, dtR_12, mono);

    // True stereo, mode I
    wLL_3  = de.fdelay(MAXN, dtIII_LL, L);
    wLR_3  = de.fdelay(MAXN, dtIII_LR, L);
    wRL_3  = de.fdelay(MAXN, dtIII_RL, R);
    wRR_3  = de.fdelay(MAXN, dtIII_RR, R);
    wL_3   = wLL_3 + wRL_3;
    wR_3   = wLR_3 + wRR_3;

    // True stereo, mode II
    wLL_4  = de.fdelay(MAXN, dtIV_LL, L);
    wLR_4  = de.fdelay(MAXN, dtIV_LR, L);
    wRL_4  = de.fdelay(MAXN, dtIV_RL, R);
    wRR_4  = de.fdelay(MAXN, dtIV_RR, R);
    wL_4   = wLL_4 + wRL_4;
    wR_4   = wLR_4 + wRR_4;

    wL_raw = select2(true_stereo, wL_12, select2(mode, wL_3, wL_4));
    wR_raw = select2(true_stereo, wR_12, select2(mode, wR_3, wR_4));
    wL     = wL_raw : fi.svf.hp(hp_freq,0.7) : fi.svf.lp(lp_freq,0.7);
    wR     = wR_raw : fi.svf.hp(hp_freq,0.7) : fi.svf.lp(lp_freq,0.7);
    // outL   = L * dry + wL * wet;
    // outR   = R * dry + wR * wet;
    outL   = wL * 0.5;
    outR   = wR * 0.5;
};

