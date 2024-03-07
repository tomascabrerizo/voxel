#include <glad/glad.h>

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

#include "common.h"
#include "os.h"
#include "gpu.h"

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

static s32 random(s32 min, s32 max) {
    f32 scale  = (f32)rand() / (f32)RAND_MAX;
    s32 result = min + (s32)(scale * (f32)(max - min));
    // assert(result >= min && result < max);
    return result;
}

#define ATLAS_W 512
#define ATLAS_H 512
#define TILE_DIM 32
#define ATLAS_ROWS (ATLAS_H / TILE_DIM)
#define ATLAS_COLS (ATLAS_W / TILE_DIM)

typedef struct R2 {
    V2 min, max;
} R2;

static inline R2 get_tile(u32 x, u32 y) {
    R2 result;
    result.min.x = (f32)(x * TILE_DIM) / (f32)ATLAS_W;
    result.min.y = (f32)(y * TILE_DIM) / (f32)ATLAS_H;
    result.max.x = (f32)(x * TILE_DIM + TILE_DIM) / (f32)ATLAS_W;
    result.max.y = (f32)(y * TILE_DIM + TILE_DIM) / (f32)ATLAS_H;
    // TODO: SDL2 dont have a flip surface function, load image with stb instead of fliping
    // coordinates
    f32 tmp      = result.min.y;
    result.min.y = result.max.y;
    result.max.y = tmp;

    return result;
}

typedef enum VoxelType {
    VOXEL_AIR,
    VOXEL_DIRT,
    VOXEL_STONE,
    VOXEL_GRASS,

    VOXEL_TYPE_COUNT,
} VoxelType;

typedef enum VoxelBockFace {
    VOXEL_BLOCK_BACK,
    VOXEL_BLOCK_FRONT,
    VOXEL_BLOCK_RIGHT,
    VOXEL_BLOCK_LEFT,
    VOXEL_BLOCK_TOP,
    VOXEL_BLOCK_BOTTOM,

    VOXEL_BLOCK_FACE_COUNT,
} VoxelBlockFace;

typedef struct VoxelBlock {
    R2 coords[VOXEL_BLOCK_FACE_COUNT];
} VoxelBlock;

static VoxelBlock voxel_block_map[VOXEL_TYPE_COUNT];

static inline void voxel_block_map_initialize(void) {
    memset(voxel_block_map, 0, sizeof(voxel_block_map));

    // NOTE: Set up dirt coordinates
    for(u32 i = 0; i < VOXEL_BLOCK_FACE_COUNT; ++i) {
        voxel_block_map[VOXEL_DIRT].coords[i] = get_tile(0, 0);
    }

    // NOTE: Set up stone coordinates
    for(u32 i = 0; i < VOXEL_BLOCK_FACE_COUNT; ++i) {
        voxel_block_map[VOXEL_STONE].coords[i] = get_tile(3, 0);
    }

    // NOTE: Set up grass coordinates
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_BACK]   = get_tile(1, 0);
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_FRONT]  = get_tile(1, 0);
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_LEFT]   = get_tile(1, 0);
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_RIGHT]  = get_tile(1, 0);
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_TOP]    = get_tile(2, 0);
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_BOTTOM] = get_tile(0, 0);
}

typedef struct Voxel {
    u8 type;
    u32 x, y, z;
} Voxel;

#define CHUNK_X 16
#define CHUNK_Y 256
#define CHUNK_Z 16
#define CHUNK_TOTAL_SIZE (CHUNK_X * CHUNK_Y * CHUNK_Z)

#define VOXEL_DIM 1.0f

typedef struct Chunk {
    s32 x, z;
    Voxel voxels[CHUNK_TOTAL_SIZE];
    Vertex *geometry;
    u32 geometry_count;

    struct Chunk *next;
    struct Chunk *prev;

    u32 vao;

} Chunk;

Chunk free_chunks;
Chunk loaded_chunks;

#define MAX_CHUNKS_X (8 * 1)
#define MAX_CHUNKS_Y (8 * 1) // TODO: This should be call MAX_CHUNKS_Z
#define MAX_CHUNK_GEOMETRY_SIZE (5 * 1024 * 1024)

