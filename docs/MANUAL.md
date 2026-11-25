# FeetBack

An original guitar feedback synthesizer for Logic Pro, built as a universal Apple Silicon / Intel Audio Unit. It follows the fundamental of a held note and generates a related harmonic that blooms out of the guitar. No third-party effect plugin is required. The two feet in the interface are original, scalable SVG artwork.

Find it in **Audio FX → Audio Units → Niko Audio → FeetBack**. Start with **guitar DI → FeetBack → drive → amp/cab → delay/reverb**. Select Auto low, leave the feedback switch on, and hold a cleanly played single note. Bends and vibrato change the feedback pitch. Chords and heavily distorted input can produce ambiguous pitch estimates; this version is designed for monophonic playing.

| Control | Behavior |
| --- | --- |
| Amount | Generated feedback strength, 0–100%. |
| Attack | Smooth swell to full feedback, 10–10,000 ms, following note acquisition or engagement. |
| Release | Fade back to the original guitar after disengagement or loss of the tracked note, 10–15,000 ms. |
| Sensitivity | Higher settings accept quieter, less periodic notes and allow longer gaps in tracking. Lower settings reject more noise. |
| Tone | Two-pole low-pass on generated feedback only, 200–12,000 Hz, capped below Nyquist at low sample rates. |
| Mix | Maximum dry/feedback blend. 0% is dry; 100% becomes feedback-only at full swell. During onset/release the dry guitar crossfades with the feedback, so waiting for a note does not reduce dry level. |
| Harmonic | Auto low, Auto high, sub octave, unison, octave, octave + fifth, two octaves, or two octaves + major third (fifth harmonic). |
| Footswitch | Latching toggles feedback; Momentary holds feedback while pressed, then uses Release. Entering Momentary disarms the switch. A held press is cleared when the editor closes or a session is restored. The Feedback on parameter can also be automated by the host. |
| Bypass | Immediate, sample-exact dry signal. Discards the generated tail and tracking history. |

All rotary controls reset to their default on double-click, and accept typed values. Hover descriptions appear at the bottom. The note readout shows the detected fundamental; the bar shows the swell envelope. Controls support host automation and state recall.

Auto modes select harmonics by register when a note is acquired or a large note step is detected. Auto low chooses octave below 220 Hz, otherwise unison; Auto high chooses the fifth harmonic below 220 Hz, otherwise the third. Harmonic choice stays stable through gradual bends. Large detected note steps briefly fade the old feedback and start a fresh attack. Small legato changes can carry the existing feedback through.

## Timing and implementation

The dry signal has no buffering or look-ahead, and the AU reports zero fixed processing latency. Pitch acquisition takes several analysis frames (typically around 60–80 ms), followed by the selected Attack. This is the feedback generator's response time, not a delay applied to the guitar. After muting, a brief detector tolerance precedes Release. Synthesized harmonics above the internal anti-alias guard are muted at unusually low host sample rates.

The detector uses an original implementation of the cumulative-normalized difference method described in [de Cheveigné and Kawahara, YIN (2002)](https://doi.org/10.1121/1.1458024). A filtered analysis path runs at no more than 12 kHz. The audio path uses fixed storage, smoothed controls, phase-continuous oscillation, and bounded synthesized feedback. Stereo detection follows the stronger channel, avoiding cancellation from opposing channels; generated feedback is centered while the dry stereo image is preserved. A source with unrelated notes in each channel is outside the monophonic design.
