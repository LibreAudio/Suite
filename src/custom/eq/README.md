# Native dynamic EQ (bell stage)

`eq-dsp.cpp` replaces the generated EQ processor. EQ configuration and parameter
metadata are native files; `src/dsp-faust/eq.dsp` is no longer compiled or used to
generate EQ metadata. The shared `FaustDSP` interface is retained for compatibility
with the suite's plugin shell. Common input/output processing still uses Faust.

There are 16 stable, reusable host parameter slots. Present, enabled, type,
channel, frequency, gain, Q, adaptive Q and slope are stored per slot. Only bell
(type 2) currently processes audio; the UI creates bells and dims the unavailable
filter types. Slots are preallocated; the audio callback never allocates. Disabled,
removed and zero-gain bells fade to unity using the upstream gain interpolation,
then stop processing. Mid and side have independent states; stereo runs both.
With no active filters, the main EQ processor returns without touching audio.

The host parameter layout replaces the previous fixed-filter layout. Old EQ
sessions/presets are not migrated. New sessions retain bands via host parameters,
including when the custom UI closes. The plugin identity is unchanged.

Piano, frequency regions (the “ranges” toggle), controls, fixed/auto dB range,
and analyser mode are saved separately in the `editor_settings` UI state. These
preferences belong to each plugin instance and are restored when its editor
reopens or the host restores its saved state. Band reset does not reset them.

## Upstream attribution

`x42-bell.hpp` is an unmodified copy of `src/filters.h` from
https://github.com/x42/fil4.lv2 at commit
`885edf4d4c8da3f1414462ec7b5fc1c2da0f3ac1`.
Copyright 2004–2009 Fons Adriaensen; GPL-2.0-or-later. Its original copyright and
license notice are preserved. The enclosing suite is GPL-3.0-or-later (see the
repository LICENSE).

The Regalia–Mitra lattice implementation, coefficient slew limiting and
interpolation are upstream code. The adapter uses exact dB-to-linear conversion,
x42's normalized frequency limits, and `bandwidth = 2*pi/(7*Qeffective)` to map
the UI's analog half-gain Q to x42's bandwidth coefficient. Adaptive Q uses the
UI's existing gain-dependent multiplier. The plotted bell uses the same digital
transfer function and actual sample rate, including near Nyquist.

## Validation

From the repository root:

```sh
c++ -std=c++17 -O2 -I src -I deps/DPF/distrho tests/eq-dsp.cpp src/custom/eq/eq-dsp.cpp -o /tmp/suite3-eq-test
/tmp/suite3-eq-test
cmake --build build --target la-eq-clap la-eq-vst3 la-eq-lv2 la-eq-lv2-ui -j 4
```

The standalone test checks upstream impulse parity, displayed versus measured
response at multiple sample rates, mid/side isolation, retirement and reuse,
zero-length blocks, and exact inactive passthrough. It also prints processing
times for 0, 1, 4 and 16 bells; these are informational, not timing assertions.
