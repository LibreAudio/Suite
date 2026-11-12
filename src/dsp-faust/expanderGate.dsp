// -*-Faust-*-

declare author "Klaus Scheuermann";
declare description "";
declare license "GPL-3.0-or-later";
declare name "Expander Gate";
declare unique_id "LAeg";

// Downward expander, built on compressor.dsp: same detector, sidechain,
// ballistics, output stage and layout, with the gain computer turned the other
// way. Below the threshold every dB the input falls takes Ratio dB off the
// output, and at the top of the Ratio travel that slope is infinite and it is
// a gate, the depth set by Range.

// declare drywet "true";

import("stdfaust.lib");

// number of channels
Nch = 2;

// The expander engine takes 4 inputs: 1-2 audio, 3-4 external sidechain.
// src/DistrhoPluginInfo.h #errors on anything but 2 inputs, and config.h.in
// takes DISTRHO_PLUGIN_NUM_INPUTS straight from this DSP's input count, so
// until the DPF side grows a sidechain port group `process` presents 2 inputs
// and feeds the engine's sidechain pair from a copy of the audio. See the
// bottom of the file for the one-line change that opens the real inputs.

maxGR = -60; // meter floor, also the widest Range setting
// span of the gain meter, which shows the gain reduction
meterFloor = -60;
meterCeiling = 0;

maxLookaheadSamples = 9600; // 50 ms at 192 kHz

//======================= GUI =======================

comp_group(x)  = vgroup("Expander", x);
meter_group(x) = comp_group(vgroup("[1]Meters", x));
knob_group(x)  = comp_group(hgroup("[0]Controls", x));

ctl_group(x)   = knob_group(hgroup("[0]Expansion Control", x));
env_group(x)   = knob_group(hgroup("[1]Expansion Response", x));
det_group(x)   = knob_group(hgroup("[2]Detector", x));
sc_group(x)    = knob_group(hgroup("[3]Sidechain Filter", x));

//---- gain computer ----

threshold = ctl_group(vslider("[0]Threshold[unit:dB][symbol:threshold][accentcolor:01]
      [tooltip: Level below which the signal is turned down]",
                              -40, maxGR, 0, 0.1));

// The top of the travel stands for infinity: the UI shows it as such, and the
// gain computer treats it as a gate.
ratioMax = 20;

ratio = ctl_group(vslider("[1]Ratio[scale:log][symbol:ratio][accentcolor:01]
      [tooltip: dB out per dB in below the threshold. 1 = no expansion, the
       top of the travel = infinite, a gate]",
                          2, 1, ratioMax, 0.1));

// The slope of the upward compressor this expander is the mirror image of:
// the fraction of every dB under the threshold that it would take away. 0 at
// Ratio 1, exactly 1 at infinity.
slope = select2(ratio >= ratioMax, 1 - 1 / max(1, ratio), 1);

knee = ctl_group(vslider("[2]Knee[unit:dB][symbol:knee][accentcolor:02]
      [tooltip: Width of the soft transition at the threshold. 1:1 from
       knee/2 dB above the threshold, the full ratio a little below it. At an
       infinite ratio it rounds the gate's corner, and the gate still closes at
       the threshold]",
                         6, 0, 24, 0.1));

range = ctl_group(vslider("[3]Range[unit:dB][symbol:range][accentcolor:02]
      [tooltip: Ceiling on total gain reduction. The expander never pulls
       the signal down by more than this, no matter how far under threshold.
       It is the depth of the gate at an infinite ratio]",
                          60, 0, 0 - maxGR, 0.1));
rangeKnee = knee; 

// rangeKnee = ctl_group(vslider("[4]Range Knee[unit:dB][symbol:range_knee]
//       [tooltip: Softens the approach to the Range ceiling the same way Knee
//        softens the approach to the threshold. 0 = hard clamp]",
//                               6, 0, 24, 0.1));

//---- ballistics ----

lookaheadMs = env_group(vslider("[0]Lookahead[unit:ms][symbol:lookahead][accentcolor:06]
      [tooltip: Delays the audio so the gain is already up when the transient
       arrives. Reported to the host as latency and compensated. 0 = off]",
                                0, 0, 50, 1));

attack = env_group(vslider("[1]Attack[unit:ms][scale:log][symbol:attack][accentcolor:04]
      [tooltip: How fast the gain opens up toward a lighter reduction. A 20 dB
       step starts out at 20 dB per Attack time and eases in as it nears the
       target]",
                           1, 0.01, 50, 0.01)) * 0.001;

