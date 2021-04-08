#include <avr/pgmspace.h>

#include "hw.h"
#include "laborlicht.h"

#define INTERIM_UPSCALE(a) ((a) * 128)
#define INTERIM_DOWNSCALE(a) ((a) / 128)

unsigned char const PROGMEM g_color_presets[color_MAX][channel_MAX] = {
    {  0,   0,   0}, // black
    {255,   0,   0}, // red
    {255, 255,   0}, // yellow
    {  0, 255,   0}, // green
    {  0, 255, 255}, // teal
    {  0,   0, 255}, // blue
    {255,   0, 255}, // pink
    {255, 255, 255}  // white
};

void color_fade(unsigned char const *fade_color,
                unsigned char const steps,
                unsigned int delay) {
    int interim_color[channel_MAX];
    int offset[channel_MAX];
    for (color_channel_t channel = 0; channel < channel_MAX; ++channel) {
        interim_color[channel] = INTERIM_UPSCALE(g_color[channel]);
        offset[channel] =
            INTERIM_UPSCALE(fade_color[channel] - g_color[channel]) / steps;
    }

    for (unsigned char step = 0; step < steps; ++step) {
        for (color_channel_t channel = 0; channel < channel_MAX; ++channel) {
            g_color[channel] =
                INTERIM_DOWNSCALE(interim_color[channel] += offset[channel]);
        }
        wait(delay);
    }

    // counter measure against rounding errors
    for (color_channel_t channel = 0; channel < channel_MAX; ++channel) {
        g_color[channel] = fade_color[channel];
    }
}

