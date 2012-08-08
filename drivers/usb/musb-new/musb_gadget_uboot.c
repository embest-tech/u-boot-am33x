#include <common.h>
#include <asm/errno.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

#define __UBOOT__
#include "linux-compat.h"
#include "musb_core.h"
#include "musb_gadget.h"

struct musb *
musb_init_controller(struct musb_hdrc_platform_data *plat, struct device *dev,
			     void *ctrl);
int musb_gadget_start(struct usb_gadget *g, struct usb_gadget_driver *driver);

struct musb *musb;

int usb_gadget_handle_interrupts(void)
{
	if (!musb || !musb->isr)
		return -EINVAL;

	return musb->isr(0, musb);
}

int musb_register(struct musb_hdrc_platform_data *plat, void *bdata,
			void *ctl_regs)
{
	musb = musb_init_controller(plat, (struct device *)bdata, ctl_regs);
	if (!musb) {
		printf("Failed to init the controller\n");
		return -EIO;
	}

	return 0;
}

int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	int ret;

	if (!driver || driver->speed < USB_SPEED_HIGH || !driver->bind ||
	    !driver->setup) {
		printf("bad parameter.\n");
		return -EINVAL;
	}

	if (!musb) {
		printf("Controller uninitialized\n");
		return -ENXIO;
	}

	ret = musb_gadget_start(&musb->g, driver);
	if (ret < 0) {
		printf("gadget_start failed with %d\n", ret);
		return ret;
	}

	ret = driver->bind(&musb->g);
	if (ret < 0) {
		printf("bind failed with %d\n", ret);
		return ret;
	}

	return 0;
}

int usb_gadget_unregister_driver(struct usb_gadget_driver *driver)
{
	/* TODO: implement me */
	return 0;
}
