#include "cg_local.h"
#include <assert.h>
#include <string.h>

#define SPLASH_RADIUS_SCALE 16.0f
#define MAX_SPLASH_RADIUS 14.0f

cg_t cg;
snapshot_t snap;
centity_t cg_entities[MAX_GENTITIES];

qboolean CG_FrictionCalc( const carPoint_t *point, float *sCOF, float *kCOF ) {
    centity_t   *cent;
    entityState_t *es;
    float       radius;
    int         i;

    for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
        es = &cg.snap->entities[ i ];

        if ( es->eType != ET_EVENTS + EV_HAZARD ) {
            continue;
        }
        if ( es->weapon != HT_OIL ) {
            continue;
        }

        cent = &cg_entities[ es->number ];

        radius = ( es->eventParm * SPLASH_RADIUS_SCALE ) + point->radius;
        radius *= radius;
        if ( DistanceSquared( cent->lerpOrigin, point->r ) > radius ) {
            continue;
        }

        *sCOF = CP_OIL_SCOF;
        *kCOF = CP_OIL_KCOF;

        return qtrue;
    }

    return qfalse;
}

static void test_no_hazard(void) {
    carPoint_t point;
    memset(&point, 0, sizeof(point));
    point.radius = 1.0f;
    point.r[0] = point.r[1] = point.r[2] = 0.0f;

    float sCOF = 0.5f;
    float kCOF = 0.4f;

    memset(&cg, 0, sizeof(cg));
    cg.snap = &snap;
    memset(&snap, 0, sizeof(snap));
    qboolean result = CG_FrictionCalc(&point, &sCOF, &kCOF);
    assert(result == qfalse);
    assert(sCOF == 0.5f);
    assert(kCOF == 0.4f);
}

static void test_oil_hazard(void) {
    carPoint_t point;
    memset(&point, 0, sizeof(point));
    point.radius = 1.0f;
    point.r[0] = point.r[1] = point.r[2] = 0.0f;

    float sCOF = 0.5f;
    float kCOF = 0.4f;

    memset(&cg_entities[0], 0, sizeof(cg_entities[0]));
    cg_entities[0].currentState.eType = ET_EVENTS + EV_HAZARD;
    cg_entities[0].currentState.weapon = HT_OIL;
    cg_entities[0].currentState.eventParm = 1;
    cg_entities[0].lerpOrigin[0] = cg_entities[0].lerpOrigin[1] = cg_entities[0].lerpOrigin[2] = 0.0f;

    memset(&cg, 0, sizeof(cg));
    cg.snap = &snap;
    memset(&snap, 0, sizeof(snap));
    snap.numEntities = 1;
    snap.entities[0].eType = ET_EVENTS + EV_HAZARD;
    snap.entities[0].weapon = HT_OIL;
    snap.entities[0].eventParm = 1;
    snap.entities[0].number = 0;

    qboolean result = CG_FrictionCalc(&point, &sCOF, &kCOF);
    assert(result == qtrue);
    assert(sCOF == CP_OIL_SCOF);
    assert(kCOF == CP_OIL_KCOF);
}

int main(void) {
    test_no_hazard();
    test_oil_hazard();
    return 0;
}
