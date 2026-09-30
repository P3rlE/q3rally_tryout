# Recorded engine audio prototype

The client has an experimental recorded-sample path in addition to the
procedural path. It plays the supplied Golf R recordings directly through the
Q3Rally mixer, crossfades loop boundaries, blends engine samples by RPM, and
uses separate recordings for gear shifts and backfires. The recorded path does
not render the waveguide noise layer.

Enable the prototype in the console:

```text
cg_engineSounds 1
cg_engineAudioMode 3
s_engineAudioSampleGain 4
```

Mode `2` remains the procedural/waveguide path. The recorded path is mode `3`.
`s_engineAudioSampleGain` controls the sample engine level independently.

## Current sample map

The engine clips in `baseq3r/sound/engine_audio/prototype_golf_r/` are loaded as
mono voices and currently loop across almost their full recording with a
120 ms end-to-start crossfade:

| Clip | Initial source RPM | Role |
| --- | ---: | --- |
| `engine_idle.wav` | 2080 | Low-load engine bed |
| `engine_accel_01.wav` | 2200 | Loaded RPM zone |
| `engine_accel_03.wav` | 2300 | Loaded RPM zone |
| `engine_accel_04.wav` | 2860 | Loaded RPM zone |
| `engine_accel_02.wav` | 3400 | Loaded RPM zone |

These initial RPM anchors are estimates from the recordings and need in-game
listening and tuning. Loaded samples are pitch-scaled to vehicle RPM and blended
between adjacent zones. The shift and backfire samples are one-shot events.
Mode `3` also suppresses the existing per-gear looping engine sample so it does
not layer over the recorded voice. The crossfade softens the loop boundary; it
does not remove any rev or shift changes inside a source recording. Manually
verified loop windows are a follow-up tuning step.

## Asset provenance

The WAVs were supplied by the project owner from the `uksm_volkwagen_golf_r_75`
sound bank. Their redistribution rights have not been confirmed. Keep these
assets local to the prototype until the rights holder or source license
explicitly allows packaging them with Q3Rally.
