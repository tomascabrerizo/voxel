#include <glad/glad.h>

#include "common.h"
#include "os.h"
#include "gpu.h"
#include "voxel.h"

typedef struct Camera {
    V3 pos;
    V3 target;
    V3 up;

    b32 update;
    f32 yaw;
    f32 pitch;
} Camera;

static Camera create_camera(V3 pos, V3 target, V3 up) {
    Camera camera = {
        .pos    = pos,
        .target = target,
        .up     = up,
        .update = false,
        .yaw    = -90.0f,
        .pitch  = 0,
    };
    return camera;
}

static void update_camera(Camera *camera, f32 dt) {

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

int main(void) {

    u32 w = 1920 / 2;
    u32 h = 1080 / 2;
    os_window_initialize(w, h);

    job_system_initialize();

    voxel_block_map_initialize();
    chunks_initialize();

    u32 program        = gpu_load_program("res/shaders/cube.vs", "res/shaders/cube.fs");
    SDL_Surface *atlas = SDL_LoadBMP("res/texture.bmp");
    u32 texture        = gpu_load_texture(atlas->pixels, atlas->w, atlas->h);

    glUseProgram(program);

    // NOTE: Setup perspective projection
    f32 aspect = (f32)w / (f32)h;
    M4 proj    = m4_perspective2(to_rad(80), aspect, 0.1f, 1000.0f);
    gpu_load_m4_uniform(program, "proj", proj);

    // NOTE: Create camera
    Camera camera = create_camera(
        v3(VOXEL_DIM * CHUNK_X * MAX_CHUNKS_X / 2, 70, -VOXEL_DIM * CHUNK_Z * MAX_CHUNKS_Y / 2),
        v3(0, 0, -1), v3(0, 1, 0));

    u32 fps           = 60;
    f32 expected_time = 1.0f / (f32)fps;
    unused(expected_time);
    f64 last_time = SDL_GetTicks() / 1000.0;
    while(!os_window_should_close()) {

        f64 current_time = SDL_GetTicks() / 1000.0f;
        f32 dt           = current_time - last_time;
        last_time        = current_time;

        //printf("ms:%f\n", dt);

        os_process_input();

        update_camera(&camera, dt);

        s32 current_chunk_x = (s32)(camera.pos.x / CHUNK_X);
        s32 current_chunk_z = (s32)(camera.pos.z / CHUNK_Z);

        job_queue_begin();

        for(s32 x = current_chunk_x - MAX_CHUNKS_X / 2; x <= current_chunk_x + MAX_CHUNKS_X / 2;
            ++x) {
            for(s32 z = current_chunk_z - MAX_CHUNKS_Y / 2; z <= current_chunk_z + MAX_CHUNKS_Y / 2;
                ++z) {
                if(!chunk_is_loaded(x, z)) {
                    // NOTE: load chunk into linklist
                    load_chunk(x, z);
                }
            }
        }

        job_queue_end();

        glClearColor(0.3f, 0.65f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glBindTexture(GL_TEXTURE_2D, texture);

        // NOTE: Update camera position
        M4 view = m4_lookat2(camera.pos, v3_add(camera.pos, camera.target), camera.up);
        gpu_load_m4_uniform(program, "view", view);

        u32 total_chunk_count     = 0;
        u32 total_geometry_memory = 0;
        unused(total_chunk_count);
        unused(total_geometry_memory);

        Chunk *chunk = loaded_chunk_list_get_first();
        while(!loaded_chunk_list_is_end(chunk)) {

            total_geometry_memory += chunk->geometry_count;
            ++total_chunk_count;

            if(chunk->just_loaded) {
                glBindBuffer(GL_ARRAY_BUFFER, chunk->vao);
                // glBufferSubData(GL_ARRAY_BUFFER, 0, chunk->geometry_count * sizeof(Vertex),
                // chunk->geometry);
                glBufferData(GL_ARRAY_BUFFER, chunk->geometry_count * sizeof(Vertex),
                             chunk->geometry, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                chunk->just_loaded = false;
            }

            if(chunk->x >= current_chunk_x - MAX_CHUNKS_X / 2 &&
               chunk->x <= current_chunk_x + MAX_CHUNKS_X / 2 &&
               chunk->z >= current_chunk_z - MAX_CHUNKS_Y / 2 &&
               chunk->z <= current_chunk_z + MAX_CHUNKS_Y / 2) {

                // NOTE: Setup model matrix
                f32 pos_x = chunk->x * VOXEL_DIM * CHUNK_X;
                f32 pos_z = chunk->z * VOXEL_DIM * CHUNK_Z;
                M4 model  = m4_translate(v3(pos_x, 0, pos_z));
                gpu_load_m4_uniform(program, "model", model);

                glBindVertexArray(chunk->vao);
                glDrawArrays(GL_TRIANGLES, 0, chunk->geometry_count);
            }
            chunk = chunk->next;
        }
#if 0
        f32 avrg_geometry_count = (f32)total_geometry_memory / (f32)total_chunk_count;
        printf("avrg geometry memory usage:%.3f MB\n",
               (avrg_geometry_count * sizeof(Vertex)) / (1024.0f * 1024.0f));
#endif
        os_swap_window();
    }

    job_system_terminate();

    os_window_terminate();

    return 0;
}
