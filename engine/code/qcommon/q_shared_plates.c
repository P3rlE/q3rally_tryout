/*
============================================================================
Shared license plate utilities for cgame and UI modules.
============================================================================
*/

#include "q_shared_plates.h"

#if defined( CGAME )
#include "../cgame/cg_local.h"
#elif defined( UI )
#if defined( MISSIONPACK )
#include "../ui/ui_local.h"
#else
#include "../q3_ui/ui_local.h"
#endif
#else
#include "q_shared.h"
#endif

#define PLATE_POOLSIZE   (512 * 1024)

static byte    plateMemoryPool[PLATE_POOLSIZE];
static int     plateAllocPoint = 0;

static void Q3R_PlateResetPool( void ) {
    plateAllocPoint = 0;
}

static void *Q3R_PlateAlloc( int size ) {
    void *p;

    if ( plateAllocPoint + size > PLATE_POOLSIZE ) {
        Com_Printf( S_COLOR_YELLOW "Q3R Warning: plate generator out of memory (%i bytes needed).\n", size );
        return NULL;
    }

    p = &plateMemoryPool[plateAllocPoint];
    plateAllocPoint += ( size + 31 ) & ~31;

    return p;
}

typedef struct {
    byte   *imageData;
    int     bpp;
    int     width;
    int     height;
} plateTexture_t;

static qboolean Q3R_LoadTGA( plateTexture_t *texture, const char *filename ) {
    static const byte TGAheader[12] = { 0,0,2,0,0,0,0,0,0,0,0,0 };
    byte TGAcompare[12];
    byte header[6];
    int bytesPerPixel;
    int imageSize;
    fileHandle_t imageFile;

    trap_FS_FOpenFile( filename, &imageFile, FS_READ );
    if ( !imageFile ) {
        Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not open %s for license plate.\n", filename );
        return qfalse;
    }

    trap_FS_Read( TGAcompare, sizeof( TGAcompare ), imageFile );
    if ( memcmp( TGAheader, TGAcompare, sizeof( TGAheader ) ) != 0 ) {
        trap_FS_FCloseFile( imageFile );
        if ( TGAcompare[2] == 10 ) {
            Com_Printf( S_COLOR_YELLOW "Q3R Warning: Cannot load %s, Run-Length Encoded TGAs are unsupported.\n", filename );
        } else {
            Com_Printf( S_COLOR_YELLOW "Q3R Warning: Header of %s does not match known header format.\n", filename );
        }
        return qfalse;
    }

    trap_FS_Read( header, sizeof( header ), imageFile );
    texture->width = header[1] * 256 + header[0];
    texture->height = header[3] * 256 + header[2];

    if ( texture->width <= 0 || texture->height <= 0 || ( header[4] != 24 && header[4] != 32 ) ) {
        trap_FS_FCloseFile( imageFile );
        Com_Printf( S_COLOR_YELLOW "Q3R Warning: %s has invalid dimensions or bpps.\n", filename );
        return qfalse;
    }

    texture->bpp = header[4];
    bytesPerPixel = texture->bpp / 8;
    imageSize = texture->width * texture->height * bytesPerPixel;

    texture->imageData = (byte *)Q3R_PlateAlloc( imageSize );
    if ( !texture->imageData ) {
        trap_FS_FCloseFile( imageFile );
        Com_Printf( S_COLOR_YELLOW "Q3R Warning: Not enough memory to load %s.\n", filename );
        return qfalse;
    }

    trap_FS_Read( texture->imageData, imageSize, imageFile );
    trap_FS_FCloseFile( imageFile );

    return qtrue;
}

