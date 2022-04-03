/* Copyright (c) 2015 LGE Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>

/*platform specified headers*/
#include <linux/gpio.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include "bq2022.h"

/** debug */
#define DEBUG			pr_err

#define DEBUG_LEVEL		2

static unsigned int debug_enable = 1;

#if (DEBUG_LEVEL == 2)
#define MSG_LOG(...) \
{ \
	if(debug_enable) \
	{ \
		DEBUG(__VA_ARGS__); \
	} \
}
#define DEV_LOG(...) \
{ \
	if(debug_enable) \
	{ \
		DEBUG(__VA_ARGS__); \
	} \
}
#elif (DEBUG_LEVEL == 1)
#define MSG_LOG(...) \
{ \
	if(debug_enable) \
	{ \
		DEBUG(__VA_ARGS__); \
	} \
}
#define DEV_LOG(...) 
#else
#define MSG_LOG(...) 
#define DEV_LOG(...) 
#endif

struct battery_device_info {
	unsigned int company_name;/*vivo*/
	unsigned short version;
	unsigned short vol_type;
	unsigned char package_factory;
	unsigned char bat_factory;
	unsigned int year;
	unsigned short month;
	unsigned short date;
};

struct bq2022_device {
	struct device 		*dev;
	struct power_supply	batid_psy;
	int					data_gpio;
	int					power_gpio;
	int					bat_id;
	struct mutex        battid_lock;
	int					def_bat_id;
	int					bat_data_col;
	int					bat_data_row;
	unsigned char		*real_bat_data;
	struct battery_device_info *bat_info;
};

static struct bq2022_device *the_chip = NULL;

static enum power_supply_property batid_power_props[] = {
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_MODEL_NAME,
};
static char *batid_power_supplied_to[] = {
	"nobody",
};

/*********************************************************************** CRC-8 ***********************************************************************************/
unsigned char crc8_table[256] = {

                                                    0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83,
                                                    0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41, 
                                                    0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
                                                    0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc, 
                                                    0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0,
                                                    0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62, 
                                                    0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d,
                                                    0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
                                                    0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5, 
                                                    0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
                                                    0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58,
                                                    0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
                                                    0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6, 
                                                    0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24, 
                                                    0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
                                                    0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9, 
                                                    0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f, 
                                                    0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
                                                    0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92,
                                                    0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50, 
                                                    0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c, 
                                                    0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee, 
                                                    0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1, 
                                                    0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73, 
                                                    0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49,
                                                    0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
                                                    0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
                                                    0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16, 
                                                    0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a, 
                                                    0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8, 
                                                    0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7,
                                                    0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
                                                    };

static unsigned char get_crc8(unsigned char *p, unsigned char counter)
{
	unsigned char crc8 = 0;

        for( ; counter > 0; counter--)
        {
	        crc8 = crc8_table[crc8^*p]; 
                p++;
        }
        
	return crc8;
}

static int crc8_check(unsigned char *data,unsigned int len,unsigned char crc)
{
	unsigned char crc8;

	crc8 = get_crc8(data,len);
		
	if(crc8 == crc)
	{
		DEV_LOG("[BQ2022]: CRC check is %d! Check is OK!\n",crc8);
		return 0;
	}
	else
	{
		DEBUG("[BQ2022 ERROR]: CRC check is fail! Check value is 0x%02x! Receive value is 0x%02x !\n",crc8,crc);
		return -1;
	}
}

/** adc pin set */
/*static unsigned int bq2022_adc_set_input(struct bq2022_device *chip)
{

	return 0;
}*/

static unsigned int bq2022_adc_set_output(struct bq2022_device *chip)
{

	return 0;
}
/*
static unsigned int bq2022_adc_set_high(struct bq2022_device *chip)
{

	return 0;
}
*/
static unsigned int bq2022_adc_set_low(struct bq2022_device *chip)
{

	return 0;
}

/***********************************************************************bq2022 gpio set **************************************************************************/
static unsigned int bq2022_data_set_input(struct bq2022_device *chip)
{
	if (chip->data_gpio)
		gpio_direction_input(chip->data_gpio);
	return 0;
}

static unsigned int bq2022_data_set_output(struct bq2022_device *chip)
{
	if (chip->data_gpio)
		gpio_direction_output(chip->data_gpio,0);
	return 0;
}

