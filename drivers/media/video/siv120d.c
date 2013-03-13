/*
 * SETi Co.,Ltd. VGA CMOS IMAGE SENSOR Driver
 *
 * A V4L2 driver for SETi SIV120D cameras.
 *
 * Copyright (C) 2011 Quanta Computer Inc.
 * Written by Peyton Huang <Peyton.Huang@quantatw.com>
 *
 * Based on ov7670 camera driver,
 * Copyright 2006-7 Jonathan Corbet <corbet@lwn.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <media/siv120d.h>

/* TODO: add debug switch */
/* static int debug; */
/* module_param(debug, bool, 0644); */
/* MODULE_PARM_DESC(debug, "Debug level (0-1)"); */
static int debug = 1;

struct regval_list {
	u16 reg_num;
	u8 value;
};

static const struct regval_list siv120d_default_regs[] = {
	/* SNR */
	{0x04, 0x00},
	{0x05, 0x03},
	/* {0x07, 0x33}, */
	{0x07, 0x32},
	{0x10, 0x34},
	{0x11, 0x27},
	{0x12, 0x21},
	{0x16, 0xce},
	{0x17, 0xaa},

	{0x20, 0x00},	/* P_BNKT */
	{0x21, 0x01},	/* P_HBNKT */
	{0x22, 0x01},	/* P_ROWFIL */
	{0x23, 0x01},	/* P_VBNKT */

	/* AE */
	{0x111, 0x14},	/* 6 fps at lowlux */
	{0x112, 0x78},	/* D65 target 0x74 */
	{0x113, 0x78},	/* CWF target 0x74 */
	{0x114, 0x78},	/* A target   0x74 */
	{0x11d, 0x04},
	{0x11E, 0x08},	/* ini gain  0x08 */
	{0x134, 0x7d},
	{0x140, 0x60},	/* Max x8 */

	{0x170, 0xd4},	/* anti-sat on */
	{0x174, 0x07},	/* anti-sat ini */
	{0x179, 0x69},	/* anti-sat */

	/* AWB */
	{0x210, 0xd0},
	{0x211, 0xc0},
	{0x212, 0x80},
	{0x213, 0x7f},
	{0x214, 0x7f},
	{0x215, 0xfe},	/* R gain Top */
	{0x216, 0x80},	/* R gain bottom */
	{0x217, 0xcb},	/* B gain Top */
	{0x218, 0x70},	/* B gain bottom 0x80 */
	{0x219, 0x94},	/* Cr top value 0x90 */
	{0x21a, 0x6c},	/* Cr bottom value 0x70 */
	{0x21b, 0x94},	/* Cb top value 0x90 */
	{0x21c, 0x6c},	/* Cb bottom value 0x70 */
	{0x21d, 0x94},	/* 0xa0 */
	{0x21e, 0x6c},	/* 0x60 */
	{0x220, 0xe8},	/* AWB luminous top value */
	{0x221, 0x30},	/* AWB luminous bottom value 0x20 */
	{0x222, 0xa4},
	{0x223, 0x20},
	{0x224, 0x20},
	{0x226, 0x0f},
	{0x227, 0x01},	/* BRTSRT */
	{0x228, 0xb4},	/* BRTRGNTOP result 0xad */
	{0x229, 0xb0},	/* BRTRGNBOT */
	{0x22a, 0x92},	/* BRTBGNTOP result 0x90 */
	{0x22b, 0x8e},	/* BRTBGNBOT */
	{0x22c, 0x88},	/* RGAINCONT */
	{0x22d, 0x88},	/* BGAINCONT */

	{0x230, 0x00},
	{0x231, 0x10},
	{0x232, 0x00},
	{0x233, 0x10},
	{0x234, 0x02},
	{0x235, 0x76},
	{0x236, 0x01},
	{0x237, 0xd6},
	{0x240, 0x01},
	{0x241, 0x04},
	{0x242, 0x08},
	{0x243, 0x10},
	{0x244, 0x12},
	{0x245, 0x35},
	{0x246, 0x64},
	{0x250, 0x33},
	{0x251, 0x20},
	{0x252, 0xe5},
	{0x253, 0xfb},
	{0x254, 0x13},
	{0x255, 0x26},
	{0x256, 0x07},
	{0x257, 0xf5},
	{0x258, 0xea},
	{0x259, 0x21},

	{0x262, 0x88},	/* G gain */

	{0x263, 0xb3},	/* R D30 to D20 */
	{0x264, 0xc3},	/* B D30 to D20 */
	{0x265, 0xb3},	/* R D20 to D30 */
	{0x266, 0xc3},	/* B D20 to D30 */

	{0x267, 0xdd},	/* R D65 to D30 */
	{0x268, 0xa0},	/* B D65 to D30 */
	{0x269, 0xdd},	/* R D30 to D65 */
	{0x26a, 0xa0},	/* B D30 to D65 */

	/* IDP */
	{0x310, 0xff},
	{0x311, 0x1d},
	{0x312, 0x3d},
	{0x314, 0x04},	/* don't change */

	/* DPCNR */
	{0x318, 0x00},	/* DPTHR */
	{0x319, 0x56},	/* C DP Number
			 * ( Normal [7:6] Dark [5:4] ) | [3:0] DPTHRMIN */
	{0x31A, 0x56},	/* G DP Number
			 * ( Normal [7:6] Dark [5:4] ) | [3:0] DPTHRMAX */
	{0x31B, 0x12},	/* DPTHRSLP( [7:4] @ Normal | [3:0] @ Dark ) */
	{0x31C, 0x04},	/* NRTHR */
	{0x31D, 0x00},	/* [5:0] NRTHRMIN 0x48 */
	{0x31E, 0x00},	/* [5:0] NRTHRMAX 0x48 */
	{0x31F, 0x08},	/* NRTHRSLP( [7:4] @ Normal | [3:0] @ Dark )  0x2f */
	{0x320, 0x04},	/* IllumiInfo STRTNOR */
	{0x321, 0x0f},	/* IllumiInfo STRTDRK */

	/* Gamma */
	{0x330, 0x00},	/* 0x0 */
	{0x331, 0x04},	/* 0x3 */
	{0x332, 0x0b},	/* 0xb */
	{0x333, 0x24},	/* 0x1f */
	{0x334, 0x49},	/* 0x43 */
	{0x335, 0x66},	/* 0x5f */
	{0x336, 0x7c},	/* 0x74 */
	{0x337, 0x8d},	/* 0x85 */
	{0x338, 0x9b},	/* 0x94 */
	{0x339, 0xaa},	/* 0xA2 */
	{0x33a, 0xb6},	/* 0xAF */
	{0x33b, 0xca},	/* 0xC6 */
	{0x33c, 0xdc},	/* 0xDB */
	{0x33d, 0xef},	/* 0xEF */
	{0x33e, 0xf8},	/* 0xF8 */
	{0x33f, 0xff},	/* 0xFF */

	/* Shading Register Setting */
	{0x340, 0x11},
	{0x341, 0x11},
	{0x342, 0x22},
	{0x343, 0x33},
	{0x344, 0x44},
	{0x345, 0x55},
	{0x346, 0x12},	/* left R gain[7:4], right R gain[3:0] */
	{0x347, 0x20},	/* top R gain[7:4], bottom R gain[3:0] */
	{0x348, 0x01},	/* left Gr gain[7:4], right Gr gain[3:0] 0x21 */
	{0x349, 0x20},	/* top Gr gain[7:4], bottom Gr gain[3:0] */
	{0x34a, 0x01},	/* left Gb gain[7:4], right Gb gain[3:0] 0x02 */
	{0x34b, 0x20},	/* top Gb gain[7:4], bottom Gb gain[3:0] */
	{0x34c, 0x01},	/* left B gain[7:4], right B gain[3:0] */
	{0x34d, 0x00},	/* top B gain[7:4], bottom B gain[3:0] */
	{0x34e, 0x04},	/* X-axis center high[3:2], Y-axis center high[1:0] */
	{0x34f, 0x50},	/* X-axis center low[7:0] 0x50 */
	{0x350, 0xd0},	/* Y-axis center low[7:0] 0xf6 */
	{0x351, 0x80},	/* Shading Center Gain */
	{0x352, 0x00},	/* Shading R Offset */
	{0x353, 0x00},	/* Shading Gr Offset */
	{0x354, 0x00},	/* Shading Gb Offset */
	{0x355, 0x00},	/* Shading B Offset */

	/* Interpolation */
	{0x360, 0x57},	/* INT outdoor condition */
	{0x361, 0xff},	/* INT normal condition */

	{0x362, 0x77},	/* ASLPCTRL 7:4 GE, 3:0 YE */
	{0x363, 0x38},	/* YDTECTRL (YE) [7] fixed, */
	{0x364, 0x38},	/* GPEVCTRL (GE) [7] fixed, */

	{0x366, 0x0c},	/* SATHRMIN */
	{0x367, 0xff},
	{0x368, 0x04},	/* SATHRSRT */
	{0x369, 0x08},	/* SATHRSLP */

	{0x36a, 0xaf},	/* PTDFATHR [7] fixed, [5:0] value */
	{0x36b, 0x78},	/* PTDLOTHR [6] fixed, [5:0] value */

	{0x36d, 0x84},	/* YFLTCTRL */

	/* Color matrix (D65) - Daylight */
	{0x371, 0x42},	/* 0x40 */
	{0x372, 0xbf},	/* 0xb9 */
	{0x373, 0x00},	/* 0x07 */
	{0x374, 0x0f},	/* 0x15 */
	{0x375, 0x31},	/* 0x21 */
	{0x376, 0x00},	/* 0x0a */
	{0x377, 0x00},	/* 0xf8 */
	{0x378, 0xbc},	/* 0xc5 */
	{0x379, 0x44},	/* 0x46 */

	/* Color matrix (D30) - CWF */
	{0x37a, 0x56},	/* 0x3a */
	{0x37b, 0xbf},	/* 0xcd */
	{0x37c, 0xeb},	/* 0xfa */
	{0x37d, 0x1a},	/* 0x12 */
	{0x37e, 0x22},	/* 0x2c */
	{0x37f, 0x04},	/* 0x02 */
	{0x380, 0xdc},	/* 0xf7 */
	{0x381, 0xc9},	/* 0xc7 */
	{0x382, 0x5b},	/* 0x42 */

	/* Color matrix (D20) - A */
	{0x383, 0x4d},	/* 0x38 */
	{0x384, 0xc0},	/* 0xc4 */
	{0x385, 0xf3},	/* 0x04 */
	{0x386, 0x18},	/* 0x07 */
	{0x387, 0x24},	/* 0x25 */
	{0x388, 0x04},	/* 0x14 */
	{0x389, 0xe0},	/* 0xf0 */
	{0x38a, 0xcb},	/* 0xc2 */
	{0x38b, 0x55},	/* 0x4f */

	{0x38c, 0x10},	/* CMA select */

	{0x38d, 0xa4},	/* programmable edge */
	{0x38e, 0x06},	/* PROGEVAL */
	{0x38f, 0x00},	/* Cb/Cr coring */

	{0x390, 0x15},	/* GEUGAIN */
	{0x391, 0x15},	/* GEUGAIN */
	{0x392, 0xf0},	/* Ucoring [7:4] max, [3:0] min */
	{0x394, 0x00},	/* Uslope (1/128) */
	{0x396, 0xf0},	/* Dcoring [7:4] max, [3:0] min */
	{0x398, 0x00},	/* Dslope (1/128) */

	{0x39a, 0x08},
	{0x39b, 0x18},

	{0x39f, 0x0c},	/* YEUGAIN */
	{0x3a0, 0x0c},	/* YEUGAIN */
	{0x3a1, 0x33},	/* Yecore [7:4]upper [3:0]down */

	{0x3a9, 0x10},	/* Cr saturation 0x12 */
	{0x3aa, 0x10},	/* Cb saturation 0x12 */
	{0x3ab, 0x82},	/* Brightness */
	{0x3ae, 0x40},	/* Hue */
	{0x3af, 0x86},	/* Hue */
	{0x3b9, 0x10},	/* 0x20 lowlux color */
	{0x3ba, 0x20},	/* 0x10 lowlux color */

	/* Window mode settings */
	{REG_WDATH, 0x24},
	{REG_WHSRTL, 0x00},
	{REG_WHWIDL, 0x80},
	{REG_WVSRTL, 0x00},
	{REG_WVWIDL, 0xe0},

	/* inverse color space conversion */
	{0x3cc, 0x40},
	{0x3cd, 0x00},
	{0x3ce, 0x58},
	{0x3cf, 0x40},
	{0x3d0, 0xea},
	{0x3d1, 0xd3},
	{0x3d2, 0x40},
	{0x3d3, 0x6f},
	{0x3d4, 0x00},

	/* ee nr */
	{0x3d9, 0x08},
	{0x3da, 0x1f},
	{0x3db, 0x05},
	{0x3dc, 0x08},
	{0x3dd, 0x3c},
	{0x3de, 0xfb},	/* NOIZCTRL */

	/* dark offset */
	{0x3df, 0x10},
	{0x3e0, 0x60},
	{0x3e1, 0x90},
	{0x3e2, 0x08},
	{0x3e3, 0x0a},

	/* memory speed */
	{0x3e5, 0x15},
	{0x3e6, 0x20},
	{0x3e7, 0x04},

	{0xff, 0xff},
};

