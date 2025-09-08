#include "cg_local.h"
#include <assert.h>
#include <string.h>

#define SPLASH_RADIUS_SCALE 16.0f
#define MAX_SPLASH_RADIUS 14.0f

static int stubEntityList[MAX_GENTITIES];
static int stubEntityCount;

centity_t cg_entities[MAX_GENTITIES];

int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount) {
    int count = stubEntityCount < maxcount ? stubEntityCount : maxcount;
    for (int i = 0; i < count; ++i) {
        entityList[i] = stubEntityList[i];
    }
    return count;
}

qboolean CG_FrictionCalc( const carPoint_t *point, float *sCOF, float *kCOF ) {
    centity_t   *cent;
    int         entityList[MAX_GENTITIES];
    int         numListedEntities;
    vec3_t      mins, maxs;
    float       radius;
    int         i;

    radius = point->radius + SPLASH_RADIUS_SCALE * MAX_SPLASH_RADIUS;

    for ( i = 0 ; i < 3 ; i++ ) {
        mins[i] = point->r[i] - radius;
        maxs[i] = point->r[i] + radius;
    }

    numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

    for ( i = 0 ; i < numListedEntities ; i++ ) {
        cent = &cg_entities[entityList[ i ]];

        if( cent->currentState.eType != ET_EVENTS + EV_HAZARD ) continue;
        if( cent->currentState.weapon != HT_OIL ) continue;

        radius = ( cent->currentState.eventParm * SPLASH_RADIUS_SCALE ) + point->radius;
        radius *= radius;
        if( DistanceSquared( cent->lerpOrigin, point->r ) > radius ) continue;

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

    stubEntityCount = 0;
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

    stubEntityList[0] = 0;
    stubEntityCount = 1;

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
