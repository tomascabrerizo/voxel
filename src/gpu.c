#include <glad/glad.h>

#include "gpu.h"
#include "os.h"

u32 gpu_load_program(char *vs_path, char *fs_path) {
    File vs_file = os_read_entire_file(vs_path);
    File fs_file = os_read_entire_file(fs_path);

    GLint vs_compile = 0;
    u32 vs_shader    = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs_shader, 1, (const char **)&vs_file.data, NULL);
    glCompileShader(vs_shader);
    glGetShaderiv(vs_shader, GL_COMPILE_STATUS, &vs_compile);
    if(vs_compile != GL_TRUE) {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetShaderInfoLog(vs_shader, 1024, &log_length, message);
        printf("Vertex Shader Error: %s\n", message);
    }

    GLint fs_compile = 0;
    u32 fs_shader    = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs_shader, 1, (const char **)&fs_file.data, NULL);
    glCompileShader(fs_shader);
    glGetShaderiv(fs_shader, GL_COMPILE_STATUS, &fs_compile);
    if(fs_compile != GL_TRUE) {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetShaderInfoLog(fs_shader, 1024, &log_length, message);
        printf("Fragment Shader Error: %s\n", message);
    }

    u32 program = glCreateProgram();

    glAttachShader(program, vs_shader);
    glAttachShader(program, fs_shader);
    GLint link = 0;
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &link);
    if(link != GL_TRUE) {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetProgramInfoLog(vs_shader, 1024, &log_length, message);
        printf("Program Error: %s\n", message);
    }

    glDeleteShader(vs_shader);
    glDeleteShader(fs_shader);

    os_free_entire_file(&vs_file);
    os_free_entire_file(&fs_file);

    return program;
}

u32 gpu_load_buffer(Vertex *data, u64 size) {
    u32 vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    u32 vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), offset_of(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(Vertex), offset_of(Vertex, nor));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_TRUE, sizeof(Vertex), offset_of(Vertex, uvs));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void gpu_load_m4_uniform(u32 program, char *name, M4 m) {
    s32 location = glGetUniformLocation(program, (const GLchar *)name);
    glUniformMatrix4fv(location, 1, GL_TRUE, (const GLfloat *)m.m);
}

u32 gpu_load_texture(void *pixels, u32 w, u32 h) {
    u32 texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 5);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

#if 0
    // NOTE: Mipmap test
    {
        u32 mipmap_w = w / 2;
        u32 mipmap_h = h / 2;
        u32 *mipmap  = (u32 *)malloc(sizeof(u32) * mipmap_w * mipmap_h);

        u32 r_shift = 16;
        u32 r_mask  = 0xff << r_shift;
        u32 g_shift = 8;
        u32 g_mask  = 0xff << g_shift;
        u32 b_shift = 0;
        u32 b_mask  = 0xff << b_shift;
        u32 a_shift = 24;
        u32 a_mask  = 0xff << a_shift;

        for(u32 y = 0; y < mipmap_h; ++y) {
            for(u32 x = 0; x < mipmap_w; ++x) {

                u32 dst_r = 0;
                u32 dst_g = 0;
                u32 dst_b = 0;
                u32 dst_a = 0;

                for(u32 yy = 0; yy < 2; ++yy) {
                    for(u32 xx = 0; xx < 2; ++xx) {

                        u32 xxx = x * 2 + xx;
                        u32 yyy = y * 2 + yy;
                        u32 src = ((u32 *)pixels)[yyy * w + xxx];

                        dst_r += ((src & r_mask) >> r_shift);
                        dst_g += ((src & g_mask) >> g_shift);
                        dst_b += ((src & b_mask) >> b_shift);
                        dst_a += ((src & a_mask) >> a_shift);
                    }
                }

                mipmap[y * mipmap_w + x] = ((dst_r / 4) << r_shift) | ((dst_g / 4) << g_shift) |
                                           ((dst_b / 4) << b_shift) | ((dst_a / 4) << a_shift);
            }
        }
        SDL_Surface *mipmap_image = SDL_CreateRGBSurfaceFrom(
            mipmap, mipmap_w, mipmap_h, 32, mipmap_w * sizeof(u32), r_mask, g_mask, b_mask, a_mask);
        SDL_SaveBMP(mipmap_image, "res/mipmap.bmp");
    }

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#endif