static const struct regval_list siv120d_yuv422_regs[] = {
	{REG_OUTFMT, 0x3d},
	{0xff, 0xff},
};

static const struct regval_list siv120d_rgb565_regs[] = {
	{REG_OUTFMT, 0xcb},
	{0xff, 0xff},
};

/*
 * If the desired resolution is standard VGA/QVGA/QQVGA, it might be more
 * intuitive to just set REG_VMODE register. But there seems to be a bug in
 * SIV120D's firmware that sometimes generate `tilted' image when we rapidly
 * turn on and off the sensor (e.g. open /dev/video0, read some status, and
 * close the device in very short time). Thus it would be safer to always
 * set window mode parameters to workaround this issue when switching
 * resolution.
 */
static const struct regval_list siv120d_vga_regs[] = {
	{REG_VMODE, 0x2},
	{REG_WDATH, 0x24},
	{REG_WHSRTL, 0x00},
	{REG_WHWIDL, 0x80},
	{REG_WVSRTL, 0x00},
	{REG_WVWIDL, 0xe0},
	{0xff, 0xff},
};

static const struct regval_list siv120d_qvga_regs[] = {
	{REG_VMODE, 0x1},
	{REG_WDATH, 0x10},
	{REG_WHSRTL, 0x00},
	{REG_WHWIDL, 0x40},
	{REG_WVSRTL, 0x00},
	{REG_WVWIDL, 0xf0},
	{0xff, 0xff},
};

