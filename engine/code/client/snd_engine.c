/*
===========================================================================
Copyright (C) 2026 Q3Rally Team

This file is part of Q3Rally source code.

Q3Rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Q3Rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
===========================================================================
*/
// snd_engine.c -- engine sound voices for cars
//
// Every car has a set of constant-rpm loops ("on" set). Each loop knows its
// measured fundamental frequency f_i. The target frequency f_target is taken
// from a linear curve over the normalized rpm. Every voice is pitched by
// f_target / f_i, so neighbouring loops always share the same fundamental and
// can be crossfaded (equal power) over the whole interval without beating.
//
// Voices keep their own playback phase (independent of s_paintedtime), use
// 4-point Hermite interpolation and ramp gain and pitch linearly over every
// mixed block. The mixer is backend independent: the DMA backend paints it
// directly in S_PaintChannels, the OpenAL backend streams it.

#ifndef SND_ENGINE_STANDALONE
#include "client.h"
#include "snd_local.h"
#include "snd_codec.h"
#define ENG_Malloc(size)	Z_Malloc(size)
#define ENG_Free(ptr)		Z_Free(ptr)
#define ENG_Printf			Com_Printf
#define ENG_Milliseconds	Com_Milliseconds
#endif

#define MAX_ENGINE_DEFS			32
#define MAX_ENGINE_SAMPLES		16
#define MAX_ENGINE_EMITTERS		16
#define ENGINE_TIMEOUT_MSEC		100
#define ENGINE_MIN_PITCH		0.5f
#define ENGINE_MAX_PITCH		2.0f
#define ENGINE_MIX_CHUNK		1024
#define ENGINE_LEGACY_SAMPLES	11
#define ENGINE_LEGACY_BASE_HZ	29.0f	// engine0 of the stock set; the set rises in semitones

typedef struct {
	float	*data;		// mono, -1..1
	int		length;		// in samples
	int		rate;
	float	f0;			// fundamental frequency in Hz
} engineSample_t;

typedef struct {
	qboolean		valid;
	char			name[MAX_QPATH];
	int				numOn;
	engineSample_t	on[MAX_ENGINE_SAMPLES];		// sorted by f0
	float			curveLo, curveHi;			// f_target at rpmFrac 0 and 1
	float			vIdle, vMax, gamma, vOff;	// loudness curve
} engineDef_t;

typedef struct {
	qboolean			active;
	int					entityNum;
	int					def;
	int					lastUpdate;
	engineSoundParams_t	params;
	// mixer state
	qboolean			primed;
	float				curFreq;
	float				curVol;
	double				phase[MAX_ENGINE_SAMPLES];
	float				gain[MAX_ENGINE_SAMPLES];
	// debug
	int					dbgA, dbgB;
	float				dbgT;
} engineEmitter_t;

static engineDef_t		engineDefs[MAX_ENGINE_DEFS];
static engineEmitter_t	engineEmitters[MAX_ENGINE_EMITTERS];

#ifndef SND_ENGINE_STANDALONE
static cvar_t	*s_engineVolume;
static cvar_t	*s_engineDebug;
#define ENGINE_VOLUME	( s_engineVolume ? s_engineVolume->value : 1.0f )
#define ENGINE_DEBUG	( s_engineDebug ? s_engineDebug->integer : 0 )
#else
#define ENGINE_VOLUME	1.0f
#define ENGINE_DEBUG	0
#endif

static float S_Engine_Clamp( float v, float lo, float hi ) {
	if ( v < lo ) {
		return lo;
	}
	if ( v > hi ) {
		return hi;
	}
	return v;
}

/*
=================
S_Engine_SortSamples
=================
*/
static void S_Engine_SortSamples( engineDef_t *def ) {
	int i, j;
	engineSample_t tmp;

	for ( i = 1; i < def->numOn; i++ ) {
		tmp = def->on[i];
		for ( j = i - 1; j >= 0 && def->on[j].f0 > tmp.f0; j-- ) {
			def->on[j + 1] = def->on[j];
		}
		def->on[j + 1] = tmp;
	}
}

