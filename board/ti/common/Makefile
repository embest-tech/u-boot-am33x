# Copyright (C) 2015 Texas Instruments Incorporated - http://www.ti.com/
#
# SPDX-License-Identifier:	GPL-2.0+
#

# Necessary for a default builtin.o
obj- = __dummy__.o

ifdef CONFIG_CREATE_BOARD_SYMLINK
	obj-${CONFIG_I2C} += board_detect.o
endif
