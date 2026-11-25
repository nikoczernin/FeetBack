# What’s been checked

## The tests

The build script compiles for Apple Silicon and Intel, runs the tests, checks the plugin’s signature, and saves a picture of the interface. The tests run on the Mac doing the build, so a successful Intel build still needs a run on an Intel Mac to confirm how it behaves there.

FeetBack’s DSP checks cover sine-wave pitch accuracy, harmonic spectra, overtone-rich signals, bends, note changes, attack/release timing, noise rejection, silence, tone response, opposing stereo channels, and host block-size independence. The processor/UI checks cover mono/stereo layouts, zero reported latency, exact plugin/host bypass, session recall, knob reset, and latching/momentary operation.

The development v1.0.0 component passed Apple’s strict AU validator after installation. Synthetic sine tests across 55–1319 Hz and 11.025–192 kHz sample rates measured a worst pitch error below 1.82 cents. That’s with generated test tones. Real guitar, especially noisy input or chords, can be trickier.

Your dry guitar isn’t delayed. The feedback usually takes around 60–80 ms to find the note before the Attack starts. Give it a try with your own guitar and adjust the settings to taste.

Pitch-detection reference: [de Cheveigné and Kawahara, YIN (2002)](https://doi.org/10.1121/1.1458024). The implementation here is original.

## Before the first GitHub release — 25 November 2026

Both Mac versions built without compiler warnings, and all the automated tests passed. The installer was checked in temporary folders: installing from scratch, updating with a backup, and refusing to replace an unrelated plugin all worked.
