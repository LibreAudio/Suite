declare author "Klaus Scheuermann";
declare description "";
declare license "GPL-3.0-or-later";
declare name "Gate";
declare unique_id "LAga";

// For some parts of this file, a large language model was involved as a coding assistant.
// The ideas, the design decisions and the listening behind it are purely human.

// declare drywet "true";

import("stdfaust.lib");

// Noise gate. The detector below is ef.gate_gain_mono (Julius O. Smith III,
// STK-4.3, misceffects.lib) with one change -- see gateGain. Below Threshold
// the signal is muted; Attack, Hold and Release shape how the gate opens and
// closes.
//
// Hysteresis gives the open and close decisions two different thresholds: the
// gate opens at Threshold but will not let go until the level has fallen a
// further Hysteresis dB. A signal hovering at the threshold would otherwise
// cross it over and over and chatter the gate open and shut. Hold fixes the
// same problem from the time axis by keeping the gate open for a minimum
// spell; Hysteresis fixes it on the level axis, with no minimum imposed, so a
// note that sustains around the threshold for seconds stays open without
// needing a Hold long enough to swallow real gaps.
//
// Range caps how far the gate is allowed to pull the signal down, so a closed
// gate ducks rather than mutes -- room tone and spill stay present instead of
// the track dropping into holes. At the top of its travel it is a true mute.
//
// Detection is *linked*: one gain, derived from |L| + |R|, drives both
// channels. Two independent detectors would open the channels by different
// amounts on the same transient and swing the stereo image with it.
//
// The sidechain section shapes what the detector hears and never the audio.
// External crossfades the detector from the plugin's own audio to an external
// sidechain. The filter is a high pass and low pass pair, a band pass or a
// bell: the pair keeps kick and hiss from holding the gate open, the band pass
// and bell key it on one region, and the tilt weights the whole detector
// towards one end of the spectrum. Listen monitors that filtered signal so it
// can be set by ear.
//
// Release Curve shapes the way down, with the same rate law as compressor.dsp:
// at 0% the gain falls in a straight line in dB, at 100% it drops quickly and
// then eases into the Range floor with a long tail.
//
// Lookahead delays the audio while the detector keeps reading it undelayed, so
// the gate is already open by the time the transient arrives at the output.
// Without it the attack always eats the leading edge of the note.
//
// No Bypass control here: the host wrapper provides one (see
// LibreAudioPlugin.cpp), and input/output trim, phase and M/S come from
// common/input.dsp and common/output.dsp.

Nch = 2;                            // gate is stereo

maxSR = 192000;
maxLookaheadMs = 10;
maxLookaheadSamples = int(maxLookaheadMs * maxSR / 1000);   // 1920

// The gate engine takes 4 inputs: 1-2 audio, 3-4 external sidechain.
// src/DistrhoPluginInfo.h #errors on anything but 2 inputs, so until the DPF
// side grows a sidechain port group `si.bus(Nch) <: si.bus(2 * Nch)` fans the
// audio out as [L, R, L, R] and the sidechain pair receives a copy of it. To
// open the real inputs, replace the line with `process = gate;`.
process = si.bus(Nch) <: si.bus(2 * Nch) : gate;

// --- UI structure ---

uiTop(x)    = hgroup("[0]Stage Top", x);
uiBottom(x) = hgroup("[8]Stage Bottom", x);
uiBottomLeft(x)  = uiBottom(hgroup("[1]Stage Bottom Left", x));
uiBottomRight(x) = uiBottom(hgroup("[1]Stage Bottom Right", x));
uiMeters(x) = hgroup("[9]", x);

uiMonitor(x) = uiTop(hgroup("[0]Monitor", x));

// --- Parameters ---