static unsigned int bq2022_data_set_low(struct bq2022_device *chip)
{
	if (chip->data_gpio)
		gpio_direction_output(chip->data_gpio,0);
	return 0;
}

static unsigned int bq2022_data_set_high(struct bq2022_device *chip)
{
	if (chip->data_gpio)
		gpio_direction_output(chip->data_gpio,1);
	return 0;
}

static unsigned char bq2022_get_data(struct bq2022_device *chip)
{
	int ret;

	//get gpio data
	ret = gpio_get_value(chip->data_gpio);

	if(ret < 0)
	{
		DEBUG("\n%s(ERROR): bq2022 data pin read data ERROR!!!\n\n",__FUNCTION__);
		return ret;
	}
	else
		return ret;
}

/** bq2022 vdd pin set */
static unsigned int bq2022_vdd_set_output(struct bq2022_device *chip)
{
	if (chip->power_gpio)
		gpio_direction_output(chip->power_gpio,0);
	return 0;
}

static unsigned int bq2022_vdd_set_input(struct bq2022_device *chip)
{
	if (chip->power_gpio)
		gpio_direction_input(chip->power_gpio);
	return 0;
}

static unsigned int bq2022_vdd_set_high(struct bq2022_device *chip)
{
	if (chip->power_gpio)
		gpio_direction_output(chip->power_gpio,1);
	return 0;
}

static unsigned int bq2022_vdd_set_low(struct bq2022_device *chip)
{
	if (chip->power_gpio)
		gpio_direction_output(chip->power_gpio,0);
	return 0;
}

static int bq2022_reset_vdd_data(struct bq2022_device *chip)
{
	int ret;
	DEBUG("\n%s reset vdd data\n",__FUNCTION__);
	ret = bq2022_vdd_set_low(chip);
	if(ret < 0)
		return -1;

	ret = bq2022_data_set_low(chip);
	if(ret < 0)
		return -1;
	return 0;
}
static int bq2022_com_config_pins(struct bq2022_device *chip)
{
	int ret;
	
	ret = bq2022_vdd_set_output(chip);								// set vdd out dir.
	if(ret < 0)
		return -1;		
	
	ret = bq2022_vdd_set_high(chip);
	if(ret < 0)
		return -1;		

	ret = bq2022_data_set_output(chip);								// set data out dir.
	if(ret < 0)
		return -1;		
	
	ret = bq2022_data_set_high(chip);
	if(ret < 0)
		return -1;		

	ret = bq2022_adc_set_output(chip);								// set adc out dir.
	if(ret < 0)
		return -1;		
	
	ret = bq2022_adc_set_low(chip);
	if(ret < 0)
		return -1;	

	return 0;	
}

static int bq2022_com_reset_pins(struct bq2022_device *chip)
{
	int ret;

	ret = bq2022_vdd_set_input(chip);								// set adc out dir.
	if(ret < 0)
		return -1;		
	
	ret = bq2022_data_set_input(chip);								// set adc out dir.
	if(ret < 0)
		return -1;		
	
	return 0;
}

static unsigned int bq2022_pins_config(struct bq2022_device *chip)
{
	if(bq2022_com_config_pins(chip) < 0)
	{
		DEBUG("\n%s(ERROR): bq2022 pins set ERROR!!!\n\n",__FUNCTION__);
		return -1;	
	}
	return 0;
}

static unsigned int bq2022_pins_reset(struct bq2022_device *chip)
{
	if(bq2022_com_reset_pins(chip) < 0)
	{
		DEBUG("\n%s(ERROR): bq2022 pins set ERROR!!!\n\n",__FUNCTION__);
		return -1;	
	}
	return 0;

}

