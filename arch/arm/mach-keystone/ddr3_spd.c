/*
 * Keystone2: DDR3 SPD configuration
 *
 * (C) Copyright 2015
 *     Texas Instruments Incorporated, <www.ti.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>

#include <i2c.h>
#include <ddr_spd.h>
#include <asm/arch/ddr3.h>
#include <asm/arch/hardware.h>

#ifdef DUMP_DDR_CONFIG
static void dump_phy_config(struct ddr3_phy_config *ptr)
{
	printf("\npllcr		0x%08X\n", ptr->pllcr);
	printf("pgcr1_mask	0x%08X\n", ptr->pgcr1_mask);
	printf("pgcr1_val	0x%08X\n", ptr->pgcr1_val);
	printf("ptr0		0x%08X\n", ptr->ptr0);
	printf("ptr1		0x%08X\n", ptr->ptr1);
	printf("ptr2		0x%08X\n", ptr->ptr2);
	printf("ptr3		0x%08X\n", ptr->ptr3);
	printf("ptr4		0x%08X\n", ptr->ptr4);
	printf("dcr_mask	0x%08X\n", ptr->dcr_mask);
	printf("dcr_val		0x%08X\n", ptr->dcr_val);
	printf("dtpr0		0x%08X\n", ptr->dtpr0);
	printf("dtpr1		0x%08X\n", ptr->dtpr1);
	printf("dtpr2		0x%08X\n", ptr->dtpr2);
	printf("mr0		0x%08X\n", ptr->mr0);
	printf("mr1		0x%08X\n", ptr->mr1);
	printf("mr2		0x%08X\n", ptr->mr2);
	printf("dtcr		0x%08X\n", ptr->dtcr);
	printf("pgcr2		0x%08X\n", ptr->pgcr2);
	printf("zq0cr1		0x%08X\n", ptr->zq0cr1);
	printf("zq1cr1		0x%08X\n", ptr->zq1cr1);
	printf("zq2cr1		0x%08X\n", ptr->zq2cr1);
	printf("pir_v1		0x%08X\n", ptr->pir_v1);
	printf("pir_v2		0x%08X\n\n", ptr->pir_v2);
};

static void dump_emif_config(struct ddr3_emif_config *ptr)
{
	printf("\nsdcfg		0x%08X\n", ptr->sdcfg);
	printf("sdtim1		0x%08X\n", ptr->sdtim1);
	printf("sdtim2		0x%08X\n", ptr->sdtim2);
	printf("sdtim3		0x%08X\n", ptr->sdtim3);
	printf("sdtim4		0x%08X\n", ptr->sdtim4);
	printf("zqcfg		0x%08X\n", ptr->zqcfg);
	printf("sdrfc		0x%08X\n\n", ptr->sdrfc);
};
#endif

#define TEMP NORMAL_TEMP
#define VBUS_CLKPERIOD 1.875 /* Corresponds to vbus=533MHz, */
#define PLLGS_VAL	(4000.0 / VBUS_CLKPERIOD) /* 4 us */
#define PLLPD_VAL	(1000.0 / VBUS_CLKPERIOD) /* 1 us */
#define PLLLOCK_VAL	(100000.0 / VBUS_CLKPERIOD) /* 100 us */
#define PLLRST_VAL	(9000.0 / VBUS_CLKPERIOD) /* 9 us */
#define PHYRST_VAL	0x10
#define DDR_TERM RZQ_4_TERM
#define SDRAM_DRIVE RZQ_7_IMP
#define DYN_ODT ODT_DISABLE

enum srt {
	NORMAL_TEMP,
	EXTENDED_TEMP
};

enum out_impedance {
	RZQ_6_IMP = 0,
	RZQ_7_IMP
};

enum die_term {
	ODT_DISABLE = 0,
	RZQ_4_TERM,
	RZQ_2_TERM,
	RZQ_6_TERM,
	RZQ_12_TERM,
	RZQ_8_TERM
};

