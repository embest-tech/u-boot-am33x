/*
 * Library to support early TI EVM EEPROM handling
 *
 * Copyright (C) 2015, Texas Instruments, Incorporated - http://www.ti.com
 *
 * SPDX-L	icense-Identifier:	GPL-2.0+
 */

#ifndef __BOARD_DETECT_H
#define __BOARD_DETECT_H

/* TI EEPROM MAGIC Header identifier */
#define TI_EEPROM_HEADER_MAGIC	0xEE3355AA

#define TI_EEPROM_HDR_NAME_LEN		8
#define TI_EEPROM_HDR_REV_LEN		4
#define TI_EEPROM_HDR_SERIAL_LEN	12
#define TI_EEPROM_HDR_CONFIG_LEN	32
#define TI_EEPROM_HDR_NO_OF_MAC_ADDR	3
#define TI_EEPROM_HDR_ETH_ALEN		6

/**
 * struct ti_am_eeprom - This structure holds data read in from the
 *                     AM335x, AM437x, AM57xx TI EVM EEPROMs.
 * @header: This holds the magic number
 * @name: The name of the board
 * @version: Board revision
 * @serial: Board serial number
 * @config: Reserved
 * @mac_addr: Any MAC addresses written in the EEPROM
 *
 * The data is this structure is read from the EEPROM on the board.
 * It is used for board detection which is based on name. It is used
 * to configure specific TI boards. This allows booting of multiple
 * TI boards with a single MLO and u-boot.
 */
struct ti_am_eeprom {
	unsigned int header;
	char name[TI_EEPROM_HDR_NAME_LEN];
	char version[TI_EEPROM_HDR_REV_LEN];
	char serial[TI_EEPROM_HDR_SERIAL_LEN];
	char config[TI_EEPROM_HDR_CONFIG_LEN];
	char mac_addr[TI_EEPROM_HDR_NO_OF_MAC_ADDR][TI_EEPROM_HDR_ETH_ALEN];
} __attribute__ ((__packed__));

/**
 * struct ti_am_eeprom_printable - Null terminated, printable EEPROM contents
 * @name:	NULL terminated name
 * @version:	NULL terminated version
 * @serial:	NULL terminated serial number
 */
struct ti_am_eeprom_printable {
	char name[TI_EEPROM_HDR_NAME_LEN + 1];
	char version[TI_EEPROM_HDR_REV_LEN + 1];
	char serial[TI_EEPROM_HDR_SERIAL_LEN + 1];
};
#define TI_AM_EEPROM_DATA ((struct ti_am_eeprom *)\
				OMAP_SRAM_SCRATCH_BOARD_EEPROM_START)

/**
 * ti_i2c_eeprom_am_get() - Consolidated eeprom data collection for AM* TI EVMs
 * @bus_addr:	I2C bus address
 * @dev_addr:	I2C slave address
 * @epp:	Pointer to eeprom structure
 *
 * *epp is populated by the this AM generic function that consolidates
 * the basic initialization logic common accross all AM* platforms.
 */
int ti_i2c_eeprom_am_get(int bus_addr, int dev_addr, struct ti_am_eeprom **epp);

/**
 * ti_i2c_eeprom_am_get_print() - Get a printable representation of eeprom data
 * @bus_addr:	I2C bus address
 * @dev_addr:	I2C slave address
 * @p:		Pointer to eeprom structure
 *
 * This reads the eeprom and converts the data into a printable string for
 * further processing for AM* platforms.
 */
int ti_i2c_eeprom_am_get_print(int bus_addr, int dev_addr,
			       struct ti_am_eeprom_printable *p);


/**
 * board_am_is() - Board detection logic for TI AM* EVMs
 * @name_tag:	Tag used in eeprom for the board
 *
 * Return: false if board information does not match OR eeprom wasn't read.
 *	   true otherwise
 */
bool board_am_is(char *name_tag);

/**
 * board_am_rev_is() - Compare board revision for AM* TI EVMs
 * @rev_tag:	Revision tag to check in eeprom
 * @cmp_len:	How many chars to compare?
 *
 * NOTE: revision information is often messed up (hence the str len match) :(
 *
 * Return: false if board information does not match OR eeprom was'nt read.
 *	   true otherwise
 */
bool board_am_rev_is(char *rev_tag, int cmp_len);

/**
 * set_board_info_env() - Setup commonly used board information environment vars
 * @name:	Name of the board
 * @revision:	Revision of the board
 * @serial:	Serial Number of the board
 *
 * In case of NULL revision or serial information "unknown" is setup.
 * If name is NULL, default_name is used.
 */
void set_board_info_env(char *name, char *revision,
			char *serial);

/**
 * ti_i2c_eeprom_am_set() - Setup the eeprom data with predefined values
 * @name:	Name of the board
 * @rev:	Revision of the board
 *
 * In some cases such as in RTC-only mode, we are able to skip reading eeprom
 * and wasting i2c based initialization time by using predefined flags for
 * detecting what platform we are booting on. For those platforms, provide
 * a handy function to pre-program information.
 *
 * NOTE: many eeprom information such as serial number, mac address etc is not
 * available.
 *
 * Return: 0 if all went fine, else return error.
 */
int ti_i2c_eeprom_am_set(const char *name, const char *rev);

#endif	/* __BOARD_DETECT_H */
