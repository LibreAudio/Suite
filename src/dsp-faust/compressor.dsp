// -*-Faust-*-

declare author "Klaus Scheuermann";
declare description "";
declare license "GPL-3.0-or-later";
declare name "Compressor";
declare unique_id "LAco";

// dry/wet is implemented by the host wrapper: this flag turns on the common
// "Dry / Wet" parameter and the latency-compensated crossfade in
// LibreAudioPlugin.cpp, so there is nothing to build here.
declare drywet "true";

import("stdfaust.lib");

// number of channels. The mid/side path below assumes exactly 2.
Nch = 2;

maxGR = -60; // meter floor, also the widest Range setting

maxLookaheadSamples = 19200; // 100 ms at 192 kHz

//======================= GUI =======================

comp_group(x)  = vgroup("Compressor", x);
meter_group(x) = comp_group(vgroup("[1]Meters", x));
knob_group(x)  = comp_group(hgroup("[0]Controls", x));

ctl_group(x)   = knob_group(hgroup("[0]Compression Control", x));
env_group(x)   = knob_group(hgroup("[1]Compression Response", x));
det_group(x)   = knob_group(hgroup("[2]Detector", x));
sc_group(x)    = knob_group(hgroup("[3]Sidechain Filter", x));

//---- gain computer ----

threshold = ctl_group(vslider("[0]Threshold[unit:dB][symbol:threshold]
      [tooltip: Level above which the signal is compressed]",
                              -18, maxGR, 0, 0.1));

ratio = ctl_group(vslider("[1]Ratio[scale:log][symbol:ratio]
      [tooltip: dB in per dB out above the threshold. 1 = no compression]",
                          4, 1, 100, 0.01));

// slope = the fraction of every dB of overshoot that gets removed
slope = 1 - 1 / max(1, ratio);

knee = ctl_group(vslider("[2]Knee[unit:dB][symbol:knee]
      [tooltip: Width of the soft transition around the threshold. The ratio
       reaches its full value knee/2 dB above the threshold and is 1:1
       knee/2 dB below it]",
                         6, 0, 24, 0.1));

range = ctl_group(vslider("[3]Range[unit:dB][symbol:range]
      [tooltip: Ceiling on total gain reduction. The compressor never pulls
       the signal down by more than this, no matter how far over threshold]",
                          60, 0, 0 - maxGR, 0.1));

rangeKnee = ctl_group(vslider("[4]Range Knee[unit:dB][symbol:range_knee]
      [tooltip: Softens the approach to the Range ceiling the same way Knee
       softens the approach to the threshold. 0 = hard clamp]",
                              6, 0, 24, 0.1));

//---- ballistics ----

attack = env_group(vslider("[0]Attack[unit:ms][scale:log][symbol:attack]
      [tooltip: How fast the gain moves toward a deeper reduction. Read as a
       1/e time constant at Attack Curve +1, or as the time to cover 20 dB at
       Attack Curve 0 and below]",
                           10, 0.01, 500, 0.01)) * 0.001;

attackCurve = env_group(vslider("[1]Attack Curve[symbol:attack_curve]
      [tooltip: Shape of the attack ramp. +1 = exponential, analog-style: quick
       off the mark then easing in, and Attack reads as a time constant.
       0 = straight line in dB. -1 = creeps in then snaps shut at the end]",
                                1, -1, 1, 0.01));

hold = env_group(vslider("[2]Hold[unit:ms][symbol:hold]
      [tooltip: How long the gain reduction is frozen at its deepest point
       before release is allowed to start]",
                         0, 0, 1000, 0.1)) * 0.001;

release = env_group(vslider("[3]Release[unit:ms][scale:log][symbol:release]
      [tooltip: How fast the gain moves back up toward a lighter reduction. Read
       as a 1/e time constant at Release Curve +1, or as the time to cover 20 dB
       at Release Curve 0 and below]",
                            150, 1, 3000, 0.1)) * 0.001;