struct ddr3_sodimm {
	uint32_t t_ck;
	uint32_t freqsel;
	uint32_t t_xp;
	uint32_t t_cke;
	uint32_t t_pllpd;
	uint32_t t_pllgs;
	uint32_t t_phyrst;
	uint32_t t_plllock;
	uint32_t t_pllrst;
	uint32_t t_rfc;
	uint32_t t_xs;
	uint32_t t_dinit0;
	uint32_t t_dinit1;
	uint32_t t_dinit2;
	uint32_t t_dinit3;
	uint32_t t_rtp;
	uint32_t t_wtr;
	uint32_t t_rp;
	uint32_t t_rcd;
	uint32_t t_ras;
	uint32_t t_rrd;
	uint32_t t_rc;
	uint32_t t_faw;
	uint32_t t_mrd;
	uint32_t t_mod;
	uint32_t t_wlo;
	uint32_t t_wlmrd;
	uint32_t t_xsdll;
	uint32_t t_xpdll;
	uint32_t t_ckesr;
	uint32_t t_dllk;
	uint32_t t_wr;
	uint32_t t_wr_bin;
	uint32_t cas;
	uint32_t cwl;
	uint32_t asr;
	uint32_t pasr;
	uint32_t t_refprd;
	uint8_t sdram_type;
	uint8_t ibank;
	uint8_t pagesize;
	uint8_t t_rrd2;
	uint8_t t_ras_max;
	uint8_t t_zqcs;
	uint32_t refresh_rate;
	uint8_t t_csta;

	uint8_t rank;
	uint8_t mirrored;
	uint8_t buswidth;
};

static uint8_t cas_latancy(uint16_t temp)
{
	int loop;
	uint8_t cas_bin = 0;

	for (loop = 0; loop < 32; loop += 2, temp >>= 1) {
		if (temp & 0x0001)
			cas_bin = (loop > 15) ? loop - 15 : loop;
	}

	return cas_bin;
}

static int ddr3_get_size_in_mb(ddr3_spd_eeprom_t *buf)
{
	return (((buf->organization & 0x38) >> 3) + 1) *
		(256 << (buf->density_banks & 0xf));
}

static int ddrtimingcalculation(ddr3_spd_eeprom_t *buf, struct ddr3_sodimm *spd,
				struct ddr3_spd_cb *spd_cb)
{
	uint32_t mtb, clk_freq;

	if ((buf->mem_type != 0x0b) ||
	    ((buf->density_banks & 0x70) != 0x00))
		return 1;

	spd->sdram_type = 0x03;
	spd->ibank = 0x03;

	mtb = buf->mtb_dividend * 1000 / buf->mtb_divisor;

	spd->t_ck = buf->tck_min * mtb;

	spd_cb->ddrspdclock = 2000000 / spd->t_ck;
	clk_freq = spd_cb->ddrspdclock / 2;

	spd->rank = ((buf->organization & 0x38) >> 3) + 1;
	if (spd->rank > 2)
		return 1;

	spd->pagesize = (buf->addressing & 0x07) + 1;
	if (spd->pagesize > 3)
		return 1;

	spd->buswidth = 8 << (buf->bus_width & 0x7);
	if ((spd->buswidth < 16) || (spd->buswidth > 64))
		return 1;

	spd->mirrored = buf->mod_section.unbuffered.addr_mapping & 1;

	if (spd_cb->ddrspdclock == 1333) {
		puts("DDR3A Speed will be configured for 1333 Operation.\n");

		spd->t_xp = ((3 * spd->t_ck) > 6000) ?
			3 : ((5999 / spd->t_ck) + 1);
		spd->t_cke = ((3 * spd->t_ck) > 5625) ?
			3 : ((5624 / spd->t_ck) + 1);
	} else if (spd_cb->ddrspdclock == 1600) {
		puts("DDR3A Speed will be configured for 1600 Operation.\n");

		spd->t_xp = ((3 * spd->t_ck) > 6000) ?
			3 : ((5999 / spd->t_ck) + 1);
		spd->t_cke = ((3 * spd->t_ck) > 5000) ?
			3 : ((4999 / spd->t_ck) + 1);
	} else {
		printf("Unsupported DDR3 speed %d\n", spd_cb->ddrspdclock);
		return 1;
	}

	spd->t_xpdll = (spd->t_ck > 2400) ? 10 : 24000 / spd->t_ck;
	spd->t_ckesr = spd->t_cke + 1;

	/* SPD Calculated Values */
	spd->cas = cas_latancy((buf->caslat_msb << 8) |
			       buf->caslat_lsb);