// Fixed shape of the attack ramp, halfway between a straight line in dB (0)
// and exponential (1)
attackCurve = 0.5;

hold = env_group(vslider("[3]Hold[unit:ms][symbol:hold][accentcolor:03]
      [tooltip: How long the gain is held open at its highest point after the
       level drops, before release is allowed to start]",
                         0, 0, 100, 1)) * 0.001;

release = env_group(vslider("[4]Release[unit:ms][symbol:release][accentcolor:01]
      [tooltip: How fast the gain closes down toward a deeper reduction. Read
       as a 1/e time constant at Release Curve +1, or as the time to cover 20 dB
       at Release Curve 0]",
                            100, 1, 1000, 1)) * 0.001;

releaseCurve = env_group(vslider("[5]Release Curve[symbol:release_curve][unit:%][accentcolor:01]
      [tooltip: Shape of the release ramp. +1 = exponential, analog-style: lets
       go quickly then a long tail, and Release reads as a time constant.
       0 = straight line in dB]",
                                 100, 0, 100, 1)) /100;

holdSamples      = hold * ma.SR;

// Two names for one number. The Listen path has to be delayed by the same
// lookahead as the audio, but it must not carry the meter with it:
// latency_meter is a widget, and reusing the metered version would declare a
// second bargraph with the same symbol, which is what the build reads the
// plugin's reported latency off.
lookaheadDelay   = int(lookaheadMs * ma.SR / 1000);
lookaheadSamples = lookaheadDelay : latency_meter;

// The dB span the time knobs are measured over at curve = 0. It only sets
// where the linear and exponential readings of the same knob agree; at
// curve = +1 the knob is a time constant and this drops out entirely.
curveRef = 20;

// How far the curve is allowed to scale the nominal rate, in either
// direction. Near the target the cap stops the curved tail from asymptoting
// forever instead of settling; far from it, it stops a deep excursion from
// slamming at many times the nominal rate. Clamping the distance is equivalent
// and cheaper than clamping the result.
curveBound = 8;

//---- detector ----

rmsTime = det_group(vslider("[0]RMS Time[unit:ms][symbol:rms_time][accentcolor:05]
      [tooltip: Averaging window of the detector. Longer follows loudness and
       ignores spikes; 0 = peak, catches every transient]",
                            10, 0, 50, 1)) * 0.001;

link = det_group(vslider("[1]Link[unit:%][symbol:link][accentcolor:05]
      [tooltip: 0% = every channel gets its own gain reduction, 100% = all
       channels follow the most-reduced one, so the stereo image never shifts]",
                         100, 0, 100, 1)) / 100;

//---- sidechain source and filter ----
// Shapes what the detector hears, never the audio.

scMix = sc_group(vslider("[0]External[unit:%][symbol:sc_mix][accentcolor:03]
      [tooltip: Crossfades the detector between the plugin's own audio (0%) and
       the external sidechain (100%). While the sidechain pair is fed from a
       copy of the audio it changes nothing, but the parameter is here so the
       layout does not change once the real inputs are wired up]",
                         0, 0, 100, 1)) / 100;

