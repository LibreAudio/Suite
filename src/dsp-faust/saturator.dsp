// -*-Faust-*-

// For some parts of this file, a large language model was involved as a coding assistant.
// The ideas, the design decisions and the listening behind it are purely human.

declare author "Klaus Scheuermann";
declare description "Satan Saturator";
declare license "GPL-3.0-or-later";
declare name "Saturator";
declare unique_id "LAsa";

// Port of Barry's Satan Maximiser (swh-plugins) as in x42's Ardour Lua script.

import("stdfaust.lib");

Nch = 2;

//======================= GUI =======================

max_group(x)   = vgroup("Maximizer", x);
knob_group(x)  = max_group(hgroup("[0]Controls", x));
out_group(x)   = max_group(hgroup("[1]Output", x));
meter_group(x) = max_group(hgroup("[2]Meters", x));

attackTime = knob_group(hslider("[0]Attack Time[unit:ms][scale:log][symbol:attack_time][style:knob][accentcolor:03]
      [tooltip: How quickly the envelope follower rises to a louder sample.
       The shortest jumps straight to it, as the original does. Longer rides over the
       waveform instead of following it and sounds cleaner, but lets peaks
       through for a moment. The audio waits half the longer of Attack and
       Decay, reported to the host as latency]",
                                0.02, 0.02, 2, 0.01));

decayTime = knob_group(hslider("[1]Decay Time[unit:ms][scale:log][symbol:decay_time][style:knob][accentcolor:03]
      [tooltip: How quickly the envelope follower lets go after a peak. Shorter
       follows the waveform closely and distorts, longer is smoother. The audio
       waits half the longer of Attack and Decay, reported to the host as
       latency]",
                               0.7, 0.05, 2, 0.01));

kneeDb = knob_group(hslider("[2]Knee Point[unit:dB][symbol:knee_point][style:knob][accentcolor:01]
      [tooltip: Level below which the gain stops rising. At 0 dB nothing is
       boosted and the plugin only holds peaks down to full scale. Lower values
       lift everything between the knee and full scale up, and half of that
       lift is taken back off at the output so the level stays in check]",
                          0, -90, 0, 0.1));

makeupDb = out_group(hslider("[0]Makeup[unit:dB][symbol:makeup][style:knob][accentcolor:05]
      [tooltip: Gain on the maximized signal, after the compensation for the
       Knee Point and before the Dry/Wet blend, so the dry side is untouched. Adds to
       whatever Auto Makeup has learned]",
                             0, -24, 24, 0.1));

