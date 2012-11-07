#ifndef LCD_H
#define LCD_H

void lcd_init();
void lcd_pstring(const char* str);
void lcd_clear();
void lcd_home();

void lcd_set_pos(uint8_t pos);
void lcd_send_8bit(uint8_t bits);

#endif//LCD_H
