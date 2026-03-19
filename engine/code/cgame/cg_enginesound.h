/*
===========================================================================
cg_enginesound.h  --  Pulse-train engine sound synthesizer for Q3Rally

Replaces the sample-based engine sound system with procedural synthesis
inspired by Engine Simulator (github.com/ange-yaghi/engine-sim).

Principle:
  1. Each cylinder fires an impulse at its crank phase.
  2. Impulses are shaped by an ADSR-style envelope and throttle amplitude.
  3. The impulse train is fed through a one-pole low-pass and a single
     digital waveguide delay line that models exhaust pipe resonance.
  4. The resulting PCM buffer is submitted via S_RawSamples() into Q3's
     existing raw-sample ring buffer -- no new traps required.

Per-vehicle configuration lives in cgEngineConfig_t. Since Q3Rally has
no car config files yet, sensible defaults are baked in and a cvar
override is provided for tuning.
===========================================================================
*/

#ifndef CG_ENGINESOUND_H
#define CG_ENGINESOUND_H

// Forward declaration -- centity_t is defined later in cg_local.h.
// cg_enginesound.h is included before centity_t, so we only need a pointer.
struct centity_s;

// ---------------------------------------------------------------------------
// tuneable constants
// ---------------------------------------------------------------------------

#define CG_ES_SAMPLE_RATE       48000           // Hz -- must match SDL mixer rate
// Buffer capacity: 100ms at 44100 Hz = 4410 samples, allocated in cg_enginesound.c
// Actual samples generated per frame are calculated dynamically from cg.frametime.
#define CG_ES_MAX_CYLINDERS     12

// Waveguide delay line size: power of two for cheap ring-buffer masking.
// 2048 samples @ 44100 Hz = ~46ms max pipe length (~8m).
#define CG_ES_WG_MAX_LEN        2048
#define CG_ES_WG_MASK           (CG_ES_WG_MAX_LEN - 1)

// Raw-stream slot for the engine synthesizer.
// Stream 0 is used by RoQ video but is idle during gameplay.
// Q3 keeps s_rawend[0] warm so there is no cold-start gap vs s_paintedtime.
#define CG_ES_RAW_STREAM        0

// ---------------------------------------------------------------------------
// per-vehicle engine configuration
// ---------------------------------------------------------------------------

typedef struct {
    // --- engine geometry ---
    int     numCylinders;           // 4, 6, 8 ...
    float   firingOrder[CG_ES_MAX_CYLINDERS]; // crank angles (degrees) of each
                                              // cylinder's power stroke, 0-720
    // --- impulse shape ---
    float   impulseAttack;          // seconds (rise time of each firing pulse)
    float   impulseDecay;           // seconds (fall time)
    float   impulsePeak;            // 0-1 relative to throttle-scaled amplitude

    // --- waveguide (exhaust resonance) ---
    float   exhaustLengthM;         // physical pipe length in metres
                                    // resonant freq = speed_of_sound / (2*L)
    float   exhaustFeedback;        // 0-1 feedback coefficient (0 = open, 1 = closed)
    float   exhaustDamping;         // per-sample decay inside the delay line (0-1)

    // --- low-pass body filter ---
    float   bodyFilterCutoff;       // Hz, applied after waveguide (engine block damp)
} cgEngineConfig_t;

// ---------------------------------------------------------------------------
// per-entity synthesizer state  (stored in centity_t)
// ---------------------------------------------------------------------------

typedef struct {
    // --- crank position (degrees, 0-720 for a 4-stroke cycle) ---
    float   crankAngle;

    // --- per-cylinder envelope state ---
    float   cylEnv[CG_ES_MAX_CYLINDERS];
    int     cylPhase[CG_ES_MAX_CYLINDERS];

    // --- waveguide delay line (inline -- no pointer, QVM safe) ---
    float   wgBuffer[CG_ES_WG_MAX_LEN];
    int     wgLen;
    int     wgWrite;
    float   wgFeedback;

    // --- body LP filter state ---
    float   lpState;

    int     configHash;
    int     debugSkipBucket;
    int     rawEndEst;              /* estimated s_rawend, for overflow guard */
    qboolean    initialized;
} cgEngineSynthState_t;
// NOTE: cgEngineSynthState_t is large (~8 KB). It must NOT be embedded in
// centity_t (which is allocated MAX_GENTITIES times). Use the single global
// cg_engineSynth declared below instead.

// ---------------------------------------------------------------------------
// API
// ---------------------------------------------------------------------------

// Single global synthesizer state for the local player.
// Must NOT be in centity_t -- too large for MAX_GENTITIES allocation.
extern cgEngineSynthState_t cg_engineSynth;

// Call once on cgame init / map load.
void CG_EngineSound_Init( void );

// Call once per rendered frame for the local player.
// entityNum    -- cent->engineSoundEntity (for S_RawSamples spatialization)
// rpm          -- from cg.predictedPlayerState.stats[STAT_RPM]
// gear         -- from cg.predictedPlayerState.stats[STAT_GEAR]
// throttle     -- 0.0 (coast) to 1.0 (full) derived from usercmd forwardmove
void CG_EngineSound_Update( int entityNum, int rpm, int gear, float throttle );

// Call on cgame shutdown.
void CG_EngineSound_Shutdown( void );

// Build a default 4-cylinder config (used until per-car configs exist).
void CG_EngineSound_DefaultConfig( cgEngineConfig_t *cfg );

#endif // CG_ENGINESOUND_H
