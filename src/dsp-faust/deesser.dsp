declare author "Klaus Scheuermann";
declare description "Mid/side de-esser with independent detection and variable channel selection";
declare license "GPL-3.0-or-later";
declare name "De-Esser";
declare unique_id "LAes";

// declare drywet "true";

import("stdfaust.lib");

process = hfLimit;

uiTop(x)    = hgroup("[0]Top", x);
uiBottom(x) = hgroup("[8]Bottom", x);
uiBottomLeft(x) = uiBottom(hgroup("[1]Stage Bottom Left", x));
uiBottomRight(x) = uiBottom(hgroup("[1]Stage Bottom Right", x));
uiHidden(x) = hgroup("[9]Hidden", x);

uiMode(x)   = uiTop((hgroup("[0]Mode",  x)));
uiDelay(x)  = uiBottom(hgroup("[1]Delay", x));
uiLFO(x)    = uiBottom(hgroup("[2]LFO",   x));
uiMix(x)    = uiBottom(hgroup("[3]Mix",   x));
uiTone(x)   = uiBottom(hgroup("[4]Tone",  x));
uiDeEss(x)  = uiBottom(hgroup("[5]De-Esser", x));

hflim_amount = uiBottomRight(hslider("[21]De-Ess[style:knob][unit:%][symbol:deess_amount][label:De-Ess][accentcolor:02]", 0, 0, 100, 1)) / 100;
hflim_meter  = uiHidden(hbargraph("[1]HFlim Reduction[unit:dB][symbol:deess_meter]", 0, 30));


// --- High Frequency Limiter ---
// Adapted from vocalDoubler.dsp for independent mid/side processing.
//
// Level-independent: splits the input into a low ("body") band and a
// high band, then compares their envelopes as a ratio (dB difference)
// instead of the high band's absolute level. A quiet "s" in a quiet
// passage still spikes that ratio, so detection doesn't depend on overall
// loudness the way a plain high-band compressor does.

// One macro control drives all four parameters. To retune the feel, edit
// the endpoint pairs below: the first value is what the parameter is at
// Intensity 0%, the second at 100%, interpolated linearly in between.
// Nothing else needs touching.
hfLimSplitAt0  = 5000;  hfLimSplitAt100  = 4500;  // Hz   - crossover; lower reaches further down into the "sh" range
hfLimThreshAt0 =   -2;  hfLimThreshAt100 =   -14; // dB   - how far the high band must stick out before it counts
hfLimRatioAt0  =    2;  hfLimRatioAt100  =     8; //      - how hard the excess is squeezed
hfLimRangeAt0  =    0;  hfLimRangeAt100  =    18; // dB   - ceiling on total reduction; 0 at the bottom makes 0% a true bypass

lerp(a, b, t) = a + (b - a) * t;

// Defaults to 0, i.e. a true bypass, because a chorus is not a vocal-only
// box — every existing patch keeps sounding exactly as it did until this is
// turned up. (vocalDoubler ships it at 50.)


// hflim_split  = lerp(hfLimSplitAt0,  hfLimSplitAt100,  hflim_amount);
// hflim_thresh = lerp(hfLimThreshAt0, hfLimThreshAt100, hflim_amount);
// hflim_ratio  = lerp(hfLimRatioAt0,  hfLimRatioAt100,  hflim_amount);
// hflim_range  = lerp(hfLimRangeAt0,  hfLimRangeAt100,  hflim_amount);

hflim_split = uiDeEss(hslider("[01]Crossover Frequency[style:knob][unit:Hz][scale:log][symbol:crossover_frequency][label:Crossover][accentcolor:02]", 5000,3000,8000,1));

// Relative (100%) vs. absolute (0%) detection, continuously blendable.
// Relative: the high band is measured against the body band, so the threshold
//   is a spectral-tilt difference and detection is level-independent — a quiet
//   "s" in a quiet passage trips it just as readily as a loud one.
// Absolute: the high band is measured against 0 dBFS, so the threshold is a
//   plain level — a level-dependent high-band compressor, which follows the
//   singer's dynamics instead of ignoring them.
// In between, the reference and the threshold are mixed by the same amount, so
// the knob sweeps without a jump at either end.
hflim_mode = uiDeEss(hslider("[02]Mode[style:knob][unit:%][symbol:mode][label:Abs/Rel][accentcolor:05]", 100, 0, 100, 1)) / 100;

// Two thresholds, because the two modes measure different things and a single
// number cannot mean both. Each keeps its own value and its own useful range;
// the Mode knob lerps between them, so turning Mode never leaves the threshold
// in the wrong units and turning it back restores exactly what was dialled in.
hflim_threshAbs = uiDeEss(hslider("[03]Threshold Absolute[unit:dB][style:knob][symbol:threshold_abs][label:Thresh Abs][accentcolor:01]",-30,-60,0,1));
hflim_threshRel = uiDeEss(hslider("[04]Threshold Relative[unit:dB][style:knob][symbol:threshold][label:Thresh Rel][accentcolor:01]",-10,-30,0,1));
hflim_thresh    = lerp(hflim_threshAbs, hflim_threshRel, hflim_mode);

hflim_ratio = uiDeEss(hslider("[05]Ratio[symbol:ratio][style:knob][label:Ratio][accentcolor:03]", 1, 1, 20, 1));
hflim_range = uiDeEss(hslider("[06]Range[unit:dB][style:knob][symbol:range][label:Range][accentcolor:04]",6, 0, 20,1));

