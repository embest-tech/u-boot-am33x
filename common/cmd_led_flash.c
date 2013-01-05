#include <common.h>
#include <net.h>
#include <command.h>
#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <config_cmd_default.h>
#include <asm/io.h>

#define GPIO1_OE (volatile unsigned int *)0x4804C134
#define GPIO1_DATAOUT (volatile unsigned int *)0x4804C13C
#define GPIO1_SETDATAOUT (volatile unsigned int *)0x4804C194
#define GPIO1_CLEARDATAOUT (volatile unsigned int *)0x4804C190

int do_led(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])  
{ 
	switch(argc) { 
		case 3:
			__raw_writel(0x0, GPIO1_OE);
			if(strcmp(argv[1], "flash") == 0) {
				if(strcmp(argv[2], "all") == 0) {
					while(1) { 
						__raw_writel(0xCC000000, GPIO1_SETDATAOUT);	
						udelay(500000);
						__raw_writel(0xCC000000, GPIO1_CLEARDATAOUT);
						udelay(500000);
					}
				} 
			}	 
			break;
		case 2:
			if(strcmp(argv[1], "on") == 0) {
				__raw_writel(0x0, GPIO1_OE);
				__raw_writel(0xCC000000, GPIO1_SETDATAOUT);

			} else if(strcmp(argv[1], "off") == 0) {
				__raw_writel(0x0, GPIO1_OE);
				__raw_writel(0xCC000000, GPIO1_CLEARDATAOUT);
			}
			break;
		case 1:          
		default:  
			return  cmd_usage(cmdtp); 
	}    
}

U_BOOT_CMD(
        led ,      3,      1,      do_led,
        "    led control\n",
        "flash all\n"
	"led on/off\n"
);