/*
=================
S_Engine_SetDefaults
=================
*/
static void S_Engine_SetDefaults( engineDef_t *def ) {
	def->curveLo = 0.0f;
	def->curveHi = 0.0f;
	def->vIdle = 0.45f;
	def->vMax = 1.0f;
	def->gamma = 1.3f;
	def->vOff = 0.6f;
}

/*
=================
S_Engine_FinishDef

Sort the samples and fill in a curve that was not given.
=================
*/
static qboolean S_Engine_FinishDef( engineDef_t *def ) {
	if ( def->numOn <= 0 ) {
		return qfalse;
	}
	S_Engine_SortSamples( def );
	if ( def->curveLo <= 0.0f || def->curveHi <= 0.0f ) {
		def->curveLo = def->on[0].f0;
		def->curveHi = def->on[def->numOn - 1].f0;
	}
	def->valid = qtrue;
	return qtrue;
}

/*
=================
S_Engine_Targets

Target frequency, overall volume and per sample gains for the given parameters.
=================
*/
static void S_Engine_Targets( engineEmitter_t *e, const engineDef_t *def, float alive,
	float *freq, float *vol, float *gains ) {
	float n, load, f, t;
	int i, a, last;

	n = S_Engine_Clamp( e->params.rpmFrac, 0.0f, 1.0f );
	load = S_Engine_Clamp( e->params.load, 0.0f, 1.0f );
	f = def->curveLo + ( def->curveHi - def->curveLo ) * n;

	for ( i = 0; i < def->numOn; i++ ) {
		gains[i] = 0.0f;
	}

	last = def->numOn - 1;
	a = 0;
	t = 0.0f;
	if ( def->numOn == 1 || f <= def->on[0].f0 ) {
		gains[0] = 1.0f;
	} else if ( f >= def->on[last].f0 ) {
		a = last;
		gains[last] = 1.0f;
	} else {
		for ( a = 0; a < last - 1; a++ ) {
			if ( f < def->on[a + 1].f0 ) {
				break;
			}
		}
		t = ( f - def->on[a].f0 ) / ( def->on[a + 1].f0 - def->on[a].f0 );
		t = S_Engine_Clamp( t, 0.0f, 1.0f );
		gains[a] = cos( t * M_PI * 0.5 );
		gains[a + 1] = sin( t * M_PI * 0.5 );
	}
	e->dbgA = a;
	e->dbgB = ( a < last && t > 0.0f ) ? a + 1 : a;
	e->dbgT = t;

	*freq = f;
	*vol = ( def->vIdle + ( def->vMax - def->vIdle ) * pow( n, def->gamma ) ) *
		( def->vOff + ( 1.0f - def->vOff ) * load ) * alive;
}

