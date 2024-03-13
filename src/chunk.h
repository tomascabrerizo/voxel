#ifndef _CHUNK_H_
#define _CHUNK_H_

#include "gpu.h"
#include "voxel.h"

#define CHUNK_X 16
#define CHUNK_Y 256
#define CHUNK_Z 16
#define CHUNK_TOTAL_SIZE (CHUNK_X * CHUNK_Y * CHUNK_Z)

typedef struct Chunk {
    s32 x, z;
    Voxel voxels[CHUNK_TOTAL_SIZE];
    Vertex *geometry;
    u32 geometry_count;

    struct Chunk *next;
    struct Chunk *prev;
    struct Chunk *prev_hash;
    struct Chunk *next_hash;

    u32 vao;

    b32 is_loaded;
    b32 just_loaded;

} Chunk;

#define MAX_CHUNKS_X (32 * 1)
#define MAX_CHUNKS_Y (32 * 1)
#define MAX_CHUNK_GEOMETRY_SIZE (5 * 1024 * 1024)
#define HASH_CHUNKS_SIZE (MAX_CHUNKS_X * MAX_CHUNKS_Y * 2)

void chunks_initialize(void);
void chunk_terminate(void);

void chunk_load(s32 x, s32 z);
void chunk_unload(Chunk *chunk);
b32 chunk_is_loaded(s32 x, s32 z);

// TODO: Remove this two functions
Chunk *loaded_chunk_list_get_first(void);
b32 loaded_chunk_list_is_end(Chunk *chunk);

#endif // _CHUNK_H_
