// -*-Faust-*-
// Based on Airwindows Compresaturator by Chris Johnson (MIT).
// Accumulated waveshaper overspill controls gain reduction.

declare author "Klaus Scheuermann";
declare description "";
declare license "GPL-3.0-or-later";
declare name "Compresaturator";
declare unique_id "LAcp";

import("stdfaust.lib");

Nch = 2;

// Delay-buffer capacity.
maxSR = 192000;
maxMs = 10;
maxWidth = int(maxMs * maxSR / 1000);

// Reference rate for overspill scaling; the window uses the host rate.
refSR = 48000;

// Inputs above pi/2 shorten the compression window.
satLimit = 1.57079633;
panicWidth = 8;

minWidth = 2;

process = si.bus(Nch) <: (si.bus(Nch), wetChain) : dryWetMix
        : (latency_meter, _);

wetChain = transStage : msEnc : satSelect : msDec : par(i, Nch, *(makeupGain));

// Smooth the L/R-to-M/S transition.
msAmt = msOn : si.smoo;

msEnc(l, r) = l + (0.5 * (l + r) - l) * msAmt,
              r + (0.5 * (l - r) - r) * msAmt;

msDec(m, s) = m + ((m + s) - m) * msAmt,
              s + ((m - s) - s) * msAmt;

// Internal dry/wet mix with latency compensation.
dryWetMix = (par(i, Nch, de.delay(OSTAPS, osLatency)), si.bus(Nch))
          : ro.interleave(Nch, 2) : par(i, Nch, blend)
with {

    // Ensure 100% wet despite float rounding in si.smoo.
    dw = min(1, drywet * 1.0001);

    blend(d, w) = d * (1 - dw) + w * dw;
};

// Controls

comp_group(x)   = vgroup("Compresaturator", x);
top_group(x)    = comp_group(hgroup("[0]Compressor", x));
red_group(x)    = top_group(vgroup("[0]Reduction", x));
clamp_group(x)  = top_group(x);
sat_group(x)    = top_group(vgroup("[2]Saturation", x));
trans_group(x)  = comp_group(hgroup("[2]Transients", x));
low_group(x)    = trans_group(hgroup("[0]Low Shelf", x));
high_group(x)   = trans_group(hgroup("[1]High Shelf", x));
knob_group(x)   = comp_group(hgroup("[1]Controls", x));
hidden_group(x) = comp_group(hgroup("[3]Hidden",x));

channel_group(x)= knob_group(hgroup("[0]Channels", x));
output_group(x) = knob_group(hgroup("[2]Output", x));

driveDb = knob_group(vslider("[1]Drive[style:knob][unit:dB][symbol:drive]
      [tooltip: Input gain into the saturator. More Drive means more overspill, which means more compression -- the two are the same control here.]",
      0, -12, 12, 0.1));

drive = driveDb : ba.db2linear : si.smoo;

compSat = clamp_group(vslider("[1]Comp / Sat[style:knob][symbol:comp_sat]
      [tooltip: Left compresses: the overspill pushes the input gain back as hard as it can. Right only saturates. The level drop that compression causes is made up automatically, so the two ends sit at a similar loudness.]",
      0, -1, 1, 0.01));

// Macro endpoints: -1 = full compression, +1 = saturation only.
clamp = (1 - compSat) * 50 * 0.02;

// Static loudness correction for 1 ms Expand and -20 LUFS input.
macroCompDb = d * (0.1609 * c + 0.4725 * c * c) + 0.0038 * d * d
with {
    d = max(0, driveDb);
    c = 1 - (compSat + 1) * 0.5;
};

satComp = clamp * (1.0 + widestRangeRef / 3000.0) : si.smoo;

expandMs = 1;

// Host-rate window, with a 50-sample minimum.
widestRange = expandMs * ma.SR / 1000.0 : max(50) : min(maxWidth) : int;

// Keep overspill scaling independent of the host sample rate.
widestRangeRef = expandMs * refSR / 1000.0 : max(50);

// In M/S mode, Link couples mid and side.
link = channel_group(vslider("[2]Link[style:knob][unit:%][symbol:link]
      [tooltip: 0% lets each channel compress on its own, as the original does. 100% ducks both by the same amount, holding the stereo image still.]",
      100, 0, 100, 1)) / 100 : si.smoo;

