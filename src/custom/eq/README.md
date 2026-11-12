# Native dynamic EQ (bell, shelves, cuts and tilt)

`eq-dsp.cpp` replaces the generated EQ processor. EQ configuration and parameter
metadata are native files; `src/dsp-faust/eq.dsp` is no longer compiled or used to
generate EQ metadata. The shared `FaustDSP` interface is retained for compatibility
with the suite's plugin shell. Common input/output processing still uses Faust.

There are 16 stable, reusable host parameter slots. Present, enabled, type,
channel, frequency, gain, Q, adaptive Q and slope are stored per slot. Bell
(type 2), low/high shelves (types 1/3), Butterworth high-pass (type 0),
Butterworth low-pass (type 4) and tilt (type 5) process audio. The UI creates a
high-pass in the leftmost 10% of the display, a low shelf from 10–27%, a bell
from 27–76%, a high shelf from 76–90%, and a low-pass in the rightmost 10%. These and Tilt are selectable
from the type menu. Slots are preallocated; the audio callback never allocates. Disabled,
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

## Butterworth low-pass

The shared `ButterworthCut` template in `highpass.hpp` also supplies `LowPass`,
using the low-pass outputs of the same trapezoidal one-pole and SVF sections.
Type 4 is now enabled in both UI curve selectors. Slopes are 6/12/18/24dB/oct,
with neutral Q=0.707 giving -3.0103dB at cutoff for every order. The 6dB slope
ignores Q and disables its control; higher orders scale the highest-Q pole pair
for resonance. The digital graph uses the same prewarping and pole arrangement.

Low-pass has independent preallocated mid/side states, coefficient smoothing,
click-reducing topology transitions, and fades to exact bypass before retiring.
Existing type IDs and host parameter slots are unchanged.

## x42 shelves

`x42-shelf.hpp` is an unmodified copy of x42/fil4.lv2 `src/iir.h` at commit
`885edf4d4c8da3f1414462ec7b5fc1c2da0f3ac1`. Copyright 2008, 2015 Robin Gareus;
GPL-2.0-or-later. Its copyright and license notice are preserved.

`shelf.hpp` wraps the original low/high shelf coefficient calculations,
transposed biquad processing and gain/frequency/Q interpolation. Interpolation
is unchanged for 32-sample chunks and adjusted for shorter tails. The UI exposes
coefficient Q directly (0.3–2.0, default 0.7), rather than x42's remapped
Bandwidth knob. DSP and graph both retain x42's coefficient Q clamp of 0.25–2
and frequency clamp of 0.0004–0.47 times sample rate. Frequencies and Q submitted
by host automation outside these limits are clamped for processing.

Low and high shelves have independent state per mid/side channel. Removed,
disabled, changed-type and zero-gain shelves smooth back to unity before retiring
to exact passthrough. No allocation occurs during processing. The plotted
response evaluates the x42 equations in double precision before coefficient
rounding, including shelf gain at DC/Nyquist and Q-dependent overshoot. This
avoids visible quantization jumps when dragging near the minimum cutoff. The
audio path retains upstream float coefficients, so its rounding error at very
low frequencies is not reproduced in the graph. Host type IDs and the
parameter layout are unchanged.

## Tilt

Type 5 is a native tilt with Frequency, Gain and Shape controls. Frequency is
its 0dB pivot (the node stays on that pivot); dragging vertically adjusts gain.
Positive gain lowers bass and raises treble; negative gain reverses the tilt.
Shape is 0% Linear through 100% Shelf, also editable by scrolling the node.
Double-clicking Shape restores Linear. The curve selectors include a tilt icon.

Twelve first-order shelves are distributed evenly in log frequency at Linear,
producing an approximately straight dB/log-frequency response. Shape gradually
collapses their center frequencies onto the pivot, yielding opposing bass and
treble shelves. Linear is a finite-filter approximation, with rounding at the
frequency extremes, not a linear-phase filter. Gain sets half the total DC-to-
Nyquist dB difference. Pivot normalization holds 0dB at the chosen frequency;
at off-center pivots the endpoint gains need not be equal and opposite.

Coefficients and states use double precision. Centers are limited to 1Hz–0.49
of the sample rate, and the pivot uses the same limits. DSP and graph evaluate
the same digital filter; graph coefficients are cached per band. Coefficients
smooth over 10ms, and enable/removal/type/routing changes fade over 5ms.
Zero-gain and inactive bands retire to exact bypass without per-sample work.
All filter state is preallocated, with independent mid/side processing.

Existing parameter IDs and slot layout remain intact. For tilt, the existing
host Q slot encodes Shape linearly: Q=0.3 is Linear, Q=8.01 is Shelf. The custom
UI shows Shape as 0–100%; host generic parameter views retain the Q slot name.
This value is saved and restored with the other band parameters.

