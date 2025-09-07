/*
=======================================================================
  cg_parcferme.c -- rendering for parc ferme podium
=======================================================================
*/

#include "cg_local.h"
#include "cg_parcferme.h"

/*
=================
CG_DrawParcFerme
Draw top three vehicles and player names during intermission
=================
*/
void CG_DrawParcFerme( void ) {
    int i;
    vec3_t origin = { 0, 0, 0 };
    vec3_t angles = { 0, 0, 0 };
    float size = 120.0f;
    float spacing = 20.0f;
    float baseX;

    CG_SetScreenPlacement( PLACE_CENTER, PLACE_TOP );

    baseX = 320 - (3 * size + 2 * spacing) / 2;

    for ( i = 0; i < 3 && i < cg.numScores; i++ ) {
        score_t *score = &cg.scores[i];
        clientInfo_t *ci = &cgs.clientinfo[ score->client ];
        float x;
        float y;

        if ( !ci->infoValid ) {
            continue;
        }

        x = baseX + i * (size + spacing);
        y = 100.0f;

        CG_Draw3DModel( x, y, size, size, ci->bodyModel, ci->bodySkin, origin, angles );
        CG_DrawBigStringColor( x, y + size + 10, va( "%i. %s", i + 1, ci->name ), colorWhite );
    }
}

