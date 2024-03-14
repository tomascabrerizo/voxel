#include "game.h"
#include "os.h"
#include "job.h"

#include <glad/glad.h>

static void game_allocate_chunk_buffer(Game *game) {

    game->chunk_buffer_count = (u32)((MAX_CHUNKS_X * MAX_CHUNKS_Y) * 2);
    game->chunk_buffer       = (Chunk *)malloc(sizeof(Chunk) * game->chunk_buffer_count);

    printf("chunk: %lld\n", sizeof(game->chunk_buffer[0]));
    printf("chunk voxels size: %lld\n", sizeof(game->chunk_buffer[0].voxels));
    printf("chunk vertex size: %lld\n", sizeof(game->chunk_buffer[0].geometry));
    printf("total chunk buffer size: %lld\n", (u64)(sizeof(Chunk) * game->chunk_buffer_count));

    for(u32 chunk_id = 0; chunk_id < game->chunk_buffer_count; ++chunk_id) {
        Chunk *chunk          = &game->chunk_buffer[chunk_id];
        chunk->vao            = gpu_load_buffer(NULL, 0);
        chunk->geometry_count = 0;
    }
}

static void game_setup_buffer_freelist(Game *game) {

    list_init(&game->free_chunks_list);

    for(u32 chunk_id = 0; chunk_id < game->chunk_buffer_count; ++chunk_id) {
        Chunk *chunk = &game->chunk_buffer[chunk_id];
        list_insert_back(&game->free_chunks_list, &chunk->header);
    }
}

static void game_setup_hash_chunk_table(Game *game) {
    list_init(&game->loaded_chunks_list);

    for(u32 i = 0; i < GAME_CHUNK_HASH_SIZE; ++i) {
        list_init_named(&game->hash_chunks[i], hash);
    }
}

static u32 game_get_hash_from_xz(s32 x, s32 z) {
    // TODO: Write an actual hash funciton
    return ((u32)x * 7 + (u32)z * 73) % GAME_CHUNK_HASH_SIZE;
}

static Chunk *game_get_farthest_chunk(Game *game, s32 x, s32 z) {

    f32 distance  = 0;
    Chunk *result = NULL;

    ChunkNode *chunk_node = list_get_top(&game->loaded_chunks_list);

    while(!list_is_end(&game->loaded_chunks_list, chunk_node)) {
        Chunk *chunk  = (Chunk *)chunk_node;
        V3 chunk_pos  = v3((f32)chunk->x, 0, (f32)chunk->z);
        V3 cam_to_pos = v3_sub(chunk_pos, v3((f32)x, 0, (f32)z));
        f32 temp      = v3_length(cam_to_pos);

        if(distance < temp) {
            distance = temp;
            result   = chunk;
        }

        chunk_node = chunk_node->next;
    }

    assert(result);
    return result;
}

Game g;

Chunk *game_get_chunk(s32 x, s32 z) {
    u32 hash          = game_get_hash_from_xz(x, z);
    ChunkNode *bucket = g.hash_chunks + hash;

    if(list_is_empty_named(bucket, hash)) {
        return NULL;
    }

    ChunkNode *chunk_node = list_get_top_named(bucket, hash);
    while(!list_is_end_named(bucket, chunk_node, hash)) {
        Chunk *chunk = (Chunk *)chunk_node;
        if(chunk->x == x && chunk->z == z) {
            return chunk;
        }
        chunk_node = chunk_node->next_hash;
    }

    return NULL;
}

void game_insert_chunk(Chunk *chunk) {
    u32 hash          = game_get_hash_from_xz(chunk->x, chunk->z);
    ChunkNode *bucket = g.hash_chunks + hash;
    list_insert_back_named(bucket, &chunk->header, hash);
    list_insert_back(&g.loaded_chunks_list, &chunk->header);
}

void game_remove_chunk(Chunk *chunk) {
    // NOTE: Remove chunk from loaded chunk hash
    list_remove_named(&chunk->header, hash);
    list_remove(&chunk->header);
}

b32 game_chunk_is_loaded(s32 x, s32 z) {
    Chunk *chunk = game_get_chunk(x, z);
    if(chunk && chunk->is_loaded) {
        return true;
    }
    return false;
}

int chunk_generate_voxels_and_geometry_job(void *data) {

    Chunk *chunk = (Chunk *)data;
    chunk_generate_voxels(chunk);
    chunk_generate_geometry(chunk);

    chunk->just_loaded = true;
    chunk->is_loaded   = true;

    return 0;
}

