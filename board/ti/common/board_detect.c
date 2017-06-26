/*
 * Library to support early TI EVM EEPROM handling
 *
 * Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
 *	Lokesh Vutla
 *	Steve Kipisz
 *
 * SPDX-License-Identifier:    GPL-2.0+
 */

#include <common.h>
#include <asm/omap_common.h>
#include <i2c.h>

#include "include/board_detect.h"

/**
 * ti_i2c_eeprom_init - Initialize an i2c bus and probe for a device
 * @i2c_bus: i2c bus number to initialize
 * @dev_addr: Device address to probe for
 *
 * Return: 0 on success or corresponding error on failure.
 */
static int __maybe_unused ti_i2c_eeprom_init(int i2c_bus, int dev_addr)
{
	int rc;

	if (i2c_bus >= 0) {
		rc = i2c_set_bus_num(i2c_bus);
		if (rc)
			return rc;
	}

	return i2c_probe(dev_addr);
}

/**
 * ti_i2c_eeprom_read - Read data from an EEPROM
 * @dev_addr: The device address of the EEPROM
 * @offset: Offset to start reading in the EEPROM
 * @ep: Pointer to a buffer to read into
 * @epsize: Size of buffer
 *
 * Return: 0 on success or corresponding result of i2c_read
 */
static int __maybe_unused ti_i2c_eeprom_read(int dev_addr, int offset,
					     uchar *ep, int epsize)
{
	return i2c_read(dev_addr, offset, 2, ep, epsize);
}

/**
 * ti_eeprom_string_cleanup() - Handle eeprom programming errors
 * @s:	eeprom string (should be NULL terminated)
 *
 * Some Board manufacturers do not add a NULL termination at the
 * end of string, instead some binary information is kludged in, hence
 * convert the string to just printable characters of ASCII chart.
 */
static void __maybe_unused ti_eeprom_string_cleanup(char *s)
{
	int i, l;

	l = strlen(s);
	for (i = 0; i < l; i++, s++)
		if (*s < ' ' || *s > '~') {
			*s = 0;
			break;
		}
}

__weak void gpi2c_init(void)
{
}

int __maybe_unused ti_i2c_eeprom_am_get(int bus_addr, int dev_addr,
					struct ti_am_eeprom **epp)
{
	int rc;
	struct ti_am_eeprom *ep;

	if (!epp)
		return -1;

	ep = TI_AM_EEPROM_DATA;
	
	if (ep->header == TI_EEPROM_HEADER_MAGIC)
		goto already_read;

#ifdef CONFIG_BOARD_SBC8600
	ep->header = TI_EEPROM_HEADER_MAGIC;
	strcpy(ep->name, "SBC8600");
	goto already_read;
#endif

	/* Initialize with a known bad marker for i2c fails.. */
	ep->header = 0xADEAD12C;

	gpi2c_init();
	rc = ti_i2c_eeprom_init(bus_addr, dev_addr);
	if (rc)
		return rc;
	rc = i2c_read(dev_addr, 0x0, 2, (uint8_t *)ep, sizeof(*ep));
	if (rc)
		return rc;
	/* Corrupted data??? */
	if (ep->header != TI_EEPROM_HEADER_MAGIC) {
		rc = i2c_read(dev_addr, 0x0, 2, (uint8_t *)ep, sizeof(*ep));
		/*
		 * read the eeprom using i2c again, but use only a 1 byte
		 * address (some legacy boards need this..)
		 */
		if (rc)
			rc = i2c_read(dev_addr, 0x0, 1, (uint8_t *)ep,
				      sizeof(*ep));
		if (rc)
			return rc;
	}
#ifdef CONFIG_BOARD_SOM_PH8800
	strcpy(ep->name, "SOM-PH88");
#endif
#ifdef CONFIG_BOARD_SOM_PH8700
	strcpy(ep->name, "SOM-PH87");
#endif
#ifdef CONFIG_BOARD_SBC_EC8800
	strcpy(ep->name, "SBC-EC88");
#endif

