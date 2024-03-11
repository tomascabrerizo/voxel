#include <glad/glad.h>
#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

#include "voxel.h"

static VoxelBlock voxel_block_map[VOXEL_TYPE_COUNT];

Chunk free_chunks;
Chunk loaded_chunks;
Chunk *hash_chunks;

static s32 random(s32 min, s32 max) {
    f32 scale  = (f32)rand() / (f32)RAND_MAX;
    s32 result = min + (s32)(scale * (f32)(max - min));
    // assert(result >= min && result < max);
    return result;
}

void voxel_block_map_initialize(void) {
    memset(voxel_block_map, 0, sizeof(voxel_block_map));

    // NOTE: Set up dirt coordinates
    for(u32 i = 0; i < VOXEL_BLOCK_FACE_COUNT; ++i) {
        voxel_block_map[VOXEL_DIRT].coords[i]                 = get_tile(0, 0);
        voxel_block_map[VOXEL_STONE].coords[i]                = get_tile(3, 0);
        voxel_block_map[VOXEL_WATER].coords[i]                = get_tile(4, 0);
        voxel_block_map[VOXEL_BLOCK_MINERAL_BLUE].coords[i]   = get_tile(1, 1);
        voxel_block_map[VOXEL_BLOCK_MINERAL_YELLOW].coords[i] = get_tile(2, 1);
        voxel_block_map[VOXEL_BLOCK_MINERAL_GREEN].coords[i]  = get_tile(3, 1);
        voxel_block_map[VOXEL_BLOCK_MINERAL_RED].coords[i]    = get_tile(4, 1);
    }

    // NOTE: Set up grass coordinates
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_BACK]   = get_tile(1, 0);
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_FRONT]  = get_tile(1, 0);
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_LEFT]   = get_tile(1, 0);
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_RIGHT]  = get_tile(1, 0);
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_TOP]    = get_tile(2, 0);
    voxel_block_map[VOXEL_GRASS].coords[VOXEL_BLOCK_BOTTOM] = get_tile(0, 0);
}