static const struct regval_list siv120d_qqvga_regs[] = {
	{REG_VMODE, 0x0},
	{REG_WDATH, 0x00},
	{REG_WHSRTL, 0x00},
	{REG_WHWIDL, 0xa0},
	{REG_WVSRTL, 0x00},
	{REG_WVWIDL, 0x78},
	{0xff, 0xff},
};

static const struct siv120d_format_struct {
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
	const struct regval_list *regs;
} siv120d_formats[] = {
	{
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace	= V4L2_COLORSPACE_JPEG,
		.regs 		= siv120d_yuv422_regs,
	},
	{
		.mbus_code	= V4L2_MBUS_FMT_RGB565_2X8_LE,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs		= siv120d_rgb565_regs,
	},
};
#define N_SIV120D_FMTS ARRAY_SIZE(siv120d_formats)

static const struct siv120d_win_size {
	int	width;
	int	height;
	const struct regval_list *regs; /* Regs to tweak */
/* h/vref stuff */
} siv120d_win_sizes[] = {
	/* VGA */
	{
		.width		= 640,
		.height		= 480,
		.regs 		= siv120d_vga_regs,
	},
	/* QVGA */
	{
		.width		= 320,
		.height		= 240,
		.regs 		= siv120d_qvga_regs,
	},
	/* QQVGA */
	{
		.width		= 160,
		.height		= 120,
		.regs 		= siv120d_qqvga_regs,
	},
};
#define N_WIN_SIZES (ARRAY_SIZE(siv120d_win_sizes))

