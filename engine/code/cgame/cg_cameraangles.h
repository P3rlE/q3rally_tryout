// cg_cameraangles.h

#ifndef __CG_CAMERAANGLES_H__
#define __CG_CAMERAANGLES_H__

// Represents a single camera angle
typedef struct {
    float angle;
    float range;
    float height;
} cameraAngle_t;

// Function to get the current camera angle
const cameraAngle_t* CG_GetCameraAngle(void);

// Function to cycle to the next camera angle
void CG_NextCameraAngle_f(void);

#endif // __CG_CAMERAANGLES_H__
