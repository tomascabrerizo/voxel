#ifndef _VOXEL_H_
#define _VOXEL_H_

#include "os.h"
#include "gpu.h"
#include "algebra.h"

#define MAX_WORKER_THREADS 7

typedef struct ThreadJob {
    int (*run)(void *data);
    void *args;
} ThreadJob;

#define MAX_THREAD_JOBS 1024

void job_system_initialize(void);
void job_system_terminate(void);

void job_queue_begin(void);
void job_queue_end(void);

void push_job(ThreadJob job);

int generate_chunk_voxels_and_geometry_job(void *data);

// ----------------------------------------------------------------------

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
    VOXEL_WATER,
    VOXEL_BLOCK_MINERAL_BLUE,
    VOXEL_BLOCK_MINERAL_YELLOW,
    VOXEL_BLOCK_MINERAL_GREEN,
    VOXEL_BLOCK_MINERAL_RED,

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

void voxel_block_map_initialize(void);

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
    struct Chunk *prev_hash;
    struct Chunk *next_hash;

    u32 vao;

    b32 just_loaded;

} Chunk;

#define MAX_CHUNKS_X (32 * 1)
#define MAX_CHUNKS_Y (32 * 1)
#define MAX_CHUNK_GEOMETRY_SIZE (5 * 1024 * 1024)
#define HASH_CHUNKS_SIZE (MAX_CHUNKS_X * MAX_CHUNKS_Y * 2)

void chunks_initialize(void);
void load_chunk(s32 x, s32 z);
void unload_chunk(Chunk *chunk);
b32 chunk_is_loaded(s32 x, s32 z);

Chunk *loaded_chunk_list_get_first(void);
b32 loaded_chunk_list_is_end(Chunk *chunk);

#endif // _VOXEL_H_
