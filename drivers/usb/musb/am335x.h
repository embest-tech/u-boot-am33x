/*
 * am335x.h - TI's AM335x platform specific usb wrapper definitions.
 *
 * Author: Ajay Kumar Gupta <ajay.gupta@ti.com>
 * Modified by Gene Zarkhin <gene_zarkhin@bose.com>
 *
 * Based on drivers/usb/musb/da8xx.h
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

#ifndef __AM335X_USB_H__
#define __AM335X_USB_H__

#include "musb_core.h"

/* Control Module Registers */
#define CM_REGISTERS			(0x44e10000)
#define USB_CTRL0_REG_OFFSET	(0x620)
#define USB_CTRL1_REG_OFFSET	(0x680)

/* Base address of musb wrapper */
#define AM335X_USB_OTG_BASE	0x47401000

/* Base address of musb core */
#define AM335X_USB_OTG_CORE_BASE	(AM335X_USB_OTG_BASE + 0x400)

/* Timeout for AM35x usb module */
#define AM335X_USB_OTG_TIMEOUT	0x3FFFFFF

/*
 * AM335x platform USB wrapper register overlay.
 */
struct am335x_usb_regs {
	u32 revision;			// 0x00
	u32 reserved0[4];
	u32 ctrl;				// 0x14
	u32 status;				// 0x18
	u32 reserved1[1];
	u32 irqmstat;			// 0x20
	u32 irqeoi;				// 0x24
	u32 irqstatraw0;		// 0x28
	u32 irqstatraw1;		// 0x2c
	u32 irqstat0;			// 0x30
	u32 irqstat1;			// 0x34
	u32 irqenableset0;		// 0x38
	u32 irqenableset1;		// 0x3c
	u32 irqenableclr0;		// 0x40
	u32 irqenableclr1;		// 0x44
	u32 reserved2[11];
	u32 txmode;				// 0x70
	u32 rxmode;				// 0x74
	u32 reserved3[2];
	u32 genrndisep1;		// 0x80
	u32 genrndisep2;		// 0x84
	u32 genrndisep3;		// 0x88
	u32 genrndisep4;		// 0x8c
	u32 genrndisep5;		// 0x90
	u32 genrndisep6;		// 0x94
	u32 genrndisep7;		// 0x98
	u32 genrndisep8;		// 0x9c
	u32 genrndisep9;		// 0xa0
	u32 genrndisep10;		// 0xa4
	u32 genrndisep11;		// 0xa8
	u32 genrndisep12;		// 0xac
	u32 genrndisep13;		// 0xb0
	u32 genrndisep14;		// 0xb4
	u32 genrndisep15;		// 0xb8
	u32 reserved4[5];
	u32 autoreq;			// 0xd0
	u32 srpfixtime;			// 0xd4
	u32 tdown;				// 0xd8
	u32 reserved5[1];
	u32 utmi;				// 0xe0
	u32 utmilb;				// 0xe4
	u32 mode;				// 0xe8
};

#define am335x_usb_regs ((struct am335x_usb_regs *)AM335X_USB_OTG_BASE)

/* USB 2.0 PHY Control */
#define CM_PHY_PWRDN		(1 << 0)
#define CM_PHY_OTG_PWRDN	(1 << 1)
#define OTGVDET_EN			(1 << 19)
#define OTGSESSENDEN 		(1 << 20)

/* USB CTRL REG FIELDS */
#define SOFT_RESET_BIT		(1 << 0)


#endif	/* __AM335X_USB_H__ */
