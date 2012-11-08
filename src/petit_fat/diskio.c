/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for Petit FatFs (C)ChaN, 2009      */
/*-----------------------------------------------------------------------*/
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#include "diskio.h"

//Chip select
#define SD_CS_PORT B
#define SD_CS_BIT  0

//Card detect
#define SD_CD_PORT D
#define SD_CD_BIT  7

#define _MAKE_PORT(name, suffix) name ## suffix
#define MAKE_PORT(name, suffix) _MAKE_PORT(name, suffix)
#define CLR_BIT(name) do { MAKE_PORT(PORT, SD_ ## name ## _PORT) &=~_BV(SD_ ## name ## _BIT); } while(0)
#define SET_BIT(name) do { MAKE_PORT(PORT, SD_ ## name ## _PORT) |= _BV(SD_ ## name ## _BIT); } while(0)

#define SELECT_SD() CLR_BIT(CS)
#define UNSELECT_SD() SET_BIT(CS)

void init_spi()
{
    //Set the ChipSelect to output, and the CardDetect to input will pullup.
    MAKE_PORT(DDR, SD_CS_PORT) |= _BV(SD_CS_BIT);
    MAKE_PORT(PORT, SD_CD_PORT) |= _BV(SD_CD_BIT);
    //Set the SPI pins to outputs
    DDRB |= _BV(1);
    DDRB |= _BV(2);
    UNSELECT_SD();
    
    SPCR = _BV(SPE) | _BV(MSTR);
    SPSR = _BV(SPI2X);
}

void xmit_spi(BYTE d)		/* Send a byte to the SD */
{
    SPDR = d;
    while (!(SPSR & (1 << SPIF)));
}
BYTE rcv_spi (void)		/* Send a 0xFF to the SD and get the received byte */
{
    SPDR = 0XFF;
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}

static BYTE CardType;

/* Definitions for MMC/SDC command */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND (MMC) */
#define	ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */


/* Card type flags (CardType) */
#define CT_MMC				0x01	/* MMC ver 3 */
#define CT_SD1				0x02	/* SD ver 1 */
#define CT_SD2				0x04	/* SD ver 2 */
#define CT_BLOCK			0x08	/* Block addressing */

static BYTE send_cmd (
    BYTE cmd,		/* 1st byte (Start + Index) */
    DWORD arg		/* Argument (32 bits) */)
{
	BYTE n, res;

	if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card */
	SELECT_SD();

	/* Send a command packet */
	xmit_spi(cmd);						/* Start + Command index */
	xmit_spi((BYTE)(arg >> 24));		/* Argument[31..24] */
	xmit_spi((BYTE)(arg >> 16));		/* Argument[23..16] */
	xmit_spi((BYTE)(arg >> 8));			/* Argument[15..8] */
	xmit_spi((BYTE)arg);				/* Argument[7..0] */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	xmit_spi(n);

	/* Receive a command response */
	n = 0xFF;								/* Wait for a valid response in timeout of 255 attempts */
	do {
		res = rcv_spi();
	} while ((res & 0x80) && --n);

	return res;			/* Return with the response value */
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (void)
{
	BYTE n, cmd, ty, ocr[4];
	UINT tmr;

	init_spi();							/* Initialize ports to control MMC */
	if (MAKE_PORT(PIN, SD_CD_PORT) & _BV(SD_CD_BIT))
	{
        //If the CardDetect pin is high then we don't have an SD card.
        return STA_NOINIT;
	}
	_delay_ms(100);
	for (n = 10; n; n--) UNSELECT_SD();	/* 80 Dummy clocks with CS=H */
	for (n = 10; n; n--) xmit_spi(0xFF);

	ty = 0;
	if (send_cmd(CMD0, 0) == 1) {			/* Enter Idle state */
		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2 */
			for (n = 0; n < 4; n++) ocr[n] = rcv_spi();		/* Get trailing return value of R7 resp */
			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {			/* The card can work at vdd range of 2.7-3.6V */
				for (tmr = 10000; tmr && send_cmd(ACMD41, 1UL << 30); tmr--) _delay_us(100);	/* Wait for leaving idle state (ACMD41 with HCS bit) */
				if (tmr && send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
					for (n = 0; n < 4; n++) ocr[n] = rcv_spi();
					ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 (HC or SC) */
				}
			}
		} else {							/* SDv1 or MMCv3 */
			if (send_cmd(ACMD41, 0) <= 1) 	{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}
			for (tmr = 10000; tmr && send_cmd(cmd, 0); tmr--) _delay_us(100);	/* Wait for leaving idle state */
			if (!tmr || send_cmd(CMD16, 512) != 0)			/* Set R/W block length to 512 */
				ty = 0;
		}
	}
	CardType = ty;
	UNSELECT_SD();

	return ty ? 0 : STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Partial Sector                                                   */
/*-----------------------------------------------------------------------*/
BYTE cacheBlock[514];
DWORD cacheLBA = -1;

DRESULT disk_readp (
	BYTE* buff,			/* Pointer to the destination object */
	DWORD lba,		/* Sector number (LBA) */
	WORD ofs,			/* Offset in the sector */
	WORD cnt			/* Byte count (bit15:destination) */
)
{
	DRESULT res;
	BYTE rc;
	WORD bc;


	if (!(CardType & CT_BLOCK)) lba *= 512;		/* Convert to byte address if needed */
	if (cacheLBA == lba)
	{
        memcpy(buff, cacheBlock + ofs, cnt);
	}

	res = RES_ERROR;
	if (send_cmd(CMD17, lba) == 0) {		/* READ_SINGLE_BLOCK */
        BYTE* cb = cacheBlock;
		bc = 40000;
		do {							/* Wait for data packet */
			rc = rcv_spi();
		} while (rc == 0xFF && --bc);

		if (rc == 0xFE) {				/* A data packet arrived */
			bc = 514 - ofs - cnt;

			/* Skip leading bytes */
			if (ofs) {
				do *cb++ = rcv_spi(); while (--ofs);
			}

			/* Receive a part of the sector */
			do {
				*cb++ = *buff++ = rcv_spi();
			} while (--cnt);

			/* Skip trailing bytes and CRC */
			do *cb++ = rcv_spi(); while (--bc);

			res = RES_OK;
		}
	}

	UNSELECT_SD();

	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Partial Sector                                                  */
/*-----------------------------------------------------------------------*/
#if _USE_WRITE
DRESULT disk_writep (
	const BYTE* buff,		/* Pointer to the data to be written, NULL:Initiate/Finalize write operation */
	DWORD sc		/* Sector number (LBA) or Number of bytes to send */
)
{
	DRESULT res;


	if (!buff) {
		if (sc) {

			// Initiate write process

		} else {

			// Finalize write process

		}
	} else {

		// Send data to the disk

	}

	return res;
}
#endif