/*
=================
S_Engine_RenderEmitter

Adds count mono samples at outRate into out.
Gain, volume and frequency ramp linearly from the last block to the new targets.
=================
*/
static void S_Engine_RenderEmitter( engineEmitter_t *e, float *out, int count, int outRate, int now ) {
	const engineDef_t *def;
	float tFreq, tVol, tGains[MAX_ENGINE_SAMPLES];
	float alive, inv;
	int i, k;

	def = &engineDefs[e->def];
	if ( !def->valid || count <= 0 ) {
		return;
	}

	alive = ( now - e->lastUpdate <= ENGINE_TIMEOUT_MSEC ) ? 1.0f : 0.0f;
	S_Engine_Targets( e, def, alive, &tFreq, &tVol, tGains );

	if ( !e->primed ) {
		// fade in from silence, start every loop at its beginning
		e->curFreq = tFreq;
		e->curVol = 0.0f;
		for ( i = 0; i < def->numOn; i++ ) {
			e->gain[i] = tGains[i];
			e->phase[i] = 0.0;
		}
		e->primed = qtrue;
	}

	inv = 1.0f / (float)count;

	for ( i = 0; i < def->numOn; i++ ) {
		const engineSample_t *s = &def->on[i];
		const float *data = s->data;
		const int len = s->length;
		float g0 = e->gain[i];
		float g1 = tGains[i];
		double pos, rateScale;

		if ( g0 <= 0.0f && g1 <= 0.0f ) {
			continue;
		}
		if ( !data || len < 4 ) {
			continue;
		}

		pos = e->phase[i];
		rateScale = (double)s->rate / (double)outRate;

		for ( k = 0; k < count; k++ ) {
			float frac = (float)( k + 1 ) * inv;
			float f = e->curFreq + ( tFreq - e->curFreq ) * frac;
			float g = ( g0 + ( g1 - g0 ) * frac ) * ( e->curVol + ( tVol - e->curVol ) * frac );
			float p = S_Engine_Clamp( f / s->f0, ENGINE_MIN_PITCH, ENGINE_MAX_PITCH );
			int ip = (int)pos;
			float fr = (float)( pos - ip );
			int im1 = ( ip > 0 ) ? ip - 1 : len - 1;
			int i1 = ( ip + 1 < len ) ? ip + 1 : ip + 1 - len;
			int i2 = ( ip + 2 < len ) ? ip + 2 : ip + 2 - len;
			float xm1 = data[im1], x0 = data[ip], x1 = data[i1], x2 = data[i2];
			float c1 = 0.5f * ( x1 - xm1 );
			float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
			float c3 = 0.5f * ( x2 - xm1 ) + 1.5f * ( x0 - x1 );

			out[k] += g * ( ( ( c3 * fr + c2 ) * fr + c1 ) * fr + x0 );

			pos += p * rateScale;
			while ( pos >= len ) {
				pos -= len;
			}
		}

		e->phase[i] = pos;
		e->gain[i] = g1;
	}

	e->curFreq = tFreq;
	e->curVol = tVol;

	if ( alive <= 0.0f ) {
		// faded out completely during this block
		e->active = qfalse;
	}
}