## Listen

Each slot has a Listen parameter (`kListen`, after Slope). The lowest present
slot with Listen on is auditioned through a trapezoidal SVF at the band's
frequency, applied to the input before the EQ and replacing the EQ output
(`listen.hpp`), so you hear the region the band works on, unprocessed: a bell
through a unity-peak band-pass of its Q (0.3–8.01), a low shelf or high-pass
through a 0.707 low-pass, a high shelf or low-pass through a 0.707 high-pass,
tilt through a 0.707 band-pass. A Mid or Side band mutes the other channel while
listening. Listen and shape changes crossfade over 5ms and its coefficients
smooth over 10ms; when off, it costs no
per-sample work. The UI sets Listen only while Control is held, for one band: the
one being dragged (node, piano dot, value label or option-bar box), else the
selected one. Releasing Control restores whatever was listened to before.
While a band is listened to, the display greys out the EQ curve and draws the
listen filter's response (`listenResponse`, the same digital SVF) in the band's
colour over it.

## Mouse controls

Modifiers follow FabFilter Pro-Q conventions; Cmd on macOS stands for Ctrl.

- Click empty display or piano: new band, dragged immediately.
- Drag a node: frequency and gain (cuts: resonance). Shift: fine (¼ speed).
- Hold Control: listen to the band being dragged, or else the selected band.
- Double-click or Alt-click a node or piano dot: band on/off.
- Wheel over a node: Q (cuts: slope). Shift: fine.
- Right-click a node: type and channel menu.
- Value labels and number boxes: drag to edit, Shift for fine, wheel to step;
  Ctrl/Cmd-click or double-click resets to default.
- Option bar: power (on/off) and X (remove) buttons.

## UI framework

The EQ UI sits in the suite's shared frame (`eq-ui.cpp`): `RootWidget` with the
common `TopBar`, `MainAreaContainerWidget` with the IO meters, and the `UI`
template's background and starfield shaders, which `EqRootWidget` confines to
the curve display's well. The central area stacks the shared `WaveformWidget`
analyser under the EQ widget, spanning the display's 20 Hz–20 kHz (the range
`LibreAudioAnalyzerIPC` bins cover). The analyser shows the EQ output; the
pre/post caption is a display setting only. The band controls are drawn by the
EQ widget itself, since the shared widgets bind fixed parameter indices.

## Validation

From the repository root:

```sh
c++ -std=c++17 -O2 -I src -I deps/DPF/distrho tests/eq-dsp.cpp src/custom/eq/eq-dsp.cpp -o /tmp/suite3-eq-test
/tmp/suite3-eq-test
c++ -std=c++17 -O2 -I src -I deps/DPF/distrho tests/eq-highpass.cpp src/custom/eq/eq-dsp.cpp -o /tmp/suite3-hp-test
/tmp/suite3-hp-test
c++ -std=c++17 -O2 -I src -I deps/DPF/distrho tests/eq-shelf.cpp src/custom/eq/eq-dsp.cpp -o /tmp/suite3-shelf-test
/tmp/suite3-shelf-test
c++ -std=c++17 -O2 -I src -I deps/DPF/distrho tests/eq-lowpass.cpp src/custom/eq/eq-dsp.cpp -o /tmp/suite3-lp-test
/tmp/suite3-lp-test
c++ -std=c++17 -O2 -I src -I deps/DPF/distrho tests/eq-tilt.cpp src/custom/eq/eq-dsp.cpp -o /tmp/suite3-tilt-test
/tmp/suite3-tilt-test
cmake --build build --target la-eq-jack la-eq-clap la-eq-vst3 la-eq-lv2 la-eq-lv2-ui -j 4
```

The standalone test checks upstream impulse parity, displayed versus measured
response at multiple sample rates, mid/side isolation, retirement and reuse,
zero-length blocks, and exact inactive passthrough. It also prints processing
times for 0, 1, 4 and 16 bells; these are informational, not timing assertions.

The high-pass test compares measured output with the analytic Butterworth
response at all four orders and three sample rates, checks that 6dB is invariant
to Q, and verifies routing, bypass retirement and live automation.

The shelf test covers upstream sample parity including smoothing, measured vs
plotted response across sample rates and Q/gain settings, endpoint gains,
mid/side isolation, exact bypass retirement, type switches and extreme automation.

The low-pass test checks all orders against the Butterworth transfer function,
DC/Nyquist endpoints, 6dB Q independence, routing, retirement and automation.

The tilt test checks measured versus plotted response across sample rates and
shape/gain settings, pivot unity, endpoint gain span, shape behavior, mid/side
isolation, retirement, and extreme live automation with changing block sizes.
