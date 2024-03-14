#ifndef _CAMERA_H_
#define _CAMERA_H_

#include "algebra.h"

typedef struct Camera {
    V3 pos;
    V3 target;
    V3 up;

    b32 update;
    f32 yaw;
    f32 pitch;
} Camera;

void camera_initialize(Camera *camera, V3 pos, V3 target, V3 up);
void camera_update(Camera *camera, f32 dt);

#endif // _CAMERA_H_
