#include "hw.h"
#include "laborlicht.h"

int main(int argc, char *argv[]) {
    init_hw();

    // to avoid compiler warnings regarding unused functions, here is a quick
    // demonstration of the set_color_preset() function
    set_color_preset(color_black); // on startup, it's black anyway

    // quick color test at startup (15 seconds)
    for (color_preset_t preset = color_red; preset < color_MAX; ++preset) {
         color_fade_preset(preset, 128, 16);
    }

    while(1) {
        // fade through all color presets (except for black)
        for (color_preset_t preset = color_red; preset < color_MAX; ++preset) {
            // fades to a color for two minutes, holds that color for one minute
            color_fade_preset(preset, 255, 470);
            wait(60000);
        }

        // fade to a random color to have a nice variety
        color_fade_random(255, 470);
        wait(60000);
    };
}

// vim: ts=4:sts=4:sw=4:et
