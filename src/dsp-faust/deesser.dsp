declare author "Klaus Scheuermann";
declare description "Mid/side de-esser with independent detection and variable channel selection";
declare license "GPL-3.0-or-later";
declare name "De-Esser";
declare unique_id "LAes";

declare graph "analyzer";
declare graphsource "input";

import("stdfaust.lib");

process = hfLimit;

// --- UI ---

uiBottom(x) = hgroup("[8]Bottom", x);
uiHidden(x) = hgroup("[9]Hidden", x);
uiDeEss(x)  = uiBottom(hgroup("[5]De-Esser", x));

hflim_split = uiDeEss(hslider("[01]Crossover[style:knob][unit:Hz][scale:log][symbol:crossover_frequency][label:Crossover][accentcolor:02]", 5000, 3000, 8000, 1));

// Detection reference, blendable: 100% measures the high band against the
// body band (level-independent spectral tilt), 0% against 0 dBFS (plain
// high-band level).
hflim_mode = uiDeEss(hslider("[02]Absolute / Relative[style:knob][unit:%][symbol:mode][label:Absolute/Relative][accentcolor:05]", 0, -100, 100, 1) + 100) /200;

// Each mode has its own threshold; Mode blends between them.
hflim_threshAbs = uiDeEss(hslider("[03]Thresh Abs[unit:dB][style:knob][symbol:threshold_abs][label:Thresh Abs][accentcolor:01]", -30, -60, 0, 1));
hflim_threshRel = uiDeEss(hslider("[04]Thresh Rel[unit:dB][style:knob][symbol:threshold][label:Thresh Rel][accentcolor:01]", -10, -30, 0, 1));
hflim_thresh    = lerp(hflim_threshAbs, hflim_threshRel, hflim_mode);

hflim_ratio = uiDeEss(hslider("[05]Ratio[unit::1][symbol:ratio][style:knob][label:Ratio][accentcolor:03]", 1, 1, 20, 1));
hflim_range = uiDeEss(hslider("[06]Range[unit:dB][style:knob][symbol:range][label:Range][accentcolor:04]", 6, 0, 20, 1));

