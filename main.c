#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <stdlib.h>

#define xstr(s) str(s)
#define str(s) #s

#define PORTFL PORTC
#define DDRFL  DDRC

#define BIT_RED   (PC0)
#define BIT_GREEN (PC1)
#define BIT_BLUE  (PC2)

#define PW(a) pgm_read_word(&(a))

// wait timer
static void init_timer0(void) {
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega8A__)
#    define MY_TIFR TIFR
    // clk/64
    TCCR0 = _BV(CS01) | _BV(CS00);
#elif defined(__AVR_ATmega48__)   || \
      defined(__AVR_ATmega48P__)  || \
      defined(__AVR_ATmega88__)   || \
      defined(__AVR_ATmega88P__)  || \
      defined(__AVR_ATmega168__)  || \
      defined(__AVR_ATmega168P__) || \
      defined(__AVR_ATmega328__)  || \
      defined(__AVR_ATmega328P__)
#    define MY_TIFR TIFR0
    // clk/64
    TCCR0B = _BV(CS01) | _BV(CS00);
#else
#    error MCU not supported
#endif
}

static void wait(unsigned int ms){
    for (; ms--;) {
        TCNT0 = 6;                      // overflow after 250 ticks, 1000 Hz
        while (!(MY_TIFR & _BV(TOV0))); // wait for timer overflow flag
        MY_TIFR = _BV(TOV0);            // reset overflow flag
    }
}

typedef enum color_channel_e {
    channel_red,
    channel_green,
    channel_blue,
    channel_MAX,
} color_channel_t;

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

unsigned char const PROGMEM g_color_presets[color_MAX][channel_MAX] = {
    {  0,   0,   0},
    {255,   0,   0},
    {255, 255,   0},
    {  0, 255,   0},
    {  0, 255, 255},
    {  0,   0, 255},
    {255,   0, 255},
    {255, 255, 255}
};

volatile unsigned char g_color[channel_MAX] = {0};

void static set_color_preset(color_preset_t preset) {
    g_color[channel_red]   = PW(g_color_presets[preset][channel_red]);
    g_color[channel_green] = PW(g_color_presets[preset][channel_green]);
    g_color[channel_blue]  = PW(g_color_presets[preset][channel_blue]);
}

#define INTERIM_UPSCALE(a) ((a) * 128)
#define INTERIM_DOWNSCALE(a) ((a) / 128)

