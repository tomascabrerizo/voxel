#include "os.h"
#include "game.h"

int main(void) {

    u32 w = 1920 / 2;
    u32 h = 1080 / 2;
    os_window_initialize(w, h);

    game_initialize(w, h);

    f64 last_time = SDL_GetTicks() / 1000.0;
    while(!os_window_should_close()) {

        os_process_input();

        f64 current_time = SDL_GetTicks() / 1000.0f;
        f32 dt           = current_time - last_time;
        last_time        = current_time;

        game_update(dt);

        game_render();

        os_swap_window();
    }

    os_window_terminate();

    return 0;
}
