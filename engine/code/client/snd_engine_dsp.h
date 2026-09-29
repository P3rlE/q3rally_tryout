#ifndef SND_ENGINE_DSP_H
#define SND_ENGINE_DSP_H

#include "snd_engine_audio.h"

typedef struct engineAudioBiquadState_s {
    float x1, x2;
    float y1, y2;
} engineAudioBiquadState_t;

typedef struct engineAudioResonatorState_s {
    engineAudioBiquadState_t biquad;
} engineAudioResonatorState_t;

#define ENGINE_AUDIO_WAVEGUIDE_MAX_DELAY 160
#define ENGINE_AUDIO_WAVEGUIDE_MAX_CYLINDERS MAX_ENGINE_AUDIO_FIRING_ORDER
#define ENGINE_AUDIO_WAVEGUIDE_MUFFLER_COUNT 4

typedef struct engineAudioWaveguide_s {
    float upper[ENGINE_AUDIO_WAVEGUIDE_MAX_DELAY];
    float lower[ENGINE_AUDIO_WAVEGUIDE_MAX_DELAY];
    unsigned short upperLength;
    unsigned short lowerLength;
    unsigned short upperIndex;
    unsigned short lowerIndex;
    float reflectionLeft;
    float reflectionRight;
    float outputLeft;
    float outputRight;
} engineAudioWaveguide_t;

typedef struct engineAudioWaveguideCylinder_s {
    engineAudioWaveguide_t cylinder;
    engineAudioWaveguide_t intake;
    engineAudioWaveguide_t exhaust;
    engineAudioWaveguide_t extractor;
    float phaseOffset;
} engineAudioWaveguideCylinder_t;

typedef struct engineAudioWaveguideState_s {
    qboolean initialized;
    qboolean enabled;
    const engineAudioPreset_t *preset;
    float sampleRate;
    float currentCycle;
    float intakeNoise;
    float crankshaftNoise;
    float engineBlock;
    float mufflerOutputLeft;
    float mufflerOutputRight;
    int cylinderCount;
    int strokeCycle;
    engineAudioWaveguideCylinder_t cylinders[ENGINE_AUDIO_WAVEGUIDE_MAX_CYLINDERS];
    engineAudioWaveguide_t straightPipe;
    engineAudioWaveguide_t muffler[ENGINE_AUDIO_WAVEGUIDE_MUFFLER_COUNT];
    engineAudioWaveguide_t outlet;
} engineAudioWaveguideState_t;

typedef struct engineAudioSynthState_s {
    float sampleRate;

    float phase;
    float crankPhase;
    float combustionPhase;
    float combustionJitter;
    float combustionTone;
    float combustionNoise;
    int firingOrderIndex;

    float smoothedRpm;
    float smoothedThrottle;
    float smoothedLoad;

    float limiterEnvelope;
    float backfireEnvelope;
    float overrunPopEnvelope;

    float cockpitLowpassL[2];
    float cockpitLowpassR[2];
    float toneLowpassL[2];
    float toneLowpassR[2];

    unsigned int noiseSeed;

    engineAudioResonatorState_t exhaustStates[MAX_ENGINE_AUDIO_RESONATORS];
    engineAudioResonatorState_t intakeStates[MAX_ENGINE_AUDIO_RESONATORS];

    float harmonicPhase[MAX_ENGINE_AUDIO_HARMONICS];
    engineAudioWaveguideState_t waveguide;
} engineAudioSynthState_t;

void S_EngineDSP_Reset( engineAudioSynthState_t *state, float sampleRate );
void S_EngineDSP_InitWaveguide(
    engineAudioSynthState_t *state,
    const engineAudioPreset_t *preset,
    float sampleRate );
void S_EngineDSP_RenderVehicle(
    engineAudioSynthState_t *synth,
    const engineAudioPreset_t *preset,
    const vehicleAudioState_t *control,
    engineAudioQualityTier_t quality,
    int sampleCount,
    float *outExhaustLeft,
    float *outExhaustRight,
    float *outEngineBayLeft,
    float *outEngineBayRight );
void S_EngineDSP_TriggerBackfire( engineAudioSynthState_t *state );

#endif /* SND_ENGINE_DSP_H */