void static color_fade(unsigned char const *fade_color,
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

static void color_fade_preset(color_preset_t const preset,
                              unsigned char const steps,
                              unsigned int delay) {
    unsigned char preset_color[channel_MAX];
    for (unsigned char channel = 0; channel < channel_MAX; ++channel) {
        preset_color[channel] = PW(g_color_presets[preset][channel]);
    }
    color_fade(preset_color, steps, delay);
}

// fade to a random color (minimum brightness per channel is 64);
static void color_fade_random(unsigned char const steps,
                              unsigned int delay) {
    unsigned char random_color[channel_MAX];
    for (color_channel_t channel = 0; channel < channel_MAX; ++channel) {
        random_color[channel] = (rand() % 192) + 64;
    }
    color_fade(random_color, steps, delay);
}

unsigned char const PROGMEM g_cie_table[256] = {
     10,  11,  10,  11,  11,  10,  11,  11,
     10,  11,  11,  10,  11,  11,  11,  10,
     11,  11,  10,  11,  11,  10,  11,  12,
     11,  13,  12,  13,  13,  14,  14,  14,
     15,  16,  15,  17,  16,  17,  18,  18,
     18,  19,  19,  20,  20,  21,  21,  22,
     22,  23,  23,  24,  24,  25,  25,  26,
     26,  27,  28,  28,  28,  30,  30,  30,
     31,  31,  33,  32,  34,  34,  35,  35,
     36,  36,  38,  37,  39,  39,  40,  41,
     41,  42,  42,  44,  44,  44,  46,  46,
     47,  47,  49,  49,  50,  50,  52,  52,
     53,  54,  54,  55,  56,  57,  58,  58,
     60,  60,  61,  62,  62,  64,  64,  65,
     66,  67,  68,  69,  69,  71,  71,  73,
     73,  74,  75,  76,  77,  78,  78,  80,
     81,  81,  83,  84,  84,  86,  86,  88,
     88,  90,  91,  91,  93,  93,  95,  96,
     96,  98,  99, 100, 101, 102, 103, 105,
    105, 106, 108, 108, 110, 111, 112, 113,
    115, 115, 117, 117, 119, 120, 122, 122,
    124, 124, 126, 127, 129, 129, 131, 132,
    134, 134, 136, 137, 138, 140, 140, 142,
    144, 144, 146, 147, 149, 150, 151, 152,
    154, 155, 156, 158, 159, 160, 162, 163,
    165, 166, 167, 168, 170, 172, 173, 174,
    175, 177, 179, 180, 181, 183, 184, 186,
    187, 189, 190, 191, 193, 195, 196, 197,
    199, 201, 202, 203, 205, 207, 208, 209,
    212, 212, 215, 216, 217, 219, 221, 222,
    224, 225, 227, 229, 230, 232, 233, 236,
    236, 239, 240, 241, 244, 245, 247, 248
};

// ISR timer
static void init_timer2(void) {
#if defined(__AVR_ATmega8__) || \
    defined(__AVR_ATmega8A__)
#    define TIMER_ISR TIMER2_COMP_vect
#    define OUT_STS out
#    define OCR OCR2
    // CTC mode, OC2 disconnected, clk/8
    TCCR2 = _BV(WGM21) | _BV(CS21);
    // initialize compare match and counter registers
    OCR2 = PW(g_cie_table[0]);
    TCNT2 = 0;
    // clear compare match flag and enable compare match interrupt handler
    TIFR  |= _BV(OCF2);
    TIMSK |= _BV(OCIE2);
#elif defined(__AVR_ATmega48__)   || \
      defined(__AVR_ATmega48P__)  || \
      defined(__AVR_ATmega88__)   || \
      defined(__AVR_ATmega88P__)  || \
      defined(__AVR_ATmega168__)  || \
      defined(__AVR_ATmega168P__) || \
      defined(__AVR_ATmega328__)  || \
      defined(__AVR_ATmega328P__)
#    define TIMER_ISR TIMER2_COMPA_vect
#    define OUT_STS sts
#    define OCR OCR2A
    // CTC mode, OC2A/OC2B disconnected, clk/8
    TCCR2A = _BV(WGM21);
    TCCR2B = _BV(CS21);
    // initialize compare match and counter registers
    OCR2A = PW(g_cie_table[0]);
    TCNT2 = 0;
    // clear compare match flag and enable compare match interrupt handler
    TIFR2  |= _BV(OCF2A);
    TIMSK2 |= _BV(OCIE2A);
#else
#    error MCU not supported
#endif
}

ISR(TIMER_ISR) {
    // duty cycle from 0 (0%) to 255 (100%)
    static unsigned char duty_cycle = 0;

    __asm__ volatile(
        "cie_to_ocr2:"                          "\n\t"
            "add r30,%[duty_cycle]; ZL"         "\n\t"
            "adc r31,__zero_reg__ ; ZH"         "\n\t"
            "lpm __tmp_reg__,Z"                 "\n\t"
            xstr(OUT_STS) " %[ocr],__tmp_reg__" "\n\t"

        "red_cmp:"                              "\n\t"
            "ld __tmp_reg__,Y+"                 "\n\t"
            "cp __tmp_reg__,%[duty_cycle]"      "\n\t"
            "brlo red_off"                      "\n\t"
            "sbi %[portfl],%[bit_red]"          "\n\t"
            "rjmp green_cmp"                    "\n\t"
        "red_off:"                              "\n\t"
            "cbi %[portfl],%[bit_red]"          "\n\t"

        "green_cmp:"                            "\n\t"
            "ld __tmp_reg__,Y+"                 "\n\t"
            "cp __tmp_reg__,%[duty_cycle]"      "\n\t"
            "brlo green_off"                    "\n\t"
            "sbi %[portfl],%[bit_green]"        "\n\t"
            "rjmp blue_cmp"                     "\n\t"
        "green_off:"                            "\n\t"
            "cbi %[portfl],%[bit_green]"        "\n\t"

        "blue_cmp:"                             "\n\t"
            "ld __tmp_reg__,Y+"                 "\n\t"
            "cp __tmp_reg__,%[duty_cycle]"      "\n\t"
            "brlo blue_off"                     "\n\t"
            "sbi %[portfl],%[bit_blue]"         "\n\t"
            "rjmp finish"                       "\n\t"
        "blue_off:"                             "\n\t"
            "cbi %[portfl],%[bit_blue]"         "\n\t"

        "finish:"                               "\n\t"
            "inc %[duty_cycle]"                 "\n\t"

            : [duty_cycle] "+d" (duty_cycle)
            : "0" (duty_cycle),
              "y" (g_color),
              "z" (&g_cie_table[0]) ,
              [ocr]       "M" _SFR_IO_ADDR(OCR),
              [portfl]    "M" _SFR_IO_ADDR(PORTFL),
              [bit_red]   "M" (BIT_RED),
              [bit_green] "M" (BIT_GREEN),
              [bit_blue]  "M" (BIT_BLUE)
    );
}

static unsigned short get_seed()
{
    unsigned short seed = 0;
    unsigned short *p = (unsigned short*) (RAMEND + 1);
    extern unsigned short __heap_start;

    while (p >= &__heap_start + 1)
        seed ^= * (--p);

    return seed;
}

// hardware initialization
static void init_hw(void) {
    srand(get_seed());
    DDRFL |= _BV(BIT_RED) | _BV(BIT_GREEN) | _BV(BIT_BLUE); // set color pins to output mode
    init_timer0();
    init_timer2();
    sei();
}

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