autoBtn = out_group(button("[1]Auto Makeup[symbol:auto_makeup][accentcolor:05]
      [tooltip: Press while typical material plays and release to set the gain
       that brings the maximized signal back to the loudness of the input,
       short-term LUFS (3 s window) measured after the knee compensation. The
       loudness is measured all the time, so there is no need to hold it down;
       the value is taken at release, so release it after a few seconds of
       material. The learned value is shown in the meters
       and adds to Makeup. Pressing it on silence clears it]"));

drywet = out_group(hslider("[2]Dry/Wet[unit:%][symbol:drywet][style:knob][accentcolor:05]
      [tooltip: Blend of the maximized signal against the untouched input, for
       parallel maximizing. 100% = maximized only, 0% = untouched. The dry side
       is delayed to match the maximizer, so the blend never combs]",
                          100, 0, 100, 1)) / 100;

//======================= maximizer =======================
//
// One envelope for all channels: a peak follower that rises towards any sample
// above it by a one-pole of Attack and otherwise decays by a one-pole of
// Decay, both set in ms and no shorter than one sample. Every channel updates
// it in turn, so it follows the loudest of them. The audio, delayed by half
// the longer of the two times, is divided by the envelope, or by the knee
// where the envelope is below it.

MAXDELAY = 256;     // samples; half of 2 ms at 192 kHz is 192

attackSamples = max(1, attackTime * ma.SR / 1000);
decaySamples  = max(1, decayTime * ma.SR / 1000);
trAttack = 1 / attackSamples;
trDecay  = 1 / decaySamples;
delaySamples = int(0.5 + max(attackSamples, decaySamples) * 0.5) : min(MAXDELAY - 1);
knee = ba.db2linear(kneeDb);

envUpdate(e, f) = select2(f > e, e + (f - e) * trDecay, e + (f - e) * trAttack);

envelope(l, r) = loop ~ _
with { loop(e) = envUpdate(envUpdate(e, abs(l)), abs(r)); };

maximizer(l, r) = (l : delayA) * g, (r : delayA) * g
with {
    delayA = de.delay(MAXDELAY, delaySamples);
    g      = 1 / max(envelope(l, r), knee);
};

// Gain compensation
//
// Below the knee the maximizer lifts the signal by 1 / knee. Taking all of it
// back off leaves the output too quiet and none of it too loud, so half the
// knee in dB is applied as make-up gain, which is its square root.
compensate = par(i, Nch, *(sqrt(knee)));

// Makeup and dry/wet
//
// Auto Makeup matches loudness: it compares the short-term loudness
// (BS.1770-4 K-weighted power over a 3 s window, the LUFS-S window) of the
// dry input, delayed to meet the wet, with that of the maximized signal after
// the knee compensation. Both are measured all the time. On release of the button
// the difference is latched and stays until the next press. If either side is
// silent it latches 0 dB. It is measured before Makeup, so Makeup is a trim on
// top of it. The dry side waits the same delay as the audio inside the
// maximizer.
//
// AUTOAPPLY switches whether the learned gain reaches the audio. At 0 the
// meter and the latch still work and the audio gets Makeup alone, so the
// learned value can be read out and set on Makeup from outside.
AUTOAPPLY = 0;      // 1 = learned gain changes the volume, 0 = meter only
AUTOMAX = 60;       // dB, most the learned gain can be
AUTOSIL = 1e-9;     // power below this is taken as silence
LUFSWIN = 3.0;      // s, the BS.1770 short-term window

autoFall = autoBtn' > autoBtn;

lufsPower(l, r) = sq(l : kf) + sq(r : kf) : si.smooth(ba.tau2pole(LUFSWIN * 0.5))
with {
    sq(x) = x * x;
    kf    = fi.itu_r_bs_1770_4_kfilter;
};

autoMakeupDb(dl, dr, wl, wr) = loop ~ _
with {
    pd      = lufsPower(dl, dr);
    pw      = lufsPower(wl, wr);
    offset  = select2((pd > AUTOSIL) & (pw > AUTOSIL), 0,
                      10 * log10(pd / max(pw, AUTOSIL)))
            : max(0 - AUTOMAX) : min(AUTOMAX);
    loop(a) = select2(autoFall, a, offset);
};

autoMeter = _ <: attach(_, meter_group(hbargraph("[1]auto_makeup_db[unit:dB][symbol:auto_makeup_db][label:Auto Makeup]",
                                         0 - AUTOMAX, AUTOMAX)));

// dry and wet in, dry passed through, wet with the makeup on it
makeupStage(dl, dr, wl, wr) = dl, dr, wl * g, wr * g
with {
    autoDb = autoMakeupDb(dl, dr, wl, wr) : autoMeter;
    // attach keeps the measurement, button and meter in the program even when
    // AUTOAPPLY is 0 and the multiplication would otherwise fold them away.
    g      = attach(makeupDb + autoDb * AUTOAPPLY, autoDb) : si.smoo : ba.db2linear;
};

dryTap = par(i, Nch, de.delay(MAXDELAY, delaySamples));

dryWetMix = ro.interleave(Nch, 2) : par(i, Nch, blend)
with {
    dryAmt = (1 - drywet) : si.smoo;
    blend(d, w) = d * dryAmt + w * (1 - dryAmt);
};

// Latency
latency_meter = attach(_, delaySamples :
    meter_group(hbargraph("[0]latency_samples[symbol:latency_samples][label:Latency]",
                          0, MAXDELAY)));

process = si.bus(Nch) <: (dryTap, (maximizer : compensate))
        : makeupStage : dryWetMix : (latency_meter, _);