// Smoothed: the threshold is compared against the detector every sample, so
// stepping it while audio runs would otherwise snap the gate open or shut.
threshold = uiBottomLeft(hslider("[01]Threshold[style:knob][unit:dB][symbol:threshold][label:Threshold][accentcolor:01][easy]
      [tooltip: Level below which the signal is muted]", -30, -120, 0, 0.1)) : si.smoo;

// Not smoothed, unlike Threshold: taken straight from the slider it stays a
// per-block scalar, so the second threshold costs one multiply per sample
// rather than a second pow. A step in it can only flip the gate while the
// level sits between the two thresholds, and the flip is handed to the
// attack/release envelope, which ramps rather than clicks.
hysteresis = uiBottomLeft(hslider("[02]Hysteresis[style:knob][unit:dB][symbol:hysteresis][label:Hysteresis][accentcolor:01]
      [tooltip: How far below Threshold the level must fall before the gate closes. Stops it chattering on a signal that hovers at the threshold. 0 dB = off]",
      0, 0, 20, 0.1));

// Range is the floor the envelope runs down to, in dB. The top of the knob is a
// true mute: once the envelope reaches -60 dB the gain drops to exactly zero.
// That step from -60 dB to silence is the "infinity" mark on a hardware Range
// control and is inaudible. Not smoothed: a move of the floor is chased by the
// envelope like any other change of target, so it ramps rather than steps.
rangeMax = 60;

range = uiBottomLeft(hslider("[03]Range[style:knob][unit:dB][symbol:range][label:Range][accentcolor:01]
      [tooltip: Ceiling on how far the gate pulls the signal down. Lower values duck instead of muting, so room tone stays. At maximum the gate mutes completely]",
      rangeMax, 0, rangeMax, 0.1));

//---- sidechain source and filter ----
// Shapes what the detector hears, never the audio. The defaults leave the
// filter effectively out of circuit.

scMix = uiBottomLeft(hslider("[04]External[style:knob][unit:%][symbol:sc_mix][label:External][accentcolor:06][bracket:SIDECHAIN]
      [tooltip: Crossfades the detector between the plugin's own audio (0%) and the external sidechain (100%). While the sidechain pair is fed from a copy of the audio it changes nothing, but the parameter is here so the layout does not change once the real inputs are wired up]",
      0, 0, 100, 1)) / 100;

scMode = uiBottomLeft(nentry("[05]SC Filter[symbol:sc_filter][label:SC Filter][accentcolor:06][bracket:SIDECHAIN]
      [style:radio{'Filters':0;'Bandpass':1;'Bell':2}]
      [tooltip: Which filter shapes the detector: a high pass and low pass pair, a band pass, or a bell. Only the chosen filter's controls apply]",
      0, 0, 2, 1)) : int;

scHp = uiBottomLeft(hslider("[06]SC High Pass[style:knob][unit:Hz][scale:log][symbol:sc_hp][label:SC HP][accentcolor:06][bracket:SIDECHAIN]
      [tooltip: Keeps bass out of the detector, so kick and rumble stop holding the gate open. 20 Hz = effectively off]",
      20, 20, 500, 1));

scLp = uiBottomLeft(hslider("[07]SC Low Pass[style:knob][unit:Hz][scale:log][symbol:sc_lp][label:SC LP][accentcolor:06][bracket:SIDECHAIN]
      [tooltip: Keeps air, hiss and cymbal spill out of the detector. 20 kHz = effectively off]",
      20000, 1000, 20000, 1));

scBandFreq = uiBottomLeft(hslider("[08]SC Band Freq[style:knob][unit:Hz][scale:log][symbol:sc_band_freq][label:Band Freq][accentcolor:06][bracket:SIDECHAIN]
      [tooltip: Centre of the band the detector hears, in Bandpass mode]",
      1000, 40, 16000, 1));

scBandQ = uiBottomLeft(hslider("[09]SC Band Q[style:knob][scale:log][symbol:sc_band_q][label:Band Q][accentcolor:06][bracket:SIDECHAIN]
      [tooltip: Width of the band the detector hears, in Bandpass mode. Higher is narrower. The centre stays at 0 dB at any setting]",
      1, 0.3, 10, 0.01));

scBellFreq = uiBottomLeft(hslider("[10]SC Bell Freq[style:knob][unit:Hz][scale:log][symbol:sc_bell_freq][label:Bell Freq][accentcolor:06][bracket:SIDECHAIN]
      [tooltip: Centre of the bell weighting the detector, in Bell mode]",
      1000, 40, 16000, 1));

scBellGain = uiBottomLeft(hslider("[11]SC Bell Gain[style:knob][unit:dB][symbol:sc_bell_gain][label:Bell Gain][accentcolor:06][bracket:SIDECHAIN]
      [tooltip: Boost or cut of the bell, in Bell mode. A boost makes the detector react more to that band, so the gate opens on it]",
      0, -18, 18, 0.1));

scBellQ = 1;   // about an octave and a half wide

// One knob instead of a bell's three: a shelf pair pivoting at a fixed
// frequency, weighting the detector towards the highs or towards the lows
// rather than picking out one band. The same tilt compressor.dsp puts on its
// sidechain. At 0 dB both shelves collapse to unity, so an idle tilt is an
// exact bypass.
scTilt = uiBottomLeft(hslider("[12]SC Tilt[style:knob][unit:dB][symbol:sc_tilt][label:SC Tilt][accentcolor:04][bracket:SIDECHAIN]
      [tooltip: Weights the detector across the spectrum, pivoting at 700 Hz. Positive makes it hear the highs, so it reacts to snare and sibilance; negative makes it hear the low end]",
      0, -12, 12, 0.1));

scTiltFreq = 700;   // pivot
scTiltRes  = 0.7;   // shelf Q -- flat, no bump at the pivot

// Monitors the filtered sidechain instead of the gated audio, so the filters
// can be set by ear: sweep until the trigger stands alone and everything else
// falls away, then set Threshold. It is a monitoring switch, not a mix
// control -- the level is whatever the filters leave behind. Smoothed so it
// cross-fades rather than hard-switches and does not click when toggled while
// playing.
scListen = uiMonitor(checkbox("[01]Listen[symbol:sc_listen][label:Listen]
      [tooltip: Monitors the filtered sidechain, ungated, for setting the sidechain filters by ear. Turn off before printing]")) : si.smoo;

// The mode's filter, then the tilt, same construction as compressor.dsp. The
// band pass is scaled by 1/Q, as svf.bp peaks at Q. The tilt's pivot is
// constant, so its SVFs' tan() folds away at compile time and only the
// gain-dependent scalars are recomputed when the knob moves.
scFilter = _ <: (scHpLp, scBand, scBell) : select3(scMode)
         : fi.svf.ls(scTiltFreq, scTiltRes, 0 - scTilt)
         : fi.svf.hs(scTiltFreq, scTiltRes, scTilt)
with {
    scHpLp = fi.highpass(2, scHp) : fi.lowpass(2, scLp);
    scBand = fi.svf.bp(scBandFreq, scBandQ) : /(scBandQ);
    scBell = fi.svf.bell(scBellFreq, scBellQ, scBellGain);
};

// Times are floored at one sample: an.amp_follower_ar turns a zero time
// constant into a division by zero.
oneSample = 1.0 / float(ma.SR);

// Reported to the host as latency and compensated, so raising it does not
// shift the plugin against the rest of the session. Not smoothed and stepped
// in whole samples: the delay has to be exactly the number the latency meter
// reports, or the host's compensation would be aligned to a different figure.
lookaheadMs = uiBottomRight(hslider("[10]Lookahead[style:knob][unit:ms][symbol:lookahead][label:Lookahead][accentcolor:03][bracket:ENVELOPE]
      [tooltip: Delays the audio so the gate is already open when the transient arrives. Reported to the host as latency and compensated. 0 = off]",
      0, 0, maxLookaheadMs, 0.1));

lookaheadDelay = int(lookaheadMs * ma.SR / 1000);

// In ms rather than the µs of the original, so all three envelope controls
// read in the same unit. 0.01 ms is the same default as the 10 µs it had.
attack = uiBottomRight(hslider("[11]Attack[style:knob][unit:ms][scale:log][symbol:attack][label:Attack][accentcolor:03][bracket:ENVELOPE]
      [tooltip: Time constant for the gate to open (exponentially) from muted to unmuted]",
      0.01, 0.01, 10, 0.001)) : *(0.001) : max(oneSample);

hold = uiBottomRight(hslider("[12]Hold[style:knob][unit:ms][scale:log][symbol:hold][label:Hold][accentcolor:03][bracket:ENVELOPE]
      [tooltip: Time the gate stays open after the level falls below the Threshold]",
      200, 1, 1000, 1)) : *(0.001) : max(oneSample);

release = uiBottomRight(hslider("[13]Release[style:knob][unit:ms][scale:log][symbol:release][label:Release][accentcolor:03][bracket:ENVELOPE]
      [tooltip: Time constant for the gate to close. At Release Curve 0% the gain falls 8.7 dB per Release time; at 100% it covers 63% of the way to the Range floor in one Release time]",
      100, 1, 1000, 1)) : *(0.001) : max(oneSample);

releaseCurve = uiBottomRight(hslider("[14]Release Curve[style:knob][unit:%][symbol:release_curve][label:Curve][accentcolor:03][bracket:ENVELOPE]
      [tooltip: Shape of the release. 0% = straight line in dB, the classic gate fade. 100% = exponential into the Range floor: lets go quickly, then a long tail]",
      0, 0, 100, 1)) / 100;

// The dB span Release is measured over at curve 0: 20/ln(10), the 8.686 dB a
// one-pole in linear gain falls per time constant. That makes curve 0 exactly
// the library gate's release, and at curve 1 curveRef drops out entirely.
curveRef = 20 / log(10);

// How far the curve may scale the nominal rate, in either direction. Near the
// floor it stops the tail from asymptoting forever instead of landing on it.
curveBound = 8;

// --- Meter ---

// Shown as reduction (positive dB down) like the other LAS meters, hence the
// sign flip. Read straight off the dB envelope, so it tops out at Range.
maxGR = 60;
gate_meter = uiMeters(hbargraph("[1]Gate Reduction[unit:dB][symbol:gate_meter]", 0, maxGR));

// A passive widget with this exact symbol is what the build turns into the
// plugin's reported latency (see the latency_samples cases in
// src/templates/dsp.cpp.in), so the host delay-compensates Lookahead. Its max
// also sizes the wrapper's latency buffer, and LibreAudioPlugin.cpp asserts
// latency < that max -- hence the spare sample, so 10 ms at 192 kHz still fits.
latency_meter = _ <: attach(_, uiMeters(hbargraph("[2]latency_samples[symbol:latency_samples][unit:samples]",
                                                  0, maxLookaheadSamples + 1)));

lookaheadSamples = lookaheadDelay : latency_meter;

// --- Gate gain ---


// ef.gate_gain_mono up to its envelope, with its one stateless comparison
// replaced by a Schmitt trigger so the threshold can differ on the way up and
// on the way down. The level follower and the hold counter are the library's,
// kept as they stand. Returns the held decision, 1 = open; gateEnvelope turns
// it into a gain.
//
// The comparison had to move in here rather than wrap the library call: the
// `>` inside gate_gain_mono is memoryless, so there is nowhere outside it to
// introduce a second threshold from.
//
// At Hysteresis 0 the two thresholds are equal, select2 returns the same value
// on both branches and the expression collapses to exactly the library's
// `inlevel > db2linear(thresh)` -- same sample, no added delay, bit-identical
// output. Verified against the library build over 120k samples.
gateGain(x) = x : extendedrawgate
with {
    // The library's detector, untouched.
    minrate = min(attack, release);
    inlevel = an.amp_follower_ar(minrate, minrate);

    // One pow per sample for the open threshold, as before; the close
    // threshold is that scaled by a per-block constant.
    openAt  = ba.db2linear(threshold);
    closeAt = openAt * ba.db2linear(0 - hysteresis);

    holdsamps = int(hold * ma.SR);

    // What is fed back as the state is the *held* gate, not the bare
    // comparison, and that is what makes this work at audio rate. At the
    // default 0.01 ms attack the library's detector tracks the waveform rather
    // than its envelope, so the level returns to zero every cycle; a Schmitt
    // trigger on that bare comparison would cross both thresholds every period
    // however far apart they were, and hysteresis would do nothing at all.
    // Held, the state survives the dips between peaks, so each new peak is
    // judged against the lower threshold and the gate stays open until the
    // envelope itself has fallen Hysteresis dB. Hold therefore sets how long
    // the decision stays sticky -- at its 200 ms default, down to a 5 Hz
    // wobble; at its 1 ms minimum, only to about 1 kHz.
    extendedrawgate(x) = inlevel(x) : gateState;

    gateState = step ~ _
    with {
        step(wasOpen, lv) = extended
        with {
            // open while shut, close while open: the sticky comparison
            raw      = lv > select2(wasOpen > 0.5, openAt, closeAt);

            // hold, verbatim from the library: a down-counter reloaded
            // whenever the raw gate falls, holding the gate open while it runs
            reload   = raw < raw';
            counter  = (max(reload * holdsamps, _) ~ -(1));
            extended = max(float(raw), counter > 0);
        };
    };
};

// --- Envelope ---

// Runs in dB, between 0 (open) and -range (closed), because Release Curve is a
// shape in dB and needs a finite floor to aim at.
//
// Attack is the library's one-pole in linear gain, taken there and back, so it
// opens exactly as before. Release is compressor.dsp's rate law:
//
//     step per sample = (curveRef / (release * SR)) * (distance / curveRef)^curve
//
// curve 0 is a constant step, a straight line in dB -- which is what a
// one-pole in linear gain does on its way to zero. curve 1 is a step
// proportional to the distance left, an exponential in dB into the floor.
// The step never overshoots the target, so the envelope lands on the floor
// exactly, and the mute at Range maximum can test for it.
//
// The state starts on the target, so the gate starts closed rather than
// fading in from open.
gateEnvelope(open) = loop ~ _
with {
    target   = (open - 1) * range;
    floorLin = ba.db2linear(0 - range);
    pole     = ba.tau2pole(attack);

    loop(yPrev) = select2(atk, rel, atk_)
    with {
        y    = select2(1 - 1', yPrev, target);
        diff = target - y;
        dist = abs(diff);
        atk  = diff > 0;

        tLin = select2(open > 0.5, floorLin, 1);
        atk_ = ba.linear2db(tLin + (ba.db2linear(y) - tLin) * pole);

        rate = curveRef / (release * ma.SR);
        step = rate * pow(min(curveBound, max(1 / curveBound, dist / curveRef)), releaseCurve);
        rel  = y - min(step, dist);
    };
};

// --- Gate ---

// The gain is computed once and applied to both channels, rather than calling
// ef.gate_stereo and re-deriving the same gain a second time for the meter.
//
// The detector reads l and r undelayed while the audio goes through the
// lookahead delay: that is the whole point, the gain is already up when the
// transient reaches the multiplier. Both channels share one delay length, so
// the two references to lookaheadSamples are the same signal and the latency
// bargraph is instantiated once.
// External is attached to the output as well as used: while the sidechain
// pair is a copy of the audio its crossfade is between two equal signals, the
// compiler folds it away and would drop the parameter with it.
gate(l, r, scInL, scInR) = attach(outL, reductionDb : gate_meter), attach(outR, scMix)
with {
    delayed = de.delay(maxLookaheadSamples, lookaheadSamples);

    // External crossfades in the signal domain, ahead of the filter, so there
    // is one filter per channel and a blend of two sources reads as their sum.
    // The sidechain is not delayed: it is only ever looked at, like the
    // undelayed audio the detector reads.
    //
    // Filtered per channel and rectified afterwards, rather than filtering a
    // mono sum: |L| + |R| is the magnitude ef.gate_stereo feeds its detector,
    // so Threshold keeps meaning exactly what it did before the filters
    // existed. A mono sum would read 6 dB lower on correlated material and
    // silently move every stored Threshold with it.
    scL = it.interpolate_linear(scMix, l, scInL) : scFilter;
    scR = it.interpolate_linear(scMix, r, scInR) : scFilter;

    envDb = gateGain(abs(scL) + abs(scR)) : gateEnvelope;

    // Mute at the top of Range, once the envelope has landed on the floor.
    gr = ba.db2linear(envDb) * ((range < rangeMax) | (envDb > 0 - range));

    // The meter reads what is actually applied, so it tops out at Range.
    reductionDb = 0 - envDb;

    // The monitored sidechain goes through the same delay as the audio, so
    // what is heard stays aligned with the latency the host is compensating
    // for. It is deliberately not gated: the point is to hear what the
    // detector is being fed, before the decision it drives.
    outL = it.interpolate_linear(scListen, delayed(l) * gr, delayed(scL));
    outR = it.interpolate_linear(scListen, delayed(r) * gr, delayed(scR));
};
