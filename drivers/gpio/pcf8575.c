/*
 * PCF8575 I2C GPIO EXPANDER DRIVER
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Driver for TI pcf-8575 16 bit I2C gpio expander. Based on
 * gpio-pcf857x Linux 4.0 kernel driver and pca953x driver in u-boot
 */

#include <common.h>
#include <i2c.h>
#include <pcf8575.h>

enum {
	PCF8575_CMD_INFO,
	PCF8575_CMD_DEVICE,
	PCF8575_CMD_OUTPUT,
	PCF8575_CMD_INPUT,
};

struct pcf8575_chip {
	uint8_t addr;
/* current direction of the pcf lines */
	unsigned int out;
};


/* NOTE:  these chips have strange "quasi-bidirectional" I/O pins.
 * We can't actually know whether a pin is configured (a) as output
 * and driving the signal low, or (b) as input and reporting a low
 * value ... without knowing the last value written since the chip
 * came out of reset (if any).  We can't read the latched output.
 * In short, the only reliable solution for setting up pin direction
 * is to do it explicitly.
 *
 * Using "out" avoids that trouble. It flags the status of the pins at
 * boot.
 *
 * Each struct stores address of an instance of pcf
 * and state(direction) of each gpio line for that instance.
 */
static struct pcf8575_chip pcf8575_chips[] =
	CONFIG_SYS_I2C_PCF8575_CHIP;

static struct pcf8575_chip *pcf8575_chip_get(uint8_t addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pcf8575_chips); i++)
		if (pcf8575_chips[i].addr == addr)
			return &pcf8575_chips[i];

	return 0;
}


/* Read/Write to 16-bit I/O expander */

static int pcf8575_i2c_write(uint8_t addr, unsigned word)
{
	unsigned word_be = ((word & 0xff) << 8) |
			   ((word & 0xff00) >> 8);
	uint8_t buf = 0;
	int status;

	status = i2c_write(addr, word_be, 2, &buf, 1);

	return (status < 0) ? status : 0;
}

static int pcf8575_i2c_read(uint8_t addr)
{
	u8 buf[2];
	int status;

	status = i2c_read(addr, 0, 1, buf, 2);
	if (status < 0)
		return status;

	return (buf[1] << 8) | buf[0];
}

int pcf8575_input(uint8_t addr, unsigned offset)
{
	struct pcf8575_chip *chip = pcf8575_chip_get(addr);
	int status;

	chip->out |= (1 << offset);
	status = pcf8575_i2c_write(addr, chip->out);

	return status;
}

int pcf8575_get_val(uint8_t addr, unsigned offset)
{
	int             value;

	value = pcf8575_i2c_read(addr);
	return (value < 0) ? 0 : (value & (1 << offset));
}

int pcf8575_output(uint8_t addr, unsigned offset, int value)
{
	struct pcf8575_chip *chip = pcf8575_chip_get(addr);
	unsigned        bit = 1 << offset;
	int             status;

	if (value)
		chip->out |= bit;
	else
		chip->out &= ~bit;
	status = pcf8575_i2c_write(addr, chip->out);

	return status;
}

/*
 * Display pcf8575 information
 */
int pcf8575_info(uint8_t addr)
{
	int i;
	uint data;
	struct pcf8575_chip *chip = pcf8575_chip_get(addr);
	int nr_gpio = 16;
	int msb = nr_gpio - 1;

	printf("pcf8575@ 0x%x (%d pins):\n\n", addr, nr_gpio);
	printf("gpio pins: ");
	for (i = msb; i >= 0; i--)
		printf("%x", i);
	printf("\n");
	for (i = 11 + nr_gpio; i > 0; i--)
		printf("-");
	printf("\n");

	data = chip->out;
	printf("dir:      ");
	for (i = msb; i >= 0; i--)
		printf("%c", data & (1 << i) ? 'i' : 'o');
	printf("\n");

	data = pcf8575_i2c_read(addr);
	if (data < 0)
		return -1;
	printf("input:     ");
	for (i = msb; i >= 0; i--)
		printf("%c", data & (1 << i) ? '1' : '0');
	printf("\n");

	return 0;
}

cmd_tbl_t cmd_pcf8575[] = {
	U_BOOT_CMD_MKENT(device, 3, 0, (void *)PCF8575_CMD_DEVICE, "", ""),
	U_BOOT_CMD_MKENT(output, 4, 0, (void *)PCF8575_CMD_OUTPUT, "", ""),
	U_BOOT_CMD_MKENT(input, 3, 0, (void *)PCF8575_CMD_INPUT, "", ""),
	U_BOOT_CMD_MKENT(info, 2, 0, (void *)PCF8575_CMD_INFO, "", ""),
};



int do_pcf8575(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	static uint8_t chip = 0x21;
	int ret = CMD_RET_USAGE, val;
	ulong ul_arg2 = 0;
	ulong ul_arg3 = 0;
	cmd_tbl_t *c;

	c = find_cmd_tbl(argv[1], cmd_pcf8575, ARRAY_SIZE(cmd_pcf8575));

	/* All commands but "device" require 'maxargs' arguments */
	if (!c || !((argc == (c->maxargs)) ||
		    (((int)c->cmd == PCF8575_CMD_DEVICE) &&
		    (argc == (c->maxargs - 1))))) {
		return CMD_RET_USAGE;
	}

	/* arg2 used as chip number or pin number */
	if (argc > 2)
		ul_arg2 = simple_strtoul(argv[2], NULL, 16);

	/* arg3 used as pin or invert value */
	if (argc > 3)
		ul_arg3 = simple_strtoul(argv[3], NULL, 16) & 0x1;

	switch ((int)c->cmd) {
	case PCF8575_CMD_INFO:
		ret = pcf8575_info(chip);
		if (ret)
			ret = CMD_RET_FAILURE;
		break;

	case PCF8575_CMD_DEVICE:
		if (argc == 3)
			chip = (uint8_t)ul_arg2;
		printf("Current device address: 0x%x\n", chip);
		ret = CMD_RET_SUCCESS;
		break;

	case PCF8575_CMD_INPUT:
		ret = pcf8575_input(chip, ul_arg2);
		val = pcf8575_get_val(chip, ul_arg2);

		if (val)
			ret = CMD_RET_FAILURE;
		else
			printf("chip 0x%02x, pin 0x%lx = %d\n", chip, ul_arg2,
			       val);
		break;

	case PCF8575_CMD_OUTPUT:
			ret = pcf8575_output(chip, ul_arg2, ul_arg3);
		if (ret)
			ret = CMD_RET_FAILURE;
		break;
	}

	if (ret == CMD_RET_FAILURE)
		eprintf("Error talking to chip at 0x%x\n", chip);

	return ret;
}

U_BOOT_CMD(
	pcf8575,	5,	1,	do_pcf8575,
	"pcf8575 gpio access",
	"device [dev]\n"
	"	- show or set current device address\n"
	"pcf8575 info\n"
	"	- display info for current chip\n"
	"pcf8575 output pin 0|1\n"
	"	- set pin as output and drive low or high\n"
	"pcf8575 input pin\n"
	"	- set pin as input and read value"
);
