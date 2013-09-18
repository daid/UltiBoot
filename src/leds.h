#ifndef LEDS_H
#define LEDS_H

#if MOTHERBOARD == 72
//Controll the leds on the Ulticontroller V2

void led_write(uint8_t addr, uint8_t data);
void led_init();

#else

static inline void led_write(uint8_t addr, uint8_t data) {}
static inline void led_init() {}

#endif

#endif//LEDS_H
