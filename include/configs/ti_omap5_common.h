/*
 * (C) Copyright 2013
 * Texas Instruments Incorporated.
 * Sricharan R	  <r.sricharan@ti.com>
 *
 * Derived from OMAP4 done by:
 *	Aneesh V <aneesh@ti.com>
 *
 * TI OMAP5 AND DRA7XX common configuration settings
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 * For more details, please see the technical documents listed at
 * http://www.ti.com/product/omap5432
 */

#ifndef __CONFIG_TI_OMAP5_COMMON_H
#define __CONFIG_TI_OMAP5_COMMON_H

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO
#define CONFIG_ARCH_CPU_INIT

/* Common ARM Erratas */
#define CONFIG_ARM_ERRATA_798870

#define CONFIG_SYS_CACHELINE_SIZE	64

/* Use General purpose timer 1 */
#define CONFIG_SYS_TIMERBASE		GPT2_BASE

/*
 * For the DDR timing information we can either dynamically determine
 * the timings to use or use pre-determined timings (based on using the
 * dynamic method.  Default to the static timing infomation.
 */
#define CONFIG_SYS_EMIF_PRECALCULATED_TIMING_REGS
#ifndef CONFIG_SYS_EMIF_PRECALCULATED_TIMING_REGS
#define CONFIG_SYS_AUTOMATIC_SDRAM_DETECTION
#define CONFIG_SYS_DEFAULT_LPDDR2_TIMINGS
#endif

#ifndef CONFIG_SPL_BUILD
#define CONFIG_PALMAS_POWER
#endif

#include <asm/arch/cpu.h>
#include <asm/arch/omap.h>

#include <configs/ti_armv7_omap.h>

#ifndef CONFIG_CM_T54
/* USB GADGET */
#define CONFIG_USB_DWC3_PHY_OMAP
#define CONFIG_USB_DWC3_OMAP
#define CONFIG_USB_DWC3
#define CONFIG_USB_DWC3_GADGET

#define CONFIG_USB_GADGET
#define CONFIG_USBDOWNLOAD_GADGET
#define CONFIG_USB_GADGET_VBUS_DRAW 2
#define CONFIG_G_DNL_MANUFACTURER "Texas Instruments"
#define CONFIG_G_DNL_VENDOR_NUM 0x0403
#define CONFIG_G_DNL_PRODUCT_NUM 0xBD00
#define CONFIG_USB_GADGET_DUALSPEED

/* USB Device Firmware Update support */
#define CONFIG_DFU_FUNCTION
#define CONFIG_DFU_RAM
#define CONFIG_CMD_DFU

#define CONFIG_DFU_MMC
#endif

/*
 * Hardware drivers
 */
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	(-4)
#define CONFIG_SYS_NS16550_CLK		48000000

/*
 * Environment setup
 */
#ifndef PARTS_DEFAULT
#define PARTS_DEFAULT
#endif


#define DFU_ALT_INFO_MMC \
	"dfu_alt_info_mmc=" \
	"boot part 0 1;" \
	"rootfs part 0 2;" \
	"MLO fat 0 1;" \
	"MLO.raw raw 0x100 0x100;" \
	"u-boot.img.raw raw 0x300 0x400;" \
	"spl-os-args.raw raw 0x80 0x80;" \
	"spl-os-image.raw raw 0x900 0x2000;" \
	"spl-os-args fat 0 1;" \
	"spl-os-image fat 0 1;" \
	"u-boot.img fat 0 1;" \
	"uEnv.txt fat 0 1\0"

#define DFU_ALT_INFO_EMMC \
	"dfu_alt_info_emmc=" \
	"rawemmc raw 0 3751936;" \
	"boot part 1 1;" \
	"rootfs part 1 2;" \
	"MLO fat 1 1;" \
	"MLO.raw raw 0x100 0x100;" \
	"u-boot.img.raw raw 0x300 0x400;" \
	"spl-os-args.raw raw 0x80 0x80;" \
	"spl-os-image.raw raw 0x900 0x2000;" \
	"spl-os-args fat 1 1;" \
	"spl-os-image fat 1 1;" \
	"u-boot.img fat 1 1;" \
	"uEnv.txt fat 1 1\0"

#define DFU_ALT_INFO_RAM \
	"dfu_alt_info_ram=" \
	"kernel ram 0x80200000 0x4000000;" \
	"fdt ram 0x80f80000 0x80000;" \
	"ramdisk ram 0x81000000 0x4000000\0"

#define DFU_ALT_INFO_QSPI \
	"dfu_alt_info_qspi=" \
	"MLO raw 0x0 0x010000;" \
	"MLO.backup1 raw 0x010000 0x010000;" \
	"MLO.backup2 raw 0x020000 0x010000;" \
	"MLO.backup3 raw 0x030000 0x010000;" \
	"u-boot.img raw 0x040000 0x0100000;" \
	"u-boot-spl-os raw 0x140000 0x080000;" \
	"u-boot-env raw 0x1C0000 0x010000;" \
	"u-boot-env.backup raw 0x1D0000 0x010000;" \
	"kernel raw 0x1E0000 0x800000\0"

