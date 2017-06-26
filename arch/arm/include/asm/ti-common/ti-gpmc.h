/*
 * TI GMPC driver
 *
 * (C) Copyright 2015
 * Texas Instruments, <www.ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef _TI_GPMC_H_
#define _TI_GPMC_H_

#define GPMC_SIZE_256M          0x0
#define GPMC_SIZE_128M          0x8
#define GPMC_SIZE_64M           0xC
#define GPMC_SIZE_32M           0xE
#define GPMC_SIZE_16M           0xF

#define M_NAND_GPMC_CONFIG1     0x00000800
#define M_NAND_GPMC_CONFIG2     0x001e1e00
#define M_NAND_GPMC_CONFIG3     0x001e1e00
#define M_NAND_GPMC_CONFIG4     0x16051807
#define M_NAND_GPMC_CONFIG5     0x00151e1e
#define M_NAND_GPMC_CONFIG6     0x16000f80
#define M_NAND_GPMC_CONFIG7     0x00000008

#define STNOR_GPMC_CONFIG1      0x00001000
#define STNOR_GPMC_CONFIG2      0x001f1f00
#define STNOR_GPMC_CONFIG3      0x001f1f01
#define STNOR_GPMC_CONFIG4      0x1f011f01
#define STNOR_GPMC_CONFIG5      0x001d1f1f
#define STNOR_GPMC_CONFIG6      0x08070280
#define STNOR_GPMC_CONFIG7      0x00000048

/* max number of GPMC Chip Selects */
#define GPMC_MAX_CS             8
/* max number of GPMC regs */
#define GPMC_MAX_REG            7

struct ti_gpmc_config {
	u32 config1;
	u32 config2;
	u32 config3;
	u32 config4;
	u32 config5;
	u32 config6;
};

void ti_gpmc_init(struct ti_gpmc_config *cfg);

#endif