	spd->t_wr = (buf->twr_min * mtb) / spd->t_ck;
	spd->t_wr_bin = (spd->t_wr / 2) & 0x07;

	spd->t_rcd = ((buf->trcd_min * mtb) - 1) / spd->t_ck + 1;
	spd->t_rrd = ((buf->trrd_min * mtb) - 1) / spd->t_ck + 1;
	spd->t_rp  = (((buf->trp_min * mtb) - 1) / spd->t_ck) + 1;

	spd->t_ras = (((buf->tras_trc_ext & 0x0f) << 8 | buf->tras_min_lsb) *
		      mtb) / spd->t_ck;

	spd->t_rc = (((((buf->tras_trc_ext & 0xf0) << 4) | buf->trc_min_lsb) *
		      mtb) - 1) / spd->t_ck + 1;

	spd->t_rfc = (buf->trfc_min_lsb | (buf->trfc_min_msb << 8)) * mtb /
		1000;
	spd->t_wtr = (buf->twtr_min * mtb) / spd->t_ck;
	spd->t_rtp = (buf->trtp_min * mtb) / spd->t_ck;

	spd->t_xs  = (((spd->t_rfc + 10) * 1000) / spd->t_ck);
	spd->t_rfc = ((spd->t_rfc * 1000) - 1) / spd->t_ck + 1;

	spd->t_faw = (((buf->tfaw_msb << 8) | buf->tfaw_min) * mtb) / spd->t_ck;
	spd->t_rrd2 = ((((buf->tfaw_msb << 8) |
			 buf->tfaw_min) * mtb) / (4 * spd->t_ck)) - 1;

	/* Hard-coded values */
	spd->t_mrd = 0x00;
	spd->t_mod = 0x00;
	spd->t_wlo = 0x0C;
	spd->t_wlmrd = 0x28;
	spd->t_xsdll = 0x200;
	spd->t_ras_max = 0x0F;
	spd->t_csta = 0x05;
	spd->t_dllk = 0x200;

	/* CAS Write Latency */
	if (spd->t_ck >= 2500)
		spd->cwl = 0;
	else if (spd->t_ck >= 1875)
		spd->cwl = 1;
	else if (spd->t_ck >= 1500)
		spd->cwl = 2;
	else if (spd->t_ck >= 1250)
		spd->cwl = 3;
	else if (spd->t_ck >= 1071)
		spd->cwl = 4;
	else
		spd->cwl = 5;

	/* SD:RAM Thermal and Refresh Options */
	spd->asr = (buf->therm_ref_opt & 0x04) >> 2;
	spd->pasr = (buf->therm_ref_opt & 0x80) >> 7;
	spd->t_zqcs = 64;

	spd->t_refprd = (TEMP == NORMAL_TEMP) ? 7812500 : 3906250;
	spd->t_refprd = spd->t_refprd / spd->t_ck;

	spd->refresh_rate = spd->t_refprd;
	spd->t_refprd = spd->t_refprd * 5;

	/* Set MISC PHY space registers fields */
	if ((clk_freq / 2) >= 166 && (clk_freq / 2 < 275))
		spd->freqsel = 0x03;
	else if ((clk_freq / 2) > 225 && (clk_freq / 2 < 385))
		spd->freqsel = 0x01;
	else if ((clk_freq / 2) > 335 && (clk_freq / 2 < 534))
		spd->freqsel = 0x00;

	spd->t_dinit0 = 500000000 / spd->t_ck; /* CKE low time 500 us */
	spd->t_dinit1 = spd->t_xs;
	spd->t_dinit2 = 200000000 / spd->t_ck; /* Reset low time 200 us */
	/* Time from ZQ initialization command to first command (1 us) */
	spd->t_dinit3 =  1000000 / spd->t_ck;

	spd->t_pllgs = PLLGS_VAL + 1;
	spd->t_pllpd = PLLPD_VAL + 1;
	spd->t_plllock = PLLLOCK_VAL + 1;
	spd->t_pllrst = PLLRST_VAL;
	spd->t_phyrst = PHYRST_VAL;

	spd_cb->ddr_size_gbyte = ddr3_get_size_in_mb(buf) / 1024;

	return 0;
}

