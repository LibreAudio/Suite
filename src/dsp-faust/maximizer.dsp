// -*-Faust-*-

// For some parts of this file, a large language model was involved as a coding assistant.
// The ideas, the design decisions and the listening behind it are purely human.

declare author "Klaus Scheuermann";
declare description "Barry's Satan Maximizer: envelope follower normaliser";
declare license "GPL-3.0-or-later";
declare name "Maximizer";
declare unique_id "LAmx";

// Port of Barry's Satan Maximiser (swh-plugins) as in x42's Ardour Lua script.

import("stdfaust.lib");

Nch = 2;

//======================= GUI =======================

max_group(x)   = vgroup("Maximizer", x);
knob_group(x)  = max_group(hgroup("[0]Controls", x));
meter_group(x) = max_group(hgroup("[1]Meters", x));

attackTime = knob_group(hslider("[0]Attack Time[unit:samples][symbol:attack_time][style:knob][accentcolor:03]
      [tooltip: How quickly the envelope follower rises to a louder sample.
       1 jumps straight to it, as the original does. Longer rides over the
       waveform instead of following it and sounds cleaner, but lets peaks
       through for a moment. The audio waits half the longer of Attack and
       Decay, reported to the host as latency]",
                                1, 1, 30, 1));

decayTime = knob_group(hslider("[1]Decay Time[unit:samples][symbol:decay_time][style:knob][accentcolor:03]
      [tooltip: How quickly the envelope follower lets go after a peak. Shorter
       follows the waveform closely and distorts, longer is smoother. The audio
       waits half the longer of Attack and Decay, reported to the host as
       latency]",
                               30, 2, 30, 1));

kneeDb = knob_group(hslider("[2]Knee Point[unit:dB][symbol:knee_point][style:knob][accentcolor:01]
      [tooltip: Level below which the gain stops rising. At 0 dB nothing is
       boosted and the plugin only holds peaks down to full scale. Lower values
       lift everything between the knee and full scale up to full scale]",
                          0, -90, 0, 0.1));

compOn = knob_group(nentry("[3]Gain Compensation[symbol:gain_compensation][accentcolor:04]
      [style:radio{'Off':0;'On':1}]
      [tooltip: Applies the Knee Point as make-up gain, taking back the lift the
       maximizer gives to everything below the knee, so lowering the knee
       does not make the output louder. Peaks end up at the knee level]",
                         1, 0, 1, 1));

//======================= maximizer =======================
//
// One envelope for all channels: a peak follower that rises towards any sample
// above it by a one-pole of 1/AttackTime and otherwise decays by a one-pole of
// 1/DecayTime. Every channel updates it in turn, so it follows the loudest of
// them. The audio, delayed by half the longer of the two times, is divided by
// the envelope, or by the knee where the envelope is below it.

MAXDELAY = 16;      // samples

trAttack = 1 / attackTime;
trDecay  = 1 / decayTime;
delaySamples = int(0.5 + max(attackTime, decayTime) * 0.5);
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
// Below the knee the maximizer lifts the signal by 1 / knee. Compensation
// applies the knee itself as make-up gain, so that lift is taken back off.
compensate = par(i, Nch, *(select2(compOn > 0.5, 1, knee)));

// Latency
latency_meter = attach(_, delaySamples :
    meter_group(hbargraph("[0]latency_samples[symbol:latency_samples][label:Latency]",
                          0, MAXDELAY)));

process = maximizer : compensate : (latency_meter, _);