/* Currently for VGA size only. */
static struct siv120d_frame_rate {
	int fps;
	int vblank;
	int hblank;
	int clkdiv;		/* Pixel clock divider */
	int shutter_step;	/* Anti-flicker shutter step */
} siv120d_frame_rates[] = {
	/* fps field must be in decending order for searching */
	{
		.fps		= 30,
		.vblank		= 1,
		.hblank		= 1,
		.clkdiv		= 0,
		.shutter_step	= 0x7d,
	},
	{
		.fps		= 25,
		.vblank		= 126,
		.hblank		= 1,
		.clkdiv		= 0,
		.shutter_step	= 0x7d,
	},
	{
		.fps		= 20,
		.vblank		= 251,
		.hblank		= 1,
		.clkdiv		= 0,
		.shutter_step	= 0x7d,
	},
	{
		.fps		= 15,
		.vblank		= 1,
		.hblank		= 1,
		.clkdiv		= 1,
		.shutter_step	= 0x3e,
	},
	{
		.fps		= 10,
		.vblank		= 251,
		.hblank		= 1,
		.clkdiv		= 1,
		.shutter_step	= 0x3e,
	},
	{
		.fps		= 5,
		.vblank		= 251,
		.hblank		= 1,
		.clkdiv		= 2,
		.shutter_step	= 0x1f,
	},
};
#define N_SIV120D_FRAME_RATES (ARRAY_SIZE(siv120d_frame_rates))

struct siv120d_info {
	struct v4l2_subdev sd;
	const struct siv120d_format_struct *fmt;  /* Current format */
	const struct siv120d_frame_rate *frame_rate;
	const struct siv120d_win_size *wsize;
	unsigned int sat;		/* Saturation value */
	int hue;			/* Hue value */
	int min_width;			/* Filter out smaller sizes */
	int min_height;			/* Filter out smaller sizes */
	int ae_enable;			/* Auto-Exposure enabled */
	int ae_target;			/* Target luminance for AE */
	struct mutex reg_mutex;		/* Mutex for accessing registers */
	u8 bank;			/* Current register bank */
	u8 mclkdiv;			/* Master pixel clock divider */
	bool use_smbus; /* Use smbus I/O instead of I2C */
};

static inline struct siv120d_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct siv120d_info, sd);
}

/* We provide communication via both SMBUS (for XO-1) and I2C (for XO-1.5
 * and newer) APIs. There is no significant electrical difference between
 * these interfaces, the key factor is that XO-1 cannot send individual
 * I2C commands, it instead operates only in terms of SMbus-protocol-level
 * register writes/reads.
 *
 * FIXME: we can probably simplify this. It looks like the i2c read/write
 * functions below just use the smbus protocol. And if a linux i2c host
 * adapter does not support smbus directly, it will be emulated via the
 * same i2c commands we use below. So we could only provide the smbus
 * functions below and things should work as they do currently, on all
 * platforms.
 */
static int siv120d_read_smbus(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret >= 0) {
		*value = (unsigned char)ret;
		ret = 0;
	}
	return ret;
}

static int siv120d_write_smbus(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	return i2c_smbus_write_byte_data(client, reg, value);
}

static int siv120d_read_i2c(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char *value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	u8 data = reg;
	struct i2c_msg msg;
	int ret;

	/*
	 * Send out the register address...
	 */
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 1;
	msg.buf = &data;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		v4l_err(client, "Error %d on register write\n", ret);
		return ret;
	}
	/*
	 * ...then read back the result.
	 */
	msg.flags = I2C_M_RD;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret >= 0) {
		*value = data;
		ret = 0;
	}
	return ret;
}

static int siv120d_write_i2c(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct i2c_msg msg;
	unsigned char data[2] = { reg, value };
	int ret;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2;
	msg.buf = data;
	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret > 0)
		ret = 0;
	return ret;
}

/*
 * siv120d_info.reg_mutex should be held before calling this function
 */
static inline int siv120d_switch_bank(struct v4l2_subdev *sd, u8 bank)
{
	struct siv120d_info *info = to_state(sd);

	if (info->bank == bank)
		return 0;

	info->bank = bank;
	if (info->use_smbus)
		return siv120d_write_smbus(sd, REG_BLK_SEL, bank);
	else
		return siv120d_write_i2c(sd, REG_BLK_SEL, bank);
}

static int siv120d_write_register(struct v4l2_subdev *sd, u16 reg, u8 value)
{
	int ret = 0;
	struct siv120d_info *info = to_state(sd);

	mutex_lock(&info->reg_mutex);

	ret = siv120d_switch_bank(sd, g_bank(reg));
	if (ret < 0)
		goto out;

	if (info->use_smbus)
		ret = siv120d_write_smbus(sd, g_reg(reg), value);
	else
		ret = siv120d_write_i2c(sd, g_reg(reg), value);

out:
	mutex_unlock(&info->reg_mutex);
	return ret;
}