scMode = sc_group(nentry("[1]SC Filter[symbol:sc_filter][accentcolor:03]
      [style:radio{'Filters':0;'Bandpass':1;'Bell':2}]
      [tooltip: Which filter shapes the detector: a high pass and low pass pair,
       a band pass, or a bell. Only the chosen filter's controls apply]",
                         0, 0, 2, 1)) : int;

scHp = sc_group(vslider("[2]SC High Pass[unit:Hz][scale:log][symbol:sc_hp][accentcolor:03]
      [tooltip: Keeps bass out of the detector so kick and low end stop driving
       the whole gain envelope. 20 Hz = effectively off]",
                        20, 20, 500, 1));

scLp = sc_group(vslider("[3]SC Low Pass[unit:Hz][scale:log][symbol:sc_lp][accentcolor:03]
      [tooltip: Keeps air and hiss out of the detector. 20 kHz = effectively off]",
                        20000, 1000, 20000, 1));

scBandFreq = sc_group(vslider("[4]SC Band Freq[unit:Hz][scale:log][symbol:sc_band_freq][accentcolor:03]
      [tooltip: Centre of the band the detector hears, in Bandpass mode]",
                              1000, 40, 16000, 1));

scBandQ = sc_group(vslider("[5]SC Band Q[scale:log][symbol:sc_band_q][accentcolor:03]
      [tooltip: Width of the band the detector hears, in Bandpass mode. Higher is
       narrower. The centre stays at 0 dB at any setting]",
                           1, 0.3, 10, 0.01));

scBellFreq = sc_group(vslider("[6]SC Bell Freq[unit:Hz][scale:log][symbol:sc_bell_freq][accentcolor:03]
      [tooltip: Centre of the bell weighting the detector, in Bell mode]",
                              1000, 40, 16000, 1));

scBellGain = sc_group(vslider("[7]SC Bell Gain[unit:dB][symbol:sc_bell_gain][accentcolor:03]
      [tooltip: Boost or cut of the bell, in Bell mode. A boost makes the
       detector react more to that band, as for de-essing]",
                              0, -18, 18, 0.1));

scTilt = sc_group(vslider("[8]SC Tilt[unit:dB][symbol:sc_tilt][accentcolor:03]
      [tooltip: Broad weighting of the detector across the spectrum, pivoting at
       SC Freq. Positive makes it hear highs, so it reacts to sibilance and
       cymbals; negative makes it hear the low end]",
                          0, -12, 12, 0.1));

scListen = sc_group(checkbox("[9]Listen[symbol:sc_listen][accentcolor:03]
      [tooltip: Replaces the output with what the detector hears - the
       sidechain mix through the sidechain filter - so the filter can be set
       by ear]"));

scFreq = 700;

scRes = 0.7; // shelf Q for the tilt pair — flat, no bump at the pivot

scBellQ = 1; // about an octave and a half wide

// The mode's filter, then the tilt: a shelf pair pivoting at scFreq, same
// construction as the Shelf mode of tiltEQ.dsp, equal and opposite low and
// high shelves. The band pass is scaled by 1/Q, as svf.bp peaks at Q.
scFilter = _ <: (scHpLp, scBand, scBell) : select3(scMode)
         : fi.svf.ls(scFreq, scRes, 0 - scTilt)
         : fi.svf.hs(scFreq, scRes, scTilt)
with {
    scHpLp = fi.highpass(2, scHp) : fi.lowpass(2, scLp);
    scBand = fi.svf.bp(scBandFreq, scBandQ) : /(scBandQ);
    scBell = fi.svf.bell(scBellFreq, scBellQ, scBellGain);
};

//---- meters ----

meterGain = max(meterFloor) : min(meterCeiling);

meter1 = _ <: (_, (meterGain : meter_group(hbargraph("[0]GR 1[unit:dB][symbol:gr_1]", meterFloor, meterCeiling)))) : attach;
meter2 = _ <: (_, (meterGain : meter_group(hbargraph("[1]GR 2[unit:dB][symbol:gr_2]", meterFloor, meterCeiling)))) : attach;

chanMeter(0) = meter1;
chanMeter(1) = meter2;

// Passive taps of the detector level for the UI's threshold meter: the level
// the threshold is compared against, after the sidechain mix and filter and
// the peak/RMS detector. Same ballistics
// as mbComp5's detector meters, rising is louder.
meterAtt = 0.005;
meterRel = 0.15;

level1 = _ <: (_, (max(maxGR) : min(0) : si.onePoleSwitching(meterAtt, meterRel)
                  : meter_group(hbargraph("[3]Level 1[unit:dB][symbol:level_1]", maxGR, 0)))) : attach;
level2 = _ <: (_, (max(maxGR) : min(0) : si.onePoleSwitching(meterAtt, meterRel)
                  : meter_group(hbargraph("[4]Level 2[unit:dB][symbol:level_2]", maxGR, 0)))) : attach;

chanLevel(0) = level1;
chanLevel(1) = level2;

// A passive widget with this exact symbol is what the build turns into the
// plugin's reported latency (see the latency_samples cases in
// src/templates/dsp.cpp.in), so the host delay-compensates Lookahead.
latency_meter = _ <: attach(_, meter_group(hbargraph("[2]latency_samples[symbol:latency_samples]", 0, maxLookaheadSamples)));

//======================= Level detector =======================
// RMS as a one-pole average of the squared signal. As rmsTime goes to 0 its
// pole goes to 0 and it collapses to |x|, so RMS Time 0 is a peak detector and
// sweeping the knob down to it moves continuously into peak.

levelDb(x) = x * x
           : si.smooth(ba.tau2pole(max(rmsTime, 1e-6)))
           : sqrt
           : max(ma.EPSILON)
           : ba.linear2db;

//======================= Gain computer =======================
// Returns gain reduction in dB, always <= 0.
//
// The curve is the mirror image, across the 1:1 diagonal, of an upward
// compressor with the same ratio and compressor.dsp's quadratic knee: that
// compressor maps input u to level L = u + slope * (knee/2 - (u - thresh))^2
// / (2 knee) inside the knee, and this one maps L back to u. Mirroring is what
// keeps the knee meaningful at an infinite ratio. A quadratic in the gain
// itself, as compressor.dsp has, would need infinite curvature there and
// collapse into a wall at thresh+knee/2; mirrored, it is the compressor's
// rounded corner turned on its side, and the gate still closes at the
// threshold.
//
// Above thresh+knee/2 nothing happens. Below thresh - (1-slope)*knee/2, where
// the mirrored knee ends, the full ratio applies: (ratio-1) dB off for every
// dB under the threshold, a slope of slope/(1-slope). That is unbounded at an
// infinite ratio, and the EPSILON keeps it finite for Range to clamp. In
// between, the knee is the compressor's quadratic solved for u, written in
// the form that stays exact as slope goes to 0.

gainComputer(level) = select3(zone, hardPart, softPart, 0)
with {
    hi       = threshold + knee / 2;
    lo       = threshold - (1 - slope) * knee / 2;
    zone     = (level > lo) + (level > hi);

    d        = hi - level;
    u        = 2 * d / (1 + sqrt(max(0, 1 - 2 * slope * d / max(ma.EPSILON, knee))));
    softPart = d - u;

    hardPart = slope / max(ma.EPSILON, 1 - slope) * (level - threshold);
};

//======================= Range ceiling =======================
// Same parabola trick as the knee, mirrored: instead of easing *into*
// expansion it eases into the floor, so a signal that slams far past the
// Range setting doesn't hit a corner in the gain curve.

rangeLimit(gr) = select3(zone, lim, soft, gr)
with {
    lim  = 0 - range;
    d    = gr - lim;

    // The soft floor peaks at lim + rk/2, so a knee wider than twice the
    // Range would lift the "gain reduction" above 0 dB and the expander
    // would quietly turn into a 3 dB boost (measured, with Range 0 and Range
    // Knee 24). Capping the knee at 2*range pins that peak to exactly 0 dB
    // and degrades to a hard clamp as Range approaches 0, which is what
    // Range 0 should mean anyway.
    rk   = min(rangeKnee, 2 * range);

    zone = (d > 0 - rk / 2) + (d > rk / 2);
    soft = lim + pow(d + rk / 2, 2) / (2 * max(ma.EPSILON, rk));
};

//======================= Ballistics =======================
// Curved attack/release with hold and program-dependent release.
//
// A one-pole lag can only ever be exponential, so the envelope is written as
// an explicit rate law instead. Each sample the gain moves toward the target
// by a step whose size depends on how far away the target still is:
//
//     step per sample = (curveRef / (time * SR)) * (|target - y| / curveRef)^m
//
// with m = the curve control:
//
//   m = +1  step proportional to the remaining distance -> exponential decay,
//           and `time` is exactly the 1/e time constant (curveRef cancels)
//   m =  0  step constant -> straight line in dB, curveRef dB per `time`
//
// Release Curve sets m for the release; the attack uses attackCurve.
//
// The step deliberately depends on the current distance and nothing else.
// The obvious alternative - interpolating s + (target-s)*p^k along a segment
// that restarts whenever the target reverses - deadlocks here: with peak
// detection the target reverses at every zero crossing, and any shape with
// zero initial slope gets its progress reset faster than it can move. Built
// that way, compressor.dsp measurably froze at 0.0001 dB of reduction.
// Distance carries no progress, so there is nothing for a reversal to reset.
//
// The directions are the expander's, the other way round from compressor.dsp:
// attack is the gain opening up as the level comes back over the threshold,
// release is the gain closing down as it falls under.
//
// Hold freezes the gain while the target is trying to release, until the
// counter runs out. Any renewed attack resets the counter, so hold always
// measures from the most recent highest point.

ballistics = loop ~ si.bus(2) : (_, !)
with {
    loop(y, hc, target) = ny, nhc
    with {
        diff = target - y;
        dist = abs(diff);
        atk  = diff > 0;        // target is higher: less gain reduction wanted

        nhc     = select2(atk, min(holdSamples, hc + 1), 0);
        holding = (diff < 0) * (nhc < holdSamples);

        tSec = select2(atk, release, attack);
        m    = select2(atk, releaseCurve, attackCurve);

        rate = curveRef / max(ma.EPSILON, tSec * ma.SR);

        step = rate * pow(min(curveBound, max(1 / curveBound, dist / curveRef)), m);

        // never step past the target, so short times land exactly on it
        // instead of chattering around it
        delta = ma.signum(diff) * min(step, dist);

        ny  = select2(holding, y + delta, y);
    };
};

//======================= Gain reduction, N channels =======================
// Feed-forward: the detector always hears the input, never the expanded
// output, so the ratio is exact.
//
// Linking happens on the gain computer's *target*, before the ballistics,
// so linked channels share one envelope instead of two envelopes racing.
//
//   gain computer -> link -> Range -> ballistics
//
// Range before the ballistics, so what the envelope chases is the finished
// target. Having it after the link costs nothing: it is monotonic, so limiting
// each channel and then taking the minimum gives the same answer as taking
// the minimum and then limiting.
//
// Takes 2*Nch inputs - the audio, then the external sidechain - and returns
// Nch gain reductions in dB. ro.interleave(Nch,2) turns [audio.., sc..] into
// one (audio, sc) pair per channel.

grComputeN = ro.interleave(Nch, 2)
           : par(i, Nch, detTarget(i))
           : linkN(Nch, link)
           : par(i, Nch, rangeLimit)
           : par(i, Nch, ballistics)
with {
    // SC Mix crossfades in the signal domain, ahead of the filter, so there is
    // only one filter instance per channel and a blend of two different
    // sources reads as their sum - which is what mixing a key into the program
    // should do.
    detTarget(i, x, sc) = it.interpolate_linear(scMix, x, sc)
                        : scFilter : levelDb : chanLevel(i) : gainComputer;
};

// crossfade each channel's own reduction against the deepest one of the set
linkN(N, l) = si.bus(N)
            <: (si.bus(N), (ba.parallelMin(N) <: si.bus(N)))
            :  ro.interleave(N, 2)
            :  par(i, N, it.interpolate_linear(l));

//======================= expander engine =======================
// 2*Nch in, Nch out: 1-2 audio, 3-4 external sidechain.
//
// The detector reads the signal undelayed while the audio goes through the
// lookahead delay, which is the whole point: by the time a transient reaches
// the multiplier its gain reduction is already in place. The sidechain is not
// delayed either, and needs no delay line of its own - it is only ever looked
// at, never heard.

expanderSC = si.bus(2 * Nch)
           <: (grComputeN,
               (par(i, Nch, de.delay(maxLookaheadSamples, lookaheadSamples)), par(i, Nch, !)))
           :  ro.interleave(Nch, 2)
           :  par(i, Nch, applyGain(i));

applyGain(i, grDb, x) = x * (grDb : chanMeter(i) : ba.db2linear);

//======================= process =======================
// Temporary 2-in front end. `_,_ <: si.bus(4)` fans out as [L, R, L, R], so
// the engine's sidechain pair receives a copy of the audio and the plugin
// still reports the 2 inputs src/DistrhoPluginInfo.h insists on. Every
// sidechain path stays live and testable; only the source is stubbed.
//
// To open the real inputs once DPF has a sidechain port group, replace the
// `si.bus(Nch) <: expanderSC` with `expanderSC`, do the same for
// `si.bus(Nch) <: scListenPath`, and widen the outer split to `si.bus(2 * Nch)`.

//======================= Sidechain listen =======================
// What the detector hears, made audible: the same mix and filter as
// detTarget. Delayed by the lookahead so switching Listen does not move the
// audio in time.

scListenPath = si.bus(2 * Nch)
             : ro.interleave(Nch, 2)
             : par(i, Nch, it.interpolate_linear(scMix) : scFilter)
             : par(i, Nch, de.delay(maxLookaheadSamples, lookaheadDelay));

listenSelect = ro.interleave(Nch, 2) : par(i, Nch, select2(scListen));

// External is attached to the output as well as used: while the sidechain
// pair is a copy of the audio its crossfade is between two equal signals, the
// compiler folds it away and would drop the parameter with it.
process = si.bus(Nch)
       <: ((si.bus(Nch) <: expanderSC),
           (si.bus(Nch) <: scListenPath))
        : listenSelect
        : attach(_, scMix), _;
