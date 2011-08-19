/*
 * Basic I2C functions
 *
 * Copyright (c) 2004 Texas Instruments
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Jian Zhang jzhang@ti.com, Texas Instruments
 *
 * Copyright (c) 2003 Wolfgang Denk, wd@denx.de
 * Rewritten to fit into the current U-Boot framework
 *
 * Adapted for OMAP2420 I2C, r-woodruff2@ti.com
 *
 */

#include <common.h>

#include <asm/arch/i2c.h>
#include <asm/io.h>

#define I2C_TIMEOUT	0xffff0000	/* Timeout mask for poll_i2c_irq() */

#define CHECK_NACK() \
	do {\
		if (tmp & (I2C_TIMEOUT | I2C_STAT_NACK)) {\
			writew(0, I2C_CON);\
			return(1);\
		}\
	} while (0)


static int wait_for_bus(void)
{
	int	stat, timeout;

	writew(0xffff, I2C_STAT);

	for (timeout = 0; timeout < 10; timeout++) {
		if (!((stat = readw(I2C_STAT)) & I2C_STAT_BB)) {
			writew(0xffff, I2C_STAT);
			return(0);
		}

		writew(stat, I2C_STAT);
		udelay(50000);
	}

	writew(0xffff, I2C_STAT);
	return(1);
}


static int poll_i2c_irq(int mask)
{
	int	stat, timeout;

	//mask |= (I2C_STAT_ROVR | I2C_STAT_XUDF | I2C_STAT_XRDY |
	//	I2C_STAT_RRDY | I2C_STAT_ARDY | I2C_STAT_NACK |
	//	I2C_STAT_AL);

	for (timeout = 0; timeout < 10; timeout++) {
		udelay(1000);
		stat = readw(I2C_STAT);
		if (stat & mask) {
			return(stat);
		}
	}

	writew(0xffff, I2C_STAT);
	return(stat | I2C_TIMEOUT);
}


static void flush_rx(void)
{
	int	dummy;

	while (1) {
		if (!(readw(I2C_STAT) & I2C_STAT_RRDY))
			break;

		dummy = readw(I2C_DATA);
		writew(I2C_STAT_RRDY, I2C_STAT);
		udelay(1000);
	}
}


void i2c_init(int speed, int slaveadd)
{
	int psc, fsscll, fssclh;
	int hsscll = 0, hssclh = 0;
	u32 scll, sclh, tmp, reg;

	/* Only handle standard, fast and high speeds */
	if ((speed != OMAP_I2C_STANDARD) &&
	    (speed != OMAP_I2C_FAST_MODE) &&
	    (speed != OMAP_I2C_HIGH_SPEED)) {
		return;
	}

	writew(readw(I2C_CON) &~I2C_CON_EN, I2C_CON);
	writew( I2C_SYST_RESET, I2C_SYSC);
	writew(I2C_CON_EN, I2C_CON);
	while(!(readw(I2C_SYSS) & I2C_SYST_RESETDONE))
		udelay(1000);

	reg = readw( I2C_SYSC);
	reg &= ~I2C_SYST_AUTOIDLE;
	reg |= I2C_SYST_NOIDLE;
	writew( reg , I2C_SYSC);

	if (readw(I2C_CON) & I2C_CON_EN) {
		writew(0, I2C_CON);
		udelay (50000);
	}

	psc = I2C_IP_CLK / I2C_INTERNAL_SAMPLING_CLK;
	psc -= 1;
	if (psc < I2C_PSC_MIN) {
		return;
	}

	/* Standard and fast speed */
	fsscll = fssclh = I2C_INTERNAL_SAMPLING_CLK / (2 * speed);

	fsscll -= I2C_FASTSPEED_SCLL_TRIM;
	fssclh -= I2C_FASTSPEED_SCLH_TRIM;
	if (((fsscll < 0) || (fssclh < 0)) ||
	    ((fsscll > 255) || (fssclh > 255))) {
		return;
	}

	scll = (unsigned int)fsscll;
	sclh = (unsigned int)fssclh;

	writew(psc, I2C_PSC);
	writew(scll, I2C_SCLL);
	writew(sclh, I2C_SCLH);

	/* own address */
	writew(slaveadd, I2C_OA);
	writew(I2C_CON_EN, I2C_CON);

	/* have to enable intrrupts or OMAP i2c module doesn't work */
	writew (I2C_IE_XRDY_IE | I2C_IE_RRDY_IE | I2C_IE_ARDY_IE |
		I2C_IE_NACK_IE | I2C_IE_AL_IE, I2C_IE);

	/* Now enable I2C controller (get it out of reset) */
	flush_rx();
	udelay(1000);

	writew(0xFFFF, I2C_STAT);
	writew(0, I2C_CNT);
}