/*********************************************************************** bq2022 function *************************************************************************/
static int bq2022_reset(struct bq2022_device *chip)
{
	int i = 0;
	
	DEV_LOG("%s(INFO): bq2022 will RESET!\n",__FUNCTION__);	

	bq2022_data_set_output(chip);								// set data out dir.
	bq2022_data_set_low(chip);									// set data low.
	udelay(BQ2022_RST_PRESENCE_T);								// wait for more than 480us.
//	bq2022_data_set_high(chip);

	udelay(BQ2022_T_PPD);									// 15~60; 60us wait for bq2022 respond.
	bq2022_data_set_input(chip);
	udelay(BQ2022_T_PPD);									// 20us 
	udelay(BQ2022_T_PPD);									// 20us 
	
	do{
		if(0 == bq2022_get_data(chip))							// bq2022 respond.
		{
			udelay(1);
			if(0 == bq2022_get_data(chip))						// confirm bq2022 respond.
			{
				udelay(BQ2022_T_PP);						// 60~240; 240us 
//				bq2022_data_set_output(chip);					// set data out dir.
//				bq2022_data_set_high(chip);

				udelay(BQ2022_T_RSTREC);					// 200us BQ2022:480us 
				DEV_LOG("%s(INFO): bq2022 is Respond!\n",__FUNCTION__);	
				return 1;
			}
		}	
		
	}while(i++ < 50);
	
	DEV_LOG("%s(ERROR): bq2022 is NOT Respond!\n",__FUNCTION__);	

	bq2022_pins_reset(chip);

	return -1;	
}

static void bq2022_write_bit(struct bq2022_device *chip,unsigned char data)
{
	bq2022_data_set_output(chip);								// set data out dir.
	bq2022_data_set_low(chip);									// set data low.
	udelay(BQ2022_SRTB_T);									// 10us wait for data setup.

	//udelay(BQ2022_WAIT_T);									// 10us wait for data output.

	if(data)										// Set Data!!!
		bq2022_data_set_high(chip);
	else
		bq2022_data_set_low(chip);								// set data low.

	udelay(BQ2022_WRITE_HOLD);								// 42us wait for data output.
	
//	bq2022_data_set_high(chip);
	bq2022_data_set_input(chip);								// release bus.             //liuchangjian 2012-05-22 modify
	udelay(BQ2022_REC_TIME);								// 7us high

	udelay(10);
}


static void bq2022_write_byte(struct bq2022_device *chip,unsigned char data)
{
	unsigned char i;
	unsigned char data_byte;
	unsigned char mask_byte;
	unsigned long flags;

	local_irq_save(flags);

	mask_byte = 0x01;

	for(i=0;i<8;i++)
	{
		data_byte = data & mask_byte;
		bq2022_write_bit(chip,data_byte);
		mask_byte = mask_byte << 1;
	}

	local_irq_restore(flags);
}

static unsigned char bq2022_read_bit(struct bq2022_device *chip)
{
	unsigned char data;

	bq2022_data_set_output(chip);						// set data out dir.
	bq2022_data_set_low(chip);						// set data low.

	//udelay(BQ2022_SRTB_T);									// 10us wait for data setup.
	udelay(8);

	bq2022_data_set_input(chip);
	udelay(BQ2022_WAIT_T);									// 10us wait for data output.

	udelay(10);
	data = bq2022_get_data(chip);								// get data!!!

	udelay(BQ2022_READ_WAIT);								// 5us wait for data output.

	udelay(20);
//	bq2022_data_set_output(chip);								// set data out dir.

//	bq2022_data_set_high(chip);
//	bq2022_data_set_input(chip);								// release bus.             //liuchangjian 2012-05-22 modify
	udelay(BQ2022_REC_TIME);								// 7us high

	udelay(10);

	return data;
}

static unsigned char bq2022_read_byte(struct bq2022_device *chip)
{
	unsigned char i;
	unsigned char data_byte;
	unsigned char mask_byte;
	unsigned long flags;

	data_byte = 0;	

	local_irq_save(flags);	

	for(i=0;i<8;i++)
	{
		mask_byte = bq2022_read_bit(chip);
		mask_byte = mask_byte << i;
		data_byte |= mask_byte;
	}

	local_irq_restore(flags);	

	return data_byte;
}

/******************************************************** bq2022 cmd functions(interface) ************************************************/

/*****************************//**
 * 1.ROM command 
 *	READ ROM
 *	MATCH ROM
 *	SEARCH ROM	
 *	SKIP ROM
 ******************************/