static void init_ddr3param(struct ddr3_spd_cb *spd_cb,
			   struct ddr3_sodimm *spd)
{
	spd_cb->phy_cfg.pllcr = (spd->freqsel & 3) << 18 | 0xE << 13;
	spd_cb->phy_cfg.pgcr1_mask = (IODDRM_MASK | ZCKSEL_MASK);
	spd_cb->phy_cfg.pgcr1_val = ((1 << 2) | (1 << 7) | (1 << 23));
	spd_cb->phy_cfg.ptr0 = ((spd->t_pllpd & 0x7ff) << 21) |
		((spd->t_pllgs & 0x7fff) << 6) | (spd->t_phyrst & 0x3f);
	spd_cb->phy_cfg.ptr1 = ((spd->t_plllock & 0xffff) << 16) |
		(spd->t_pllrst & 0x1fff);
	spd_cb->phy_cfg.ptr2 = 0;
	spd_cb->phy_cfg.ptr3 = ((spd->t_dinit1 & 0x1ff) << 20) |
		(spd->t_dinit0 & 0xfffff);
	spd_cb->phy_cfg.ptr4 = ((spd->t_dinit3 & 0x3ff) << 18) |
		(spd->t_dinit2 & 0x3ffff);

	spd_cb->phy_cfg.dcr_mask = PDQ_MASK | MPRDQ_MASK | BYTEMASK_MASK;
	spd_cb->phy_cfg.dcr_val = 1 << 10;

	if (spd->mirrored) {
		spd_cb->phy_cfg.dcr_mask |= NOSRA_MASK | UDIMM_MASK;
		spd_cb->phy_cfg.dcr_val |= (1 << 27) | (1 << 29);
	}

	spd_cb->phy_cfg.dtpr0 = (spd->t_rc & 0x3f) << 26 |
		(spd->t_rrd & 0xf) << 22 |
		(spd->t_ras & 0x3f) << 16 | (spd->t_rcd & 0xf) << 12 |
		(spd->t_rp & 0xf) << 8 | (spd->t_wtr & 0xf) << 4 |
		(spd->t_rtp & 0xf);
	spd_cb->phy_cfg.dtpr1 = (spd->t_wlo & 0xf) << 26 |
		(spd->t_wlmrd & 0x3f) << 20 | (spd->t_rfc & 0x1ff) << 11 |
		(spd->t_faw & 0x3f) << 5 | (spd->t_mod & 0x7) << 2|
		(spd->t_mrd & 0x3);

	spd_cb->phy_cfg.dtpr2 = 0 << 31 | 1 << 30 | 0 << 29 |
		(spd->t_dllk & 0x3ff) << 19 | (spd->t_ckesr & 0xf) << 15;

	spd_cb->phy_cfg.dtpr2 |= (((spd->t_xp > spd->t_xpdll) ?
				   spd->t_xp : spd->t_xpdll) &
				  0x1f) << 10;

	spd_cb->phy_cfg.dtpr2 |= (((spd->t_xs > spd->t_xsdll) ?
			      spd->t_xs : spd->t_xsdll) &
			     0x3ff);

	spd_cb->phy_cfg.mr0 = 1 << 12 | (spd->t_wr_bin & 0x7) << 9 | 0 << 8 |
		0 << 7 | ((spd->cas & 0x0E) >> 1) << 4 | 0 << 3 |
		(spd->cas & 0x01) << 2;

	spd_cb->phy_cfg.mr1 = 0 << 12 | 0 << 11 | 0 << 7 | 0 << 3 |
		((DDR_TERM >> 2) & 1) << 9 | ((DDR_TERM >> 1) & 1) << 6 |
		(DDR_TERM & 0x1) << 2 | ((SDRAM_DRIVE >> 1) & 1) << 5 |
		(SDRAM_DRIVE & 1) << 1 | 0 << 0;

	spd_cb->phy_cfg.mr2 = DYN_ODT << 9 | TEMP << 7 | (spd->asr & 1) << 6 |
		(spd->cwl & 7) << 3 | (spd->pasr & 7);

	spd_cb->phy_cfg.dtcr = (spd->rank == 2) ? 0x730035C7 : 0x710035C7;
	spd_cb->phy_cfg.pgcr2 = (0xF << 20) | ((int)spd->t_refprd & 0x3ffff);

