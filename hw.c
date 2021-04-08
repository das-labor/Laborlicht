#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "hw.h"

volatile unsigned char g_color[channel_MAX] = {0};

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
            "lds __tmp_reg__,g_color"           "\n\t"
            "cp __tmp_reg__,%[duty_cycle]"      "\n\t"
            "brlo red_off"                      "\n\t"
            "sbi %[port_red],%[bit_red]"        "\n\t"
            "rjmp green_cmp"                    "\n\t"
        "red_off:"                              "\n\t"
            "cbi %[port_red],%[bit_red]"        "\n\t"

        "green_cmp:"                            "\n\t"
            "lds __tmp_reg__,g_color+1"         "\n\t"
            "cp __tmp_reg__,%[duty_cycle]"      "\n\t"
            "brlo green_off"                    "\n\t"
            "sbi %[port_green],%[bit_green]"    "\n\t"
            "rjmp blue_cmp"                     "\n\t"
        "green_off:"                            "\n\t"
            "cbi %[port_green],%[bit_green]"    "\n\t"

        "blue_cmp:"                             "\n\t"
            "lds __tmp_reg__,g_color+2"         "\n\t"
            "cp __tmp_reg__,%[duty_cycle]"      "\n\t"
            "brlo blue_off"                     "\n\t"
            "sbi %[port_blue],%[bit_blue]"      "\n\t"
            "rjmp finish"                       "\n\t"
        "blue_off:"                             "\n\t"
            "cbi %[port_blue],%[bit_blue]"      "\n\t"

        "finish:"                               "\n\t"
            "inc %[duty_cycle]"                 "\n\t"

            : [duty_cycle] "+r" (duty_cycle)
            : "[duty_cycle]" (duty_cycle),
              "z" (&g_cie_table[0]) ,
              [ocr]        "M" _SFR_IO_ADDR(OCR),
              [port_red]   "M" _SFR_IO_ADDR(PORT_RED),
              [port_green] "M" _SFR_IO_ADDR(PORT_GREEN),
              [port_blue]  "M" _SFR_IO_ADDR(PORT_BLUE),
              [bit_red]    "M" (BIT_RED),
              [bit_green]  "M" (BIT_GREEN),
              [bit_blue]   "M" (BIT_BLUE)
    );
}