/** READ ROM */
static int bq2022_read_rom(struct bq2022_device *chip,unsigned char *data)
{
	int i;

	bq2022_pins_config(chip);

	if(bq2022_reset(chip) < 0)
	{
		DEBUG("\n%s(ERROR): bq2022 reset is FAIL!!!\n\n",__FUNCTION__);
		return -1;
	}

	bq2022_write_byte(chip,BQ2022_CMD_READ_ROM);

	for(i=0; i<BQ2022_READ_ROM_LEN; i++)
	{
		data[i] = bq2022_read_byte(chip);
	}

	MSG_LOG("\n\n bq2022 family and identification is 0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x!\n\n",
				data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
	if(crc8_check(data,BQ2022_READ_ROM_LEN-1,data[BQ2022_READ_ROM_LEN-1]) < 0)
		return -1;
	else
		return 1;

//	gpio_dump_regs(chip);
}

/** SKIP ROM */
static int bq2022_skip_rom(struct bq2022_device *chip)
{
	DEV_LOG("%s:",__FUNCTION__);

	if(bq2022_reset(chip) < 0)
	{
		DEBUG("%s(ERROR): bq2022 reset is FAIL!!!\n",__FUNCTION__);
		return -1;
	}

	bq2022_write_byte(chip,BQ2022_CMD_SKIP_ROM);

	return 1;
}

/*****************************//**
 *  2.Memory command 
 *	Read Memory/Field CRC
 *	Read EPROM Status
 *	Read Memory/Page CRC
 *	Write Memory
 *	Programming Profile
 *	Write EPROM Status
 *****************************/

/** Read Memory/Field CRC */
#define BBK_DATA_ADDR_LOW						0x00
#define BBK_DATA_ADDR_HIGH						0x00

static int bq2022_read_mem_field(struct bq2022_device *chip,unsigned char *data)
{
	unsigned int i;
	unsigned char crc,crc_cmd;
	unsigned char send_data[3];

	bq2022_pins_config(chip);

	send_data[0] = BQ2022_MEM_CMD_READ_PAGE_CRC;
	send_data[1] = BBK_DATA_ADDR_LOW;	
	send_data[2] = BBK_DATA_ADDR_HIGH;	

	if(bq2022_skip_rom(chip) < 0)
	{
		DEV_LOG("\n%s(ERROR): bq2022 skip rom is FAIL!!!\n\n",__FUNCTION__);
		return -1;
	}

	bq2022_write_byte(chip,send_data[0]);
	bq2022_write_byte(chip,send_data[1]);
	bq2022_write_byte(chip,send_data[2]);
	
	crc_cmd =  bq2022_read_byte(chip);
	
	if(crc8_check(send_data,3,crc_cmd) < 0)
		return -1;
	
	for(i=0; i<BQ2022_DATA_LEN; i++)
		*(data+i) = bq2022_read_byte(chip);

	crc = bq2022_read_byte(chip);

	if(crc8_check(data,BQ2022_DATA_LEN,crc) < 0)
		return -1;
	else
		return 0;
}

/** Read Memory/Page CRC */
static int bq2022_read_mem_page(struct bq2022_device *chip,unsigned short page_addr,unsigned char *data)
{
	int i;
	unsigned char crc,crc_cmd;
	unsigned char send_data[3];

	bq2022_pins_config(chip);

	send_data[0] = BQ2022_MEM_CMD_READ_PAGE_CRC;
	send_data[1] = page_addr&0xff;	
	send_data[2] = (page_addr >> 8)&0xff;	

	if(bq2022_skip_rom(chip) < 0)
	{
		DEBUG("%s(ERROR): bq2022 skip rom is FAIL!!!\n\n",__FUNCTION__);
		return -1;
	}

	bq2022_write_byte(chip,send_data[0]);
	bq2022_write_byte(chip,send_data[1]);
	bq2022_write_byte(chip,send_data[2]);

	crc_cmd =  bq2022_read_byte(chip);
	
	if(crc8_check(send_data,3,crc_cmd) < 0)
	{
		DEBUG("%s(ERROR): Send cmd CRC check is ERROR!!!\n\n",__FUNCTION__);
		return -1;
	}
	
	for(i=0; i<BQ2022_PAGE_LEN; i++)
		*(data+i) = bq2022_read_byte(chip);

	crc = bq2022_read_byte(chip);

	if(crc8_check(data,BQ2022_PAGE_LEN,crc) < 0)
	{
		DEBUG("%s(ERROR): receive data CRC check is ERROR!!!\n\n",__FUNCTION__);
		DEBUG("\n%s read data:\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n0x%02x,0x%02x\n",__FUNCTION__,
				data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],
				data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],
				data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],
				data[24],data[25],data[26],data[27],data[28],data[29],data[30],data[31]);
		return -1;
	}
	else
		return 1;
}

