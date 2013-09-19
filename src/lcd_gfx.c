#include <util/delay.h>
#include <avr/pgmspace.h>

#include "lcd.h"

#include "fastio.h"
#include "pinconfig.h"

#define LCD_GFX_WIDTH 128
#define LCD_GFX_HEIGHT 64

#define I2C_LED_ADDRESS 0b1100000

#define I2C_LCD_ADDRESS 0b0111100
#define I2C_WRITE   0x00
#define I2C_LCD_READ    0x01
#define I2C_LCD_SEND_COMMAND 0x00
#define I2C_LCD_SEND_DATA    0x40

#define LCD_COMMAND_CONTRAST                0x81
#define LCD_COMMAND_FULL_DISPLAY_ON_DISABLE 0xA4
#define LCD_COMMAND_FULL_DISPLAY_ON_ENABLE  0xA5
#define LCD_COMMAND_INVERT_DISABLE          0xA6
#define LCD_COMMAND_INVERT_ENABLE           0xA7
#define LCD_COMMAND_DISPLAY_OFF             0xAE
#define LCD_COMMAND_DISPLAY_ON              0xAF
#define LCD_COMMAND_NOP                     0xE3
#define LCD_COMMAND_LOCK_COMMANDS           0xDF

#define LCD_COMMAND_SET_ADDRESSING_MODE     0x20

#define LCD_RESET_PIN 5
#define LCD_CS_PIN    6

#define LCD_I2C_FREQ 200000

static inline void i2c_start()
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
}

static inline void i2c_restart()
{
    while (!(TWCR & (1<<TWINT))) {}
    i2c_start();
}

static inline void i2c_send_raw(uint8_t data)
{
    while (!(TWCR & (1<<TWINT))) {}
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
}

static inline void i2c_end()
{
    while (!(TWCR & (1<<TWINT))) {}
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

void led_write(uint8_t addr, uint8_t data)
{
    i2c_start();
    i2c_send_raw(I2C_LED_ADDRESS << 1 | I2C_WRITE);
    i2c_send_raw(addr);
    i2c_send_raw(data);
    i2c_end();
}

void lcd_init()
{
    SET_OUTPUT(LCD_CS_PIN);
    SET_OUTPUT(LCD_RESET_PIN);
    SET_OUTPUT(20);
    SET_OUTPUT(21);
    SET_OUTPUT(54);
    
    WRITE(54, 1);
    WRITE(LCD_CS_PIN, 0);
    WRITE(20, 1);
    WRITE(21, 1);
    
    WRITE(LCD_RESET_PIN, 0);
    _delay_ms(1);
    WRITE(LCD_RESET_PIN, 1);
    //_delay_ms(100);

    SET_OUTPUT(15);
    WRITE(15, 0);
    
    //ClockFreq = (F_CPU) / (16 + 2*TWBR * 4^TWPS)
    //TWBR = ((F_CPU / ClockFreq) - 16)/2*4^TWPS
    TWBR = ((F_CPU / LCD_I2C_FREQ) - 16)/2*1;
    TWSR = 0x00;

    led_write(0, 0x80);//MODE1
    led_write(1, 0x1C);//MODE2
    led_write(2, 0x20);//PWM0
    led_write(3, 0x20);//PWM1
    led_write(4, 0x20);//PWM2
    led_write(5, 0x20);//PWM3
    led_write(6, 0xFF);//GRPPWM
    led_write(7, 0x00);//GRPFREQ
    led_write(8, 0x00);//LEDOUT
/*
    i2c_start();
    i2c_send_raw(I2C_LCD_ADDRESS << 1 | I2C_WRITE);
    i2c_send_raw(I2C_LCD_SEND_COMMAND);
    
    i2c_send_raw(LCD_COMMAND_LOCK_COMMANDS);
    i2c_send_raw(0x12);
    
    i2c_send_raw(LCD_COMMAND_DISPLAY_OFF);

    i2c_send_raw(0xD5);//Display clock divider/freq
    i2c_send_raw(0xA0);

    i2c_send_raw(0xA8);//Multiplex ratio
    i2c_send_raw(0x3F);

    i2c_send_raw(0xD3);//Display offset
    i2c_send_raw(0x00);

    i2c_send_raw(0x40);//Set start line

    i2c_send_raw(0xA1);//Segment remap

    i2c_send_raw(0xC8);//COM scan output direction
    i2c_send_raw(0xDA);//COM pins hardware configuration
    i2c_send_raw(0x12);

    i2c_send_raw(LCD_COMMAND_CONTRAST);
    i2c_send_raw(0xDF);
    
    i2c_send_raw(0xD9);//Pre charge period
    i2c_send_raw(0x82);

    i2c_send_raw(0xDB);//VCOMH Deslect level
    i2c_send_raw(0x34);

    i2c_send_raw(LCD_COMMAND_SET_ADDRESSING_MODE);

    i2c_send_raw(LCD_COMMAND_FULL_DISPLAY_ON_DISABLE);
    
    i2c_send_raw(LCD_COMMAND_DISPLAY_ON);
    
    i2c_end();

    lcd_clear();
*/
}

void lcd_string(const char* str) {}
void lcd_pstring(const char* str) {}

void lcd_clear()
{
    /*
    uint16_t n;
    
    i2c_start();
    i2c_send_raw(I2C_LCD_ADDRESS << 1 | I2C_WRITE);
    //Set the drawin position to 0,0
    i2c_send_raw(I2C_LCD_SEND_COMMAND);
    i2c_send_raw(0x00 | (0 & 0x0F));
    i2c_send_raw(0x10 | (0 >> 4));
    i2c_send_raw(0xB0 | 0);

    i2c_restart();    
    i2c_send_raw(I2C_LCD_ADDRESS << 1 | I2C_WRITE);
    i2c_send_raw(I2C_LCD_SEND_DATA);
    for(n=0; n<128*64/8; n++)
        i2c_send_raw(0);
    i2c_end();
    */
}
void lcd_home() {}

void lcd_set_pos(uint8_t pos) {}
void lcd_send_8bit(uint8_t bits) {}
