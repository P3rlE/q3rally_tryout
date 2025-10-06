#ifndef Q_SHARED_PLATES_H
#define Q_SHARED_PLATES_H

#include "q_shared.h"

#if defined( CGAME ) || defined( UI )
qboolean Q3R_IsGeneratedPlateShaderName( const char *shaderName );
qhandle_t Q3R_RegisterGeneratedPlateShader( const char *shaderName );
#endif

qboolean Q3R_CreateLicensePlateImage( const char *templateImage,
                                      const char *outputImage,
                                      const char *name,
                                      int maxChars );

qhandle_t Q3R_RegisterGeneratedPlateShader( const char *shaderName );

#endif /* Q_SHARED_PLATES_H */