static int bq2022_dump_all_data(struct bq2022_device *chip,unsigned char *data)
{
	int i;

	bq2022_read_mem_field(chip,data);
	
	MSG_LOG("\n%s:\n",__FUNCTION__);
	for(i=0; i<4; i++)
	{
		MSG_LOG("page%d:0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x\n",i,
				data[i*32+0],data[i*32+1],data[i*32+2],data[i*32+3],data[i*32+4],data[i*32+5],data[i*32+6],data[i*32+7],
				data[i*32+8],data[i*32+9],data[i*32+10],data[i*32+11],data[i*32+12],data[i*32+13],data[i*32+14],data[i*32+15],
				data[i*32+16],data[i*32+17],data[i*32+18],data[i*32+19],data[i*32+20],data[i*32+21],data[i*32+22],data[i*32+23],
				data[i*32+24],data[i*32+25],data[i*32+26],data[i*32+27],data[i*32+28],data[i*32+29],data[i*32+30],data[i*32+31]);
	}
	MSG_LOG("\n");
	return 1;
}

static int bq2022_read_page0(struct bq2022_device *chip,unsigned char *data)
{
	bq2022_read_mem_page(chip,0,data);
	
	MSG_LOG("\n%s page:\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n0x%02x,0x%02x\n",__FUNCTION__,
				data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],
				data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],
				data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],
				data[24],data[25],data[26],data[27],data[28],data[29],data[30],data[31]);
	return 0;
}

static int bq2022_distinguish_battery_id(struct bq2022_device *chip)
{	
	int i;
	int rows = chip->bat_data_row;
	int cols = chip->bat_data_col;
	struct battery_device_info *bat_info = chip->bat_info;
	unsigned char *real_bat_data = chip->real_bat_data;
	unsigned short vol_type;
	int bat_id = 0;

	for(i=0;i < rows;i++){
		vol_type = (unsigned short)real_bat_data[i*cols+7];
		vol_type = (unsigned short)real_bat_data[i*cols+6] | vol_type<<8;
		//pr_err("real_bat vol_type = 0x%04x \n",vol_type);
		if(bat_info->vol_type == vol_type &&
			bat_info->package_factory == real_bat_data[i*cols+8] && 
			bat_info->bat_factory == real_bat_data[i*cols+9]){
			bat_id = i + 1;
			break;
		}
	}
	chip->bat_id = bat_id;
	pr_err("bat_id = %d\n",bat_id);
	return bat_id;
}
static int bq2022_parse_battery_info(struct bq2022_device *chip,const char *buf,int maxlen)
{
	const char *ref;
	int len;
	int bat_id;

	ref = buf;
	memset(chip->bat_info,0,sizeof(*chip->bat_info));

	len = sizeof(chip->bat_info->company_name);
	memcpy(&chip->bat_info->company_name,ref,len);
	ref += len;

	len = sizeof(chip->bat_info->version);
	memcpy(&chip->bat_info->version,ref,len);
	ref += len;

	len = sizeof(chip->bat_info->vol_type);
	memcpy(&chip->bat_info->vol_type,ref,len);
	ref += len;

	len = sizeof(chip->bat_info->package_factory);
	memcpy(&chip->bat_info->package_factory,ref,len);
	ref += len;

	len = sizeof(chip->bat_info->bat_factory);
	memcpy(&chip->bat_info->bat_factory,ref,len);
	ref += len;

	len = sizeof(chip->bat_info->year);
	memcpy(&chip->bat_info->year,ref,len);
	ref += len;

	len = sizeof(chip->bat_info->month);
	memcpy(&chip->bat_info->month,ref,len);
	ref += len;
	
	len = sizeof(chip->bat_info->date);
	memcpy(&chip->bat_info->date,ref,len);
	ref += len;	

	pr_warn("company_name = 0x%08x,version = 0x%04x,vol_type = 0x%04x,package_factory = 0x%02x,"
		"bat_factory = 0x%02x,year = 0x%08x,moth=0x%04x,date=0x%04x\n",chip->bat_info->company_name,chip->bat_info->version,chip->bat_info->vol_type,
		chip->bat_info->package_factory,chip->bat_info->bat_factory,chip->bat_info->year,chip->bat_info->month,chip->bat_info->date);

	bat_id = bq2022_distinguish_battery_id(chip);

	return bat_id;

}