static void chunks_initialize(void) {

    list_init(&loaded_chunks);
    list_init(&free_chunks);

    u32 total_chunk_count = (u32)((MAX_CHUNKS_X * MAX_CHUNKS_Y) * 2);
    Chunk *chunks         = (Chunk *)malloc(sizeof(Chunk) * total_chunk_count);
    for(u32 chunk_id = 0; chunk_id < total_chunk_count; ++chunk_id) {
        Chunk *chunk = &chunks[chunk_id];
        // NOTE: Insert chunk in the free list
        list_insert_back(&free_chunks, chunk);
        // NOTE: Allocate geometry memory on CPU
        chunk->geometry = (Vertex *)malloc(MAX_CHUNK_GEOMETRY_SIZE);
        // NOTE: Allocate geometry memory on GPU
        chunk->vao            = gpu_load_buffer(NULL, MAX_CHUNK_GEOMETRY_SIZE);
        chunk->geometry_count = 0;
    }

    printf("Total chunk memory:%.3lf MB\n",
           (f64)(sizeof(Chunk) * total_chunk_count / (1024.0 * 1024.0)));
    printf("Total chunk geometry memory:%.3lf MB\n",
           (f64)(MAX_CHUNK_GEOMETRY_SIZE * total_chunk_count / (1024.0 * 1024.0)));
}

static inline void add_vertex(Chunk *chunk, V3 pos, V3 normal, V2 uvs) {
    assert((chunk->geometry_count * sizeof(Vertex)) < MAX_CHUNK_GEOMETRY_SIZE);
    chunk->geometry[chunk->geometry_count++] = (Vertex){ pos, normal, uvs };
}

static inline Voxel *get_chunk_voxel(Chunk *chunk, u32 x, u32 y, u32 z) {
    if((x < 0) || (x >= CHUNK_X))
        return NULL;
    if((y < 0) || (y >= CHUNK_Y))
        return NULL;
    if((z < 0) || (z >= CHUNK_Z))
        return NULL;

    return &chunk->voxels[z * (CHUNK_Y * CHUNK_X) + y * (CHUNK_X) + x];
}

void generate_chunk_voxels(Chunk *chunk) {
    unused(random);

    for(s32 x = 0; x < CHUNK_X; ++x) {
        for(s32 z = 0; z < CHUNK_Z; ++z) {

            f32 abs_x = (chunk->x * CHUNK_X + x) / ((f32)CHUNK_X * 2.0f);
            f32 abs_z = (chunk->z * CHUNK_Z + z) / ((f32)CHUNK_Z * 2.0f);
            f32 noise = (stb_perlin_noise3(abs_x, 0, abs_z, 0, 0, 0) + 1) / 2.0f;
            f32 h     = 20 + noise * (u32)((CHUNK_Y / 2) - 50);

            assert(h >= 0 && h < CHUNK_Y);

            for(s32 y = 0; y < CHUNK_Y; ++y) {

                Voxel *voxel = get_chunk_voxel(chunk, x, y, z);
                voxel->x     = x;
                voxel->y     = y;
                voxel->z     = z;

                if(y < (h * 0.7f)) {
                    voxel->type = VOXEL_STONE;
                } else if(y < h) {
                    if((f32)y < ((f32)CHUNK_Y / 2) - ((f32)CHUNK_Y / 3.4f)) {
                        voxel->type = VOXEL_STONE;
                    } else if(y < (s32)((((f32)CHUNK_Y / 2) - ((f32)CHUNK_Y / 6)) * noise)) {
                        voxel->type = VOXEL_STONE;

                    } else {
                        voxel->type = VOXEL_DIRT;
                    }
                } else {
                    voxel->type = VOXEL_AIR;
                }
            }
        }
    }

    for(u32 x = 0; x < CHUNK_X; ++x) {
        for(u32 y = 0; y < CHUNK_Y; ++y) {
            for(u32 z = 0; z < CHUNK_Z; ++z) {
                Voxel *voxel = get_chunk_voxel(chunk, x, y, z);
                if(voxel->type != VOXEL_DIRT)
                    continue;
                Voxel *up = get_chunk_voxel(chunk, voxel->x, voxel->y + 1, voxel->z);
                if(up && up->type == VOXEL_AIR) {
                    voxel->type = VOXEL_GRASS;
                }
            }
        }
    }
}

static inline bool back_voxels_solid(Chunk *chunk, Voxel *voxel) {
    Voxel *other = get_chunk_voxel(chunk, voxel->x, voxel->y, voxel->z - 1);
    if(!other) {
        return false;
    }
    return other->type != VOXEL_AIR;
}

