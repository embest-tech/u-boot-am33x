/*
 * am335x.c - TI's AM335x platform specific usb wrapper functions.
 *
 * Author: Ajay Kumar Gupta <ajay.gupta@ti.com>
 * Modified by Gene Zarkhin <gene_zarkhin@bose.com>
*
 * Based on drivers/usb/musb/da8xx.c
 *
 * Copyright (c) 2010 Texas Instruments Incorporated
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <common.h>

#include "am335x.h"

/* MUSB platform configuration */
struct musb_config musb_cfg = {
	.regs		= (struct musb_regs *)AM335X_USB_OTG_CORE_BASE,
	.timeout	= AM335X_USB_OTG_TIMEOUT,
	.musb_speed	= 0,
};

/*
 * Enable the USB phy
 */
static u8 phy_on(void)
{
	u32 timeout;
	u32 regAddr = CM_REGISTERS+USB_CTRL0_REG_OFFSET;
	u32 usb_ctrl_reg;

	usb_ctrl_reg = readl(regAddr);
	usb_ctrl_reg &= ~(CM_PHY_PWRDN | CM_PHY_OTG_PWRDN);
	usb_ctrl_reg |= (OTGVDET_EN | OTGSESSENDEN);
	writel(usb_ctrl_reg, regAddr);

	timeout = musb_cfg.timeout;
	writel(1, &am335x_usb_regs->ctrl);
	while(timeout--) {
		if((readl(&am335x_usb_regs->ctrl) & SOFT_RESET_BIT) == 0) {
			// select usb host interface
			writel(0, &am335x_usb_regs->mode);
			return 1;
			/* USB phy was not turned on */
		}
	}
	return 0;
}

/*
 * Disable the USB phy
 */
static void phy_off(void)
{
	u32 regAddr = CM_REGISTERS+USB_CTRL0_REG_OFFSET;
	u32 usb_ctrl_reg;

	usb_ctrl_reg = readl(regAddr);
	usb_ctrl_reg |= (CM_PHY_PWRDN | CM_PHY_OTG_PWRDN);
	writel(usb_ctrl_reg, regAddr);
}

/*
 * This function performs platform specific initialization for usb0.
 */
int musb_platform_init(void)
{
	u32 revision;

	/* start the on-chip usb phy and its pll */
	if (phy_on() == 0)
		return -1;

	/* Returns zero if e.g. not clocked */
	revision = readl(&am335x_usb_regs->revision);
	if (revision == 0)
		return -1;

	return 0;
}

/*
 * This function performs platform specific deinitialization for usb0.
 */
void musb_platform_deinit(void)
{
	/* Turn off the phy */
	phy_off();
}

/*
 * This function reads data from endpoint fifo for AM35x
 * which supports only 32bit read operation.
 *
 * ep           - endpoint number
 * length       - number of bytes to read from FIFO
 * fifo_data    - pointer to data buffer into which data is read
 */
__attribute__((weak))
void read_fifo(u8 ep, u32 length, void *fifo_data)
{
	u8  *data = (u8 *)fifo_data;
	u32 val;
	int i;

	/* select the endpoint index */
	writeb(ep, &musbr->index);

	if (length > 4) {
		for (i = 0; i < (length >> 2); i++) {
			val = readl(&musbr->fifox[ep]);
			memcpy(data, &val, 4);
			data += 4;
		}
		length %= 4;
	}
	if (length > 0) {
		val = readl(&musbr->fifox[ep]);
		memcpy(data, &val, length);
	}
}
