#include <glad/glad.h>

#include "os.h"

static SDL_Window *window = NULL;
static SDL_GLContext context;
static bool window_should_close = false;

static u8 *keyboard = NULL;
static u8 last_keyboard[SDL_NUM_SCANCODES];

static u32 buttons      = 0;
static u32 last_buttons = 0;
static s32 mouse_x      = 0;
static s32 mouse_y      = 0;
static s32 mouse_rel_x  = 0;
static s32 mouse_rel_y  = 0;

void os_window_initialize(u32 w, u32 h) {
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("cannot initialize sdl2\n");
        exit(-1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    window = SDL_CreateWindow("Voxel Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
                              SDL_WINDOW_OPENGL);
    if(!window) {
        printf("Error creting window\n");
        exit(-1);
    }

    context = SDL_GL_CreateContext(window);
    if(!context) {
        printf("Error Creating OpenGL context\n");
        exit(-1);
    }

    if(SDL_GL_MakeCurrent(window, context) < 0) {
        printf("Error Creating OpenGL context\n");
        exit(-1);
    }

    if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        printf("Error loading OpenGL functions\n");
        exit(-1);
    }

    printf("OpenGL version: %s\n", glGetString(GL_VERSION));

    SDL_GL_SetSwapInterval(1);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glViewport(0, 0, w, h);
}

void os_window_terminate(void) {
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
}

void os_swap_window(void) {
    SDL_GL_SwapWindow(window);
}

b32 os_window_should_close(void) {
    return window_should_close;
}

void os_process_input(void) {
    // NOTE: Update mouse state
    last_buttons = buttons;
    buttons      = SDL_GetMouseState(&mouse_x, &mouse_y);
    SDL_GetRelativeMouseState(&mouse_rel_x, &mouse_rel_y);
    mouse_rel_y = -mouse_rel_y;

    // NOTE: Initialize keyboard state
    if(keyboard == NULL) {
        keyboard = (u8 *)SDL_GetKeyboardState(NULL);
    }

    // NOTE: Update last keyboard state
    for(s32 i = 0; i < SDL_NUM_SCANCODES; ++i) {
        last_keyboard[i] = keyboard[i];
    }

    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_QUIT: {
            window_should_close = true;
            printf("Closing application ...\n");
        } break;
        }
    }
}

void os_mouse_pos(s32 *x, s32 *y) {
    *x = mouse_x;
    *y = mouse_y;
}

void os_mouse_rel_pos(s32 *x, s32 *y) {
    *x = mouse_rel_x;
    *y = mouse_rel_y;
}

b32 os_button_down(u32 button) {
    return buttons & SDL_BUTTON(button);
}

b32 os_button_just_down(u32 button) {
    return (buttons & SDL_BUTTON(button)) && !(last_buttons & SDL_BUTTON(button));
}

b32 os_button_just_up(u32 button) {
    return !(buttons & SDL_BUTTON(button)) && (last_buttons & SDL_BUTTON(button));
}

b32 os_key_just_down(SDL_Scancode code) {
    return keyboard[code] && !last_keyboard[code];
}

b32 os_key_just_up(SDL_Scancode code) {
    return !keyboard[code] && last_keyboard[code];
}

b32 os_key_down(SDL_Scancode code) {
    return keyboard[code];
}

File os_read_entire_file(char *path) {
    File resutl = { 0 };

    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    u32 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    resutl.data = (u8 *)malloc(size + 1);
    resutl.size = size;

    fread(resutl.data, resutl.size, 1, file);
    resutl.data[resutl.size] = '\0';

    fclose(file);

    return resutl;
}

void os_free_entire_file(File *file) {
    assert(file->data && file->size > 0);
    free(file->data);
    file->size = 0;
}