msOn = channel_group(checkbox("[3]Mid / Side[symbol:mid_side]
      [tooltip: Saturate and compress mid and side instead of left and right. The two meters then read mid and side. Link ties whichever pair is being processed, so back it off in this mode.]"));

makeupDb = output_group(vslider("[7]Makeup[style:knob][unit:dB][symbol:makeup]
      [tooltip: Output trim, to put back the level the saturator and the compressor took away. Applied to the wet path only, before the dry/wet mix.]",
      0, -12, 12, 0.1));

// Wet-path trim, inverse Drive gain and macro compensation.
makeupGain = ba.db2linear(makeupDb - driveDb + macroCompDb) : si.smoo;

drywet = output_group(vslider("[8]Dry / Wet[style:knob][unit:%][symbol:drywet]
      [tooltip: Blend of the saturated signal against the untouched input, for parallel compression. 100% = saturator only, 0% = bypassed.]",
      100, 0, 100, 1)) / 100 : si.smoo;

osFactor = output_group(nentry("[9]Oversampling[symbol:oversampling]
      [style:radio{'Off':0;'2x':1;'4x':2;'8x':3}]
      [tooltip: Runs the saturator at a multiple of the sample rate, so the harmonics it generates do not fold back down as aliasing. Off is zero-latency; any factor adds 40 samples, which the host compensates.]",
      0, 0, 3, 1)) : int;

// The wrapper uses latency_samples and its maximum for latency reporting.
maxLatency = 64;
latency_meter = attach(_, osLatency :
    hidden_group(hbargraph("[10]latency_samples[symbol:latency_samples][label:Latency]",
                         0, maxLatency)));

maxRed = 6;

satFloor = ba.db2linear(0 - maxRed);

// Peak hold with exponential decay for block-rate UI reads.
meterHold = max ~ *(ba.tau2pole(0.3));

redMeter1 = red_group(hbargraph("[0]Reduction 1[unit:dB][symbol:reduction_1]", 0, maxRed));
redMeter2 = red_group(hbargraph("[1]Reduction 2[unit:dB][symbol:reduction_2]", 0, maxRed));

satMeter1 = sat_group(hbargraph("[0]Saturation 1[unit:dB][symbol:saturation_1]", 0, maxRed));
satMeter2 = sat_group(hbargraph("[1]Saturation 2[unit:dB][symbol:saturation_2]", 0, maxRed));

// Transient shelves

lowFreq = low_group(vslider("[1]Low Freq[style:knob][unit:Hz][scale:log][symbol:low_freq]
      [tooltip: Corner of the low shelf the low transient gain acts on.]",
      80, 40, 1000, 1)) : si.smoo;

lowAttack = low_group(vslider("[2]Low Attack[style:knob][unit:%][symbol:low_attack]
      [tooltip: Pushes (positive) or softens (negative) the leading edge of every hit in the low shelf.]",
      0, -100, 100, 1)) / 100 : si.smoo;

lowSustain = low_group(vslider("[3]Low Sustain[style:knob][unit:%][symbol:low_sustain]
      [tooltip: Lifts (positive) or shortens (negative) the tail of every hit in the low shelf.]",
      0, -100, 100, 1)) / 100 : si.smoo;

highFreq = high_group(vslider("[0]High Freq[style:knob][unit:Hz][scale:log][symbol:high_freq]
      [tooltip: Corner of the high shelf the high transient gain acts on.]",
      5000, 1000, 16000, 1)) : si.smoo;

highAttack = high_group(vslider("[1]High Attack[style:knob][unit:%][symbol:high_attack]
      [tooltip: Pushes (positive) or softens (negative) the leading edge of every hit in the high shelf.]",
      0, -100, 100, 1)) / 100 : si.smoo;

highSustain = high_group(vslider("[2]High Sustain[style:knob][unit:%][symbol:high_sustain]
      [tooltip: Lifts (positive) or shortens (negative) the tail of every hit in the high shelf.]",
      0, -100, 100, 1)) / 100 : si.smoo;

oneSample = 1.0 / float(ma.SR);

// Detector times in milliseconds, floored at one sample.
attackTime = 15  : *(0.001) : max(oneSample);

sustainTime = 200 : *(0.001) : max(oneSample);

maxTransRange = 24;

transRange = 12;

shelfQ = 0.707;

detFloorDb = -90;
detFloorLin = ba.db2linear(detFloorDb);
detRelease = 0.050;
gainSmoothTau = 0.0005;

lowGainMeter  = low_group(hbargraph("[0]Low Transient[unit:dB][symbol:low_transient]",
                                      0 - maxTransRange, maxTransRange));
highGainMeter = high_group(hbargraph("[3]High Transient[unit:dB][symbol:high_transient]",
                                      0 - maxTransRange, maxTransRange));

// Separate band detectors; both channels share the shelf gains.
transStage(l, r) = attach(shelves(l), lowDb : lowGainMeter)
                 : attach(_, highDb : highGainMeter),
                   shelves(r)
with {

    lowBand  = fi.svf.lp(lowFreq, shelfQ);
    highBand = fi.svf.hp(highFreq, shelfQ);

    // Sum rectified channels to avoid stereo cancellation.
    levelDb(band) = abs(l : band) + abs(r : band)
                  : si.onePoleSwitching(oneSample, detRelease)
                  : max(detFloorLin)
                  : ba.linear2db
                  : -(detFloorDb);

    lagAttack  = si.smooth(ba.tau2pole(attackTime));
    lagSustain = si.onePoleSwitching(oneSample, sustainTime);

    // Limit shelf gain, then smooth in dB.
    shelfDb(band, atk, sus) = transRange * ma.tanh((atk * dAttack + sus * dSustain) / transRange)
                            : si.smooth(ba.tau2pole(gainSmoothTau))
    with {
        lvl      = levelDb(band);
        dAttack  = max(0, lvl - (lvl : lagAttack));
        dSustain = max(0, (lvl : lagSustain) - lvl);
    };

    lowDb  = shelfDb(lowBand, lowAttack, lowSustain);
    highDb = shelfDb(highBand, highAttack, highSustain);

    shelves = fi.svf.ls(lowFreq, shelfQ, lowDb) : fi.svf.hs(highFreq, shelfQ, highDb);
};

// Saturator
// L parallel phases share one host-rate state per channel.

satStereo(L) = (step ~ si.bus(6)) : (si.block(6), si.bus(2 * L + 4))
with {
    step(pf0, lw0, sp0, pf1, lw1, sp1) =
        (chan(L, no.noises(2, 0), vsL(v0), pf0, lw0, sp0),
         chan(L, no.noises(2, 1), vsL(v1), pf1, lw1, sp1))

        // Feedback state first, then audio and meters.
        : route(N, N, (1, 1), (2, 2), (3, 3),
                      (K + 1, 4), (K + 2, 5), (K + 3, 6),
                      par(p, L, (4 + p, 7 + p)),
                      par(p, L, (K + 4 + p, 7 + L + p)),
                      (4 + L, 7 + 2 * L), (5 + L, 8 + 2 * L),
                      (K + 4 + L, 9 + 2 * L), (K + 5 + L, 10 + 2 * L))
    with {
        K = 5 + L;
        N = 2 * K;

        vs(pf, lw) = max(1.0, 1.0 + (pf / max(minWidth, lw)) * satComp);

        v0 = vs(pf0, lw0);
        v1 = vs(pf1, lw1);

        // Link toward the stronger reduction.
        vsL(v) = v + link * (max(v0, v1) - v);
    };
};

// Average phase overspill; use peak phase levels for window control.
chan(L, rnd, variSpeed, pf, lw, sPrev) =
    par(p, L, phase(p))

    // Group phase outputs by signal: overspill, level, reduction, audio.
    : route(4 * L, 4 * L, par(p, L, par(k, 4, (4 * p + k + 1, k * L + p + 1))))
    : (ba.parallelMean(L), ba.parallelMax(L), ba.parallelMax(L), si.bus(L))
    : (post, (_ <: redDb, _), si.bus(L))

    // Output order: state, audio, meters.
    : route(5 + L, 5 + L, (1, 1), (2, 2), (3, 3),
                          (4, 4 + L), (5, 5 + L),
                          par(p, L, (6 + p, 4 + p)))
with {

    // Feedback state starts at zero.
    lwc = max(minWidth, lw);

    redDb = ba.linear2db(variSpeed) : min(maxRed);

    // Interpolate reduction across phases.
    phase(p, x) = s1, rect, satGr, out
    with {
        w = float(p + 1) / L;
        gain = variSpeed * w + variSpeed' * (1.0 - w);

        totalgain = drive / gain;
        driven = x * totalgain;

        // Overspill reference follows gain cuts, but not boosts.
        temp = select2(totalgain < 1.0, x, driven);

        rect = abs(driven);
        shaped = sin(min(rect, satLimit));

        out = select2(driven < 0.0, shaped, -shaped);

        // Floor both terms to avoid 0/0 at silence.
        satGr = 0 - ba.linear2db(max(satFloor, max(ma.EPSILON, shaped)
                                              / max(ma.EPSILON, rect)));

        s1 = (abs(temp) - shaped) * satComp;
    };

    post(s, rect) = padFactor, lastWidth, s
    with {

        targetWidth = select2(rect > satLimit, widestRange, panicWidth);

        // Release residual reduction in near-silence.
        pfDecayed = select2(rect < 0.01, pf, pf * 0.9999);

        added = pfDecayed + s;
        tapOld = tap(lwc);
        tapNew = tap(shrunk);
        shrunk = max(minWidth, lwc - 1);

        // Randomize window growth to avoid periodic modulation.
        expanding = (targetWidth * uniform) > lwc;
        shrinking = targetWidth < lwc;

        lastWidth = select2(expanding, select2(shrinking, lwc, shrunk), lwc + 1)
                  : min(maxWidth) : int;
        // Keep the tail when growing; remove two samples when shrinking.
        padFactor = select2(expanding, select2(shrinking, added - tapOld,
                                               added - tapOld - tapNew), added)
                  : max(0.0);
    };

    uniform = (rnd + 1.0) * 0.5;
    // Feedback supplies the remaining one-sample delay.
    tap(w) = de.delay(maxWidth, int(w) - 1, sPrev);
};

// Oversampling
// Parallel polyphase streams run at the host rate.

OSTAPS  = 40;   // Taps per phase and round-trip latency in host samples.
OSDELAY = 20;   // Phase-0 delay.

up(L)   = _ <: (@(OSDELAY), par(p, L - 1, fi.fir(hp(L, p + 1))));
down(L) = (@(OSDELAY), par(j, L - 1, fi.fir(hp(L, L - 1 - j)) : mem)) :> /(L);

osSat(L) = (up(L), up(L)) : satStereo(L) : (down(L), down(L), si.bus(4));

// Run all factors to preserve state when switching.
satSelect = _, _ <: (satStereo(1), osSat(2), osSat(4), osSat(8))
          : route(24, 24, par(f, 4, par(k, 6, (6 * f + k + 1, 4 * k + f + 1))))
          : par(k, 6, ba.selectn(4, osFactor))
          : meters
with {
    meters(l, r, red0, sat0, red1, sat1) =
        attach(attach(l, red0 : meterHold : redMeter1), sat0 : meterHold : satMeter1),
        attach(attach(r, red1 : meterHold : redMeter2), sat1 : meterHold : satMeter2);
};

osLatency = select2(osFactor > 0, 0, OSTAPS);

// Kaiser FIR, beta 8.68, cutoff at host Nyquist; unity DC gain per phase.
// Phase 0 is a pure delay, handled by OSDELAY.
hp(2, 1) = (
      -4.37797177881e-05,  1.32549617198e-04, -3.02781836516e-04,  5.96906681537e-04,
      -1.06950572975e-03,  1.78856600971e-03, -2.83678365476e-03,  4.31330283769e-03,
      -6.33662513756e-03,  9.05001914332e-03, -1.26318474443e-02,  1.73153852784e-02,
      -2.34273674458e-02,  3.14655455896e-02, -4.22646132617e-02,  5.73870875127e-02,
      -8.01873632972e-02,  1.19430867794e-01, -2.07386721498e-01,  6.35007158559e-01,
       6.35007158559e-01, -2.07386721498e-01,  1.19430867794e-01, -8.01873632972e-02,
       5.73870875127e-02, -4.22646132617e-02,  3.14655455896e-02, -2.34273674458e-02,
       1.73153852784e-02, -1.26318474443e-02,  9.05001914332e-03, -6.33662513756e-03,
       4.31330283769e-03, -2.83678365476e-03,  1.78856600971e-03, -1.06950572975e-03,
       5.96906681537e-04, -3.02781836516e-04,  1.32549617198e-04, -4.37797177881e-05);

hp(4, 1) = (
      -2.14894508149e-05,  7.36303681592e-05, -1.77104100382e-04,  3.59912708193e-04,
      -6.58417687801e-04,  1.11824137361e-03, -1.79516162624e-03,  2.75622755283e-03,
      -4.08154220788e-03,  5.86752411159e-03, -8.23311572781e-03,  1.13316821683e-02,
      -1.53740399632e-02,  2.06742936260e-02, -2.77461215241e-02,  3.75233567197e-02,
      -5.19358470099e-02,  7.57431173569e-02, -1.24653814479e-01,  2.98390647561e-01,
       8.99752645352e-01, -1.77214487865e-01,  9.49850571143e-02, -6.21375712475e-02,
       4.39582300018e-02, -3.22105465844e-02,  2.39442875389e-02, -1.78411885619e-02,
       1.32184544300e-02, -9.67933121573e-03,  6.96928675184e-03, -4.91014613971e-03,
       3.36776353522e-03, -2.23550422633e-03,  1.42565417993e-03, -8.64927793521e-04,
       4.92078946753e-04, -2.56525808975e-04,  1.17385276571e-04, -4.25934528967e-05);
hp(4, 2) = (
      -4.37797177881e-05,  1.32549617198e-04, -3.02781836516e-04,  5.96906681537e-04,
      -1.06950572975e-03,  1.78856600971e-03, -2.83678365476e-03,  4.31330283769e-03,
      -6.33662513756e-03,  9.05001914332e-03, -1.26318474443e-02,  1.73153852784e-02,
      -2.34273674458e-02,  3.14655455896e-02, -4.22646132617e-02,  5.73870875127e-02,
      -8.01873632972e-02,  1.19430867794e-01, -2.07386721498e-01,  6.35007158559e-01,
       6.35007158559e-01, -2.07386721498e-01,  1.19430867794e-01, -8.01873632972e-02,
       5.73870875127e-02, -4.22646132617e-02,  3.14655455896e-02, -2.34273674458e-02,
       1.73153852784e-02, -1.26318474443e-02,  9.05001914332e-03, -6.33662513756e-03,
       4.31330283769e-03, -2.83678365476e-03,  1.78856600971e-03, -1.06950572975e-03,
       5.96906681537e-04, -3.02781836516e-04,  1.32549617198e-04, -4.37797177881e-05);
hp(4, 3) = (
      -4.25934528967e-05,  1.17385276571e-04, -2.56525808975e-04,  4.92078946753e-04,
      -8.64927793521e-04,  1.42565417993e-03, -2.23550422633e-03,  3.36776353522e-03,
      -4.91014613971e-03,  6.96928675184e-03, -9.67933121573e-03,  1.32184544300e-02,
      -1.78411885619e-02,  2.39442875389e-02, -3.22105465844e-02,  4.39582300018e-02,
      -6.21375712475e-02,  9.49850571143e-02, -1.77214487865e-01,  8.99752645352e-01,
       2.98390647561e-01, -1.24653814479e-01,  7.57431173569e-02, -5.19358470099e-02,
       3.75233567197e-02, -2.77461215241e-02,  2.06742936260e-02, -1.53740399632e-02,
       1.13316821683e-02, -8.23311572781e-03,  5.86752411159e-03, -4.08154220788e-03,
       2.75622755283e-03, -1.79516162624e-03,  1.11824137361e-03, -6.58417687801e-04,
       3.59912708193e-04, -1.77104100382e-04,  7.36303681592e-05, -2.14894508149e-05);

hp(8, 1) = (
      -9.46018969446e-06,  3.50667645661e-05, -8.68518308623e-05,  1.79441191382e-04,
      -3.31920416670e-04,  5.68327793787e-04, -9.18134081817e-04,  1.41682117445e-03,
      -2.10678287128e-03,  3.03895597505e-03, -4.27591871362e-03,  5.89782570319e-03,
      -8.01387129876e-03,  1.07849973115e-02, -1.44711959951e-02,  1.95384830781e-02,
      -2.69328147536e-02,  3.89264988873e-02, -6.26727946549e-02,  1.38130670659e-01,
       9.74346591165e-01, -1.06887558318e-01,  5.47354558721e-02, -3.52626087754e-02,
       2.47796605618e-02, -1.81031168114e-02,  1.34435206354e-02, -1.00189613626e-02,
       7.43093536138e-03, -5.45094264796e-03,  3.93410179344e-03, -2.78002422638e-03,
       1.91374287709e-03, -1.27600323913e-03,  8.18220418792e-04, -4.99843619673e-04,
       2.86960393526e-04, -1.51505715063e-04,  7.07236996204e-05, -2.66917952592e-05);
hp(8, 2) = (
      -2.14894508149e-05,  7.36303681592e-05, -1.77104100382e-04,  3.59912708193e-04,
      -6.58417687801e-04,  1.11824137361e-03, -1.79516162624e-03,  2.75622755283e-03,
      -4.08154220788e-03,  5.86752411159e-03, -8.23311572781e-03,  1.13316821683e-02,
      -1.53740399632e-02,  2.06742936260e-02, -2.77461215241e-02,  3.75233567197e-02,
      -5.19358470099e-02,  7.57431173569e-02, -1.24653814479e-01,  2.98390647561e-01,
       8.99752645352e-01, -1.77214487865e-01,  9.49850571143e-02, -6.21375712475e-02,
       4.39582300018e-02, -3.22105465844e-02,  2.39442875389e-02, -1.78411885619e-02,
       1.32184544300e-02, -9.67933121573e-03,  6.96928675184e-03, -4.91014613971e-03,
       3.36776353522e-03, -2.23550422633e-03,  1.42565417993e-03, -8.64927793521e-04,
       4.92078946753e-04, -2.56525808975e-04,  1.17385276571e-04, -4.25934528967e-05);
hp(8, 3) = (
      -3.39317592273e-05,  1.08782275504e-04, -2.54721756160e-04,  5.09633651278e-04,
      -9.22476661180e-04,  1.55445192627e-03, -2.48018724302e-03,  3.78926443624e-03,
      -5.58871924642e-03,  8.00765529090e-03, -1.12060412008e-02,  1.53915809851e-02,
      -2.08524405949e-02,  2.80227261651e-02, -3.76212565021e-02,  5.09738332570e-02,
      -7.08718931553e-02,  1.04395343349e-01, -1.76156100794e-01,  4.68662408851e-01,
       7.83099856745e-01, -2.09788162540e-01,  1.16876574472e-01, -7.75135066887e-02,
       5.51692746017e-02, -4.05340673391e-02,  3.01570454258e-02, -2.24630877120e-02,
       1.66234805642e-02, -1.21503471941e-02,  8.72705867906e-03, -6.12974822752e-03,
       4.18853766467e-03, -2.76766100721e-03,  1.75511644625e-03, -1.05725093279e-03,
       5.95865602680e-04, -3.06519678793e-04,  1.37300431835e-04, -4.76705866007e-05);
hp(8, 4) = (
      -4.37797177881e-05,  1.32549617198e-04, -3.02781836516e-04,  5.96906681537e-04,
      -1.06950572975e-03,  1.78856600971e-03, -2.83678365476e-03,  4.31330283769e-03,
      -6.33662513756e-03,  9.05001914332e-03, -1.26318474443e-02,  1.73153852784e-02,
      -2.34273674458e-02,  3.14655455896e-02, -4.22646132617e-02,  5.73870875127e-02,
      -8.01873632972e-02,  1.19430867794e-01, -2.07386721498e-01,  6.35007158559e-01,
       6.35007158559e-01, -2.07386721498e-01,  1.19430867794e-01, -8.01873632972e-02,
       5.73870875127e-02, -4.22646132617e-02,  3.14655455896e-02, -2.34273674458e-02,
       1.73153852784e-02, -1.26318474443e-02,  9.05001914332e-03, -6.33662513756e-03,
       4.31330283769e-03, -2.83678365476e-03,  1.78856600971e-03, -1.06950572975e-03,
       5.96906681537e-04, -3.02781836516e-04,  1.32549617198e-04, -4.37797177881e-05);
hp(8, 5) = (
      -4.76705866007e-05,  1.37300431835e-04, -3.06519678793e-04,  5.95865602680e-04,
      -1.05725093279e-03,  1.75511644625e-03, -2.76766100721e-03,  4.18853766467e-03,
      -6.12974822752e-03,  8.72705867906e-03, -1.21503471941e-02,  1.66234805642e-02,
      -2.24630877120e-02,  3.01570454258e-02, -4.05340673391e-02,  5.51692746017e-02,
      -7.75135066887e-02,  1.16876574472e-01, -2.09788162540e-01,  7.83099856745e-01,
       4.68662408851e-01, -1.76156100794e-01,  1.04395343349e-01, -7.08718931553e-02,
       5.09738332570e-02, -3.76212565021e-02,  2.80227261651e-02, -2.08524405949e-02,
       1.53915809851e-02, -1.12060412008e-02,  8.00765529090e-03, -5.58871924642e-03,
       3.78926443624e-03, -2.48018724302e-03,  1.55445192627e-03, -9.22476661180e-04,
       5.09633651278e-04, -2.54721756160e-04,  1.08782275504e-04, -3.39317592273e-05);
hp(8, 6) = (
      -4.25934528967e-05,  1.17385276571e-04, -2.56525808975e-04,  4.92078946753e-04,
      -8.64927793521e-04,  1.42565417993e-03, -2.23550422633e-03,  3.36776353522e-03,
      -4.91014613971e-03,  6.96928675184e-03, -9.67933121573e-03,  1.32184544300e-02,
      -1.78411885619e-02,  2.39442875389e-02, -3.22105465844e-02,  4.39582300018e-02,
      -6.21375712475e-02,  9.49850571143e-02, -1.77214487865e-01,  8.99752645352e-01,
       2.98390647561e-01, -1.24653814479e-01,  7.57431173569e-02, -5.19358470099e-02,
       3.75233567197e-02, -2.77461215241e-02,  2.06742936260e-02, -1.53740399632e-02,
       1.13316821683e-02, -8.23311572781e-03,  5.86752411159e-03, -4.08154220788e-03,
       2.75622755283e-03, -1.79516162624e-03,  1.11824137361e-03, -6.58417687801e-04,
       3.59912708193e-04, -1.77104100382e-04,  7.36303681592e-05, -2.14894508149e-05);
hp(8, 7) = (
      -2.66917952592e-05,  7.07236996204e-05, -1.51505715063e-04,  2.86960393526e-04,
      -4.99843619673e-04,  8.18220418792e-04, -1.27600323913e-03,  1.91374287709e-03,
      -2.78002422638e-03,  3.93410179344e-03, -5.45094264796e-03,  7.43093536138e-03,
      -1.00189613626e-02,  1.34435206354e-02, -1.81031168114e-02,  2.47796605618e-02,
      -3.52626087754e-02,  5.47354558721e-02, -1.06887558318e-01,  9.74346591165e-01,
       1.38130670659e-01, -6.26727946549e-02,  3.89264988873e-02, -2.69328147536e-02,
       1.95384830781e-02, -1.44711959951e-02,  1.07849973115e-02, -8.01387129876e-03,
       5.89782570319e-03, -4.27591871362e-03,  3.03895597505e-03, -2.10678287128e-03,
       1.41682117445e-03, -9.18134081817e-04,  5.68327793787e-04, -3.31920416670e-04,
       1.79441191382e-04, -8.68518308623e-05,  3.50667645661e-05, -9.46018969446e-06);
