#ifndef PINCONFIG_H
#define PINCONFIG_H

#define BUTTON_PORT D
#define BUTTON_BIT  2

/** Display line config **/
#define LCD_RS_PORT D
#define LCD_RS_BIT  1
#define LCD_EN_PORT H
#define LCD_EN_BIT  0
#define LCD_D4_PORT H
#define LCD_D4_BIT  1
#define LCD_D5_PORT D
#define LCD_D5_BIT  0
#define LCD_D6_PORT E
#define LCD_D6_BIT  3
#define LCD_D7_PORT H
#define LCD_D7_BIT  3

/** SD card pins **/
//Chip select
#define SD_CS_PORT B
#define SD_CS_BIT  0
//Card detect
#define SD_CD_PORT D
#define SD_CD_BIT  7
//SPI CLK
#define SD_CLK_PORT  B
#define SD_CLK_BIT   1
//SPI MOSI
#define SD_MOSI_PORT B
#define SD_MOSI_BIT  2
//SPI MISO
#define SD_MISO_PORT B
#define SD_MISO_BIT  3

#endif//PINCONFIG_H
