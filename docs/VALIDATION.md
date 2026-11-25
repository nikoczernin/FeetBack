# Validation

## Automated checks

`bash scripts/build.sh` builds both Apple Silicon and Intel slices, runs the CTest suite, verifies the component’s ad-hoc signature, and renders an interface preview. The tests run on the build machine’s native architecture; compiling both slices does not substitute for testing on an Intel Mac.

FeetBack’s DSP checks cover sine-wave pitch accuracy, harmonic spectra, overtone-rich signals, bends, note changes, attack/release timing, noise rejection, silence, tone response, opposing stereo channels, and host block-size independence. The processor/UI checks cover mono/stereo layouts, zero reported latency, exact plugin/host bypass, session recall, knob reset, and latching/momentary operation.

The development v1.0.0 component passed Apple’s strict AU validator after installation. Synthetic sine tests across 55–1319 Hz and 11.025–192 kHz sample rates measured a worst pitch error below 1.82 cents. This is a synthetic test result, not a guarantee for noisy or polyphonic guitar recordings.

The dry path is undelayed. Pitch acquisition has its own response time, typically around 60–80 ms before the selected Attack. Test your preferred settings with your own guitar and signal chain.

Pitch-detection reference: [de Cheveigné and Kawahara, YIN (2002)](https://doi.org/10.1121/1.1458024). The implementation here is original.

## Repository release checks — 25 November 2026

A fresh standalone universal Release build completed without compiler warnings. All CTest suites passed. Both binary architectures and the code signature were checked. The installer passed fresh-install, backup/update, and unrelated-component protection checks in isolated temporary folders; L8BLOOMER also passed migration from the old Slow Bloom bundle name.

These packaging checks did not replace the plugins already installed in Logic.