static inline bool front_voxels_solid(Chunk *chunk, Voxel *voxel) {
    Voxel *other = get_chunk_voxel(chunk, voxel->x, voxel->y, voxel->z + 1);
    if(!other) {
        return false;
    }
    return other->type != VOXEL_AIR;
}

static inline bool left_voxels_solid(Chunk *chunk, Voxel *voxel) {
    Voxel *other = get_chunk_voxel(chunk, voxel->x - 1, voxel->y, voxel->z);
    if(!other) {
        return false;
    }
    return other->type != VOXEL_AIR;
}

static inline bool right_voxels_solid(Chunk *chunk, Voxel *voxel) {
    Voxel *other = get_chunk_voxel(chunk, voxel->x + 1, voxel->y, voxel->z);
    if(!other) {
        return false;
    }
    return other->type != VOXEL_AIR;
}

static inline bool top_voxels_solid(Chunk *chunk, Voxel *voxel) {
    Voxel *other = get_chunk_voxel(chunk, voxel->x, voxel->y + 1, voxel->z);
    if(!other) {
        return false;
    }
    return other->type != VOXEL_AIR;
}

static inline bool bottom_voxels_solid(Chunk *chunk, Voxel *voxel) {
    Voxel *other = get_chunk_voxel(chunk, voxel->x, voxel->y - 1, voxel->z);
    if(!other) {
        return false;
    }
    return other->type != VOXEL_AIR;
}

