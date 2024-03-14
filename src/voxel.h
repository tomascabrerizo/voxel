#ifndef _VOXEL_H_
#define _VOXEL_H_

#include "algebra.h"

#define ATLAS_W 512
#define ATLAS_H 512
#define TILE_DIM 32
#define ATLAS_ROWS (ATLAS_H / TILE_DIM)
#define ATLAS_COLS (ATLAS_W / TILE_DIM)

typedef struct R2 {
    V2 min, max;
} R2;

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

typedef struct Voxel {
    u8 type;
} Voxel;

#define VOXEL_DIM 1.0f

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

void voxel_block_map_initialize(void);

#endif // _VOXEL_H_
