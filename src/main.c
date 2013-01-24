#include <avr/io.h>
#include <avr/boot.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "pinutil.h"
#include "pinconfig.h"
#include "command.h"
#include "lcd.h"
#include "petit_fat/pff.h"

//ATMega1280: Linker setting: -Wl,--section-start=.text=0x1E000
//ATMega2560: Linker setting: -Wl,--section-start=.text=0x3E000

/************/
/** CONFIG **/
/************/

#define UART_BAUD 115200

/*
 * HW and SW version, reported to AVRISP, must match version of AVRStudio
 */
#define CONFIG_PARAM_BUILD_NUMBER_LOW	0
#define CONFIG_PARAM_BUILD_NUMBER_HIGH	0
#define CONFIG_PARAM_HW_VER				0x0F
#define CONFIG_PARAM_SW_MAJOR			2
#define CONFIG_PARAM_SW_MINOR			0x0A

#define BOOTSIZE (SPM_PAGESIZE * 32)

/**********/
/** CODE **/
/**********/

/*
 * Signature bytes are not available in avr-gcc io_xxx.h
 */
#if defined (__AVR_ATmega8__)
	#define SIGNATURE_BYTES 0x1E9307
#elif defined (__AVR_ATmega16__)
	#define SIGNATURE_BYTES 0x1E9403
#elif defined (__AVR_ATmega32__)
	#define SIGNATURE_BYTES 0x1E9502
#elif defined (__AVR_ATmega8515__)
	#define SIGNATURE_BYTES 0x1E9306
#elif defined (__AVR_ATmega8535__)
	#define SIGNATURE_BYTES 0x1E9308
#elif defined (__AVR_ATmega162__)
	#define SIGNATURE_BYTES 0x1E9404
#elif defined (__AVR_ATmega128__)
	#define SIGNATURE_BYTES 0x1E9702
#elif defined (__AVR_ATmega1280__)
	#define SIGNATURE_BYTES 0x1E9703
#elif defined (__AVR_ATmega2560__)
	#define SIGNATURE_BYTES 0x1E9801
#elif defined (__AVR_ATmega2561__)
	#define SIGNATURE_BYTES 0x1e9802
#else
	#error "no signature definition for MCU available"
#endif

#define SERIAL_DATA_AVAILABLE() (UCSR0A & _BV(RXC0))
#define RECV_SERIAL_DATA() (UDR0)
#define SEND_SERIAL_DATA(n) do { UDR0 = (n); while(!(UCSR0A & _BV(TXC0))); UCSR0A |= _BV(TXC0); } while(0)
#define BAUDRATE_DIVIDER() (((F_CPU + (4 * UART_BAUD)) / (8 * UART_BAUD)) - 1)

void serial_send_pstring(const char* str)
{
    char c;
    while((c = pgm_read_byte_far((long)(short)str | 0x10000)) != '\0')
    {
        SEND_SERIAL_DATA(c);
        str++;
    }
}

