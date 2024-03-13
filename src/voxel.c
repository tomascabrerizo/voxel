#include "voxel.h"

VoxelBlock voxel_block_map[VOXEL_TYPE_COUNT];

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
