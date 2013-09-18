#ifndef PINCONFIG_H
#define PINCONFIG_H

#define MOTHERBOARD 72

#if MOTHERBOARD == 7
    //Ultimaker board v1.5.x
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

#endif

#if MOTHERBOARD == 72
    #define KNOWN_BOARD
    /*****************************************************************
    * Ultiboard v2.0 pin assignment
    ******************************************************************/

    #ifndef __AVR_ATmega2560__
     #error Oops!  Make sure you have 'Arduino Mega 2560' selected from the 'Tools -> Boards' menu.
    #endif

    #define X_STEP_PIN 25
    #define X_DIR_PIN 23
    #define X_STOP_PIN 22
    #define X_ENABLE_PIN 27

    #define Y_STEP_PIN 32
    #define Y_DIR_PIN 33
    #define Y_STOP_PIN 26
    #define Y_ENABLE_PIN 31

    #define Z_STEP_PIN 35
    #define Z_DIR_PIN 36
    #define Z_STOP_PIN 29
    #define Z_ENABLE_PIN 34

    #define HEATER_BED_PIN 4
    #define TEMP_BED_PIN 10

    #define HEATER_0_PIN  2
    #define TEMP_0_PIN 8   

    #define HEATER_1_PIN 3
    #define TEMP_1_PIN 9

    #define HEATER_2_PIN -1
    #define TEMP_2_PIN -1

    #define E0_STEP_PIN         42
    #define E0_DIR_PIN          43
    #define E0_ENABLE_PIN       37

    #define E1_STEP_PIN         49
    #define E1_DIR_PIN          47
    #define E1_ENABLE_PIN       48

    #define SDPOWER            -1
    #define SDSS               53
    #define SCK_PIN            52
    #define MOSI_PIN           51
    #define MISO_PIN           50

    #define LED_PIN            8
    #define FAN_PIN            7
    #define PS_ON_PIN          12
    #define KILL_PIN           -1
    #define SUICIDE_PIN        -1  //PIN that has to be turned on right after start, to keep power flowing.
    #define SAFETY_TRIGGERED_PIN     28 //PIN to detect the safety circuit has triggered
    #define MAIN_VOLTAGE_MEASURE_PIN 14 //Analogue PIN to measure the main voltage, with a 100k - 4k7 resitor divider.

    #undef MOTOR_CURRENT_PWM_XY_PIN
    #undef MOTOR_CURRENT_PWM_Z_PIN
    #undef MOTOR_CURRENT_PWM_E_PIN
    #define MOTOR_CURRENT_PWM_XY_PIN 44
    #define MOTOR_CURRENT_PWM_Z_PIN 45
    #define MOTOR_CURRENT_PWM_E_PIN 46
    //Motor current PWM conversion, PWM value = MotorCurrentSetting * Range / 255
    #define MOTOR_CURRENT_PWM_RANGE 2000

    //arduino pin witch triggers an piezzo beeper
    #define BEEPER 18

    #define LCD_PINS_RS 20 
    #define LCD_PINS_ENABLE 15
    #define LCD_PINS_D4 14
    #define LCD_PINS_D5 21 
    #define LCD_PINS_D6 5
    #define LCD_PINS_D7 6

    //buttons are directly attached
    #define BTN_EN1 40
    #define BTN_EN2 41
    #define BTN_ENC 19  //the click

    #define SDCARDDETECT 39

#endif//MOTHERBOARD == 72

#endif//PINCONFIG_H