/** interface EPROM */
static int bq2022_read_bat_info(struct bq2022_device *chip)
{
	int ret;
	int i,j;
	unsigned short page_addr = 0;
	unsigned char data[32];
	int bat_id = -1;

	for(i=0;i<2;i++){
		for(j=0;j<3;j++){
			memset(data,0,sizeof(data));
			ret = bq2022_read_mem_page(chip,page_addr,data);
			if(ret > 0)
				break;
			else {
				bq2022_reset_vdd_data(chip);
				udelay(100);
			}
		}
		if(j==3 && page_addr == 0){
			page_addr = 0x20;
			continue;
		}else if(j==3 && page_addr == 0x20){
			bat_id = 0;
			break;
		}
		bat_id = bq2022_parse_battery_info(chip,data,sizeof(data));	
		if(i==0 && !bat_id){
			page_addr = 0x20;
			continue;
		}else if(i==0 && bat_id){
			break;
		}
	}
	if(bat_id)
		pr_warn("successfully ,company_name = 0x%08x,version = 0x%04x,vol_type = 0x%04x,package_factory = 0x%02x,"
			"bat_factory = 0x%02x,year = 0x%08x,moth=0x%04x,date=0x%04x\n",chip->bat_info->company_name,chip->bat_info->version,chip->bat_info->vol_type,
			chip->bat_info->package_factory,chip->bat_info->bat_factory,chip->bat_info->year,chip->bat_info->month,chip->bat_info->date);

	return bat_id;
}

/** interface EPROM end */

static ssize_t bq2022_show_read_rom(struct device *dev,struct device_attribute *attr, char *buf)
{
	unsigned char data[BQ2022_READ_ROM_LEN];

	if(bq2022_read_rom(the_chip,data) < 0)
		return sprintf(buf,"bq2022 read rom Fail!\n");
	else
		return sprintf(buf,"bq2022 read rom OK!\nbq2022 family and identification is 0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x!\n\n",
											data[0],data[1],data[2],data[3],data[4],data[5],data[6]);
}
static DEVICE_ATTR(bq2022_info,0440,bq2022_show_read_rom,NULL);

static ssize_t bq2022_show_dump_data(struct device *dev,struct device_attribute *attr, char *buf)
{
	unsigned char data[128];

	if(bq2022_dump_all_data(the_chip,data) < 0)
		return sprintf(buf,"bq2022 dump rom Fail!\n");
	else
		return sprintf(buf,"bq2022 dump rom OK!\n");
}
static DEVICE_ATTR(bq2022_data,0440,bq2022_show_dump_data,NULL);