uint8_t __attribute__((section(".noinit"))) MCUSR_backup;
void disable_watchdog_asap(void) __attribute__((naked)) __attribute__((section(".init3")));
void disable_watchdog_asap(void)
{
    //Disable the watchdog timer, it could have been left on after reset. We need to do this ASAP, as the watchdog could trigger before main is called.
    //the .init3 section is run before data initialization.
    MCUSR_backup = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

#define STATE_START  0
#define STATE_SEQ    1
#define STATE_SIZE_1 2
#define STATE_SIZE_2 3
#define STATE_TOKEN  4
#define STATE_DATA   5
#define STATE_CHECK  6

typedef union {
    uint16_t i16;
    uint8_t  i8[2];
} union16t;
typedef union {
    uint32_t i32;
    uint16_t i16[2];
    uint8_t  i8[4];
} union32t;

uint8_t msgBuffer[1024];
uint8_t seq;
union16t msgLen;
uint16_t msgPos;
uint8_t checksum;
union32t address;

static void handleMessage()
{
    switch(msgBuffer[0])
    {
    case CMD_SIGN_ON:
        msgLen.i16		=	11;
        msgBuffer[1] 	=	STATUS_CMD_OK;
        msgBuffer[2] 	=	8;
        msgBuffer[3] 	=	'A';
        msgBuffer[4] 	=	'V';
        msgBuffer[5] 	=	'R';
        msgBuffer[6] 	=	'I';
        msgBuffer[7] 	=	'S';
        msgBuffer[8] 	=	'P';
        msgBuffer[9] 	=	'_';
        msgBuffer[10]	=	'2';
        break;
    case CMD_GET_PARAMETER:
        switch(msgBuffer[1])
        {
        case PARAM_BUILD_NUMBER_LOW:
            msgBuffer[2] = CONFIG_PARAM_BUILD_NUMBER_LOW;
            break;
        case PARAM_BUILD_NUMBER_HIGH:
            msgBuffer[2] = CONFIG_PARAM_BUILD_NUMBER_HIGH;
            break;
        case PARAM_HW_VER:
            msgBuffer[2] = CONFIG_PARAM_HW_VER;
            break;
        case PARAM_SW_MAJOR:
            msgBuffer[2] = CONFIG_PARAM_SW_MAJOR;
            break;
        case PARAM_SW_MINOR:
            msgBuffer[2] = CONFIG_PARAM_SW_MINOR;
            break;
        }
        msgLen.i16  	=	3;
        msgBuffer[1] 	=	STATUS_CMD_OK;
        break;
//    case CMD_SET_DEVICE_PARAMETERS:
//    case CMD_OSCCAL:
    case CMD_LOAD_ADDRESS:
        address.i8[3]   = msgBuffer[1];
        address.i8[2]   = msgBuffer[2];
        address.i8[1]   = msgBuffer[3];
        address.i8[0]   = msgBuffer[4];
        address.i32   <<= 1;
        msgLen.i16 		= 2;
        msgBuffer[1] 	= STATUS_CMD_OK;
        break;
//    case CMD_FIRMWARE_UPGRADE:
    case CMD_SET_PARAMETER:
    case CMD_ENTER_PROGMODE_ISP:
        msgLen.i16 		=	2;
        msgBuffer[1] 	=	STATUS_CMD_OK;
        break;
    case CMD_LEAVE_PROGMODE_ISP:
        msgLen.i16 		=	2;
        msgBuffer[1] 	=	STATUS_CMD_OK;
        //To leave the bootloader, set the timer really fast
        TCCR1B = _BV(CS10);
        break;
    case CMD_CHIP_ERASE_ISP:
        msgLen.i16 		=	2;
        msgBuffer[1] 	=	STATUS_CMD_OK;
        break;
    case CMD_PROGRAM_FLASH_ISP:
        {
            lcd_home();
            lcd_pstring(PSTR("Loading new firmware"));
            
            union16t size;
            size.i8[0] = msgBuffer[2];
            size.i8[1] = msgBuffer[1];
            uint8_t* c = &msgBuffer[10];
            uint32_t oldAddress = address.i32;
            
            msgLen.i16 		=	2;
            msgBuffer[1] 	=	STATUS_CMD_OK;
            
            //Protect the bootloader
            if (address.i32 > FLASHEND - BOOTSIZE - SPM_PAGESIZE)
                break;

            boot_page_erase(address.i32);	// Perform page erase
			boot_spm_busy_wait();		// Wait until the memory is erased.
			do
			{
                union16t data;
                data.i8[0] = *c++;
                data.i8[1] = *c++;
                boot_page_fill(address.i32, data.i16);
                address.i32 += 2;
                size.i16 -= 2;
			} while(size.i16);
            boot_page_write(oldAddress);
            boot_spm_busy_wait();
            boot_rww_enable();
        }
        break;
    case CMD_READ_FLASH_ISP:
        {
            union16t size, data;
            size.i8[0] = msgBuffer[2];
            size.i8[1] = msgBuffer[1];
            uint8_t* c = &msgBuffer[2];
            msgLen.i16 = size.i16 + 3;
            
            msgBuffer[1] = STATUS_CMD_OK;
            do
            {
                data.i16 = pgm_read_word_far(address.i32);
                *c++ = data.i8[0];
                *c++ = data.i8[1];
                address.i32 += 2;
                size.i16 -= 2;
            }while(size.i16);
        }
        break;
    case CMD_ULTI_CHECKSUM:
        {
            union16t checksum;
            uint32_t tmpAddr = 0;
            checksum.i16 = 0;
            while(tmpAddr < address.i32)
            {
                union16t tmp;
                tmp.i16 = pgm_read_word_far(tmpAddr);
                checksum.i16 += tmp.i8[0];
                checksum.i16 += tmp.i8[1];
                tmpAddr += 2;
            }
            msgBuffer[2] = checksum.i8[0];
            msgBuffer[3] = checksum.i8[1];
            msgLen.i16 = 4;
            msgBuffer[1] = STATUS_CMD_OK;
        }
        break;
//    case CMD_PROGRAM_EEPROM_ISP:
        //TODO
        break;
//    case CMD_READ_EEPROM_ISP:
        //TODO
        break;
//    case CMD_PROGRAM_FUSE_ISP:
    case CMD_READ_FUSE_ISP:
        if ( msgBuffer[2] == 0x50 )
        {
            if ( msgBuffer[3] == 0x08 )
                msgBuffer[2]	=	boot_lock_fuse_bits_get( GET_EXTENDED_FUSE_BITS );
            else
                msgBuffer[2]	=	boot_lock_fuse_bits_get( GET_LOW_FUSE_BITS );
        }
        else
        {
            msgBuffer[2]	=	boot_lock_fuse_bits_get( GET_HIGH_FUSE_BITS );
        }
        msgLen.i16		=	4;
        msgBuffer[1]	=	STATUS_CMD_OK;
        msgBuffer[3]	=	STATUS_CMD_OK;
        break;
    //case CMD_PROGRAM_LOCK_ISP:
    case CMD_READ_LOCK_ISP:
        msgLen.i16		=	4;
        msgBuffer[1]	=	STATUS_CMD_OK;
        msgBuffer[2]	=	boot_lock_fuse_bits_get( GET_LOCK_BITS );
        msgBuffer[3]	=	STATUS_CMD_OK;
        break;
    case CMD_READ_SIGNATURE_ISP:
        if ( msgBuffer[4] == 0 )
            msgBuffer[2] = (SIGNATURE_BYTES >>16) & 0x000000FF;
        else if ( msgBuffer[4] == 1 )
            msgBuffer[2] = (SIGNATURE_BYTES >> 8) & 0x000000FF;
        else
            msgBuffer[2] = SIGNATURE_BYTES & 0x000000FF;
        msgLen.i16      =	4;
        msgBuffer[1]	=	STATUS_CMD_OK;
        msgBuffer[3]	=	STATUS_CMD_OK;
        break;
//    case CMD_READ_OSCCAL_ISP:
    case CMD_SPI_MULTI:
        //Implement only the signature bytes reading for raw ISP commands, avrdude needs this.
        msgBuffer[5]	=	0;
        if ( msgBuffer[4] == 0x30 )
        {
            if ( msgBuffer[6] == 0 )
                msgBuffer[5]	=	(SIGNATURE_BYTES >>16) & 0x000000FF;
            else if ( msgBuffer[6] == 1 )
                msgBuffer[5]	=	(SIGNATURE_BYTES >> 8) & 0x000000FF;
            else
                msgBuffer[5]	=	SIGNATURE_BYTES & 0x000000FF;
        }
        
        msgLen.i16		=	7;
        msgBuffer[1]	=	STATUS_CMD_OK;
        msgBuffer[2]	=	0;
        msgBuffer[3]	=	msgBuffer[4];
        msgBuffer[4]	=	0;
        msgBuffer[6]	=	STATUS_CMD_OK;
        break;
    default:
        msgLen.i16 		=	2;
        msgBuffer[1] 	=	STATUS_CMD_FAILED;
        break;
    }
    SEND_SERIAL_DATA(MESSAGE_START);
    SEND_SERIAL_DATA(seq);
    SEND_SERIAL_DATA(msgLen.i8[1]);
    SEND_SERIAL_DATA(msgLen.i8[0]);
    SEND_SERIAL_DATA(TOKEN);
    checksum = MESSAGE_START ^ seq ^ (msgLen.i8[0]) ^ (msgLen.i8[1]) ^ (TOKEN);
    for(msgPos = 0; msgPos < msgLen.i16; msgPos++)
    {
        SEND_SERIAL_DATA(msgBuffer[msgPos]);
        checksum ^= msgBuffer[msgPos];
    }
    SEND_SERIAL_DATA(checksum);
    
    seq++;
}

void main()
{
    uint8_t recvState = STATE_START;
    uint8_t hasFirmware = (pgm_read_byte(0) == 0xFF);
    FATFS fat;
    
    lcd_init();
    if (hasFirmware)
        lcd_pstring(PSTR("No firmware found..."));
    else
        lcd_pstring(PSTR("Ultimaker starting.."));
        
    SET_PIN(BUTTON);//Enable the pull-up on the button input.
    
    //Setup the serial with 8n1, no interrupts and the configured baudrate.
    UCSR0A = _BV(U2X0);
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
    UBRR0H = BAUDRATE_DIVIDER() >> 8;
    UBRR0L = BAUDRATE_DIVIDER();
    
    if (MCUSR_backup & _BV(WDRF))
    {
        //If we had a hardware watchdog timeout, report that to the user and don't do anything else.
        //As a hardware watchdog error is a critical error and should not recover.
        lcd_clear();
        lcd_pstring(PSTR("Hardware watchdog"));
        lcd_set_pos(0x40);
        lcd_pstring(PSTR("timeout..."));
        while(1)
        {
            serial_send_pstring(PSTR("Error: Hardware watchdog timeout\n"));
        }
    }

    //Setup a bootloader timeout timer, use a 16bit timer. And wait for the overflow.
    //With a 1024 prescaler this gives a ~4.1 second timeout.
    //With a 256 prescaler this gives a ~1 second timeout.
    //TCCR1B = _BV(CS12) | _BV(CS10);//1024 prescaler
    TCCR1B = _BV(CS12);//256 prescaler
    TIFR1 = _BV(TOV1);
    TCNT1H = 0;
    TCNT1L = 0;
    
    while(!(TIFR1 & _BV(TOV1)))
    {
        if (SERIAL_DATA_AVAILABLE())
        {
            uint8_t data = RECV_SERIAL_DATA();
            
            checksum ^= data;
            switch(recvState)
            {
            case STATE_START:
                if (data == MESSAGE_START)
                {
                    recvState = STATE_SEQ;
                    checksum = MESSAGE_START;
                    //Reset the bootloader timeout
                    TCNT1H = 0;
                    TCNT1L = 0;
                }
                break;
            case STATE_SEQ:
                if ((data == 1) || (data == seq))
                {
                    seq = data;
                    recvState = STATE_SIZE_1;
                }else{
                    recvState = STATE_START;
                }
                break;
            case STATE_SIZE_1:
                msgLen.i8[1] = data;
                recvState = STATE_SIZE_2;
                break;
            case STATE_SIZE_2:
                msgLen.i8[0] = data;
                recvState = STATE_TOKEN;
                break;
            case STATE_TOKEN:
                if (data == TOKEN)
                {
                    recvState = STATE_DATA;
                    msgPos = 0;
                }else{
                    recvState = STATE_START;
                }
                break;
            case STATE_DATA:
                msgBuffer[msgPos++] = data;
                if (msgPos == msgLen.i16)
                    recvState = STATE_CHECK;
                break;
            case STATE_CHECK:
                if (checksum == 0)
                    handleMessage();
                recvState = STATE_START;
                break;
            }
        }
    }

    //Try the SD card to see if there is a firmware on it.
    if (pf_mount(&fat) == 0 && pf_open("/firmware.bin") == 0)
    {
        TCCR1B = _BV(CS12) | _BV(CS10);//1024 prescaler for 4 second timeout
        TIFR1 = _BV(TOV1);
        TCNT1H = 0;
        TCNT1L = 0;
        lcd_clear();
        lcd_pstring(PSTR("Press button to"));
        lcd_set_pos(0x40);
        lcd_pstring(PSTR("upgrade firmware"));
        while(!(TIFR1 & _BV(TOV1)))
        {
            if (hasFirmware || !GET_PIN(BUTTON))
            {
                lcd_clear();
                lcd_pstring(PSTR("Upgrading firmware"));
                
                uint8_t buffer[SPM_PAGESIZE];
                uint32_t address = 0;
                while(1)
                {
                    WORD len;
                    long oldAddress = address;
                    
                    pf_read(buffer, SPM_PAGESIZE, &len);
                    if (len == 0)
                        break;
                    lcd_set_pos(0x40 + address * 20L / fat.fsize);
                    lcd_send_8bit(0xFF);
                    //Protect the bootloader
                    if (address > FLASHEND - BOOTSIZE - SPM_PAGESIZE)
                        break;

                    boot_page_erase(address);	// Perform page erase
                    boot_spm_busy_wait();		// Wait until the memory is erased.
                    uint8_t* c = buffer;
                    do
                    {
                        union16t data;
                        data.i8[0] = *c++;
                        data.i8[1] = *c++;
                        boot_page_fill(address, data.i16);
                        address += 2;
                        len -= 2;
                    } while(len);
                    boot_page_write(oldAddress);
                    boot_spm_busy_wait();
                    boot_rww_enable();
                }
                lcd_clear();
                lcd_pstring(PSTR("Checking firmware"));
                //Reopen the firmware to reset the file read pointer
                pf_open("/firmware.bin");
                address = 0;
                while(1)
                {
                    WORD len;
                    
                    pf_read(buffer, SPM_PAGESIZE, &len);
                    if (len == 0)
                        break;
                    lcd_set_pos(0x40 + address * 20L / fat.fsize);
                    lcd_send_8bit(0xFF);
                    
                    uint8_t* c = buffer;
                    do
                    {
                        if (*c++ != pgm_read_byte_far(address))
                        {
                            lcd_clear();
                            lcd_pstring(PSTR("FAILED!"));
                            while(1);
                        }
                        address ++;
                        len --;
                    } while(len);
                }
                lcd_clear();
                lcd_pstring(PSTR("DONE!"));
                break;
            }
        }
    }
    
    //Jump to address 0x0000
	asm volatile(
			"clr	r30		\n\t"
			"clr	r31		\n\t"
			"ijmp	\n\t"
			);
}