void generate_chunk_geometry(Chunk *chunk) {

    chunk->geometry_count = 0;

    for(u32 x = 0; x < CHUNK_X; ++x) {
        for(u32 y = 0; y < CHUNK_Y; ++y) {
            for(u32 z = 0; z < CHUNK_Z; ++z) {

                Voxel *voxel = get_chunk_voxel(chunk, x, y, z);
                if(voxel->type == VOXEL_AIR)
                    continue;

                f32 min_x = x * VOXEL_DIM - VOXEL_DIM * 0.5f;
                f32 max_x = min_x + VOXEL_DIM;
                f32 min_y = y * VOXEL_DIM - VOXEL_DIM * 0.5f;
                f32 max_y = min_y + VOXEL_DIM;
                f32 min_z = z * VOXEL_DIM - VOXEL_DIM * 0.5f;
                f32 max_z = min_z + VOXEL_DIM;

                VoxelBlock block_coords = voxel_block_map[voxel->type];

                if(!back_voxels_solid(chunk, voxel)) {
                    R2 coords = block_coords.coords[VOXEL_BLOCK_BACK];
                    // NOTE: Back face
                    add_vertex(chunk, v3(min_x, min_y, min_z), v3(0, 0, -1), coords.min);
                    add_vertex(chunk, v3(min_x, max_y, min_z), v3(0, 0, -1),
                               v2(coords.min.x, coords.max.y));
                    add_vertex(chunk, v3(max_x, max_y, min_z), v3(0, 0, -1), coords.max);
                    add_vertex(chunk, v3(max_x, max_y, min_z), v3(0, 0, -1), coords.max);
                    add_vertex(chunk, v3(max_x, min_y, min_z), v3(0, 0, -1),
                               v2(coords.max.x, coords.min.y));
                    add_vertex(chunk, v3(min_x, min_y, min_z), v3(0, 0, -1), coords.min);
                }

                if(!front_voxels_solid(chunk, voxel)) {
                    R2 coords = block_coords.coords[VOXEL_BLOCK_FRONT];
                    // NOTE: Front face
                    add_vertex(chunk, v3(min_x, min_y, max_z), v3(0, 0, 1), coords.min);
                    add_vertex(chunk, v3(max_x, max_y, max_z), v3(0, 0, 1), coords.max);
                    add_vertex(chunk, v3(min_x, max_y, max_z), v3(0, 0, 1),
                               v2(coords.min.x, coords.max.y));
                    add_vertex(chunk, v3(max_x, max_y, max_z), v3(0, 0, 1), coords.max);
                    add_vertex(chunk, v3(min_x, min_y, max_z), v3(0, 0, 1), coords.min);
                    add_vertex(chunk, v3(max_x, min_y, max_z), v3(0, 0, 1),
                               v2(coords.max.x, coords.min.y));
                }

                if(!right_voxels_solid(chunk, voxel)) {
                    R2 coords = block_coords.coords[VOXEL_BLOCK_RIGHT];
                    // NOTE: Right face
                    add_vertex(chunk, v3(max_x, min_y, min_z), v3(1, 0, 0),
                               v2(coords.max.x, coords.min.y));
                    add_vertex(chunk, v3(max_x, max_y, min_z), v3(1, 0, 0),
                               v2(coords.max.x, coords.max.y));
                    add_vertex(chunk, v3(max_x, min_y, max_z), v3(1, 0, 0),
                               v2(coords.min.x, coords.min.y));
                    add_vertex(chunk, v3(max_x, min_y, max_z), v3(1, 0, 0),
                               v2(coords.min.x, coords.min.y));
                    add_vertex(chunk, v3(max_x, max_y, min_z), v3(1, 0, 0),
                               v2(coords.max.x, coords.max.y));
                    add_vertex(chunk, v3(max_x, max_y, max_z), v3(1, 0, 0),
                               v2(coords.min.x, coords.max.y));
                }

                if(!left_voxels_solid(chunk, voxel)) {
                    R2 coords = block_coords.coords[VOXEL_BLOCK_LEFT];
                    // NOTE: Left face
                    add_vertex(chunk, v3(min_x, min_y, min_z), v3(-1, 0, 0),
                               v2(coords.min.x, coords.min.y));
                    add_vertex(chunk, v3(min_x, min_y, max_z), v3(-1, 0, 0),
                               v2(coords.max.x, coords.min.y));
                    add_vertex(chunk, v3(min_x, max_y, min_z), v3(-1, 0, 0),
                               v2(coords.min.x, coords.max.y));
                    add_vertex(chunk, v3(min_x, min_y, max_z), v3(-1, 0, 0),
                               v2(coords.max.x, coords.min.y));
                    add_vertex(chunk, v3(min_x, max_y, max_z), v3(-1, 0, 0),
                               v2(coords.max.x, coords.max.y));
                    add_vertex(chunk, v3(min_x, max_y, min_z), v3(-1, 0, 0),
                               v2(coords.min.x, coords.max.y));
                }

                if(!top_voxels_solid(chunk, voxel)) {
                    R2 coords = block_coords.coords[VOXEL_BLOCK_TOP];
                    // NOTE: Top face
                    add_vertex(chunk, v3(min_x, max_y, min_z), v3(0, 1, 0),
                               v2(coords.min.x, coords.max.y));
                    add_vertex(chunk, v3(min_x, max_y, max_z), v3(0, 1, 0),
                               v2(coords.min.x, coords.min.y));
                    add_vertex(chunk, v3(max_x, max_y, max_z), v3(0, 1, 0),
                               v2(coords.max.x, coords.min.y));
                    add_vertex(chunk, v3(max_x, max_y, max_z), v3(0, 1, 0),
                               v2(coords.max.x, coords.min.y));
                    add_vertex(chunk, v3(max_x, max_y, min_z), v3(0, 1, 0),
                               v2(coords.max.x, coords.max.y));
                    add_vertex(chunk, v3(min_x, max_y, min_z), v3(0, 1, 0),
                               v2(coords.min.x, coords.max.y));
                }

                if(!bottom_voxels_solid(chunk, voxel)) {
                    R2 coords = block_coords.coords[VOXEL_BLOCK_BOTTOM];
                    // NOTE: Bottom face
                    add_vertex(chunk, v3(min_x, min_y, min_z), v3(0, -1, 0),
                               v2(coords.min.x, coords.max.y));
                    add_vertex(chunk, v3(max_x, min_y, max_z), v3(0, -1, 0),
                               v2(coords.max.x, coords.min.y));
                    add_vertex(chunk, v3(min_x, min_y, max_z), v3(0, -1, 0),
                               v2(coords.min.x, coords.min.y));
                    add_vertex(chunk, v3(max_x, min_y, max_z), v3(0, -1, 0),
                               v2(coords.max.x, coords.min.y));
                    add_vertex(chunk, v3(min_x, min_y, min_z), v3(0, -1, 0),
                               v2(coords.min.x, coords.max.y));
                    add_vertex(chunk, v3(max_x, min_y, min_z), v3(0, -1, 0),
                               v2(coords.max.x, coords.max.y));
                }
            }
        }
    }
}