releaseCurve = env_group(vslider("[4]Release Curve[symbol:release_curve]
      [tooltip: Shape of the release ramp. +1 = exponential, analog-style: lets
       go quickly then a long tail, and Release reads as a time constant.
       0 = straight line in dB. -1 = hangs, then lets go all at once]",
                                 1, -1, 1, 0.01));

autoRelease = env_group(vslider("[5]Auto Release[unit:%][symbol:auto_release]
      [tooltip: Program dependence. Stretches Release in proportion to how much
       reduction has been sustained recently, so isolated peaks let go quickly
       while a loud passage releases slowly and stops pumping. 0% = fixed]",
                                0, 0, 100, 1)) / 100;

lookaheadMs = env_group(vslider("[6]Lookahead[unit:ms][symbol:lookahead]
      [tooltip: Delays the audio so the gain is already down when the transient
       arrives. Reported to the host as latency and compensated. 0 = off]",
                                0, 0, 100, 0.1));

holdSamples      = hold * ma.SR;
lookaheadSamples = int(lookaheadMs * ma.SR / 1000) : latency_meter;

// How far back Auto Release looks, and how much sustained reduction it takes
// to double the release time.
autoTau   = 0.5;
autoRefDb = 6;

// The dB span the time knobs are measured over at curve = 0. It only sets
// where the linear and exponential readings of the same knob agree; at
// curve = +1 the knob is a time constant and this drops out entirely.
curveRef = 20;

// How far the curve is allowed to scale the nominal rate, in either
// direction. Both ends need the cap: at curve = -1 the rate goes as 1/distance
// and would run away to thousands of dB/s in the last fraction of a dB (a peak
// detector's ripple then wipes out the release entirely), and at curve = +1 the
// exponential tail would asymptote forever instead of settling. Clamping the
// distance is equivalent and cheaper than clamping the result.
curveBound = 8;

//---- detector ----

peakRms = det_group(vslider("[0]Peak / RMS[unit:%][symbol:peak_rms]
      [tooltip: What the detector measures. 0% = peak, catches every transient.
       100% = RMS over the window below, follows loudness and ignores spikes]",
                            0, 0, 100, 1)) / 100;

rmsTime = det_group(vslider("[1]RMS Time[unit:ms][symbol:rms_time]
      [tooltip: Averaging window of the RMS half of the detector. Has no effect
       at Peak / RMS 0%]",
                            10, 0, 250, 0.1)) * 0.001;

feedback = det_group(vslider("[2]Feedback[unit:%][symbol:feedback]
      [tooltip: Where the detector listens. 0% = feed-forward (detector sees
       the input, exact ratio), 100% = feed-back (detector sees the compressed
       output, softer and more program dependent)]",
                             0, 0, 100, 1)) / 100;

link = det_group(vslider("[3]Link[unit:%][symbol:link]
      [tooltip: 0% = every channel gets its own gain reduction, 100% = all
       channels follow the most-reduced one, so the stereo image never shifts]",
                         100, 0, 100, 1)) / 100;

