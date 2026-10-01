declare name "Dynamics";
declare author "Klaus Scheuermann";
declare description "One-knob upward compression and downward expansion";
declare license "GPL-3.0-or-later";
declare unique_id "LAdy";

import("stdfaust.lib");

amount = hgroup("[1]Dynamics", hslider("[1]Dynamics[style:knob][unit:%][symbol:dynamics][easy][accentcolor:01][tooltip:Left expands quiet passages; right lifts them with pumping compression. Center is unity.]",
    0, -100, 100, 0.1)) / 100 : si.smooth(ba.tau2pole(0.010));

// Peak linking preserves the stereo image, including antiphase material.
// Log-domain ballistics: 0.2 ms / 500 ms to travel 90% of a level step.
// Clamp BEFORE the follower, so zero crossings do not drag it toward -infinity.
detector(l, r) = max(abs(l), abs(r)) : max(ba.db2linear(-48))
    : ba.linear2db : si.lag_ud(0.0002 / log(10), 0.500 / log(10));

// 12 dB soft knee centered at -12 dBFS. Gain is bounded to +/-36 dB.
// Positive control is upward compression, negative is downward expansion.
quietGain(level) = min(36, select2(level < -18,
    pow(max(0, -6 - level), 2) / 24, -12 - level));

process(l, r) = l * gain, r * gain
with {
    gain = detector(l, r) : quietGain : *(amount) : ba.db2linear;
};
