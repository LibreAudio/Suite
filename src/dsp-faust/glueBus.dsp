// -*-Faust-*-

// Based on Airwindows Compresaturator by Chris Johnson (MIT).
// Accumulated waveshaper overspill controls gain reduction.

// For some parts of this file, a large language model was involved as a coding assistant.
// The ideas, the design decisions and the listening behind it are purely human.

declare author "Klaus Scheuermann";
declare description "";
declare license "GPL-3.0-or-later";
declare name "Glue Bus";
declare unique_id "LAgb";

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

process = si.bus(Nch) <: (si.bus(Nch), wetChain) : dryWetMix;

wetChain = transStage : msEnc : saturator : msDec : par(i, Nch, *(makeupGain));

// Smooth the L/R-to-M/S transition.
msAmt = msOn : si.smoo;

msEnc(l, r) = l + (0.5 * (l + r) - l) * msAmt,
              r + (0.5 * (l - r) - r) * msAmt;

msDec(m, s) = m + ((m + s) - m) * msAmt,
              s + ((m - s) - s) * msAmt;

// Internal dry/wet mix.
dryWetMix = ro.interleave(Nch, 2) : par(i, Nch, blend)
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

// Saturator: one host-rate state per channel.

satStereo = (step ~ si.bus(6)) : (si.block(6), si.bus(6))
with {
    step(pf0, lw0, sp0, pf1, lw1, sp1) =
        (chan(no.noises(2, 0), vsL(v0), pf0, lw0, sp0),
         chan(no.noises(2, 1), vsL(v1), pf1, lw1, sp1))

        // Feedback state first, then audio and meters.
        : route(12, 12, (1, 1), (2, 2), (3, 3),
                        (7, 4), (8, 5), (9, 6),
                        (4, 7), (10, 8),
                        (5, 9), (6, 10), (11, 11), (12, 12))
    with {
        vs(pf, lw) = max(1.0, 1.0 + (pf / max(minWidth, lw)) * satComp);

        v0 = vs(pf0, lw0);
        v1 = vs(pf1, lw1);

        // Link toward the stronger reduction.
        vsL(v) = v + link * (max(v0, v1) - v);
    };
};

chan(rnd, variSpeed, pf, lw, sPrev) =
    shape : (post, (_ <: redDb, _), _)

    // Output order: state, audio, meters.
    : route(6, 6, (1, 1), (2, 2), (3, 3),
                  (4, 5), (5, 6), (6, 4))
with {

    // Feedback state starts at zero.
    lwc = max(minWidth, lw);

    redDb = ba.linear2db(variSpeed) : min(maxRed);

    shape(x) = s1, rect, satGr, out
    with {
        totalgain = drive / variSpeed;
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

// Attach channel reduction and saturation meters.
saturator = satStereo : meters
with {
    meters(l, r, red0, sat0, red1, sat1) =
        attach(attach(l, red0 : meterHold : redMeter1), sat0 : meterHold : satMeter1),
        attach(attach(r, red1 : meterHold : redMeter2), sat1 : meterHold : satMeter2);
};
