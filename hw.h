#ifndef HW_H
#define HW_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>

#include "config.h"

#define xstr(s) str(s)
#define str(s) #s

#define PW(a) pgm_read_word(&(a))

typedef enum color_channel_e {
    channel_red,
    channel_green,
    channel_blue,
    channel_MAX,
} color_channel_t;

extern volatile unsigned char g_color[channel_MAX];
extern unsigned char const g_cie_table[256]; 

// wait timer
inline static void init_timer0(void) {
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

// ISR timer
inline static void init_timer2(void) {
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

inline static unsigned short get_seed()
{
    unsigned short seed = 0;
    unsigned short *p = (unsigned short*) (RAMEND + 1);
    extern unsigned short __heap_start;

    while (p >= &__heap_start + 1)
        seed ^= * (--p);

    return seed;
}

// hardware initialization
inline static void init_hw(void) {
    srand(get_seed());

    // set color pins to output mode
    DDR_RED |= _BV(BIT_RED);
    DDR_GREEN |= _BV(BIT_GREEN);
    DDR_BLUE |= _BV(BIT_BLUE);

    init_timer0();
    init_timer2();
    sei();
}

inline static void wait(unsigned int ms){
    for (; ms--;) {
        TCNT0 = 6;                      // overflow after 250 ticks, 1000 Hz
        while (!(MY_TIFR & _BV(TOV0))); // wait for timer overflow flag
        MY_TIFR = _BV(TOV0);            // reset overflow flag
    }
}

#endif // HW_H