	spd_cb->phy_cfg.zq0cr1 = 0x0000005D;
	spd_cb->phy_cfg.zq1cr1 = 0x0000005B;
	spd_cb->phy_cfg.zq2cr1 = 0x0000005B;

	spd_cb->phy_cfg.pir_v1 = 0x00000033;
	spd_cb->phy_cfg.pir_v2 = 0x0000FF81;

	/* EMIF Registers */
	spd_cb->emif_cfg.sdcfg = spd->sdram_type << 29 | (DDR_TERM & 7) << 25 |
		(DYN_ODT & 3) << 22 | (spd->cwl & 0x7) << 14 |
		(spd->cas & 0xf) << 8 | (spd->ibank & 3) << 5 |
		(spd->buswidth & 3) << 12 | (spd->pagesize & 3);

	if (spd->rank == 2)
		spd_cb->emif_cfg.sdcfg |= 1 << 3;

	spd_cb->emif_cfg.sdtim1 = ((spd->t_wr - 1) & 0x1f) << 25 |
		((spd->t_ras - 1) & 0x7f) << 18 |
		((spd->t_rc - 1) & 0xff) << 10 |
		(spd->t_rrd2 & 0x3f) << 4  |
		((spd->t_wtr - 1) & 0xf);

	spd_cb->emif_cfg.sdtim2 = 0x07 << 10 | ((spd->t_rp - 1) & 0x1f) << 5 |
		((spd->t_rcd-1) & 0x1f);

	spd_cb->emif_cfg.sdtim3 = ((spd->t_xp - 2) & 0xf) << 28 |
		((spd->t_xs - 1) & 0x3ff) << 18 |
		((spd->t_xsdll - 1) & 0x3ff) << 8 |
		((spd->t_rtp - 1) & 0xf) << 4 | ((spd->t_cke) & 0xf);

	spd_cb->emif_cfg.sdtim4 = (spd->t_csta & 0xf) << 28 |
		((spd->t_ckesr - 1) & 0xf) << 24 |
		((spd->t_zqcs - 1) & 0xff) << 16 |
		((spd->t_rfc - 1) & 0x3ff) << 4 |
		(spd->t_ras_max & 0xf);

	spd_cb->emif_cfg.sdrfc = (spd->refresh_rate-1) & 0xffff;

	/* TODO zqcfg value fixed ,May be required correction for K2E evm. */
	spd_cb->emif_cfg.zqcfg = (spd->rank == 2) ? 0xF0073200 : 0x70073200;
}

static int ddr3_read_spd(ddr3_spd_eeprom_t *spd_params)
{
	int ret;
	int old_bus;

	i2c_init(CONFIG_SYS_DAVINCI_I2C_SPEED, CONFIG_SYS_DAVINCI_I2C_SLAVE);

	old_bus = i2c_get_bus_num();
	i2c_set_bus_num(1);

	ret = i2c_read(0x53, 0, 1, (unsigned char *)spd_params, 256);

	i2c_set_bus_num(old_bus);

	if (ret) {
		puts("Cannot read DIMM params\n");
		return 1;
	}

	if (ddr3_spd_check(spd_params))
		return 1;

	return 0;
}

int ddr3_get_size(void)
{
	ddr3_spd_eeprom_t spd_params;

	if (ddr3_read_spd(&spd_params))
		return 0;

	return ddr3_get_size_in_mb(&spd_params) / 1024;
}

int ddr3_get_dimm_params_from_spd(struct ddr3_spd_cb *spd_cb)
{
	struct ddr3_sodimm spd;
	ddr3_spd_eeprom_t spd_params;

	memset(&spd, sizeof(spd), 0);

	if (ddr3_read_spd(&spd_params))
		return 1;

	if (ddrtimingcalculation(&spd_params, &spd, spd_cb)) {
		puts("Timing caclulation error\n");
		return 1;
	}

	strncpy(spd_cb->dimm_name, (char *)spd_params.mpart, 18);
	spd_cb->dimm_name[18] = '\0';

	init_ddr3param(spd_cb, &spd);

#ifdef DUMP_DDR_CONFIG
	dump_emif_config(&(spd_cb->emif_cfg));
	dump_phy_config(&(spd_cb->phy_cfg));
#endif

	return 0;
}