int i2c_probe(u_int8_t chip)
{
	int	rc = 1, tmp;

	if (chip == readw(I2C_OA)) {
		return(rc);
	}

	//writew(0, I2C_CON);

	if (tmp = wait_for_bus()) {
		return(1);
	}

	/* try to read one byte from current (or only) address */
	writew(1, I2C_CNT);
	writew(chip, I2C_SA);
	writew((I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_STP), I2C_CON);
	udelay (50000);

	if (!(readw(I2C_STAT) & I2C_STAT_NACK)) {
		rc = 0;
		flush_rx();
		writew(0xffff, I2C_STAT);
	} else {
		writew(0xffff, I2C_STAT);
		writew(readw(I2C_CON) | I2C_CON_STP, I2C_CON);
		udelay(20000);
		if (wait_for_bus()) {return(1);}
	}

	flush_rx();
	writew(0xffff, I2C_STAT);
	writew(0, I2C_CNT);
	return(rc);
}


int i2c_read(u_int8_t chip, u_int32_t addr, int alen, u_int8_t *buf, int len)
{
	u_int32_t	tmp;
	int		i;

	if ((alen < 0) || (alen > 2)) {
		return(1);
	}

	if (tmp = wait_for_bus()) {
		return(1);
	}

	if (alen != 0) {
		/* Start address phase */
		tmp = I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX;
		writew(alen, I2C_CNT);
		writew(chip, I2C_SA);
		writew((readw(I2C_BUF) | (I2C_TXFIFO_CLEAR |I2C_RXFIFO_CLEAR)), I2C_BUF);
		writew(tmp, I2C_CON);

		tmp = poll_i2c_irq(I2C_STAT_XRDY | I2C_STAT_NACK);

		CHECK_NACK();

		switch (alen) {
			case 2:
				/* Send address MSByte */
				if (tmp & I2C_STAT_XRDY) {
					writew(((addr >> 8) & 0xff), I2C_DATA);
				} else {
					writew(0, I2C_CON);
					return(1);
				}

				tmp = poll_i2c_irq(I2C_STAT_XRDY | I2C_STAT_NACK);

				CHECK_NACK();
				/* No break, fall through */
			case 1:
				/* Send address LSByte */
				if (tmp & I2C_STAT_XRDY) {
					writew((addr & 0xff), I2C_DATA);
				} else {
					writew(0, I2C_CON);
					return(1);
				}

				//tmp = poll_i2c_irq(I2C_STAT_XRDY | I2C_STAT_NACK | I2C_STAT_ARDY);
				tmp = poll_i2c_irq(I2C_STAT_NACK | I2C_STAT_ARDY);

				CHECK_NACK();

				if (!(tmp & I2C_STAT_ARDY)) {
					writew(0, I2C_CON);
					return(1);
				}
		}
	}

	/* Address phase is over, now read 'len' bytes and stop */
	tmp = I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_STP;
	writew((len & 0xffff), I2C_CNT);
	writew((readw(I2C_BUF) | (I2C_TXFIFO_CLEAR |I2C_RXFIFO_CLEAR)), I2C_BUF);
	writew(chip, I2C_SA);
	writew(tmp, I2C_CON);

	for (i = 0; i < len; i++) {
		tmp = poll_i2c_irq(I2C_STAT_RRDY | I2C_STAT_NACK | I2C_STAT_ROVR);

		CHECK_NACK();

		if (tmp & I2C_STAT_RRDY) {
			buf[i] = readw(I2C_DATA);
		} else {
			writew(0, I2C_CON);
			return(1);
		}
	}

	tmp = poll_i2c_irq(I2C_STAT_ARDY | I2C_STAT_NACK);

	CHECK_NACK();

	if (!(tmp & I2C_STAT_ARDY)) {
		writew(0, I2C_CON);
		return(1);
	}

	flush_rx();
	writew(0xffff, I2C_STAT);
	writew(0, I2C_CNT);
	writew(0, I2C_CON);

	return(0);
}

int i2c_write (uchar chip, uint addr, int alen, uchar * buffer, int len)
{
	return 1;
}