static qboolean Q3R_WriteNameOnTexture( plateTexture_t *texture, const char *name, int maxChars ) {
    vec4_t color;
    const char *s;
    unsigned char ch;
    float ax, ay, aw, ah;
    float frow, fcol, fwidth, fheight;
    int i, j;
    int bytesPerPixelF, bytesPerPixelT;
    float a;
    int t, f, len, cnt;
    int texMaxBytes;
    int fontMaxIndex;
    plateTexture_t font;

    if ( !Q3R_LoadTGA( &font, "gfx/2d/bigchars_plates.tga" ) ) {
        return qfalse;
    }

    bytesPerPixelF = font.bpp / 8;
    bytesPerPixelT = texture->bpp / 8;

    if ( bytesPerPixelF < 4 ) {
        Com_Printf( S_COLOR_YELLOW "Q3R Warning: Font texture for plates must have alpha channel.\n" );
        return qfalse;
    }

    len = (int)( texture->width / SMALLCHAR_WIDTH ) - 1;
    if ( len < maxChars ) {
        maxChars = len;
    }

    len = (int)strlen( name );
    if ( len > maxChars ) {
        len = maxChars;
    }

    ax = (float)(( texture->width / 2.0f ) - ( len * ( SMALLCHAR_WIDTH + 1 ) / 2.0f ) - 3 );
    if ( ax < -3 ) {
        ax = -3;
    }
    ay = 11.0f;
    ah = SMALLCHAR_HEIGHT;

    color[0] = 0.0f;
    color[1] = 0.0f;
    color[2] = 0.0f;
    color[3] = 1.0f;

    s = name;
    cnt = 0;
    texMaxBytes = texture->width * texture->height * bytesPerPixelT;
    fontMaxIndex = font.width * font.height - 1;
    while ( *s && cnt < maxChars ) {
        if ( Q_IsColorString( s ) ) {
            memcpy( color, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( color ) );
            s += 2;
            continue;
        }

        ch = *s & 127;

        if ( ch == ' ' ) {
            aw = SMALLCHAR_WIDTH + 8.0f;
        } else if ( ch >= 32 && ch < 127 ) {
            frow = (float)( ( ch >> 4 ) * 16 );
            fcol = (float)( ( ch & 15 ) * 16 );
            fwidth = 16.0f;
            fheight = 16.0f;
            aw = SMALLCHAR_WIDTH + 8.0f;

            for ( i = 0; i < (int)ah; i++ ) {
                int texRow = (int)( texture->height - ( ay + ah - i ) );
                if ( texRow < 0 || texRow >= texture->height ) {
                    continue;
                }

                t = ( texRow * texture->width + (int)ax ) * bytesPerPixelT;
                f = (int)( font.height - ( frow + fheight - fheight * ( i / ah ) ) ) * font.width;

                for ( j = 0; j < (int)aw; j++ ) {
                    int fontColumn = (int)( fcol + fwidth * ( j / aw ) );
                    int fontIndex = f + fontColumn;

                    if ( fontIndex < 0 || fontIndex > fontMaxIndex ) {
                        continue;
                    }

                    a = font.imageData[fontIndex * bytesPerPixelF + 3];

                    if ( t >= 0 && t + 2 < texMaxBytes ) {
                        texture->imageData[t]   = (byte)( texture->imageData[t]   * ( 1.0f - ( a / 255.0f ) ) + color[2] * a );
                        texture->imageData[t+1] = (byte)( texture->imageData[t+1] * ( 1.0f - ( a / 255.0f ) ) + color[1] * a );
                        texture->imageData[t+2] = (byte)( texture->imageData[t+2] * ( 1.0f - ( a / 255.0f ) ) + color[0] * a );
                    }

                    t += bytesPerPixelT;
                }
            }
        } else {
            aw = 0.0f;
        }

        ax += aw - 7.0f;
        cnt++;
        s++;
    }

    return qtrue;
}

static qboolean Q3R_SaveTGA( plateTexture_t *texture, const char *filename ) {
    byte TGAheader[12] = { 0,0,2,0,0,0,0,0,0,0,0,0 };
    byte header[6];
    int bytesPerPixel;
    int imageSize;
    fileHandle_t imageFile;

    trap_FS_FOpenFile( filename, &imageFile, FS_WRITE );
    if ( !imageFile ) {
        Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not open %s for texture output.\n", filename );
        return qfalse;
    }

    bytesPerPixel = texture->bpp / 8;
    imageSize = texture->width * texture->height * bytesPerPixel;

    header[0] = texture->width % 256;
    header[1] = texture->width / 256;
    header[2] = texture->height % 256;
    header[3] = texture->height / 256;
    header[4] = texture->bpp;
    header[5] = 0;

    trap_FS_Write( TGAheader, sizeof( TGAheader ), imageFile );
    trap_FS_Write( header, sizeof( header ), imageFile );
    trap_FS_Write( texture->imageData, imageSize, imageFile );
    trap_FS_FCloseFile( imageFile );

    return qtrue;
}

qhandle_t Q3R_RegisterGeneratedPlateShader( const char *shaderName ) {
#if defined( CGAME ) || defined( UI )
    if ( !shaderName || !*shaderName ) {
        return 0;
    }

    return trap_R_RegisterShaderLightMap( shaderName, -1 );
#else
    (void)shaderName;
    return 0;
#endif
}

qboolean Q3R_CreateLicensePlateImage( const char *templateImage,
                                      const char *outputImage,
                                      const char *name,
                                      int maxChars ) {
    plateTexture_t tga;

    Q3R_PlateResetPool();

    if ( !Q3R_LoadTGA( &tga, templateImage ) ) {
        return qfalse;
    }
    if ( !Q3R_WriteNameOnTexture( &tga, name, maxChars ) ) {
        return qfalse;
    }
    if ( !Q3R_SaveTGA( &tga, outputImage ) ) {
        return qfalse;
    }

    return qtrue;
}