static int siv120d_read_register(struct v4l2_subdev *sd, u16 reg, u8 *value)
{
	int ret = 0;
	struct siv120d_info *info = to_state(sd);

	mutex_lock(&info->reg_mutex);

	ret = siv120d_switch_bank(sd, g_bank(reg));
	if (ret < 0)
		goto out;

	if (info->use_smbus)
		ret = siv120d_read_smbus(sd, g_reg(reg), value);
	else
		ret = siv120d_read_i2c(sd, g_reg(reg), value);

out:
	mutex_unlock(&info->reg_mutex);
	return ret;
}

/*
 * Write a list of register settings; ff/ff stops the process.
 */
static int siv120d_write_array(struct v4l2_subdev *sd,
				const struct regval_list *vals)
{
	while (vals->reg_num != 0xff || vals->value != 0xff) {
		int ret = siv120d_write_register(sd, vals->reg_num,
							vals->value);
		if (ret < 0)
			return ret;
		vals++;
	}
	return 0;
}

static int siv120d_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SIV120D, 0);
}

#define SIN_STEP 5
/*
 * sine table is generated using the following formula:
 * round(sin(theta)*64), where theta = 0, 5, 10, ..., 90 degs
 *
 */
static const unsigned char siv120d_sin_table[] = {
	0, 6, 11, 17, 22,
	27, 32, 37, 41, 45,
	49, 52, 55, 58, 60,
	62, 63, 64, 64
};

static unsigned char siv120d_sine(int theta)
{
	unsigned char chs = 0;
	unsigned char sine;

	if (theta < 0) {
		theta = -theta;
		chs = 0x80;
	}
	if (theta <= 90)
		sine = siv120d_sin_table[theta/SIN_STEP];
	else {
		theta -= 90;
		sine = 64 - siv120d_sin_table[theta/SIN_STEP];
	}
	return sine ^ chs;
}

static unsigned char siv120d_cosine(int theta)
{
	theta = 90 - theta;
	if (theta > 180)
		theta -= 360;
	else if (theta < -180)
		theta += 360;
	return siv120d_sine(theta);
}

static int siv120d_s_sat(struct v4l2_subdev *sd, int value)
{
	struct siv120d_info *info = to_state(sd);
	int ret;

	if(value < 0 || value > 63)
		return -ERANGE;

	info->sat = value;

	ret = siv120d_write_register(sd, REG_CRGAIN, value);
	if(ret < 0)
		return ret;
	ret = siv120d_write_register(sd, REG_CBGAIN, value);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv120d_g_sat(struct v4l2_subdev *sd, __s32 *value)
{
	struct siv120d_info *info = to_state(sd);
	*value = info->sat;
	return 0;
}

static int siv120d_s_hue(struct v4l2_subdev *sd, int value)
{
	struct siv120d_info *info = to_state(sd);
	int ret;
	unsigned char sinth, costh;

	if (value < -180 || value > 180)
		return -ERANGE;

	info->hue = value;

	sinth = siv120d_sine(value);
	costh = siv120d_cosine(value);

	ret = siv120d_write_register(sd, REG_HUECOS, costh);
	if(ret < 0)
		return ret;
	ret = siv120d_write_register(sd, REG_HUESIN, sinth);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv120d_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	struct siv120d_info *info = to_state(sd);
	*value = info->hue;
	return 0;
}

static int siv120d_s_brightness(struct v4l2_subdev *sd, int value)
{
	int ret;

	if (value < -127 || value > 127)
		return -ERANGE;

	if(value < 0) {
		value = -value;
		value |= 0x80;
	}

	ret = siv120d_write_register(sd, REG_BRTCNT, (unsigned char)value);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv120d_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	unsigned char tmp;
	int ret, v;

	ret = siv120d_read_register(sd, REG_BRTCNT, &tmp);
	if(ret < 0)
		return ret;

	v = tmp & BRTCNT_MASK;
	v = (tmp & BRTCNT_SIGN) ? -v : v;

	*value = v;

	return 0;
}

static int siv120d_s_contrast(struct v4l2_subdev *sd, int value)
{
	int ret;

	if (value < 0 || value > 63)
		return -ERANGE;

	ret = siv120d_write_register(sd, REG_CONTGAIN,
		(unsigned char)(value | CONTGAIN_ENABLE));
	if(ret < 0)
		return ret;

	return 0;
}

static int siv120d_g_contrast(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	unsigned char tmp;

	ret = siv120d_read_register(sd, REG_CONTGAIN, &tmp);
	if(ret < 0)
		return ret;
	*value = tmp & CONTGAIN_MASK;
	return 0;
}

static int siv120d_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	unsigned char tmp;

	ret = siv120d_read_register(sd, REG_CNTR_B, &tmp);
	if(ret < 0)
		return ret;

	*value = (tmp & CNTR_B_HFLIP) == CNTR_B_HFLIP;

	return 0;
}


static int siv120d_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	unsigned char tmp;

	ret = siv120d_read_register(sd, REG_CNTR_B, &tmp);
	if(ret < 0)
		return ret;

	if(value)
		tmp |= CNTR_B_HFLIP;
	else
		tmp &= ~CNTR_B_HFLIP;

	ret = siv120d_write_register(sd, REG_CNTR_B, tmp);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv120d_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	unsigned char tmp;

	ret = siv120d_read_register(sd, REG_CNTR_B, &tmp);
	if(ret < 0)
		return ret;

	*value = (tmp & CNTR_B_VFLIP) == CNTR_B_VFLIP;

	return 0;
}