	if (ep->header != TI_EEPROM_HEADER_MAGIC){
		/*
		* strcpy(ep->name, "SOM-PH8700");
		* ep->header = TI_EEPROM_HEADER_MAGIC;
		*
		* Temporary configuration, because PH8700 eeprom is empty now;
		* If the eeprom written content in the future ,please delete them;
		*/
		//strcpy(ep->name, "SOM-PH8700");
#if defined CONFIG_BOARD_SOM_PH8800 || defined CONFIG_BOARD_SOM_PH8700 || defined CONFIG_BOARD_SBC_EC8800

		ep->header = TI_EEPROM_HEADER_MAGIC;
#endif

		/*return -1;*/
        }

already_read:
	*epp = ep;

	return 0;
}

int __maybe_unused ti_i2c_eeprom_am_get_print(int bus_addr, int dev_addr,
					      struct ti_am_eeprom_printable *p)
{
	struct ti_am_eeprom *ep;
	int rc;

	/* Incase of invalid eeprom contents */
	p->name[0] = 0x00;
	p->version[0] = 0x00;
	p->serial[0] = 0x00;

	rc = ti_i2c_eeprom_am_get(bus_addr, dev_addr, &ep);
	if (rc)
		return rc;

	/*
	 * Alas! we have to null terminate and cleanup the strings!
	 */
	strlcpy(p->name, ep->name, TI_EEPROM_HDR_NAME_LEN + 1);
	ti_eeprom_string_cleanup(p->name);
	strlcpy(p->version, ep->version, TI_EEPROM_HDR_REV_LEN + 1);
	ti_eeprom_string_cleanup(p->version);
	strlcpy(p->serial, ep->serial, TI_EEPROM_HDR_SERIAL_LEN + 1);
	ti_eeprom_string_cleanup(p->serial);
	return 0;
}

int __maybe_unused ti_i2c_eeprom_am_set(const char *name, const char *rev)
{
	struct ti_am_eeprom *ep;

	if (!name || !rev)
		return -1;

	ep = TI_AM_EEPROM_DATA;
	if (ep->header == TI_EEPROM_HEADER_MAGIC)
		goto already_set;

	/* Set to 0 all fields */
	memset(ep, 0, sizeof(*ep));
	strncpy(ep->name, name, TI_EEPROM_HDR_NAME_LEN);
	strncpy(ep->version, rev, TI_EEPROM_HDR_REV_LEN);
	/* Some dummy serial number to identify the platform */
	strncpy(ep->serial, "0000", TI_EEPROM_HDR_SERIAL_LEN);
	/* Mark it with a valid header */
	ep->header = TI_EEPROM_HEADER_MAGIC;

already_set:
	return 0;
}

bool __maybe_unused board_am_is(char *name_tag)
{
	struct ti_am_eeprom *ep = TI_AM_EEPROM_DATA;

	if (ep->header != TI_EEPROM_HEADER_MAGIC)
		return false;
	return !strncmp(ep->name, name_tag, TI_EEPROM_HDR_NAME_LEN);
}

bool __maybe_unused board_am_rev_is(char *rev_tag, int cmp_len)
{
	struct ti_am_eeprom *ep = TI_AM_EEPROM_DATA;
	int l;

	if (ep->header != TI_EEPROM_HEADER_MAGIC)
		return false;

	l = cmp_len > TI_EEPROM_HDR_REV_LEN ? TI_EEPROM_HDR_REV_LEN : cmp_len;
	return !strncmp(ep->version, rev_tag, l);
}

void __maybe_unused set_board_info_env(char *name, char *revision,
				       char *serial)
{
	char *unknown = "unknown";

	if (name)
		setenv("board_name", name);
	else
		setenv("board_name", unknown);

	if (revision)
		setenv("board_rev", revision);
	else
		setenv("board_rev", unknown);

	if (serial)
		setenv("board_serial", serial);
	else
		setenv("board_serial", unknown);
}
