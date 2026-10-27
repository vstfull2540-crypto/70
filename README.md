# Arm Nong Muang Analyzer — GitHub Ready Fix11

This revision aligns the dynamic overlays and transparent hit targets to the supplied 1414×1113 PNG artwork, rendered at 50% scale. The yellow focus outline is drawn at the selected control's artwork bounds. The three meter lanes now show Left RMS, combined stereo RMS, and Right RMS, with a more sensitive visual range of −72 to 0 dBFS.

**Audio path:** this analyzer remains bit-transparent; the meter-scale change does not add output gain or otherwise change the audio. If audible loudness needs to be increased, that is a separate output-gain feature and should be added explicitly with a controllable gain and clipping protection.

## Build

Open the repository's **Actions** tab and run **Build Windows VST3**. The workflow builds VST3 and Standalone on Windows x64 and uploads both artifacts. This package has not yet been verified by a successful GitHub Actions run, so do not treat the build as confirmed until the run completes successfully.


## Fix12 — meter placement and live loudness readout

- Shifted the three live meter fills 20 reference pixels to the right and 15 pixels downward, while preserving the 50% PNG-based interface.
- Added a live numerical RMS loudness strip for Left, Stereo RMS, and Right, in dBFS. This is a measurement display only and does not add gain or alter audio.
- CI and DAW runtime testing are still required to confirm the final build and visual alignment on the target host.


## Fix13 — remove numeric readout and lower meter bars

- Removed the live numerical RMS dBFS loudness readout from the interface.
- Moved all three live meter fills down by 30 additional reference pixels (Y=603), keeping their horizontal positions and the PNG background unchanged.
- Audio remains analysis-only; this visual adjustment does not change the signal path.
- Windows GitHub Actions and DAW runtime testing are still required to confirm compilation and final alignment.


## Fix14 — reposition Stereo and Right meters

- Moved all three meter fills down another 30 reference pixels (Y=633).
- Moved the combined Stereo RMS meter and Right-channel RMS meter 20 reference pixels farther right; the Left meter keeps its existing horizontal position.
- Kept the live numeric loudness readout removed and left the PNG background and audio path unchanged.
- GitHub Actions and DAW runtime testing are still required to confirm compilation and final visual alignment.


## Fix15 — meter alignment refinement

- Moved all three meter bars up 10 reference pixels from Fix14 (Y=623).
- Moved the combined Stereo RMS and Right-channel RMS bars 10 reference pixels left; Left RMS retains its X position.
- Kept the live numeric loudness readout removed. The background PNG and audio signal path remain unchanged.
- Build status remains unverified until GitHub Actions succeeds and the plugin is checked in a DAW.


## Fix15 — meter alignment update

- Stereo RMS meter moved 2 reference pixels left (x = 1228; y = 623 unchanged).
- Right RMS meter moved 5 reference pixels left (x = 1284; y = 623 unchanged).
- This package is based on the Fix15 project; no other source behavior was intentionally changed in this update.
- Windows compilation and in-host DAW verification still need to be confirmed by running the included GitHub Actions workflow.


## Fix15 — Spectrum Analyzer position adjustment

- Moved the Spectrum Analyzer drawing/clip area 20 reference pixels to the right and 10 reference pixels down from the previous adjusted Fix15 package (X=119, Y=137; width=990, height=334).
- Kept the background PNG, meters, controls, audio processing, and all other behavior unchanged.
- Windows GitHub Actions build and DAW runtime alignment still need to be verified.


## Fix15 — Spectrum Analyzer position adjustment v3

- Moved the Spectrum Analyzer drawing/clip area 50 reference pixels to the right and 5 reference pixels up from the previous package (X=169, Y=132; width=990, height=334).
- Kept the background PNG, meters, controls, audio processing, and all other behavior unchanged.
- Windows GitHub Actions build and DAW runtime alignment still need to be verified.


### Spectrum Analyzer position adjustment (latest)
- Moved Spectrum Analyzer plot 50 px left and 2 px up relative to the previous package.
- Plot bounds: x=119, y=130, width=990, height=334.
- No other UI positions or audio-processing code were intentionally changed in this adjustment.


## Wavelet RTA update — 68 bands

- Replaced the previous generic logarithmic FFT-bin grouping with a 68-band Morlet-style wavelet filter bank. Each band uses a Gaussian frequency response around its logarithmically spaced center frequency with a constant-Q relationship; it is not a fixed 1/3-octave analyzer.
- Increased the FFT size to 8192 points to improve low-frequency resolution for the wavelet bank. The 68 wavelet measurements are resampled into the existing 256-point memory interface for compatibility, while the visible Spectrum/RTA curve is drawn with 68 band positions. This preserves the PNG UI, Peak Hold, graph memory, and existing controls.
- Moved the Spectrum Analyzer drawing/clip rectangle 20 reference pixels left and 5 pixels down from the prior package: X=99, Y=135, width=990, height=334. Other UI positions and audio pass-through behavior are unchanged.
- Build validation still requires running the included GitHub Actions workflow on Windows; no successful CI run is claimed by this source update alone.


Spectrum Analyzer position update: moved 15 pixels to the right from the previous version. Plot bounds are X=114, Y=135, W=990, H=334. Other source settings are unchanged.