void chunks_initialize(void) {

    list_init(&loaded_chunks);
    list_init(&free_chunks);

    // NOTE: Initialize hash table for chunks
    hash_chunks = (Chunk *)malloc(HASH_CHUNKS_SIZE * sizeof(Chunk));
    for(u32 i = 0; i < HASH_CHUNKS_SIZE; ++i) {
        list_init_named(&hash_chunks[i], hash);
    }

    u32 total_chunk_count = (u32)((MAX_CHUNKS_X * MAX_CHUNKS_Y) * 2);
    Chunk *chunks         = (Chunk *)malloc(sizeof(Chunk) * total_chunk_count);
    for(u32 chunk_id = 0; chunk_id < total_chunk_count; ++chunk_id) {
        Chunk *chunk = &chunks[chunk_id];
        // NOTE: Insert chunk in the free list
        list_insert_back(&free_chunks, chunk);
        // NOTE: Allocate geometry memory on CPU
        chunk->geometry = (Vertex *)malloc(MAX_CHUNK_GEOMETRY_SIZE);
        // NOTE: Allocate geometry memory on GPU
        chunk->vao            = gpu_load_buffer(NULL, 0);
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

static void generate_chunk_voxels(Chunk *chunk) {
    unused(random);

    if(!chunk) {
        return;
    }

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

                if(y <= h && y < 50) {
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
#if 0
                if(y < CHUNK_Y / 2) {
                    voxel->type = VOXEL_STONE;
                } else {
                    voxel->type = VOXEL_AIR;
                }
#endif
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

static u32 chunk_get_hash(s32 x, s32 z) {
    // TODO: Write an actual hash funciton
    return ((u32)x * 7 + (u32)z * 73) % HASH_CHUNKS_SIZE;
}

static Chunk *hash_chunk_get(s32 x, s32 z) {
    u32 hash      = chunk_get_hash(x, z);
    Chunk *bucket = hash_chunks + hash;

    if(list_is_empty_named(bucket, hash)) {
        return NULL;
    }

    Chunk *chunk = list_get_top_named(bucket, hash);
    while(!list_is_end_named(bucket, chunk, hash)) {
        if(chunk->x == x && chunk->z == z) {
            return chunk;
        }
        chunk = chunk->next_hash;
    }

    return NULL;
}

static inline bool back_voxels_solid(Chunk *chunk, Voxel *voxel) {
    Voxel *other = NULL;

    if(voxel->z == 0) {
        Chunk *other_chunk = hash_chunk_get(chunk->x, chunk->z - 1);
        if(other_chunk) {
            other = get_chunk_voxel(other_chunk, voxel->x, voxel->y, (CHUNK_Z - 1));
        }
    } else {
        other = get_chunk_voxel(chunk, voxel->x, voxel->y, voxel->z - 1);
    }

    if(!other) {
        return false;
    }
    return other->type != VOXEL_AIR;
}

static inline bool front_voxels_solid(Chunk *chunk, Voxel *voxel) {
    Voxel *other = NULL;

    if(voxel->z == (CHUNK_Z - 1)) {
        Chunk *other_chunk = hash_chunk_get(chunk->x, chunk->z + 1);
        if(other_chunk) {
            other = get_chunk_voxel(other_chunk, voxel->x, voxel->y, 0);
        }
    } else {
        other = get_chunk_voxel(chunk, voxel->x, voxel->y, voxel->z + 1);
    }

    if(!other) {
        return false;
    }

    return other->type != VOXEL_AIR;
}

static inline bool left_voxels_solid(Chunk *chunk, Voxel *voxel) {

    Voxel *other = NULL;
    if(voxel->x == 0) {
        Chunk *other_chunk = hash_chunk_get(chunk->x - 1, chunk->z);
        if(other_chunk) {
            other = get_chunk_voxel(other_chunk, CHUNK_X - 1, voxel->y, voxel->z);
        }
    } else {
        other = get_chunk_voxel(chunk, voxel->x - 1, voxel->y, voxel->z);
    }

    if(!other) {
        return false;
    }
    return other->type != VOXEL_AIR;
}

static inline bool right_voxels_solid(Chunk *chunk, Voxel *voxel) {

    Voxel *other = NULL;
    if(voxel->x == CHUNK_X - 1) {
        Chunk *other_chunk = hash_chunk_get(chunk->x + 1, chunk->z);
        if(other_chunk) {
            other = get_chunk_voxel(other_chunk, 0, voxel->y, voxel->z);
        }
    } else {
        other = get_chunk_voxel(chunk, voxel->x + 1, voxel->y, voxel->z);
    }

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

static void generate_chunk_geometry(Chunk *chunk) {

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

    glBindBuffer(GL_ARRAY_BUFFER, chunk->vao);
    // glBufferSubData(GL_ARRAY_BUFFER, 0, chunk->geometry_count * sizeof(Vertex), chunk->geometry);
    glBufferData(GL_ARRAY_BUFFER, chunk->geometry_count * sizeof(Vertex), chunk->geometry,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void update_adjecent_chunks(Chunk *chunk) {
    Chunk *b = hash_chunk_get(chunk->x, chunk->z - 1);
    Chunk *f = hash_chunk_get(chunk->x, chunk->z + 1);
    Chunk *l = hash_chunk_get(chunk->x - 1, chunk->z);
    Chunk *r = hash_chunk_get(chunk->x + 1, chunk->z);
    generate_chunk_geometry(b);
    generate_chunk_geometry(f);
    generate_chunk_geometry(l);
    generate_chunk_geometry(r);
}

static Chunk *get_farthest_chunk(s32 x, s32 z) {

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

void unload_chunk(Chunk *chunk) {
    // NOTE: Remove chunk from loaded chunk hash
    list_remove_named(chunk, hash);

    // NOTE: Remove chunk from loaded chunks
    list_remove(chunk);
    list_insert_front(&free_chunks, chunk);
}

Chunk *load_chunk(s32 x, s32 z) {

    if(list_is_empty(&free_chunks)) {
        Chunk *chunk_to_unload = get_farthest_chunk(x, z);
        unload_chunk(chunk_to_unload);
    }

    // NOTE: Insert chunk into loaded chunk list
    Chunk *result = list_get_top(&free_chunks);
    list_remove(result);
    result->x              = x;
    result->z              = z;
    result->geometry_count = 0;
    list_insert_front(&loaded_chunks, result);

    // NOTE: Insert chunk into loaded chunk hash
    u32 hash      = chunk_get_hash(x, z);
    Chunk *bucket = hash_chunks + hash;
    list_insert_back_named(bucket, result, hash);

    generate_chunk_voxels(result);
    generate_chunk_geometry(result);

    update_adjecent_chunks(result);

    return result;
}

b32 chunk_is_loaded(s32 x, s32 z) {
    Chunk *chunk = hash_chunk_get(x, z);
    if(chunk) {
        return true;
    } else {
        return false;
    }
}

Chunk *loaded_chunk_list_get_first(void) {
    return list_get_top(&loaded_chunks);
}

b32 loaded_chunk_list_is_end(Chunk *chunk) {
    return list_is_end(&loaded_chunks, chunk);
}