static int siv120d_s_vflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	unsigned char tmp;

	ret = siv120d_read_register(sd, REG_CNTR_B, &tmp);
	if(ret < 0)
		return ret;

	if(value)
		tmp |= CNTR_B_VFLIP;
	else
		tmp &= ~CNTR_B_VFLIP;

	ret = siv120d_write_register(sd, REG_CNTR_B, tmp);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv120d_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	unsigned char tmp;

	ret = siv120d_read_register(sd, REG_AGAIN, &tmp);
	if(ret < 0)
		return ret;

	*value = tmp;

	return 0;
}

static int siv120d_s_gain(struct v4l2_subdev *sd, int value)
{
	struct siv120d_info *info = to_state(sd);
	int ret;

	if(info->ae_enable)
		return -EBUSY;

	if (value < 0 || value > 0x9f)
		return -ERANGE;

	ret = siv120d_write_register(sd, REG_AGAIN, (unsigned char)value);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv120d_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	/* Not feasible for siv120d? AE also handles gain control */
	*value = 0;
	return 0;
}

static int siv120d_s_autogain(struct v4l2_subdev *sd, int value)
{
	/* Not feasible for siv120d? AE also handles gain control */
	return 0;
}

static int siv120d_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct siv120d_info *info = to_state(sd);
	*value = info->ae_target;
	return 0;
}

static int siv120d_s_exp(struct v4l2_subdev *sd, int value)
{
	struct siv120d_info *info = to_state(sd);
	int ret;

	/* Exposure target values are input parameters for AE */
	if(!info->ae_enable)
		return -EBUSY;

	if (value < 0 || value > 0xff)
		return -ERANGE;

	info->ae_target = value;

	ret = siv120d_write_register(sd, REG_Y_TARGET_N, (unsigned char)value);
	if(ret < 0)
		return ret;
	ret = siv120d_write_register(sd, REG_Y_TARGET_CWF, (unsigned char)value);
	if(ret < 0)
		return ret;
	ret = siv120d_write_register(sd, REG_Y_TARGET_A, (unsigned char)value);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv120d_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	unsigned char tmp;

	ret = siv120d_read_register(sd, REG_AE_CNTR, &tmp);
	if(ret < 0)
		return ret;

	*value = ((tmp & AE_CNTR_AE_ENABLE) == AE_CNTR_AE_ENABLE) ?
			V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;

	return 0;
}

static int siv120d_s_autoexp(struct v4l2_subdev *sd,
		enum v4l2_exposure_auto_type value)
{
	struct siv120d_info *info = to_state(sd);
	int ret;
	unsigned char tmp;

	ret = siv120d_read_register(sd, REG_AE_CNTR, &tmp);
	if(ret < 0)
		return ret;

	if(value == V4L2_EXPOSURE_AUTO) {
		tmp |= AE_CNTR_AE_ENABLE;
		info->ae_enable = 1;
	}
	else {
		tmp &= ~AE_CNTR_AE_ENABLE;
		info->ae_enable = 0;
	}

	ret = siv120d_write_register(sd, REG_AE_CNTR, tmp);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv120d_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	struct siv120d_info *info = to_state(sd);
	int ret;

	/* Fill in min, max, step and default value for these controls. */
	switch (qc->id) {
	case V4L2_CID_BRIGHTNESS:
		return v4l2_ctrl_query_fill(qc, -127, 127, 1, 0);
	case V4L2_CID_CONTRAST:
		return v4l2_ctrl_query_fill(qc, 0, 63, 1, 16);
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_SATURATION:
		return v4l2_ctrl_query_fill(qc, 0, 63, 1, 32);
	case V4L2_CID_HUE:
		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);
	case V4L2_CID_GAIN:
		ret = v4l2_ctrl_query_fill(qc, 0, 0x9f, 1, 8);
		if(info->ae_enable)
			qc->flags |= V4L2_CTRL_FLAG_INACTIVE;
		return ret;
	case V4L2_CID_AUTOGAIN:
		ret = v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
		qc->flags |= V4L2_CTRL_FLAG_DISABLED;
		return ret;
	case V4L2_CID_EXPOSURE:
		ret = v4l2_ctrl_query_fill(qc, 0, 0xff, 1, 0x80);
		if(!info->ae_enable)
			qc->flags |= V4L2_CTRL_FLAG_INACTIVE;
		return ret;
	case V4L2_CID_EXPOSURE_AUTO:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 1);
	}
	return -EINVAL;
}

