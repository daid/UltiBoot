#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "fastio.h"
#include "pinconfig.h"
#include "lcd.h"

#if MOTHERBOARD != 72

/*************************/

#define LCD_CMD_CLEAR()                        (0x01)
#define LCD_CMD_RET_HOME()                     (0x02)
#define LCD_CMD_ENTRY_MODE(inc, shift)         (0x04 | (inc ? 0x02 : 0) | (shift ? 0x01 : 0))
#define LCD_CMD_DISPLAY_ON_OFF(d, c, b)        (0x08 | (d ? 0x04 : 0) | (c ? 0x02 : 0) | (b ? 0x01 : 0))
#define LCD_CMD_SHIFT(display, right)          (0x10 | (display ? 0x08 : 0) | (right ? 0x04 : 0))
#define LCD_CMD_FUNC_SET(dataLen, lines, font) (0x20 | (dataLen ? 0x10 : 0) | (lines ? 0x08 : 0) | (font ? 0x04 : 0))
#define LCD_CMD_CGRAM_ADDR(addr)               (0x40 | (addr))
#define LCD_CMD_DDRAM_ADDR(addr)               (0x80 | (addr))

#define SET_RS()   WRITE(LCD_PINS_RS, 1)
#define CLEAR_RS() WRITE(LCD_PINS_RS, 0)

static void lcd_send_4bit(uint8_t bits)
{
    WRITE(LCD_PINS_D4, 0);
    WRITE(LCD_PINS_D5, 0);
    WRITE(LCD_PINS_D6, 0);
    WRITE(LCD_PINS_D7, 0);
    if ((bits) & 0x01) WRITE(LCD_PINS_D4, 1);
    if ((bits) & 0x02) WRITE(LCD_PINS_D5, 1);
    if ((bits) & 0x04) WRITE(LCD_PINS_D6, 1);
    if ((bits) & 0x08) WRITE(LCD_PINS_D7, 1);
    WRITE(LCD_PINS_ENABLE, 1);
    _delay_us(1);
    WRITE(LCD_PINS_ENABLE, 0);
}

void lcd_send_8bit(uint8_t bits)
{
    lcd_send_4bit(bits >> 4);
    lcd_send_4bit(bits);
    _delay_us(53);
}

//See: http://web.alfredstate.edu/weimandn/lcd/lcd_initialization/lcd_initialization_index.html
void lcd_init()
{
    SET_OUTPUT(LCD_PINS_D4);
    SET_OUTPUT(LCD_PINS_D5);
    SET_OUTPUT(LCD_PINS_D6);
    SET_OUTPUT(LCD_PINS_D7);
    SET_OUTPUT(LCD_PINS_ENABLE);
    SET_OUTPUT(LCD_PINS_RS);
    
    _delay_ms(100);
    CLEAR_RS();
    lcd_send_4bit(0x03);
    _delay_ms(4);
    lcd_send_4bit(0x03);
    _delay_us(100);
    lcd_send_4bit(0x03);
    _delay_us(100);
    lcd_send_4bit(0x02);
    //Interface is now 4bits, so we can send 8bit data
    _delay_us(100);
    lcd_send_8bit(LCD_CMD_FUNC_SET(0, 1, 0));
    lcd_send_8bit(LCD_CMD_DISPLAY_ON_OFF(0,0,0));
    lcd_send_8bit(LCD_CMD_CLEAR());
    _delay_ms(2);
    lcd_send_8bit(LCD_CMD_ENTRY_MODE(1, 0));
    lcd_send_8bit(LCD_CMD_DISPLAY_ON_OFF(1,0,0));
    
    lcd_send_8bit(LCD_CMD_CGRAM_ADDR(0));
    SET_RS();
    lcd_send_8bit(0b00111);
    lcd_send_8bit(0b00111);
    lcd_send_8bit(0b00111);
    lcd_send_8bit(0b00111);
    lcd_send_8bit(0b01110);
    lcd_send_8bit(0b11110);
    lcd_send_8bit(0b11110);
    lcd_send_8bit(0b00110);
    
    lcd_send_8bit(0b00010);
    lcd_send_8bit(0b11010);
    lcd_send_8bit(0b11110);
    lcd_send_8bit(0b11110);
    lcd_send_8bit(0b11110);
    lcd_send_8bit(0b11111);
    lcd_send_8bit(0b11000);
    lcd_send_8bit(0b00000);

    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b11111);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b01110);
    lcd_send_8bit(0b01110);
    lcd_send_8bit(0b01110);

    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b01111);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b11111);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b00000);

    lcd_send_8bit(0b00111);
    lcd_send_8bit(0b00111);
    lcd_send_8bit(0b11111);
    lcd_send_8bit(0b00001);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b01110);
    lcd_send_8bit(0b01110);
    lcd_send_8bit(0b01110);

    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b11110);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b11111);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b00000);

    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b00000);
    lcd_send_8bit(0b10000);
    lcd_send_8bit(0b11000);
    lcd_send_8bit(0b01100);
    lcd_send_8bit(0b01100);
    lcd_send_8bit(0b01100);
    lcd_send_8bit(0b01110);

    lcd_send_8bit(0b01110);
    lcd_send_8bit(0b01110);
    lcd_send_8bit(0b01110);
    lcd_send_8bit(0b01100);
    lcd_send_8bit(0b01100);
    lcd_send_8bit(0b11100);
    lcd_send_8bit(0b00110);
    lcd_send_8bit(0b00011);

    lcd_clear();
}

void lcd_pstring(const char* str)
{
    char c;
    while((c = pgm_read_byte_far((long)(short)str | 0x10000)) != '\0')
    {
        lcd_send_8bit(c);
        str++;
    }
}

void lcd_string(const char* str)
{
    char c;
    while((c = (*str)) != '\0')
    {
        lcd_send_8bit(c);
        str++;
    }
}

void lcd_clear()
{
    CLEAR_RS();
    lcd_send_8bit(LCD_CMD_CLEAR());
    SET_RS();
    _delay_ms(2);
    lcd_set_pos(0x14);
    lcd_send_8bit(0);
    lcd_send_8bit(2);
    lcd_send_8bit(4);
    lcd_send_8bit(6);

    lcd_set_pos(0x54);
    lcd_send_8bit(1);
    lcd_send_8bit(3);
    lcd_send_8bit(5);
    lcd_send_8bit(7);

    lcd_set_pos(0x00);
}

void lcd_home()
{
    CLEAR_RS();
    lcd_send_8bit(LCD_CMD_RET_HOME());
    SET_RS();
    _delay_ms(1);
}

void lcd_set_pos(uint8_t pos)
{
    CLEAR_RS();
    lcd_send_8bit(LCD_CMD_DDRAM_ADDR(pos));
    SET_RS();
}

#endif
