# Native dynamic EQ (bell, Butterworth and ladder high-pass)

`eq-dsp.cpp` replaces the generated EQ processor. EQ configuration and parameter
metadata are native files; `src/dsp-faust/eq.dsp` is no longer compiled or used to
generate EQ metadata. The shared `FaustDSP` interface is retained for compatibility
with the suite's plugin shell. Common input/output processing still uses Faust.

There are 16 stable, reusable host parameter slots. Present, enabled, type,
channel, frequency, gain, Q, adaptive Q and slope are stored per slot. Bell
(type 2), Butterworth high-pass (type 0) and ladder high-pass (type 5) process audio. The UI creates a
high-pass in the leftmost 10% of the display, otherwise a bell. All three types are
selectable from the type menu; shelves and low-pass remain unavailable. Slots are preallocated; the audio callback never allocates. Disabled,
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

## Butterworth high-pass

`highpass.hpp` implements prewarped trapezoidal one-pole and state-variable
sections, with 6/12/18/24dB/oct slopes (orders 1–4). Neutral UI Q = 0.707 gives
a complete Butterworth cascade at every slope: -3.0103dB at cutoff. The 24dB
cascade uses section Q values 0.5411961001 and 1.306562965; the 18dB cascade
uses a one-pole plus Q=1 section.

6dB has no resonance: Q is ignored by the DSP and display, and its UI control is
disabled. For higher slopes, the Q control scales the highest-Q section relative
to the neutral value. These resonant responses intentionally depart from the
Butterworth alignment. The graph evaluates the same digital transfer function.
The SVF equations use the standard trapezoidal-integrator formulation described
in https://www.cytomic.com/files/dsp/SvfLinearTrapOptimised.pdf; this is a native
implementation, not a port of x42's high-pass.

Cutoff and damping are smoothed over 10ms. Enable/routing/type changes use a
5ms wet/dry ramp. Slope changes fade the old topology to dry, reset its history,
and fade the new topology in. Inactive high-pass sections stop processing once
the fade completes. All state is preallocated; mid and side have independent
filter state. Frequency is limited to just below Nyquist in both DSP and graph.

## Ladder high-pass

`ladder-highpass.hpp` adds a Moog-inspired alternative as appended type 5, keeping
all existing type IDs and parameter indices intact. The UI places its resonant
high-pass/rung icon beside Butterworth HP in both the control row and context
menu. Selecting it defaults to 24dB/oct; 6/12/18/24dB/oct remain available.

It is a cascade of one-pole trapezoidal high-pass stages with global negative
feedback, input tanh saturation and tanh saturation in the feedback path. The
implicit feedback equation is solved with at most 16 safeguarded Newton steps.
This is not a transistor-level emulation of a particular Moog model. The ladder
structure follows the high-pass ladder discussion in Vadim Zavalishin's
*The Art of VA Filter Design*. Implementation is original native C++.

Q=0.707 is zero feedback. Values above that increase feedback up to below the
three/four-stage oscillation thresholds. Values below it also give zero feedback.
6dB ignores Q completely. Without resonance, each stage contributes -3.0103dB at
the selected cutoff, so a four-stage ladder is -12.0412dB there, unlike the
Butterworth cascade. Passband gain is compensated in the small-signal limit.
The displayed curve is this small-signal response; saturation changes the
response at higher levels. This initial model runs at the host sample rate,
without oversampling, so saturation can introduce aliasing.

The ladder uses the same parameter smoothing, fade-out/reconfigure/fade-in
slope transitions and inactive-band retirement policy as the Butterworth HP.
Nonlinear stereo runs independently on L/R. Mid/side selections use separate
M/S state banks, allowing routing changes to fade without reusing the wrong
channel history. No processing-time allocations are needed.

## Validation

From the repository root:

```sh
c++ -std=c++17 -O2 -I src -I deps/DPF/distrho tests/eq-dsp.cpp src/custom/eq/eq-dsp.cpp -o /tmp/suite3-eq-test
/tmp/suite3-eq-test
c++ -std=c++17 -O2 -I src -I deps/DPF/distrho tests/eq-highpass.cpp src/custom/eq/eq-dsp.cpp -o /tmp/suite3-hp-test
/tmp/suite3-hp-test
c++ -std=c++17 -O2 -I src -I deps/DPF/distrho tests/eq-ladder.cpp src/custom/eq/eq-dsp.cpp -o /tmp/suite3-ladder-test
/tmp/suite3-ladder-test
cmake --build build --target la-eq-jack la-eq-clap la-eq-vst3 la-eq-lv2 la-eq-lv2-ui -j 4
```

The standalone test checks upstream impulse parity, displayed versus measured
response at multiple sample rates, mid/side isolation, retirement and reuse,
zero-length blocks, and exact inactive passthrough. It also prints processing
times for 0, 1, 4 and 16 bells; these are informational, not timing assertions.

The high-pass test compares measured output with the analytic Butterworth
response at all four orders and three sample rates, checks that 6dB is invariant
to Q, and verifies routing, bypass retirement and live automation.

The ladder test checks all slopes and multiple resonance settings against the
small-signal graph at three sample rates, nonlinear stereo crosstalk, mid/side
isolation, 6dB Q independence, retirement and extreme live parameter changes.
