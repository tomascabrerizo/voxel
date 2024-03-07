#ifndef _GPU_H_
#define _GPU_H_

#include "algebra.h"

typedef struct Vertex {
    V3 pos;
    V3 nor;
    V2 uvs;
} Vertex;

u32 gpu_load_program(char *vs_path, char *fs_path);
u32 gpu_load_buffer(Vertex *data, u64 size);
void gpu_load_m4_uniform(u32 program, char *name, M4 m);
u32 gpu_load_texture(void *pixels, u32 w, u32 h);

#endif // _GPU_H_
