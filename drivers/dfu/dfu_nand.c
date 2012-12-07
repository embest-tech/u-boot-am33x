/*
 * dfu_nand.c -- DFU for NAND routines.
 *
 * Copyright (C) 2012 Texas Instruments, Inc.
 *
 * Based on dfu_mmc.c which is:
 * Copyright (C) 2012 Samsung Electronics
 * author: Lukasz Majewski <l.majewski@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <div64.h>
#include <dfu.h>
#include <linux/mtd/mtd.h>
#include <jffs2/load_kernel.h>
#include <nand.h>

enum dfu_nand_op {
	DFU_OP_READ = 1,
	DFU_OP_WRITE,
};

static int nand_block_op(enum dfu_nand_op op, struct dfu_entity *dfu,
			u64 offset, void *buf, long *len)
{
	char cmd_buf[DFU_CMD_BUF_SIZE];
	u64 start, count;
	int ret;
	int dev;
	loff_t actual;

	/* if buf == NULL return total size of the area */
	if (buf == NULL) {
		*len = dfu->data.nand.size;
		return 0;
	}

	start = dfu->data.nand.start + offset + dfu->bad_skip;
	count = *len;
	if (start + count >
			dfu->data.nand.start + dfu->data.nand.size) {
		printf("%s: block_op out of bounds\n", __func__);
		return -1;
	}
	dev = nand_curr_device;
	if (dev < 0 || dev >= CONFIG_SYS_MAX_NAND_DEVICE ||
		!nand_info[dev].name) {
		printf("%s: invalid nand device\n", __func__);
		return -1;
	}

	sprintf(cmd_buf, "nand %s %p %llx %llx",
		op == DFU_OP_READ ? "read" : "write",
		 buf, start, count);

	debug("%s: %s 0x%p\n", __func__, cmd_buf, cmd_buf);
	ret = run_command(cmd_buf, 0);

	/* find out how much actual bytes have been written */
	/* the difference is the amount of skip we must add from now on */
	actual = nand_extent_skip_bad(&nand_info[dev], start, count);
	if (actual == (loff_t)-1) {
		printf("nand_extend_skip_bad: error!\n");
		return ret;
	}

	if (actual > (start + count)) {
		debug("%s: skipped %llx bad bytes at %llx\n", __func__,
				actual - (start + count), start);
		dfu->bad_skip += (u32)(actual - (start + count));
	}

	return ret;
}

static inline int nand_block_write(struct dfu_entity *dfu,
		u64 offset, void *buf, long *len)
{
	return nand_block_op(DFU_OP_WRITE, dfu, offset, buf, len);
}

static inline int nand_block_read(struct dfu_entity *dfu,
		u64 offset, void *buf, long *len)
{
	return nand_block_op(DFU_OP_READ, dfu, offset, buf, len);
}

static int dfu_write_medium_nand(struct dfu_entity *dfu,
		u64 offset, void *buf, long *len)
{
	int ret = -1;

	switch (dfu->layout) {
	case DFU_RAW_ADDR:
		ret = nand_block_write(dfu, offset, buf, len);
		break;
	default:
		printf("%s: Layout (%s) not (yet) supported!\n", __func__,
		       dfu_get_layout(dfu->layout));
	}

	return ret;
}

static int dfu_read_medium_nand(struct dfu_entity *dfu, u64 offset, void *buf,
		long *len)
{
	int ret = -1;

	switch (dfu->layout) {
	case DFU_RAW_ADDR:
		ret = nand_block_read(dfu, offset, buf, len);
		break;
	default:
		printf("%s: Layout (%s) not (yet) supported!\n", __func__,
		       dfu_get_layout(dfu->layout));
	}

	return ret;
}

extern int mtdparts_init(void);
extern struct part_info* mtd_part_info(struct mtd_device *dev, unsigned int part_num);
extern int find_dev_and_part(const char *id, struct mtd_device **dev,
		u8 *part_num, struct part_info **part);


int dfu_fill_entity_nand(struct dfu_entity *dfu, char *s)
{
	char *st;
	int ret, dev, part;

	dfu->dev_type = DFU_DEV_NAND;
	st = strsep(&s, " ");
	if (!strcmp(st, "raw")) {
		dfu->layout = DFU_RAW_ADDR;
		dfu->data.nand.start = simple_strtoul(s, &s, 16);
		s++;
		dfu->data.nand.size = simple_strtoul(s, &s, 16);
	} else if (!strcmp(st, "part")) {
		char mtd_id[32];
		struct mtd_device *mtd_dev;
		u8 part_num;
		struct part_info *pi;

		dfu->layout = DFU_RAW_ADDR;

		dev = simple_strtoul(s, &s, 10);
		s++;
		part = simple_strtoul(s, &s, 10);

		sprintf(mtd_id, "%s%d,%d", "nand", dev, part - 1);
		printf("using id '%s'\n", mtd_id);

		mtdparts_init();

		ret = find_dev_and_part(mtd_id, &mtd_dev, &part_num, &pi);
		if (ret != 0) {
			printf("Could not locate '%s'\n", mtd_id);
			return -1;
		}

		dfu->data.nand.start = pi->offset;
		dfu->data.nand.size = pi->size;

		printf("\n%s -> %llx %llx\n", mtd_id,
				dfu->data.nand.start, dfu->data.nand.size);
	} else {
		printf("%s: Memory layout (%s) not supported!\n", __func__, st);
		return -1;
	}

	dfu->read_medium = dfu_read_medium_nand;
	dfu->write_medium = dfu_write_medium_nand;

	/* initial state */
	dfu->inited = 0;

	return 0;
}