/*
=================
S_Engine_Active
=================
*/
qboolean S_Engine_Active( void ) {
	int i;

	for ( i = 0; i < MAX_ENGINE_EMITTERS; i++ ) {
		if ( engineEmitters[i].active ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
S_Engine_StopAll
=================
*/
void S_Engine_StopAll( void ) {
	Com_Memset( engineEmitters, 0, sizeof( engineEmitters ) );
}

/*
=================
S_Engine_Update

Called by cgame every frame for every car that should be heard.
=================
*/
void S_Engine_Update( int entityNum, int handle, const engineSoundParams_t *params ) {
	engineEmitter_t *e, *freeSlot;
	int i, now;

	if ( handle <= 0 || handle > MAX_ENGINE_DEFS || !engineDefs[handle - 1].valid || !params ) {
		return;
	}

	now = ENG_Milliseconds();
	e = NULL;
	freeSlot = NULL;
	for ( i = 0; i < MAX_ENGINE_EMITTERS; i++ ) {
		if ( engineEmitters[i].active ) {
			if ( engineEmitters[i].entityNum == entityNum ) {
				e = &engineEmitters[i];
				break;
			}
		} else if ( !freeSlot ) {
			freeSlot = &engineEmitters[i];
		}
	}

	if ( e && e->def != handle - 1 ) {
		// car changed: restart the emitter
		Com_Memset( e, 0, sizeof( *e ) );
	} else if ( !e ) {
		if ( !freeSlot ) {
			return;
		}
		e = freeSlot;
		Com_Memset( e, 0, sizeof( *e ) );
	}

	e->active = qtrue;
	e->entityNum = entityNum;
	e->def = handle - 1;
	e->lastUpdate = now;
	e->params = *params;

	if ( ENGINE_DEBUG ) {
		static int nextPrint;
		const engineDef_t *def = &engineDefs[e->def];

		if ( now >= nextPrint ) {
			nextPrint = now + 250;
			ENG_Printf( "engine %i: rpm %4.0f n %.2f load %.2f gear %i | f %.1f Hz | sample %i (%.1f Hz) %.2f + sample %i (%.1f Hz) %.2f\n",
				entityNum, params->rpm, params->rpmFrac, params->load, params->gear, e->curFreq,
				e->dbgA, def->on[e->dbgA].f0, e->gain[e->dbgA],
				e->dbgB, def->on[e->dbgB].f0, e->gain[e->dbgB] );
		}
	}
}

/*
=================
S_Engine_RenderMono

Renders all emitters into a mono buffer, together with their spatialization.
Used by both backends.
=================
*/
static int S_Engine_RenderOne( engineEmitter_t *e, float *mono, int count, int outRate, int now ) {
	Com_Memset( mono, 0, count * sizeof( float ) );
	S_Engine_RenderEmitter( e, mono, count, outRate, now );
	return count;
}

/*
=================
S_Engine_RenderPCM16

Mixes all emitters, not spatialized, into 16 bit mono PCM (OpenAL stream).
=================
*/
void S_Engine_RenderPCM16( short *out, int count, int outRate ) {
	static float mix[ENGINE_MIX_CHUNK];
	static float mono[ENGINE_MIX_CHUNK];
	int i, k, done, n, now;
	float scale;

	now = ENG_Milliseconds();
	scale = 32767.0f * 0.5f * ENGINE_VOLUME;

	for ( done = 0; done < count; done += n ) {
		n = count - done;
		if ( n > ENGINE_MIX_CHUNK ) {
			n = ENGINE_MIX_CHUNK;
		}
		Com_Memset( mix, 0, n * sizeof( float ) );
		for ( i = 0; i < MAX_ENGINE_EMITTERS; i++ ) {
			if ( !engineEmitters[i].active ) {
				continue;
			}
			S_Engine_RenderOne( &engineEmitters[i], mono, n, outRate, now );
			for ( k = 0; k < n; k++ ) {
				mix[k] += mono[k];
			}
		}
		for ( k = 0; k < n; k++ ) {
			float v = mix[k] * scale;
			if ( v > 32767.0f ) {
				v = 32767.0f;
			} else if ( v < -32768.0f ) {
				v = -32768.0f;
			}
			out[done + k] = (short)v;
		}
	}
}

#ifndef SND_ENGINE_STANDALONE

/*
=================
S_Engine_PaintDMA

Adds all engine voices into the DMA paint buffer (same scale as S_PaintChannelFrom16).
=================
*/
void S_Engine_PaintDMA( portable_samplepair_t *paintbuffer, int count, int sndVol ) {
	static float mono[ENGINE_MIX_CHUNK];
	int i, k, done, n, now, leftvol, rightvol;
	float scaleL, scaleR;

	if ( count <= 0 ) {
		return;
	}

	now = ENG_Milliseconds();

	for ( i = 0; i < MAX_ENGINE_EMITTERS; i++ ) {
		engineEmitter_t *e = &engineEmitters[i];

		if ( !e->active ) {
			continue;
		}

		if ( e->params.flags & ENGINE_SOUND_LOCAL ) {
			leftvol = rightvol = 127;
		} else {
			S_SpatializeOrigin( e->params.origin, 127, &leftvol, &rightvol );
		}
		scaleL = 32767.0f * (float)( leftvol * sndVol ) / 256.0f * ENGINE_VOLUME;
		scaleR = 32767.0f * (float)( rightvol * sndVol ) / 256.0f * ENGINE_VOLUME;

		for ( done = 0; done < count && e->active; done += n ) {
			n = count - done;
			if ( n > ENGINE_MIX_CHUNK ) {
				n = ENGINE_MIX_CHUNK;
			}
			S_Engine_RenderOne( e, mono, n, dma.speed, now );
			for ( k = 0; k < n; k++ ) {
				paintbuffer[done + k].left += (int)( mono[k] * scaleL );
				paintbuffer[done + k].right += (int)( mono[k] * scaleR );
			}
		}
	}
}

/*
=================
S_Engine_LoadSample

Loads a sound file as mono float data.
=================
*/
static qboolean S_Engine_LoadSample( const char *path, float f0, engineSample_t *out ) {
	snd_info_t info;
	byte *data;
	float *mono;
	int i, c, frames;

	if ( f0 <= 0.0f ) {
		ENG_Printf( S_COLOR_YELLOW "WARNING: engine sound %s has no frequency\n", path );
		return qfalse;
	}

	data = S_CodecLoad( path, &info );
	if ( !data ) {
		return qfalse;
	}

	frames = info.samples;
	if ( frames < 4 || ( info.width != 1 && info.width != 2 ) || info.channels < 1 ) {
		Hunk_FreeTempMemory( data );
		return qfalse;
	}

	mono = ENG_Malloc( frames * sizeof( float ) );
	for ( i = 0; i < frames; i++ ) {
		float sum = 0.0f;
		for ( c = 0; c < info.channels; c++ ) {
			int idx = i * info.channels + c;
			if ( info.width == 2 ) {
				sum += ( (short *)data )[idx] / 32768.0f;
			} else {
				sum += ( (int)data[idx] - 128 ) / 128.0f;
			}
		}
		mono[i] = sum / info.channels;
	}
	Hunk_FreeTempMemory( data );

	out->data = mono;
	out->length = frames;
	out->rate = info.rate;
	out->f0 = f0;
	return qtrue;
}

/*
=================
S_Engine_FreeDef
=================
*/
static void S_Engine_FreeDef( engineDef_t *def ) {
	int i;

	for ( i = 0; i < def->numOn; i++ ) {
		if ( def->on[i].data ) {
			ENG_Free( def->on[i].data );
		}
	}
	Com_Memset( def, 0, sizeof( *def ) );
}

/*
=================
S_Engine_ParseConfig

engine.cfg:
	on      <file> <fundamental in Hz>
	curve   <f at rpmFrac 0> <f at rpmFrac 1>
	volume  idle <v> max <v> gamma <v> off <v>
Unknown keywords are skipped, so later phases can add lines.
=================
*/
static qboolean S_Engine_ParseConfig( engineDef_t *def, const char *dir ) {
	char *buffer;
	char *p, *lp, *token;
	char line[1024];
	char file[MAX_QPATH];
	long len;
	int n;

	len = FS_ReadFile( va( "%s/engine.cfg", dir ), (void **)&buffer );
	if ( len <= 0 || !buffer ) {
		return qfalse;
	}

	p = buffer;
	while ( *p ) {
		// copy one line, so tokens never run into the next line
		for ( n = 0; p[n] && p[n] != '\n' && n < (int)sizeof( line ) - 1; n++ ) {
			line[n] = p[n];
		}
		line[n] = 0;
		p += n;
		while ( *p && *p != '\n' ) {
			p++;
		}
		if ( *p == '\n' ) {
			p++;
		}

		lp = line;
		token = COM_ParseExt( &lp, qfalse );
		if ( !token[0] ) {
			continue;
		}

		if ( !Q_stricmp( token, "on" ) ) {
			float f0;

			token = COM_ParseExt( &lp, qfalse );
			Q_strncpyz( file, token, sizeof( file ) );
			f0 = atof( COM_ParseExt( &lp, qfalse ) );
			if ( def->numOn >= MAX_ENGINE_SAMPLES ) {
				ENG_Printf( S_COLOR_YELLOW "WARNING: %s/engine.cfg: too many samples\n", dir );
			} else if ( file[0] && S_Engine_LoadSample( va( "%s/%s", dir, file ), f0, &def->on[def->numOn] ) ) {
				def->numOn++;
			} else {
				ENG_Printf( S_COLOR_YELLOW "WARNING: %s/engine.cfg: can't load %s\n", dir, file );
			}
		} else if ( !Q_stricmp( token, "curve" ) ) {
			def->curveLo = atof( COM_ParseExt( &lp, qfalse ) );
			def->curveHi = atof( COM_ParseExt( &lp, qfalse ) );
		} else if ( !Q_stricmp( token, "volume" ) ) {
			while ( 1 ) {
				char key[32];

				token = COM_ParseExt( &lp, qfalse );
				if ( !token[0] ) {
					break;
				}
				Q_strncpyz( key, token, sizeof( key ) );
				token = COM_ParseExt( &lp, qfalse );
				if ( !Q_stricmp( key, "idle" ) ) {
					def->vIdle = atof( token );
				} else if ( !Q_stricmp( key, "max" ) ) {
					def->vMax = atof( token );
				} else if ( !Q_stricmp( key, "gamma" ) ) {
					def->gamma = atof( token );
				} else if ( !Q_stricmp( key, "off" ) ) {
					def->vOff = atof( token );
				}
			}
		}
		// other keywords are reserved for later phases (off, idle, limiter, shift, pops, turbo)
	}

	FS_FreeFile( buffer );
	return def->numOn > 0;
}

/*
=================
S_Engine_LoadLegacy

Stock engine0..engine10: one sample resampled in semitone steps.
=================
*/
static qboolean S_Engine_LoadLegacy( engineDef_t *def, const char *dir ) {
	int i;

	// stay silent for cars that simply have no engine sound of their own
	if ( FS_ReadFile( va( "%s/engine0.wav", dir ), NULL ) <= 0 &&
		FS_ReadFile( va( "%s/engine0.ogg", dir ), NULL ) <= 0 ) {
		return qfalse;
	}

	for ( i = 0; i < ENGINE_LEGACY_SAMPLES; i++ ) {
		float f0 = ENGINE_LEGACY_BASE_HZ * pow( 2.0, i / 12.0 );

		if ( S_Engine_LoadSample( va( "%s/engine%i.wav", dir, i ), f0, &def->on[def->numOn] ) ) {
			def->numOn++;
		}
	}
	return def->numOn > 0;
}

/*
=================
S_Engine_Register

dir is a sound directory like "sound/player/sidepipe".
Returns a handle > 0, or 0 if the directory has no engine sound.
=================
*/
int S_Engine_Register( const char *dir ) {
	engineDef_t *def;
	int i, slot;

	if ( !dir || !dir[0] ) {
		return 0;
	}

	slot = -1;
	for ( i = 0; i < MAX_ENGINE_DEFS; i++ ) {
		if ( engineDefs[i].valid ) {
			if ( !Q_stricmp( engineDefs[i].name, dir ) ) {
				return i + 1;
			}
		} else if ( slot < 0 ) {
			slot = i;
		}
	}
	if ( slot < 0 ) {
		ENG_Printf( S_COLOR_YELLOW "WARNING: S_Engine_Register: too many engine sounds\n" );
		return 0;
	}

	def = &engineDefs[slot];
	Com_Memset( def, 0, sizeof( *def ) );
	Q_strncpyz( def->name, dir, sizeof( def->name ) );
	S_Engine_SetDefaults( def );

	if ( !S_Engine_ParseConfig( def, dir ) ) {
		S_Engine_FreeDef( def );
		Q_strncpyz( def->name, dir, sizeof( def->name ) );
		S_Engine_SetDefaults( def );
		if ( !S_Engine_LoadLegacy( def, dir ) ) {
			S_Engine_FreeDef( def );
			return 0;
		}
	}

	if ( !S_Engine_FinishDef( def ) ) {
		S_Engine_FreeDef( def );
		return 0;
	}

	if ( ENGINE_DEBUG ) {
		ENG_Printf( "engine sound %s: %i samples, %.1f-%.1f Hz\n", dir, def->numOn, def->curveLo, def->curveHi );
	}
	return slot + 1;
}

/*
=================
S_Engine_Init / S_Engine_Shutdown
=================
*/
void S_Engine_Init( void ) {
	s_engineVolume = Cvar_Get( "s_engineVolume", "1.0", CVAR_ARCHIVE );
	s_engineDebug = Cvar_Get( "s_engineDebug", "0", CVAR_TEMP );
}

void S_Engine_Shutdown( void ) {
	int i;

	S_Engine_StopAll();
	for ( i = 0; i < MAX_ENGINE_DEFS; i++ ) {
		if ( engineDefs[i].valid || engineDefs[i].numOn ) {
			S_Engine_FreeDef( &engineDefs[i] );
		}
	}
}

#endif // SND_ENGINE_STANDALONE
