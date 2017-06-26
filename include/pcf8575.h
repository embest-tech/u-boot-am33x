/*
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

#ifndef __PCF8575_H_
#define __PCF8575_H_

#define PCF8575_OUT_LOW		0
#define PCF8575_OUT_HIGH	1


int pcf8575_input(uint8_t addr, unsigned offset);
int pcf8575_get_val(uint8_t addr, unsigned offset);
int pcf8575_output(uint8_t addr, unsigned offset, int value);

#endif /* __PCF8575_H_ */