static ssize_t bq2022_show_read_page0(struct device *dev,struct device_attribute *attr, char *buf)
{
	unsigned char data[32];

	if(bq2022_read_page0(the_chip,data) < 0)
		return sprintf(buf,"bq2022 read page Fail!\n");
	else
		return sprintf(buf,"bq2022 read page OK!\npage:\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,\n0x%02x,0x%02x\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8],data[9],data[10],data[11],data[12],data[13],data[14],data[15],data[16],data[17],data[18],data[19],data[20],data[21],data[22],data[23],data[24],data[25],data[26],data[27],data[28],data[29],data[30],data[31]);

}
static DEVICE_ATTR(bq2022_page0_data,0440,bq2022_show_read_page0,NULL);

static ssize_t bq2022_show_bat_type(struct device *dev,struct device_attribute *attr, char *buf)
{
	//struct bq2022_device *chip = container_of(dev, struct bq2022_device, dev);
	if(bq2022_read_bat_info(the_chip) < 0)
	{
		return sprintf(buf,"\n(ERROR): BBK GET bat type is ERROR!!!\n\n");
	}

	return sprintf(buf,"BBK Bat package is %d! bat is %d!\n\n",the_chip->bat_info->package_factory,the_chip->bat_info->bat_factory);
		
}
static DEVICE_ATTR(bq2022_bat_type,0440,bq2022_show_bat_type,NULL);

static struct attribute *bq2022_attrs[] = {
	&dev_attr_bq2022_info.attr,
	&dev_attr_bq2022_data.attr,
	&dev_attr_bq2022_page0_data.attr,
	&dev_attr_bq2022_bat_type.attr,
	NULL,
};

static struct attribute_group bq2022_attr_group = {
	.attrs = bq2022_attrs,
};

static int batid_get_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	int ret = 0;
	struct bq2022_device *chip = container_of(psy, struct bq2022_device, batid_psy);
	switch(psp){
		case POWER_SUPPLY_PROP_CHARGE_TYPE:
			//val->strval = "xxx";
			if(!chip->bat_id){
				mutex_lock(&chip->battid_lock);
				ret = bq2022_read_bat_info(chip);
				if(ret <= 0)
				{
					pr_err("read bat type ERROR!!!\n");
					chip->bat_id = chip->def_bat_id;
				}
				mutex_unlock(&chip->battid_lock);
			}
			val->intval = chip->bat_id;
			break;
		case POWER_SUPPLY_PROP_MODEL_NAME:
			val->strval = "bq2022a";
			break;
		default:
			ret = -EINVAL;
	}
	return ret;
}
static int batid_set_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       const union power_supply_propval *val)
{
	int ret = 0;
	//struct bq2022_device *chip = container_of(psy, struct bq2022_device, batid_psy);
	switch(psp){
		default:
			ret = -EINVAL;
	}
	return ret;
}

static int bq2022_register_psy(struct bq2022_device *chip)
{
	int ret;
	chip->batid_psy.name = "batid";
	chip->batid_psy.supplied_to = batid_power_supplied_to;
	chip->batid_psy.num_supplicants = ARRAY_SIZE(batid_power_supplied_to);
	chip->batid_psy.properties = batid_power_props;
	chip->batid_psy.num_properties = ARRAY_SIZE(batid_power_props);
	chip->batid_psy.get_property = batid_get_property;
	chip->batid_psy.set_property = batid_set_property;
	ret = power_supply_register(chip->dev, &chip->batid_psy);
	if (ret) {
		pr_err("failed to register power_supply. ret=%d.\n", ret);
		return ret;
	}

	return 0;
}

static int bq2022_parse_dt(struct bq2022_device *chip)
{
	struct device_node *node = chip->dev->of_node;
	int ret;
	struct property *prop;
	const __be32 *data;
	int cols, rows, size, i, j;

	if (!node) {
		dev_err(chip->dev, "device tree info. missing\n");
		return -EINVAL;
	}
	chip->data_gpio = of_get_named_gpio(node, "ti,data-gpio", 0);
	if (chip->data_gpio < 0) {
		pr_err("failed to get data-gpio.\n");
		goto out;
	}

	chip->power_gpio = of_get_named_gpio(node, "ti,power-gpio", 0);
	if (chip->power_gpio < 0) {
		pr_err("failed to get power-gpio.\n");
		goto out;
	}

	ret = of_property_read_u32(node, "vivo,def-bat-id",
						&(chip->def_bat_id));
	if (ret) {
		pr_err("Unable to read def_bat_id.\n");
		chip->def_bat_id = 0;
	}

	ret = of_property_read_u32(node, "vivo,bat-data-col",
				   		&(chip->bat_data_col));
	if (ret) {
		pr_err("Unable to read bat_data_col.\n");
		return ret;
	}
	cols = chip->bat_data_col;
	ret = of_property_read_u32(node, "vivo,bat-data-row",
				   		&(chip->bat_data_row));
	if (ret) {
		pr_err("Unable to read bat_data_row.\n");
		return ret;
	}
	rows = chip->bat_data_row;
	if(rows*cols){
		chip->real_bat_data = kzalloc(sizeof(unsigned char) * cols * rows, GFP_KERNEL);
		if (!chip->real_bat_data) {
			pr_err("%s kzalloc failed %d\n", __func__, __LINE__);
		}

		prop = of_find_property(node, "vivo,bat-data", NULL);
		if (!prop) {
			pr_err("prop 'qcom,lut-data' not found\n");
			kfree(chip->real_bat_data);
			return -EINVAL;
		}
		data = prop->value;
		size = prop->length/sizeof(int);
		if (size != cols * rows) {
			pr_err("%s: data size mismatch, %dx%d != %d\n",
					node->name, cols, rows, size);
			kfree(chip->real_bat_data);
			return -EINVAL;
		}
		for (i = 0; i < rows; i++) {
			for (j = 0; j < cols; j++) {
				(chip->real_bat_data)[i*cols+j] = (unsigned char)be32_to_cpup(data++);
				//pr_err("Value = 0x%x\n", (chip->real_bat_data)[i*cols+j]);
			}
		}
	}
	return 0;
out:
	return -EINVAL;
}

