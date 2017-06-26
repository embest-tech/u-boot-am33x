/*
 * TI GMPC driver
 *
 * (C) Copyright 2015
 * Texas Instruments, <www.ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/ti-common/ti-gpmc.h>
#include <linux/mtd/omap_gpmc.h>

struct gpmc *gpmc_cfg;

/*****************************************************
 * omap_gpmc_init(): init gpmc bus
 * Init GPMC for x16, MuxMode (SDRAM in x32).
 * This code can only be executed from SRAM or SDRAM.
 *****************************************************/
/* FIXME: for now we only support one CS */
void ti_gpmc_init(struct ti_gpmc_config *cfg)
{
	/* putting a blanket check on GPMC based on ZeBu for now */
	gpmc_cfg = (struct gpmc *)TI_GPMC_BASE;
#if defined(CONFIG_NOR)
	u32 base = CONFIG_SYS_FLASH_BASE;
	u32 size =	(CONFIG_SYS_FLASH_SIZE  > 0x08000000) ? GPMC_SIZE_256M :
	/* > 64MB */	((CONFIG_SYS_FLASH_SIZE > 0x04000000) ? GPMC_SIZE_128M :
	/* > 32MB */	((CONFIG_SYS_FLASH_SIZE > 0x02000000) ? GPMC_SIZE_64M  :
	/* > 16MB */	((CONFIG_SYS_FLASH_SIZE > 0x01000000) ? GPMC_SIZE_32M  :
	/* min 16MB */	GPMC_SIZE_16M)));
#elif defined(CONFIG_NAND) || defined(CONFIG_CMD_NAND)
/* configure GPMC for NAND */
	u32 base = CONFIG_SYS_NAND_BASE;
	u32 size = GPMC_SIZE_16M;
#elif defined(CONFIG_CMD_ONENAND)
	u32 size = GPMC_SIZE_128M;
	u32 base = CONFIG_SYS_ONENAND_BASE;
#else
	u32 size = 0;
	u32 base = 0;
#endif
	/* global settings */
	writel(0x00000008, &gpmc_cfg->sysconfig);
	writel(0x00000000, &gpmc_cfg->irqstatus);
	writel(0x00000000, &gpmc_cfg->irqenable);
	/* disable timeout, set a safe reset value */
	writel(0x00001ff0, &gpmc_cfg->timeout_control);
#ifdef CONFIG_NOR
	writel(0x00000200, &gpmc_cfg->config);
#else
	writel(0x00000012, &gpmc_cfg->config);
#endif
	/*
	 * Disable the GPMC0 config set by ROM code
	 */
	writel(0, &gpmc_cfg->cs[0].config7);
	sdelay(1000);
	/* enable chip-select specific configurations */
	if (base != 0) {
		writel(0, &gpmc_cfg->cs[0].config7);
		sdelay(1000);
		/* Delay for settling */
		writel(cfg->config1, &gpmc_cfg->cs[0].config1);
		writel(cfg->config2, &gpmc_cfg->cs[0].config2);
		writel(cfg->config3, &gpmc_cfg->cs[0].config3);
		writel(cfg->config4, &gpmc_cfg->cs[0].config4);
		writel(cfg->config5, &gpmc_cfg->cs[0].config5);
		writel(cfg->config6, &gpmc_cfg->cs[0].config6);
		/* Enable the config */
		writel((((size & 0xF) << 8) | ((base >> 24) & 0x3F) |
				(1 << 6)), &gpmc_cfg->cs[0].config7);
		sdelay(2000);
	}
}