Chunk *get_farthest_chunk(s32 x, s32 z) {

    f32 distance  = 0;
    Chunk *result = NULL;

    Chunk *chunk = list_get_top(&loaded_chunks);
    while(!list_is_end(&loaded_chunks, chunk)) {

        V3 chunk_pos  = v3((f32)chunk->x, 0, (f32)chunk->z);
        V3 cam_to_pos = v3_sub(chunk_pos, v3((f32)x, 0, (f32)z));
        f32 temp      = v3_length(cam_to_pos);

        if(distance < temp) {
            distance = temp;
            result   = chunk;
        }

        chunk = chunk->next;
    }
    assert(result);
    return result;
}

static void unload_chunk(Chunk *chunk) {
    list_remove(chunk);
    list_insert_front(&free_chunks, chunk);
}

static Chunk *load_chunk(s32 x, s32 z) {

    if(list_is_empty(&free_chunks)) {
        Chunk *chunk_to_unload = get_farthest_chunk(x, z);
        unload_chunk(chunk_to_unload);
    }

    Chunk *result = list_get_top(&free_chunks);
    list_remove(result);
    result->x              = x;
    result->z              = z;
    result->geometry_count = 0;
    list_insert_front(&loaded_chunks, result);
    generate_chunk_voxels(result);
    generate_chunk_geometry(result);

    glBindBuffer(GL_ARRAY_BUFFER, result->vao);
    glBufferSubData(GL_ARRAY_BUFFER, 0, result->geometry_count * sizeof(Vertex), result->geometry);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return result;
}

static b32 chunk_is_loaded(s32 x, s32 z) {
    Chunk *chunk = list_get_top(&loaded_chunks);
    while(!list_is_end(&loaded_chunks, chunk)) {
        if(chunk->x == x && chunk->z == z) {
            return true;
        }
        chunk = chunk->next;
    }
    return false;
}

int main(void) {

    u32 w = 1920 / 2;
    u32 h = 1080 / 2;
    os_window_initialize(w, h);

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

        // printf("ms:%f\n", dt);

        os_process_input();

        update_camera(&camera, dt);

        s32 current_chunk_x = (s32)(camera.pos.x / CHUNK_X);
        s32 current_chunk_z = (s32)(camera.pos.z / CHUNK_Z);

        for(s32 x = current_chunk_x - MAX_CHUNKS_X / 2; x <= current_chunk_x + MAX_CHUNKS_X / 2;
            ++x) {
            for(s32 z = current_chunk_z - MAX_CHUNKS_Y / 2; z <= current_chunk_z + MAX_CHUNKS_Y / 2;
                ++z) {
                if(!chunk_is_loaded(x, z)) {
                    load_chunk(x, z);
                    // printf("loding chunk!\n");
                }
            }
        }

        glClearColor(0.3f, 0.65f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glBindTexture(GL_TEXTURE_2D, texture);

        // NOTE: Update camera position
        M4 view = m4_lookat2(camera.pos, v3_add(camera.pos, camera.target), camera.up);
        gpu_load_m4_uniform(program, "view", view);

        Chunk *chunk = list_get_top(&loaded_chunks);
        while(!list_is_end(&loaded_chunks, chunk)) {

            if(chunk->x >= current_chunk_x - MAX_CHUNKS_X / 2 &&
               chunk->x <= current_chunk_x + MAX_CHUNKS_X / 2 &&
               chunk->z >= current_chunk_z - MAX_CHUNKS_Y / 2 &&
               chunk->z <= current_chunk_z + MAX_CHUNKS_Y / 2) {

                // NOTE: Setup model matrix
                f32 pos_x = chunk->x * VOXEL_DIM * CHUNK_X;
                f32 pos_z = chunk->z * VOXEL_DIM * CHUNK_Z;
                M4 model  = m4_translate(v3(pos_x, 0, pos_z));
                gpu_load_m4_uniform(program, "model", model);

#if 0
                glBufferSubData(GL_ARRAY_BUFFER, 0, chunk->geometry_count * sizeof(Vertex),
                                chunk->geometry);
#endif
                glBindVertexArray(chunk->vao);
                glDrawArrays(GL_TRIANGLES, 0, chunk->geometry_count);
            }
            chunk = chunk->next;
        }

        os_swap_window();
    }

    os_window_terminate();

    return 0;
}
