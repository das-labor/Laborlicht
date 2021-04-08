#ifndef LABORLICHT_H
#define LABORLICHT_H

#include <stdlib.h>

#include "hw.h"

typedef enum color_preset_e {
    color_black,
    color_red,
    color_yellow,
    color_green,
    color_cyan,
    color_blue,
    color_purple,
    color_white,
    color_MAX
} color_preset_t;

extern unsigned char const g_color_presets[color_MAX][channel_MAX];

inline static void set_color_preset(color_preset_t preset) {
    g_color[channel_red]   = PW(g_color_presets[preset][channel_red]);
    g_color[channel_green] = PW(g_color_presets[preset][channel_green]);
    g_color[channel_blue]  = PW(g_color_presets[preset][channel_blue]);
}

void color_fade(unsigned char const *fade_color,
                unsigned char const steps,
                unsigned int delay);

inline static void color_fade_preset(color_preset_t const preset,
                                     unsigned char const steps,
                                     unsigned int delay) {
    unsigned char preset_color[channel_MAX];
    for (unsigned char channel = 0; channel < channel_MAX; ++channel) {
        preset_color[channel] = PW(g_color_presets[preset][channel]);
    }
    color_fade(preset_color, steps, delay);
}

// fade to a random color (minimum brightness per channel is 64);
inline static void color_fade_random(unsigned char const steps,
                                     unsigned int delay) {
    unsigned char random_color[channel_MAX];
    for (color_channel_t channel = 0; channel < channel_MAX; ++channel) {
        random_color[channel] = (rand() % 192) + 64;
    }
    color_fade(random_color, steps, delay);
}

#endif // LABORLICHT_H