static int siv120d_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return siv120d_g_brightness(sd, &ctrl->value);
	case V4L2_CID_CONTRAST:
		return siv120d_g_contrast(sd, &ctrl->value);
	case V4L2_CID_SATURATION:
		return siv120d_g_sat(sd, &ctrl->value);
	case V4L2_CID_HUE:
		return siv120d_g_hue(sd, &ctrl->value);
	case V4L2_CID_VFLIP:
		return siv120d_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return siv120d_g_hflip(sd, &ctrl->value);
	case V4L2_CID_GAIN:
		return siv120d_g_gain(sd, &ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return siv120d_g_autogain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return siv120d_g_exp(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return siv120d_g_autoexp(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int siv120d_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return siv120d_s_brightness(sd, ctrl->value);
	case V4L2_CID_CONTRAST:
		return siv120d_s_contrast(sd, ctrl->value);
	case V4L2_CID_SATURATION:
		return siv120d_s_sat(sd, ctrl->value);
	case V4L2_CID_HUE:
		return siv120d_s_hue(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return siv120d_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return siv120d_s_hflip(sd, ctrl->value);
	case V4L2_CID_GAIN:
		return siv120d_s_gain(sd, ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return siv120d_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return siv120d_s_exp(sd, ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return siv120d_s_autoexp(sd,
				(enum v4l2_exposure_auto_type) ctrl->value);
	}
	return -EINVAL;
}

static int siv120d_reset(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	v4l_info(client, "reset...\n");
	/*
	 * siv120d doesn't have i2c reset command. The only way to reset
	 * the module is asserting the reset signal, which is done in
	 * mcam->plat_power_up().
	 *
	 */
	return 0;
}

static int siv120d_set_frame_rate(struct siv120d_info *info, int fps);

static int siv120d_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct siv120d_info *info = to_state(sd);
	int ret;

	v4l_info(client, "init...\n");

	/* force bank switching on first register access */
	info->bank = 0xff;

	/* Load default settings */
	ret = siv120d_write_array(sd, siv120d_default_regs);
	if (ret)
		return ret;

	/* Restore previous state */
	ret = siv120d_set_frame_rate(info, info->frame_rate->fps);
	if (ret)
		return ret;
	ret = siv120d_write_array(sd, info->fmt->regs);
	if (ret)
		return ret;
	ret = siv120d_write_array(sd, info->wsize->regs);
	if (ret)
		return ret;

	/* Start capturing */
	ret = siv120d_write_register(sd, REG_CNTR_A, 0x5);

	return ret;
}

static int siv120d_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned index,
					enum v4l2_mbus_pixelcode *code)
{
	if(index >= N_SIV120D_FMTS)
		return -EINVAL;

	*code = siv120d_formats[index].mbus_code;
	return 0;
}

static int siv120d_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt,
		const struct siv120d_format_struct **ret_fmt,
		const struct siv120d_win_size **ret_wsize)
{
	int index;
	const struct siv120d_win_size *wsize;

	for (index = 0; index < N_SIV120D_FMTS; index++)
		if (siv120d_formats[index].mbus_code == fmt->code)
			break;
	if (index >= N_SIV120D_FMTS) {
		/* default to first format */
		index = 0;
		fmt->code = siv120d_formats[0].mbus_code;
	}
	if (ret_fmt != NULL)
		*ret_fmt = siv120d_formats + index;
	fmt->field = V4L2_FIELD_NONE;
	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = siv120d_win_sizes; wsize < siv120d_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;
	if (wsize >= siv120d_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;
	fmt->height = wsize->height;
	fmt->colorspace = siv120d_formats[index].colorspace;
	return 0;
}

static int siv120d_try_mbus_fmt(struct v4l2_subdev *sd,
			    struct v4l2_mbus_framefmt *fmt)
{
	return siv120d_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int siv120d_s_mbus_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *fmt)
{
	const struct siv120d_format_struct *tfmt;
	const struct siv120d_win_size *wsize;
	struct siv120d_info *info = to_state(sd);
	int ret;

	ret = siv120d_try_fmt_internal(sd, fmt, &tfmt, &wsize);

	if (ret)
		return ret;

	siv120d_write_array(sd, tfmt->regs);
	siv120d_write_array(sd, wsize->regs);
	info->fmt = tfmt;

	return 0;
}

static int siv120d_set_frame_rate(struct siv120d_info *info, int fps)
{
	struct v4l2_subdev *sd = &info->sd;
	const struct siv120d_frame_rate *fr;
	int ret;
	int i;
	unsigned char tmp;

	/* Currently support VGA size only */
	if (info->wsize->width != 640 || info->wsize->height != 480)
		return -EINVAL;

	for (i = 0; i < N_SIV120D_FRAME_RATES; i++) {
		if (fps >= siv120d_frame_rates[i].fps) {
			info->frame_rate = &siv120d_frame_rates[i];
			break;
		}
	}
	if (i >= N_SIV120D_FRAME_RATES)
		return -EINVAL;

	fr = info->frame_rate;

	ret = siv120d_read_register(sd, REG_CNTR_B, &tmp);
	if (ret)
		return ret;
	tmp &= ~CNTR_B_PCLK_MASK;
	tmp |= ((info->mclkdiv + fr->clkdiv) << 2) & CNTR_B_PCLK_MASK;
	ret = siv120d_write_register(sd, REG_CNTR_B, tmp);
	if (ret)
		return ret;

	ret = siv120d_write_register(sd, REG_P_HBNKT, fr->hblank & 0xff);
	if (ret)
		return ret;
	ret = siv120d_write_register(sd, REG_P_VBNKT, fr->vblank & 0xff);
	if (ret)
		return ret;
	tmp = ((fr->hblank & 0xf00) >> 4) | ((fr->vblank & 0xf00) >> 8);
	ret = siv120d_write_register(sd, REG_P_BNKT, tmp);
	if (ret)
		return ret;

	ret = siv120d_write_register(sd, REG_STST_P, fr->shutter_step);
	if (ret)
		return ret;

	return 0;
}

static int siv120d_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct siv120d_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));

	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	cp->timeperframe.denominator = info->frame_rate->fps;

	return 0;
}

static int siv120d_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct siv120d_info *info = to_state(sd);
	struct v4l2_fract *tpf = &cp->timeperframe;
	int fps;
	int ret;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (cp->extendedmode != 0)
		return -EINVAL;

	if (tpf->numerator == 0 || tpf->denominator == 0)
		fps = siv120d_frame_rates[0].fps;
	else
		fps = tpf->denominator / tpf->numerator;

	ret = siv120d_set_frame_rate(info, fps);
	if (ret)
		return ret;

	return 0;
}

