# FeetBack

Hold a note and FeetBack builds a matching feedback tone around it. You can choose the harmonic, let it swell in slowly, and blend it with your guitar. It runs on Intel and Apple Silicon Macs and includes its own feedback engine. The two feet are vector artwork, so they stay crisp.

Find it in **Audio FX → Audio Units → Niko Audio → FeetBack**. Start with **guitar DI → FeetBack → drive → amp/cab → delay/reverb**. Select Auto low, leave the feedback switch on, and hold a cleanly played single note. Bends and vibrato change the feedback pitch. It works best with one note at a time. Chords and heavily distorted input can make it harder to find the pitch.

| Control | What it does |
| --- | --- |
| Amount | Generated feedback strength, 0–100%. |
| Attack | Smooth swell to full feedback, 10–10,000 ms, after it finds a note or you turn feedback on. |
| Release | Fade back to the original guitar after you turn feedback off or it loses the note, 10–15,000 ms. |
| Sensitivity | Higher settings accept quieter, less periodic notes and allow longer gaps in tracking. Lower settings reject more noise. |
| Tone | Two-pole low-pass on generated feedback only, 200–12,000 Hz, capped below Nyquist at low sample rates. |
| Mix | Maximum dry/feedback blend. 0% is dry; 100% becomes feedback-only at full swell. During onset/release the dry guitar crossfades with the feedback, so waiting for a note does not reduce dry level. |
| Harmonic | Auto low, Auto high, sub octave, unison, octave, octave + fifth, two octaves, or two octaves + major third (fifth harmonic). |
| Footswitch | Latching toggles feedback; Momentary holds feedback while pressed, then uses Release. Switching to Momentary starts with feedback off. Closing the window or reopening a session also releases a held press. The Feedback on parameter can also be automated by the host. |
| Bypass | Passes your original guitar straight through, ending the feedback tail and resetting note tracking. |

Double-click a knob to reset it, or click its value to type one in. Hover over a control for a quick explanation at the bottom. The note display shows what FeetBack is hearing, and the bar shows how far the swell has gone. You can automate the controls, and your settings save with the session.

Auto modes select harmonics by register when a note is acquired or a large note step is detected. Auto low chooses octave below 220 Hz, otherwise unison; Auto high chooses the fifth harmonic below 220 Hz, otherwise the third. Harmonic choice stays stable through gradual bends. Large detected note steps briefly fade the old feedback and start a fresh attack. Small legato changes can carry the existing feedback through.

## A little more detail

Your dry guitar passes through without added buffering, and the plugin reports zero fixed latency. The feedback usually needs around 60–80 ms to find the pitch before the Attack starts. After you mute the guitar, it waits briefly before starting Release so small gaps don’t cut it off. At unusually low sample rates, harmonics that would go above its anti-aliasing limit are muted.

The detector uses an original implementation of the cumulative-normalized difference method described in [de Cheveigné and Kawahara, YIN (2002)](https://doi.org/10.1121/1.1458024). A filtered analysis path runs at no more than 12 kHz. The audio path uses fixed storage, smoothed controls, phase-continuous oscillation, and bounded synthesized feedback. Stereo detection follows the stronger channel, avoiding cancellation from opposing channels; generated feedback is centered while the dry stereo image is preserved. It’s still tracking one note, so unrelated notes in the left and right channels can confuse it.