// Solos the high band with the reduction applied, for setting Crossover and
// Threshold by ear. Smoothed so toggling does not click.
hflim_listen = uiDeEss(checkbox("[07]Listen[symbol:listen][label:Listen]
      [tooltip: Monitors the high band alone, with the de-essing applied, for setting Crossover and Threshold by ear. Turn off before printing]")) : si.smoo;

// -1 = mid only, 0 = both (linked to the higher reduction), +1 = side only.
// Moving away from the centre fades the link and the other channel out.
hflim_channel = uiDeEss(hslider("[08]Mid/Side[style:knob][symbol:channel][label:Mid/Side][accentcolor:05]
      [tooltip: -1 processes mid only (unlinked), 0 processes both (linked to the higher reduction), +1 processes side only (unlinked)]", 0, -100, 100, 1)) /100 : si.smoo;
hflim_midWeight  = 1 - max(0, hflim_channel);
hflim_sideWeight = 1 + min(0, hflim_channel);
hflim_link       = 1 - abs(hflim_channel);

// Delays the audio (and the Listen band) behind the detector so the gain is
// already down when the "s" arrives. Reported to the host as latency.
hflim_lookaheadMs = uiDeEss(hslider("[09]Lookahead[style:knob][unit:ms][symbol:lookahead][label:Lookahead][accentcolor:04]
      [tooltip: Delays the audio so the reduction is already in place when the sibilant arrives. Reported to the host as latency and compensated. 0 = off]", 0, 0, 100, 1));
maxLookaheadSamples = 19200; // 100 ms at 192 kHz
lookaheadSamples    = int(hflim_lookaheadMs * ma.SR / 1000) : min(maxLookaheadSamples);
lookahead           = de.delay(maxLookaheadSamples, lookaheadSamples);

// Parallel de-essing. Dry is the lookahead-delayed input, so the blend stays
// phase aligned. Listen is unaffected.
hflim_drywet = uiDeEss(hslider("[10]Dry/Wet[style:knob][unit:%][symbol:drywet][label:Dry/Wet][accentcolor:02]
      [tooltip: Blends the unprocessed signal with the de-essed signal. 100% = fully de-essed]", 100, 0, 100, 1)) / 100 : si.smoo;

// Outputs only what the de-esser removes (dry minus processed, after
// Dry/Wet), for hearing exactly what is being taken out. Smoothed so toggling
// does not click.
hflim_delta = uiDeEss(checkbox("[11]Delta[symbol:delta][label:Delta]
      [tooltip: Outputs only the signal the de-esser removes, for checking that only sibilance is affected. Turn off before printing]")) : si.smoo;

// Meter ballistics, on the meters only (detection and audio are unaffected):
// a rise within a millisecond and a release of meterRelease seconds (time
// constant), so short sibilants stay readable.
meterRelease = 0.2;
meterBallistics = si.onePoleSwitching(0.001, meterRelease);

hflim_meter = meterBallistics : uiHidden(hbargraph("[1]HFlim Reduction[unit:dB][symbol:deess_meter]", 0, 30));
// A passive widget with this exact symbol becomes the plugin's reported latency
// (see the latency_samples cases in src/templates/dsp.cpp.in).
latency_meter = uiHidden(hbargraph("[2]latency_samples[symbol:latency_samples]", 0, maxLookaheadSamples));

// Sidechain levels per channel, on the same scales as the two thresholds:
// absolute = high band in dBFS, relative = high band above the body band.
scAbsMin = -60; scAbsMax = 0;
scRelMin = -30; scRelMax = 10;
scMeter(meter, lo, hi) = meterBallistics : max(lo) : min(hi) : meter;
sc_mid_abs  = scMeter(uiHidden(hbargraph("[3]Sidechain Mid Absolute[unit:dB][symbol:sc_mid_abs]",   scAbsMin, scAbsMax)), scAbsMin, scAbsMax);
sc_mid_rel  = scMeter(uiHidden(hbargraph("[4]Sidechain Mid Relative[unit:dB][symbol:sc_mid_rel]",   scRelMin, scRelMax)), scRelMin, scRelMax);
sc_side_abs = scMeter(uiHidden(hbargraph("[5]Sidechain Side Absolute[unit:dB][symbol:sc_side_abs]", scAbsMin, scAbsMax)), scAbsMin, scAbsMax);
sc_side_rel = scMeter(uiHidden(hbargraph("[6]Sidechain Side Relative[unit:dB][symbol:sc_side_rel]", scRelMin, scRelMax)), scRelMin, scRelMax);

lerp(a, b, t) = a + (b - a) * t;

// --- DSP ---

// Half-scaled M/S encode, unity decode. Mid and side each detect their own
// sibilance (including anti-phase side content) on the undelayed signal; the
// audio path is delayed by the lookahead.
hfLimit(l, r) = (mid, side, hfDetect(sc_mid_abs, sc_mid_rel, mid), hfDetect(sc_side_abs, sc_side_rel, side)) : render
with {
    mid  = (l + r) * 0.5;
    side = (l - r) * 0.5;
    render(m, s, midHigh, midDetected, sideHigh, sideDetected) =
        attach(attach(outMid + outSide, max(midReduction, sideReduction) : hflim_meter),
               lookaheadSamples : latency_meter),
        outMid - outSide
    with {
        linked        = max(midDetected, sideDetected);
        midReduction  = hflim_midWeight  * lerp(midDetected,  linked, hflim_link);
        sideReduction = hflim_sideWeight * lerp(sideDetected, linked, hflim_link);
        outMid  = hfApply(hflim_midWeight,  midReduction,  midHigh : lookahead,  m : lookahead);
        outSide = hfApply(hflim_sideWeight, sideReduction, sideHigh : lookahead, s : lookahead);
    };
};

// Returns the high band (for Listen) and the gain reduction in dB, and feeds
// the absolute and relative sidechain levels to the given meters.
// The bands only feed envelope followers, so they need not be complementary;
// a true 4th-order highpass keeps vowel energy out of the detector.
hfDetect(absMeter, relMeter, x) = high, attach(attach(reductionDb, hiDb : absMeter), relShown : relMeter)
with {
    low  = fi.lowpass(4, hflim_split, x);
    high = fi.highpass(4, hflim_split, x);

    // Floored at -120 dB so silence never yields -inf/NaN.
    env(y) = an.amp_follower_ar(0.001, 0.03, y) : max(ba.db2linear(-120)) : ba.linear2db;

    hiDb  = env(high);
    lowDb = env(low);
    relDb = hiDb - lowDb;

    // Excess of the high band over its reference (body band scaled by Mode).
    diff   = hiDb - lowDb * hflim_mode;
    excess = max(0, diff - hflim_thresh);

    // Fades detection in between gateFloor and gateFloor + gateWidth of
    // high-band level, so silence and noise floors never trigger reduction.
    gateFloor = -70;
    gateWidth = 10;
    gate      = (hiDb - gateFloor) / gateWidth : max(0) : min(1);

    reductionDb = min(excess * (1 - 1 / hflim_ratio), hflim_range) * gate;

    // The relative reading is meaningless without high-band signal, so it
    // falls to the bottom of its scale through the same gate.
    relShown = lerp(scRelMin, relDb, gate);
};

// Applies the reduction as a TPT SVF high shelf on the full-band signal,
// which is safe to modulate per sample and passes through untouched at 0 dB.
// Dry/Wet blends the shelved signal with x, Delta swaps in the removed part,
// and Listen (taking precedence) crossfades to the weighted, reduced high band.
hfApply(weight, reductionDb, high, x) = lerp(lerp(processed, x - processed, hflim_delta), high * gr * weight, hflim_listen)
with {
    processed = lerp(x, x : shelf, hflim_drywet);
    gr    = ba.db2linear(0 - reductionDb);
    shelf = fi.svf.hs(hflim_split, 0.7, 0 - reductionDb);
};