msOn = det_group(checkbox("[4]Mid / Side[symbol:mid_side]
      [tooltip: Compress mid and side separately instead of left and right.
       Note the common Input and Output sections already carry a global
       mid/side switch that wraps every plugin the same way]"));

//---- sidechain filter ----
// Shapes what the detector hears, never the audio.

scHp = sc_group(vslider("[0]SC High Pass[unit:Hz][scale:log][symbol:sc_hp]
      [tooltip: Keeps bass out of the detector so kick and low end stop driving
       the whole gain envelope. 20 Hz = effectively off]",
                        20, 20, 500, 1));

scLp = sc_group(vslider("[1]SC Low Pass[unit:Hz][scale:log][symbol:sc_lp]
      [tooltip: Keeps air and hiss out of the detector. 20 kHz = effectively off]",
                        20000, 1000, 20000, 1));

scTilt = sc_group(vslider("[2]SC Tilt[unit:dB][symbol:sc_tilt]
      [tooltip: Broad weighting of the detector across the spectrum, pivoting at
       SC Freq. Positive makes it hear highs, so it reacts to sibilance and
       cymbals; negative makes it hear the low end]",
                          0, -12, 12, 0.1));

scFreq = sc_group(vslider("[3]SC Freq[unit:Hz][scale:log][symbol:sc_freq]
      [tooltip: Pivot frequency of SC Tilt]",
                          1000, 100, 8000, 1));

scRes = 0.7; // shelf Q for the tilt pair — flat, no bump at the pivot

// Shelf pair pivoting at scFreq, same construction as the Shelf mode of
// tiltEQ.dsp: equal and opposite low and high shelves.
scFilter = fi.highpass(2, scHp)
         : fi.lowpass(2, scLp)
         : fi.svf.ls(scFreq, scRes, 0 - scTilt)
         : fi.svf.hs(scFreq, scRes, scTilt);

//---- meters ----

meter1 = _ <: (_, (max(maxGR) : meter_group(hbargraph("[0]GR 1[unit:dB][symbol:gr_1]", maxGR, 0)))) : attach;
meter2 = _ <: (_, (max(maxGR) : meter_group(hbargraph("[1]GR 2[unit:dB][symbol:gr_2]", maxGR, 0)))) : attach;
chanMeter(0) = meter1;
chanMeter(1) = meter2;

// A passive widget with this exact symbol is what the build turns into the
// plugin's reported latency (see the latency_samples cases in
// src/templates/dsp.cpp.in), so the host delay-compensates Lookahead.
latency_meter = _ <: attach(_, meter_group(hbargraph("[2]latency_samples[symbol:latency_samples]", 0, maxLookaheadSamples)));

//======================= Level detector =======================
// Peak and RMS are computed as two dB readings and crossfaded, rather than
// switched. The RMS half is a one-pole average of the squared signal; as
// rmsTime goes to 0 its pole goes to 0 and it collapses to |x|, so the two
// halves meet continuously and no setting of the pair can produce a jump.

levelDb(x) = it.interpolate_linear(peakRms, peakDb, rmsDb)
with {
    peakDb = abs(x)
           : max(ma.EPSILON)
           : ba.linear2db;

    rmsDb  = x * x
           : si.smooth(ba.tau2pole(max(rmsTime, 1e-6)))
           : sqrt
           : max(ma.EPSILON)
           : ba.linear2db;
};

//======================= Gain computer =======================
// Returns gain reduction in dB, always <= 0.
//
// Below thresh-knee/2 nothing happens, above thresh+knee/2 the full ratio
// applies, and in between a quadratic bridges the two. The quadratic is the
// unique parabola that matches both value and slope at each end, which is
// what makes the knee audibly smooth rather than merely continuous.

gainComputer(level) = select3(zone, 0, softPart, hardPart)
with {
    over     = level - threshold;
    zone     = (over > 0 - knee / 2) + (over > knee / 2);
    softPart = 0 - slope * pow(over + knee / 2, 2) / (2 * max(ma.EPSILON, knee));
    hardPart = 0 - slope * over;
};

//======================= Range ceiling =======================
// Same parabola trick as the knee, mirrored: instead of easing *into*
// compression it eases into the floor, so a signal that slams far past the
// Range setting doesn't hit a corner in the gain curve.

rangeLimit(gr) = select3(zone, lim, soft, gr)
with {
    lim  = 0 - range;
    d    = gr - lim;

    // The soft floor peaks at lim + rk/2, so a knee wider than twice the
    // Range would lift the "gain reduction" above 0 dB and the compressor
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
//   m = -1  step inversely proportional to the distance -> hangs back, then
//           accelerates into the target
//
// The step deliberately depends on the current distance and nothing else.
// The obvious alternative - interpolating s + (target-s)*p^k along a segment
// that restarts whenever the target reverses - deadlocks here: with peak
// detection the target reverses at every zero crossing, and any shape with
// zero initial slope gets its progress reset faster than it can move. Built
// that way, this compressor measurably froze at 0.0001 dB of reduction.
// Distance carries no progress, so there is nothing for a reversal to reset.
//
// Hold freezes the gain while the target is trying to release, until the
// counter runs out. Any renewed attack resets the counter, so hold always
// measures from the most recent deepest point.

ballistics = loop ~ si.bus(2) : (_, !)
with {
    loop(y, hc, target) = ny, nhc
    with {
        diff = target - y;
        dist = abs(diff);
        atk  = diff < 0;        // target is lower: more gain reduction wanted

        nhc     = select2(atk, min(holdSamples, hc + 1), 0);
        holding = (diff > 0) * (nhc < holdSamples);

        // Program dependence, read off the compressor's own recent history:
        // a slow average of how much reduction has been in effect. A lone
        // transient barely moves it and releases at the knob setting, while a
        // sustained loud passage stretches the release out.
        hist   = abs(y) : si.smooth(ba.tau2pole(autoTau));
        relEff = release * (1 + autoRelease * hist / autoRefDb);

        tSec = select2(atk, relEff, attack);
        m    = select2(atk, releaseCurve, attackCurve);

        // Integrating the rate law over a full curveRef-dB span gives a
        // transit time of tSec/(1-m), so without this the same knob setting
        // would run twice as fast at curve = -1 as at curve = 0. Only the
        // m <= 0 half is corrected; above it the knob is deliberately drifting
        // toward its time-constant meaning, which diverges at m = 1.
        norm = 1 / (1 - min(0, m));
        rate = norm * curveRef / max(ma.EPSILON, tSec * ma.SR);
        step = rate * pow(min(curveBound, max(1 / curveBound, dist / curveRef)), m);

        // never step past the target, so short times land exactly on it
        // instead of chattering around it
        delta = ma.signum(diff) * min(step, dist);

        ny  = select2(holding, y + delta, y);
    };
};

//======================= Gain reduction, N channels =======================
// The whole detector chain sits inside one feedback loop so that the
// detector can be fed from the *output* gain. Feedback at 0% multiplies the
// input by 0 dB, i.e. leaves it alone, so feed-forward is not a special
// case - it is the same code path with the loop gain turned off.
//
// Note the ratio knob is only literally true feed-forward. In feed-back the
// detector already sees the reduced signal, so the effective ratio rises
// with drive; that program dependence is the point of the mode, not a bug.
//
// Linking happens on the gain computer's *target*, before the ballistics,
// so linked channels share one envelope instead of two envelopes racing.

grComputeN = loop ~ si.bus(Nch)
with {
    loop = ro.interleave(Nch, 2)
         : par(i, Nch, detTarget)
         : linkN(Nch, link)
         : par(i, Nch, ballistics);

    detTarget(grPrev, x) = x * ba.db2linear(grPrev * feedback)
                         : scFilter : levelDb : gainComputer : rangeLimit;
};

// crossfade each channel's own reduction against the deepest one of the set
linkN(N, l) = si.bus(N)
            <: (si.bus(N), (ba.parallelMin(N) <: si.bus(N)))
            :  ro.interleave(N, 2)
            :  par(i, N, it.interpolate_linear(l));

//======================= Mid/side =======================
// Same encode/decode as common/input.dsp and common/output.dsp, so the two
// agree on scaling and cancel exactly when both are engaged.

msEnc(l, r) = select2(msOn, l, (l + r) * 0.5),
              select2(msOn, r, (l - r) * 0.5);

msDec(m, s) = select2(msOn, m, m + s),
              select2(msOn, s, m - s);

//======================= process =======================
// The detector reads the signal undelayed while the audio goes through the
// lookahead delay, which is the whole point: by the time a transient reaches
// the multiplier its gain reduction is already in place.

process = si.bus(Nch)
        :  msEnc
        <: (grComputeN, par(i, Nch, de.delay(maxLookaheadSamples, lookaheadSamples)))
        :  ro.interleave(Nch, 2)
        :  par(i, Nch, applyGain(i))
        :  msDec;

applyGain(i, grDb, x) = x * (grDb : chanMeter(i) : ba.db2linear);
