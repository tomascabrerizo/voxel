#ifndef _GAME_H_
#define _GAME_H_

#include "common.h"
#include "chunk.h"
#include "camera.h"

#define GAME_CHUNK_HASH_SIZE (MAX_CHUNKS_X * MAX_CHUNKS_Y * 2)

typedef struct Game {

    Camera camera;

    Chunk *chunk_buffer;
    u32 chunk_buffer_count;

    ChunkNode free_chunks_list;
    ChunkNode loaded_chunks_list;
    ChunkNode hash_chunks[GAME_CHUNK_HASH_SIZE];

    u32 program;
    u32 texture;

} Game;

void game_initialize(u32 w, u32 h);

void game_terminate(void);

void game_update(f32 dt);

void game_render(void);

Chunk *game_chunk_load(s32 x, s32 z);
void game_chunk_unload(Chunk *chunk);

Chunk *game_get_chunk(s32 x, s32 z);
b32 game_chunk_is_loaded(s32 x, s32 z);

#endif // _GAME_H_
