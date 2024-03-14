#include "chunk.h"
#include "job.h"

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

extern VoxelBlock voxel_block_map[VOXEL_TYPE_COUNT];

static s32 random(s32 min, s32 max) {
    f32 scale  = (f32)rand() / (f32)RAND_MAX;
    s32 result = min + (s32)(scale * (f32)(max - min));
    // assert(result >= min && result < max);
    return result;
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

static f32 calculate_xz_height(s32 chunk_x, s32 chunk_z, s32 x, s32 z) {
    f32 abs_x = (chunk_x * CHUNK_X + x) / ((f32)CHUNK_X * 2.0f);
    f32 abs_z = (chunk_z * CHUNK_Z + z) / ((f32)CHUNK_Z * 2.0f);
    f32 noise = (stb_perlin_noise3(abs_x, 0, abs_z, 0, 0, 0) + 1) / 2.0f;
    f32 h     = 20 + noise * (u32)((CHUNK_Y / 2) - 50);
    assert(h >= 0 && h < CHUNK_Y);
    return h;
}

void chunk_generate_voxels(Chunk *chunk) {
    unused(random);

    if(!chunk) {
        return;
    }

    for(s32 x = 0; x < CHUNK_X; ++x) {
        for(s32 z = 0; z < CHUNK_Z; ++z) {

            f32 h = calculate_xz_height(chunk->x, chunk->z, x, z);

            for(s32 y = 0; y < CHUNK_Y; ++y) {

                Voxel *voxel = get_chunk_voxel(chunk, x, y, z);
                voxel->x     = x;
                voxel->y     = y;
                voxel->z     = z;

                if(y <= h && y < 50) {

                    // NOTE: srand is not thread save
                    // srand(x * 7 + y * 69 + z * 83);

                    voxel->type = VOXEL_STONE;
                    s32 num     = random(0, 100);
                    if(num < 6) {
                        voxel->type = random(VOXEL_BLOCK_MINERAL_BLUE, VOXEL_BLOCK_MINERAL_RED + 1);
                    }
                } else if(y < h) {
                    voxel->type = VOXEL_DIRT;

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
                if(voxel->type == VOXEL_AIR && y < 49) {
                    voxel->type = VOXEL_WATER;
                }

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

    if(voxel->z == 0) {
        // if(chunk_is_loaded(chunk->x, chunk->z - 1)) {
        f32 h = calculate_xz_height(chunk->x, chunk->z - 1, voxel->x, (CHUNK_Z - 1));
        if(voxel->y < h) {
            return true;
        }
        //}
    }

    Voxel *other = get_chunk_voxel(chunk, voxel->x, voxel->y, voxel->z - 1);

    if(!other) {
        return false;
    }
    return other->type != VOXEL_AIR;
}

static inline bool front_voxels_solid(Chunk *chunk, Voxel *voxel) {

    if(voxel->z == (CHUNK_Z - 1)) {
        // if(chunk_is_loaded(chunk->x, chunk->z + 1)) {
        f32 h = calculate_xz_height(chunk->x, chunk->z + 1, voxel->x, 0);
        if(voxel->y < h) {
            return true;
        }
        //}
    }

    Voxel *other = get_chunk_voxel(chunk, voxel->x, voxel->y, voxel->z + 1);

    if(!other) {
        return false;
    }

    return other->type != VOXEL_AIR;
}

static inline bool left_voxels_solid(Chunk *chunk, Voxel *voxel) {

    if(voxel->x == 0) {

        // if(chunk_is_loaded(chunk->x - 1, chunk->z)) {
        f32 h = calculate_xz_height(chunk->x - 1, chunk->z, (CHUNK_X - 1), voxel->z);
        if(voxel->y < h) {
            return true;
        }
        //}
    }

    Voxel *other = get_chunk_voxel(chunk, voxel->x - 1, voxel->y, voxel->z);

    if(!other) {
        return false;
    }
    return other->type != VOXEL_AIR;
}

static inline bool right_voxels_solid(Chunk *chunk, Voxel *voxel) {

    if(voxel->x == CHUNK_X - 1) {
        // if(chunk_is_loaded(chunk->x + 1, chunk->z)) {
        f32 h = calculate_xz_height(chunk->x + 1, chunk->z, 0, voxel->z);
        if(voxel->y < h) {
            return true;
        }
        //}
    }

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

void chunk_generate_geometry(Chunk *chunk) {

    if(!chunk) {
        return;
    }

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
