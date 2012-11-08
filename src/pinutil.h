#ifndef PINUTIL_H
#define PINUTIL_H

/*
PinUtil are utiliy functions to help using AVR pins.
    They require 2 defines:
            {name}_PORT
            {name}_BIT
    Which define the last bit of the registers, and which bit in these registers
    are used for this pin.

Example:
    If you have a button on pin A4
        #define BUTTON_PORT A
        #define BUTTON_BIT  4
        if (GET_BIT(BUTTON))
*/

#define _MAKE_PORT(name, suffix) name ## suffix
#define MAKE_PORT(name, suffix) _MAKE_PORT(name, suffix)

#define SET_PIN_AS_OUTPUT(name) do { MAKE_PORT(DDR, name ##_PORT) |= _BV(name ## _BIT); } while(0)
#define CLR_PIN(name) do { MAKE_PORT(PORT, name ## _PORT) &=~_BV(name ## _BIT); } while(0)
#define SET_PIN(name) do { MAKE_PORT(PORT, name ## _PORT) |= _BV(name ## _BIT); } while(0)
#define GET_PIN(name) (MAKE_PORT(PIN, name ## _PORT) & _BV(name ## _BIT))

#endif//PINUTIL_H