static int bq2022_probe(struct platform_device *pdev)
{
	int ret;
	struct bq2022_device *chip;
	struct device_node *dev_node = pdev->dev.of_node;
	struct battery_device_info *bat_info;

	printk("## %s: bq2022 probe!!! ##\n",__FUNCTION__);
	
	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip) {
		pr_err("alloc fail.\n");
		return -ENOMEM;
	}
	
	bat_info = kzalloc(sizeof(*bat_info),GFP_KERNEL);
	if (!bat_info) {
		pr_err("failed to allocate bat info\n");
		ret = -ENOMEM;
		goto batt_alloc_err;
	}

	chip->dev = &pdev->dev;
	chip->bat_info = bat_info;
	platform_set_drvdata(pdev,chip);
	the_chip = chip;
	mutex_init(&chip->battid_lock);

	if (dev_node){
		ret = bq2022_parse_dt(chip);
		if (ret < 0) {
			dev_err(chip->dev, "Unable to parse DT nodes\n");
			goto err_data_gpio;
		}
	}

	if(chip->data_gpio){
		ret =  gpio_request_one(chip->data_gpio, GPIOF_DIR_OUT, "bq2022_data_gpio");
		if (ret) {
			pr_err("failed to request data_gpio\n");
			goto err_data_gpio;
		}
	}

	if(chip->power_gpio){
		ret =  gpio_request_one(chip->power_gpio, GPIOF_DIR_OUT, "bq2022_power_gpio");
		if (ret) {
			pr_err("failed to request power_gpio\n");
			goto err_power_gpio;
		}
	}

	mutex_lock(&chip->battid_lock);
	ret = bq2022_read_bat_info(chip);
	if(ret <= 0)
	{
		pr_err("read bat type ERROR!!!\n");
		chip->bat_id = chip->def_bat_id;
	}
	mutex_unlock(&chip->battid_lock);

	ret = bq2022_register_psy(chip);
	if(ret){
		pr_err("bq2022 register psy ERROR!!!\n");
		goto err_register_psy;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &bq2022_attr_group);
	if (ret) {
		pr_err("create attr error: %d\n", ret);
		goto err_sysfs_create;
	}

	printk("## %s: bq2022 end!!! ##\n",__FUNCTION__);

	return 0;
err_sysfs_create:
err_register_psy:
	if (chip->power_gpio)
		gpio_free(chip->power_gpio);
err_power_gpio:
	if (chip->data_gpio)
		gpio_free(chip->data_gpio);
err_data_gpio:
	kfree(bat_info);
batt_alloc_err:
	kfree(chip);

	return ret;

}

static int bq2022_remove(struct platform_device *pdev)
{
	struct bq2022_device *chip = platform_get_drvdata(pdev);

	kfree(chip->bat_info);
	kfree(chip);

	return 0;
}

static const struct of_device_id bq2022_match[] = {
	{ .compatible = "ti,bq2022a", },
	{ },
};

static const struct platform_device_id bq2022_id[] = {
	{ "bq2022a", 0 },
	{ },
};

MODULE_DEVICE_TABLE(platform, bq2022_id);

static struct platform_driver bq2022_driver = {
	.driver = {
		.name = "bq2022a",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(bq2022_match),
	},
	.probe = bq2022_probe,
	.remove = bq2022_remove,
	.id_table = bq2022_id,
};

static int __init bq2022_init(void)
{
	return platform_driver_register(&bq2022_driver);
}
subsys_initcall(bq2022_init);

static void __exit bq2022_exit(void)
{
	platform_driver_unregister(&bq2022_driver);
}
module_exit(bq2022_exit);

MODULE_DESCRIPTION("bq2022 battery id driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yangzh");
