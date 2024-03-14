#include "camera.h"
#include "os.h"

void camera_initialize(Camera *camera, V3 pos, V3 target, V3 up) {
    Camera init = {
        .pos    = pos,
        .target = target,
        .up     = up,
        .update = false,
        .yaw    = -90.0f,
        .pitch  = 0,
    };
    *camera = init;
}

void camera_update(Camera *camera, f32 dt) {

    if(os_button_just_down(SDL_BUTTON_LEFT)) {
        SDL_SetRelativeMouseMode(true);
        camera->update = true;
    }

    if(os_button_just_up(SDL_BUTTON_LEFT)) {
        SDL_SetRelativeMouseMode(false);
        camera->update = false;
    }

    if(camera->update) {

        s32 rel_mouse_x, rel_mouse_y;
        os_mouse_rel_pos(&rel_mouse_x, &rel_mouse_y);
        f32 sensitivity = 0.1f;

        camera->yaw += rel_mouse_x * sensitivity;
        camera->pitch += rel_mouse_y * sensitivity;

        if(camera->pitch > 89.0f)
            camera->pitch = 89.0f;
        if(camera->pitch < -89.0f)
            camera->pitch = -89.0f;

        V3 dir;
        dir.x          = cosf(to_rad(camera->yaw)) * cosf(to_rad(camera->pitch));
        dir.y          = sinf(to_rad(camera->pitch));
        dir.z          = sinf(to_rad(camera->yaw)) * cosf(to_rad(camera->pitch));
        camera->target = v3_normalize(dir);
    }

    // NOTE update camera position
    f32 speed = 20.0f * dt;
    V3 front  = camera->target;
    V3 right  = v3_normalize(v3_cross(v3(0, 1, 0), front));
    if(os_key_down(SDL_SCANCODE_A)) {
        camera->pos = v3_add(camera->pos, v3_scale(right, speed));
    }
    if(os_key_down(SDL_SCANCODE_D)) {
        camera->pos = v3_sub(camera->pos, v3_scale(right, speed));
    }
    if(os_key_down(SDL_SCANCODE_W)) {
        camera->pos = v3_add(camera->pos, v3_scale(front, speed));
    }
    if(os_key_down(SDL_SCANCODE_S)) {
        camera->pos = v3_sub(camera->pos, v3_scale(front, speed));
    }
    if(os_key_down(SDL_SCANCODE_LSHIFT)) {
        camera->pos = v3_add(camera->pos, v3_scale(v3(0, 1, 0), speed));
    }
    if(os_key_down(SDL_SCANCODE_LCTRL)) {
        camera->pos = v3_sub(camera->pos, v3_scale(v3(0, 1, 0), speed));
    }
}
