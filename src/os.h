#ifndef _OS_
#define _OS_

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "common.h"

void os_window_initialize(u32 w, u32 h);
void os_window_terminate(void);
void os_process_input(void);
b32 os_window_should_close(void);
void os_swap_window(void);

void os_mouse_pos(s32 *x, s32 *y);
void os_mouse_rel_pos(s32 *x, s32 *y);

b32 os_button_down(u32 button);
b32 os_button_just_down(u32 button);
b32 os_button_just_up(u32 button);
b32 os_key_just_down(SDL_Scancode code);
b32 os_key_just_up(SDL_Scancode code);
b32 os_key_down(SDL_Scancode code);

typedef struct File {
    u8 *data;
    u32 size;
} File;

File os_read_entire_file(char *path);
void os_free_entire_file(File *file);

#endif // _OS_