// Solo the band the de-esser acts on, with the reduction applied, so the
// crossover and threshold can be set by ear: sweep Crossover until the "s"
// dominates what you hear and the vowels drop away, then set Threshold /
// Ratio until only the "s" ducks. Expect a large level drop when engaging --
// it is a monitoring switch, not a mix control. Smoothed to cross-fade rather
// than hard-switch, so toggling it while playing does not click.
hflim_listen = uiDeEss(checkbox("[07]Listen[symbol:listen][label:Listen]
      [tooltip: Monitors the high band alone, with the de-essing applied, for setting Crossover and Threshold by ear. Turn off before printing]")) : si.smoo;

// At the centre, both channels share the higher detected reduction. Moving
// toward either endpoint removes the link and fades the other channel out.
hflim_channel = uiDeEss(hslider("[08]Channel[style:knob][symbol:channel][label:Mid/Side][accentcolor:05]
      [tooltip: -1 processes mid only (unlinked), 0 processes both (linked to the higher reduction), +1 processes side only (unlinked)]", 0, -1, 1, 0.01)) : si.smoo;
hflim_midWeight = 1 - max(0, hflim_channel);
hflim_sideWeight = 1 + min(0, hflim_channel);
hflim_link = 1 - abs(hflim_channel);

// Half-scaled encode and unity decode preserve the original stereo level.
// Each channel detects its own sibilance, including anti-phase side content.
hfLimit(l, r) = (mid, side, hfDetect(mid), hfDetect(side)) : render
with {
    mid = (l + r) * 0.5;
    side = (l - r) * 0.5;
    render(m, s, midHigh, midDetected, sideHigh, sideDetected) =
        attach(outMid + outSide, max(midReduction, sideReduction) : hflim_meter),
        outMid - outSide
    with {
        linked = max(midDetected, sideDetected);
        midReduction = hflim_midWeight * lerp(midDetected, linked, hflim_link);
        sideReduction = hflim_sideWeight * lerp(sideDetected, linked, hflim_link);
        outMid = hfApply(hflim_midWeight, midReduction, midHigh, m);
        outSide = hfApply(hflim_sideWeight, sideReduction, sideHigh, s);
    };
};

hfDetect(x) = high, reductionDb
with {
    // A real highpass, not `mono - lowpass`. Subtracting a Butterworth lowpass
    // does not give a Butterworth highpass: B(s)-1 has a single zero at DC, so
    // the complement rolls off at 6 dB/oct no matter what order the lowpass is,
    // and it overshoots at the corner. Measured against a 5 kHz split, that
    // "high band" was only -6 dB at 1 kHz, -0.1 dB at 2 kHz and +4.7 dB at
    // 5 kHz -- i.e. the detector was fed most of the vowel range plus a
    // resonant bump, so a plain 2.5 kHz tone pulled 1.9 dB of reduction. The
    // true 4th-order highpass is -33 dB at 2 kHz and -57 dB at 1 kHz.
    //
    // The two bands no longer need to be complementary: nothing reconstructs
    // the signal from them any more, they only feed the envelope followers.
    low  = fi.lowpass(4, hflim_split, x);
    high = fi.highpass(4, hflim_split, x);

    // Floored at -120 dB: on digital silence the follower reaches exactly 0,
    // and ba.linear2db(0) is -inf — which turns into NaN both in the relative
    // subtraction (-inf - -inf) and when the mode blend scales it by 0.
    env(x) = an.amp_follower_ar(0.001, 0.03, x) : max(ba.db2linear(-120)) : ba.linear2db;

    hiDb  = env(high);
    refDb = env(low);

    // dB the high band sticks out above its reference; only the excess over
    // threshold is limited. The reference is the body band scaled by the mode
    // blend: at 100% it is the full body level (relative — spectral tilt, level
    // independent), at 0% it is 0 dBFS (absolute — plain high-band level).
    // hflim_thresh is lerped by the same knob, so both sides of this comparison
    // cross-fade together.
    diff   = hiDb - refDb * hflim_mode;
    excess = max(0, diff - hflim_thresh);

    reductionDb = min(excess * (1 - 1 / hflim_ratio), hflim_range);
};

hfApply(weight, reductionDb, high, x) = out
with {
    gr = ba.db2linear(0 - reductionDb);

    // The reduction is applied as a real high shelf on the full-band signal,
    // not by re-summing a split. Rebuilding from a 4th-order split (low +
    // high*gr) is a shelf too, but an accidental one: the lowpass and its
    // complement differ in phase, so away from unity gain they no longer sum
    // flat and the response ripples around the corner.
    //
    // A TPT state variable filter: the structure is designed for exactly this,
    // coefficients that move every sample. At 0 dB the shelf mix collapses to
    // (1,0,0), so an idle de-esser passes the signal through untouched -- no
    // phase shift to comb against a dry path. The cost is one pow and one sqrt
    // per sample; tan(fc) depends only on hflim_split, so Faust hoists it out
    // of the sample loop.
    //
    // Second order, so gentler than a 3rd-order shelf: at a 5 kHz corner and
    // -12 dB it is -6.0 dB at the corner and still -1.6 dB down at 3 kHz.
    // Raise Q to confine the cut closer to the corner.
    //shelf = fi.highshelf(3, 0 - reductionDb, hflim_split);
    shelf = fi.svf.hs(hflim_split, 0.7, 0 - reductionDb );

    // Listen monitors the selected high bands in their stereo positions.
    out = lerp(x : shelf, high * gr * weight, hflim_listen);
};