static int siv120d_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	if (interval->index >= N_SIV120D_FRAME_RATES)
		return -EINVAL;

	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator =
		siv120d_frame_rates[interval->index].fps;

	return 0;
}

/*
 * Frame size enumeration
 */
static int siv120d_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	struct siv120d_info *info = to_state(sd);
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	/*
	 * If a minimum width/height was requested, filter out the capture
	 * windows that fall outside that.
	 */
	for (i = 0; i < N_WIN_SIZES; i++) {
		const struct siv120d_win_size *win = &siv120d_win_sizes[index];
		if (info->min_width && win->width < info->min_width)
			continue;
		if (info->min_height && win->height < info->min_height)
			continue;
		if (index == ++num_valid) {
			fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
			fsize->discrete.width = win->width;
			fsize->discrete.height = win->height;
			return 0;
		}
	}

	return -EINVAL;
}

static int siv120d_detect(struct v4l2_subdev *sd)
{
	unsigned char v;
	int ret;

	ret = siv120d_read_register(sd, REG_CHIPID, &v);
	if (ret < 0)
		return ret;
	if (v != 0x12)
		return -ENODEV;
	ret = siv120d_read_register(sd, REG_INFO, &v);
	if (ret < 0)
		return ret;
	if (v != 0x13)
		return -ENODEV;

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int siv120d_g_register(struct v4l2_subdev *sd,
				struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	ret = siv120d_read_register(sd, reg->reg, &val);
	reg->val = val;
	reg->size = 1;

	return ret;
}

static int siv120d_s_register(struct v4l2_subdev *sd,
				struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	siv120d_write_register(sd, reg->reg, reg->val);

	return 0;
}
#endif

static const struct v4l2_subdev_core_ops siv120d_core_ops = {
	.g_chip_ident = siv120d_g_chip_ident,

	.g_ctrl = siv120d_g_ctrl,
	.s_ctrl = siv120d_s_ctrl,
	.queryctrl = siv120d_queryctrl,
	.reset = siv120d_reset,
	.init = siv120d_init,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = siv120d_g_register,
	.s_register = siv120d_s_register,
#endif

};

static const struct v4l2_subdev_video_ops siv120d_video_ops = {
	.enum_mbus_fmt = siv120d_enum_mbus_fmt,
	.try_mbus_fmt = siv120d_try_mbus_fmt,
	.s_mbus_fmt = siv120d_s_mbus_fmt,
	.s_parm = siv120d_s_parm,
	.g_parm = siv120d_g_parm,
	.enum_frameintervals = siv120d_enum_frameintervals,
	.enum_framesizes = siv120d_enum_framesizes,
};

static const struct v4l2_subdev_ops siv120d_ops = {
	.core = &siv120d_core_ops,
	.video = &siv120d_video_ops,
};

static int siv120d_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct siv120d_info *info;
	int ret;

	info = kzalloc(sizeof(struct siv120d_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;

	/* Initialize private info */
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &siv120d_ops);

	info->bank = 0xff; /* force bank switching on first register access */
	mutex_init(&info->reg_mutex);

	if (client->dev.platform_data) {
		struct siv120d_config *config = client->dev.platform_data;

		info->min_width = config->min_width;
		info->min_height = config->min_height;
		info->use_smbus = config->use_smbus;

		if(config->clock_speed) {
			/* Override clock divider setting */
			switch (config->clock_speed) {
			case 48:
				info->mclkdiv = 1;
				break;
			case 24:
				info->mclkdiv = 0;
				break;
			default:
				info->mclkdiv = 0;
				v4l_warn(client,
					"Unknown clock input setting: %d MHz. "
					"Clock divider disabled.\n",
					config->clock_speed);
			}
		}
	}

	info->fmt = &siv120d_formats[0];
	info->sat = 32;
	info->hue = 0;
	info->ae_enable = 1;
	info->ae_target = 0x80;
	info->frame_rate = &siv120d_frame_rates[0];
	info->wsize = &siv120d_win_sizes[0];

	/* Make sure it's an siv120d */
	ret = siv120d_detect(sd);
	if (ret) {
		v4l_dbg(1, debug, client,
			"chip found @ 0x%x (%s) is not an siv120d chip.\n",
			client->addr << 1, client->adapter->name);
		kfree(info);
		return ret;
	}
	v4l_info(client, "chip found @ 0x%02x (%s)\n",
			client->addr << 1, client->adapter->name);

	return 0;
}

static int siv120d_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id siv120d_id[] = {
	{ "siv120d", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, siv120d_id);

static struct i2c_driver siv120d_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "siv120d",
	},
	.probe		= siv120d_probe,
	.remove		= siv120d_remove,
	.id_table	= siv120d_id,
};

static __init int init_siv120d(void)
{
	return i2c_add_driver(&siv120d_driver);
}

static __exit void exit_siv120d(void)
{
	i2c_del_driver(&siv120d_driver);
}

module_init(init_siv120d);
module_exit(exit_siv120d);

MODULE_AUTHOR("Peyton Huang <peyton.huang@quantatw.com>");
MODULE_DESCRIPTION("A low-level driver for SETi SIV120D sensors");
MODULE_LICENSE("GPL");
