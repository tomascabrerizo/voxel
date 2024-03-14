#ifndef _CHUNK_H_
#define _CHUNK_H_

#include "gpu.h"
#include "voxel.h"

#define CHUNK_X 16
#define CHUNK_Y 256
#define CHUNK_Z 16
#define CHUNK_TOTAL_SIZE (CHUNK_X * CHUNK_Y * CHUNK_Z)

#define MAX_CHUNKS_X (32 * 1)
#define MAX_CHUNKS_Y (32 * 1)
#define MAX_CHUNK_GEOMETRY_SIZE (1 * 1024 * 1024)

typedef struct ChunkNode {
    struct ChunkNode *prev;
    struct ChunkNode *next;
    struct ChunkNode *prev_hash;
    struct ChunkNode *next_hash;
} ChunkNode;

typedef struct Chunk {

    ChunkNode header;

    s32 x, z;
    Voxel voxels[CHUNK_TOTAL_SIZE];
    Vertex geometry[(MAX_CHUNK_GEOMETRY_SIZE / sizeof(Vertex))];
    u32 geometry_count;

    u32 vao;

    b32 is_loaded;
    b32 just_loaded;

} Chunk;

void chunk_generate_voxels(Chunk *chunk);
void chunk_generate_geometry(Chunk *chunk);

#endif // _CHUNK_H_
