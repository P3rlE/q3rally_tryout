# Waveguide engine audio

The client mixer now uses a native C adaptation of the waveguide engine sound
model from [Antonio-R1/engine-sound-generator](https://github.com/Antonio-R1/engine-sound-generator).
The generator continuously models cylinder pressure, intake and exhaust paths,
collectors, a multi-path muffler and outlet delay. Q3Rally supplies RPM,
throttle, load, cylinder count and firing order. Existing cockpit, event and
spatial mixing remains in the Q3Rally audio path.

The upstream project is licensed under MIT; the notice and license are in
`engine/code/thirdparty/engine-sound-generator-LICENSE.txt`. The native
adaptation initializes delay buffers explicitly and uses throttle and load to
shape intake airflow and combustion energy.

Set `s_engineAudioWaveguideEnable 0` to compare against the previous procedural
synthesizer. Set it to `1` to use the waveguide model. Delay lengths are based
on the upstream example configuration and scaled to the active mixer sample
rate; vehicle-specific pipe tuning is still a separate task.

To enable the procedural vehicle audio path in-game, set:

```text
cg_engineSounds 1
cg_engineAudioMode 2
s_engineAudioWaveguideEnable 1
```