Chunk *game_chunk_load(s32 x, s32 z) {

    if(list_is_empty(&g.free_chunks_list)) {
        Chunk *chunk_to_unload = game_get_farthest_chunk(&g, x, z);
        game_chunk_unload(chunk_to_unload);
    }

    ChunkNode *chunk_node = list_get_top(&g.free_chunks_list);
    list_remove(chunk_node);

    Chunk *chunk          = (Chunk *)chunk_node;
    chunk->x              = x;
    chunk->z              = z;
    chunk->geometry_count = 0;

    game_insert_chunk(chunk);

    ThreadJob job;
    job.run  = chunk_generate_voxels_and_geometry_job;
    job.args = (void *)chunk;
    push_job(job);

    return chunk;
}

void game_chunk_unload(Chunk *chunk) {
    chunk->is_loaded = false;
    game_remove_chunk(chunk);
    list_insert_front(&g.free_chunks_list, &chunk->header);
}

void game_initialize(u32 w, u32 h) {

    voxel_block_map_initialize();
    job_system_initialize();

    game_allocate_chunk_buffer(&g);
    game_setup_buffer_freelist(&g);
    game_setup_hash_chunk_table(&g);

    camera_initialize(
        &g.camera,
        v3(VOXEL_DIM * CHUNK_X * MAX_CHUNKS_X / 2, 70, -VOXEL_DIM * CHUNK_Z * MAX_CHUNKS_Y / 2),
        v3(0, 0, -1), v3(0, 1, 0));

    g.program          = gpu_load_program("res/shaders/cube.vs", "res/shaders/cube.fs");
    SDL_Surface *atlas = SDL_LoadBMP("res/texture.bmp");
    g.texture          = gpu_load_texture(atlas->pixels, atlas->w, atlas->h);

    glUseProgram(g.program);

    // NOTE: Setup perspective projection
    f32 aspect = (f32)w / (f32)h;
    M4 proj    = m4_perspective2(to_rad(80), aspect, 0.1f, 1000.0f);
    gpu_load_m4_uniform(g.program, "proj", proj);
}

void game_terminate(void) {
    job_system_terminate();
}

void game_update(f32 dt) {

    camera_update(&g.camera, dt);

    s32 current_chunk_x = (s32)(g.camera.pos.x / CHUNK_X);
    s32 current_chunk_z = (s32)(g.camera.pos.z / CHUNK_Z);

    job_queue_begin();

    for(s32 x = current_chunk_x - MAX_CHUNKS_X / 2; x <= current_chunk_x + MAX_CHUNKS_X / 2; ++x) {
        for(s32 z = current_chunk_z - MAX_CHUNKS_Y / 2; z <= current_chunk_z + MAX_CHUNKS_Y / 2;
            ++z) {
            if(!game_chunk_is_loaded(x, z)) {
                game_chunk_load(x, z);
            }
        }
    }

    job_queue_end();
}

void game_render(void) {

    glClearColor(0.3f, 0.65f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(g.program);
    glBindTexture(GL_TEXTURE_2D, g.texture);

    // NOTE: Update camera position
    M4 view = m4_lookat2(g.camera.pos, v3_add(g.camera.pos, g.camera.target), g.camera.up);
    gpu_load_m4_uniform(g.program, "view", view);

    u32 chunk_count             = 0;
    u32 chunk_total_vertex_size = 0;

    ChunkNode *chunk_node = list_get_top(&g.loaded_chunks_list);
    while(!list_is_end(&g.loaded_chunks_list, chunk_node)) {
        Chunk *chunk = (Chunk *)chunk_node;

        if(chunk->just_loaded) {
            glBindBuffer(GL_ARRAY_BUFFER, chunk->vao);
            glBufferData(GL_ARRAY_BUFFER, chunk->geometry_count * sizeof(Vertex), chunk->geometry,
                         GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            chunk->just_loaded = false;
        }

        if(chunk->is_loaded) {
            // NOTE: Setup model matrix
            f32 pos_x = chunk->x * VOXEL_DIM * CHUNK_X;
            f32 pos_z = chunk->z * VOXEL_DIM * CHUNK_Z;
            M4 model  = m4_translate(v3(pos_x, 0, pos_z));
            gpu_load_m4_uniform(g.program, "model", model);

            glBindVertexArray(chunk->vao);
            glDrawArrays(GL_TRIANGLES, 0, chunk->geometry_count);

            chunk_count += 1;
            chunk_total_vertex_size += chunk->geometry_count * sizeof(Vertex);
        }

        chunk_node = chunk_node->next;
    }

    unused(chunk_count);
    unused(chunk_total_vertex_size);
#if 0
    f64 avrg_chunk_vertex_size = (f64)chunk_total_vertex_size / (f64)chunk_count;
    printf("avrg chunk vertex size: %lfMB\n", avrg_chunk_vertex_size / (1024.0 * 1014.0));
#endif
}
