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
        float x, y, wheelAngle, x2, y2, w2, h2;
        score_t *score;
        clientInfo_t *ci;
        centity_t *cent;
        refdef_t refdef;
        refEntity_t body;

        score = &cg.scores[i];
        ci = &cgs.clientinfo[ score->client ];
        cent = &cg_entities[ score->client ];

        if ( !ci->infoValid ) {
            continue;
        }

        x = baseX + i * (size + spacing);
        y = 100.0f;

        x2 = x;
        y2 = y;
        w2 = size;
        h2 = size;
        CG_AdjustFrom640( &x2, &y2, &w2, &h2 );

        memset( &refdef, 0, sizeof( refdef ) );
        refdef.rdflags = RDF_NOWORLDMODEL;
        AxisClear( refdef.viewaxis );
        refdef.fov_x = 30;
        refdef.fov_y = 30;
        refdef.x = x2;
        refdef.y = y2;
        refdef.width = w2;
        refdef.height = h2;
        refdef.time = cg.time;

        memset( &body, 0, sizeof( body ) );
        body.hModel = ci->bodyModel;
        body.customSkin = ci->bodySkin;

        if ( body.hModel ) {
            VectorCopy( origin, body.origin );
            AnglesToAxis( angles, body.axis );
            body.renderfx = RF_NOSHADOW;

            if ( ci->controlMode == CT_MOUSE ) {
                wheelAngle = WheelAngle( cent->currentState.apos.trBase[YAW],
                                        cent->currentState.angles2[YAW] );
            } else {
                wheelAngle = cent->currentState.angles2[YAW];
            }

            trap_R_ClearScene();
            CG_AddRefEntityWithPowerups( &body, &cent->currentState, ci->team );
            CG_AddWheels( cent, &body, wheelAngle );
            trap_R_RenderScene( &refdef );
        } else {
            qhandle_t icon;

            icon = ci->modelIcon;
            if ( !icon ) {
                icon = cgs.media.deferShader;
            }

            if ( icon ) {
                CG_DrawPic( x, y, size, size, icon );
            }
        }

        CG_DrawBigStringColor( x, y + size + 10, va( "%i. %s", i + 1, ci->name ), colorWhite );
    }
}