#define DFUARGS \
	"dfu_bufsiz=0x10000\0" \
	DFU_ALT_INFO_MMC \
	DFU_ALT_INFO_EMMC \
	DFU_ALT_INFO_RAM \
	DFU_ALT_INFO_QSPI

#ifndef CONFIG_SPL_BUILD
#define CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
#define CONFIG_EXTRA_ENV_SETTINGS \
	DEFAULT_LINUX_BOOT_ENV \
	DEFAULT_MMC_TI_ARGS \
	"console=" CONSOLEDEV ",115200n8\0" \
	"fdtfile=undefined\0" \
	"bootpart=0:2\0" \
	"bootdir=/boot\0" \
	"bootfile=zImage\0" \
	"usbtty=cdc_acm\0" \
	"vram=16M\0" \
	"partitions=" PARTS_DEFAULT "\0" \
	"optargs=\0" \
	"dofastboot=0\0" \
	"loadbootscript=fatload mmc ${mmcdev} ${loadaddr} boot.scr\0" \
	"bootscript=echo Running bootscript from mmc${mmcdev} ...; " \
		"source ${loadaddr}\0" \
	"loadimage=load mmc ${bootpart} ${loadaddr} ${bootdir}/${bootfile}\0" \
	"mmcboot=mmc dev ${mmcdev}; " \
		"if mmc rescan; then " \
			"echo SD/MMC found on device ${mmcdev};" \
			"if run loadimage; then " \
				"run loadfdt; " \
				"echo Booting from mmc${mmcdev} ...; " \
				"run args_mmc; " \
				"bootz ${loadaddr} - ${fdtaddr}; " \
			"fi;" \
		"fi;\0" \
	"findfdt="\
		"if test $board_name = omap5_uevm; then " \
			"setenv fdtfile omap5-uevm.dtb; fi; " \
		"if test $board_name = dra7xx; then " \
			"setenv fdtfile dra7-evm.dtb; fi;" \
		"if test $board_name = dra72x; then " \
			"setenv fdtfile dra72-evm.dtb; fi;" \
		"if test $board_name = beagle_x15; then " \
			"setenv fdtfile am57xx-beagle-x15.dtb; fi;" \
		"if test $board_name = am57xx_evm; then " \
			"setenv fdtfile am57xx-evm.dtb; fi;" \
		"if test $board_name = am572x_idk; then " \
			"setenv fdtfile am572x-idk.dtb; fi;" \
		"if test $board_name = am571x_idk; then " \
			"setenv fdtfile am571x-idk.dtb; fi;" \
		"if test $board_name = am572x_idk_osd; then " \
			"setenv fdtfile am572x-idk-lcd-osd.dtb; fi;" \
		"if test $board_name = am571x_idk_osd; then " \
			"setenv fdtfile am571x-idk-lcd-osd.dtb; fi;" \
		"if test $fdtfile = undefined; then " \
			"echo WARNING: Could not determine device tree to use; fi; \0" \
	"loadfdt=load mmc ${bootpart} ${fdtaddr} ${bootdir}/${fdtfile};\0" \
	DFUARGS \
	NETARGS \


#define CONFIG_BOOTCOMMAND \
	"if test ${dofastboot} -eq 1; then " \
		"echo Boot fastboot requested, resetting dofastboot ...;" \
		"setenv dofastboot 0; saveenv;" \
		"echo Booting into fastboot ...; fastboot;" \
	"fi;" \
	"run findfdt; " \
	"run envboot; " \
	"run mmcboot;" \
	"setenv mmcdev 1; " \
	"setenv bootpart 1:2; " \
	"setenv mmcroot /dev/mmcblk0p2 rw; " \
	"run mmcboot;" \
	""
#endif


/*
 * SPL related defines.  The Public RAM memory map the ROM defines the
 * area between 0x40300000 and 0x4031E000 as a download area for OMAP5
 * (dra7xx is larger, but we do not need to be larger at this time).  We
 * set CONFIG_SPL_DISPLAY_PRINT to have omap_rev_string() called and
 * print some information.
 */
#define CONFIG_SPL_TEXT_BASE		0x40300000
#define CONFIG_SPL_MAX_SIZE		(0x4031E000 - CONFIG_SPL_TEXT_BASE)
#define CONFIG_SPL_DISPLAY_PRINT
#define CONFIG_SPL_LDSCRIPT "$(CPUDIR)/omap-common/u-boot-spl.lds"
#define CONFIG_SYS_SPL_ARGS_ADDR	(CONFIG_SYS_SDRAM_BASE + \
					 (128 << 20))

#ifdef CONFIG_NAND
#define CONFIG_SPL_NAND_AM33XX_BCH	/* ELM support */
#endif

#endif /* __CONFIG_TI_OMAP5_COMMON_H */
