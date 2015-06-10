/*
 * SETi Co.,Ltd. VGA CMOS IMAGE SENSOR Driver
 *
 * A V4L2 driver for SETi SIV121C cameras.
 *
 * Copyright (C) 2014 OLPC,INC
 * Written by James Cameron <quozl@laptop.org>
 *
 * Based on siv120d camera driver,
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
#include <media/siv121c.h>

/* TODO: add debug switch */
/* static int debug; */
/* module_param(debug, bool, 0644); */
/* MODULE_PARM_DESC(debug, "Debug level (0-1)"); */
static int debug = 1;

struct regval_list {
	u16 reg_num;
	u8 value;
};

static const struct regval_list siv121c_default_regs[] = {

	/* Timing Control Block */
	{0x103, 0x02}, /* CNTR_A - Control global reset and device
			  enable - global reset, idle mode with clock
			  disabled. */

	/* PMU Block */
	{0x003, 0x04}, /* CHIP_CNTR - Chip control - enable output
			  pads, disable software stand-by power-down
			  mode. */

	/* Timing Control Block */
	{0x107, 0xc1}, /* recommend, BLC_CAL */
	{0x110, 0x1e}, /* recommend, ramp_ref */
	{0x111, 0x81}, /* recommend, ramp_sig_start level */
	{0x117, 0x84}, /* recommend, abs_cntr1 */

	/* Automatic Exposure Block */
	{0x211, 0x04}, /* MAX_SHUTSTEP, minimum exposure time when
			  automatic exposure in use, reduce to
			  increase frame rate at low light levels at
			  the expense of image brightness */

	/* IDP */
	{0x410, 0x7f}, /* IPFUN */

	/* Timing Control Block */
	{0x103, 0x02}, /* CNTR_A - Control global reset and device
			  enable - no reset, dynamic mode with clock
			  enabled. */

	{0xff, 0xff},
};

static const struct regval_list siv121c_yuv422_regs[] = {
	{REG_OUTFMT, 0x3d},
	{0xff, 0xff},
};

static const struct regval_list siv121c_rgb565_regs[] = {
	{REG_OUTFMT, 0x8b},
	{0xff, 0xff},
};

static const struct regval_list siv121c_vga_regs[] = {
	{REG_VMODE, 0x4},
	{0xff, 0xff},
};

static const struct siv121c_format_struct {
	enum v4l2_mbus_pixelcode mbus_code;
	enum v4l2_colorspace colorspace;
	const struct regval_list *regs;
} siv121c_formats[] = {
	{
		.mbus_code	= V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace	= V4L2_COLORSPACE_JPEG,
		.regs 		= siv121c_yuv422_regs,
	},
	{
		.mbus_code	= V4L2_MBUS_FMT_RGB565_2X8_LE,
		.colorspace	= V4L2_COLORSPACE_SRGB,
		.regs		= siv121c_rgb565_regs,
	},
};
#define N_SIV121C_FMTS ARRAY_SIZE(siv121c_formats)

static const struct siv121c_win_size {
	int	width;
	int	height;
	const struct regval_list *regs; /* Regs to tweak */
} siv121c_win_sizes[] = {
	/* VGA */
	{
		.width		= 640,
		.height		= 480,
		.regs 		= siv121c_vga_regs,
	},
};
#define N_WIN_SIZES (ARRAY_SIZE(siv121c_win_sizes))

static struct siv121c_frame_rate {
	int fps;
	int vblank;
	int hblank;
	int clkdiv;		/* Pixel clock divider */
	int shutter_step;	/* Anti-flicker shutter step */
} siv121c_frame_rates[] = {
	/* fps field must be in decending order for searching */
	{
		.fps		= 30,
		.vblank		= 0x3,
		.hblank		= 0x65,
		.clkdiv		= 0,
		.shutter_step	= 0x7d,
	},
	{
		.fps		= 25,
		.vblank		= 0x3,
		.hblank		= 0x65,
		.clkdiv		= 0,
		.shutter_step	= 0x7d,
	},
	{
		.fps		= 20,
		.vblank		= 0x3,
		.hblank		= 0x65,
		.clkdiv		= 0,
		.shutter_step	= 0x7d,
	},
	{
		.fps		= 15,
		.vblank		= 0x3,
		.hblank		= 0x65,
		.clkdiv		= 1,
		.shutter_step	= 0x3e,
	},
	{
		.fps		= 10,
		.vblank		= 0x3,
		.hblank		= 0x65,
		.clkdiv		= 1,
		.shutter_step	= 0x3e,
	},
	{
		.fps		= 5,
		.vblank		= 0x3,
		.hblank		= 0x65,
		.clkdiv		= 2,
		.shutter_step	= 0x1f,
	},
};
#define N_SIV121C_FRAME_RATES (ARRAY_SIZE(siv121c_frame_rates))

struct siv121c_info {
	struct v4l2_subdev sd;
	const struct siv121c_format_struct *fmt;  /* Current format */
	const struct siv121c_frame_rate *frame_rate;
	const struct siv121c_win_size *wsize;
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

static inline struct siv121c_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct siv121c_info, sd);
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
static int siv121c_read_smbus(struct v4l2_subdev *sd, unsigned char reg,
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

static int siv121c_write_smbus(struct v4l2_subdev *sd, unsigned char reg,
		unsigned char value)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	return i2c_smbus_write_byte_data(client, reg, value);
}

static int siv121c_read_i2c(struct v4l2_subdev *sd, unsigned char reg,
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

static int siv121c_write_i2c(struct v4l2_subdev *sd, unsigned char reg,
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
 * siv121c_info.reg_mutex should be held before calling this function
 */
static inline int siv121c_switch_bank(struct v4l2_subdev *sd, u8 bank)
{
	struct siv121c_info *info = to_state(sd);

	if (info->bank == bank)
		return 0;

	info->bank = bank;
	if (info->use_smbus)
		return siv121c_write_smbus(sd, REG_BLK_SEL, bank);
	else
		return siv121c_write_i2c(sd, REG_BLK_SEL, bank);
}

static int siv121c_write_register(struct v4l2_subdev *sd, u16 reg, u8 value)
{
	int ret = 0;
	struct siv121c_info *info = to_state(sd);

	mutex_lock(&info->reg_mutex);

	ret = siv121c_switch_bank(sd, g_bank(reg));
	if (ret < 0)
		goto out;

	if (info->use_smbus)
		ret = siv121c_write_smbus(sd, g_reg(reg), value);
	else
		ret = siv121c_write_i2c(sd, g_reg(reg), value);

out:
	mutex_unlock(&info->reg_mutex);
	return ret;
}

static int siv121c_read_register(struct v4l2_subdev *sd, u16 reg, u8 *value)
{
	int ret = 0;
	struct siv121c_info *info = to_state(sd);

	mutex_lock(&info->reg_mutex);

	ret = siv121c_switch_bank(sd, g_bank(reg));
	if (ret < 0)
		goto out;

	if (info->use_smbus)
		ret = siv121c_read_smbus(sd, g_reg(reg), value);
	else
		ret = siv121c_read_i2c(sd, g_reg(reg), value);

out:
	mutex_unlock(&info->reg_mutex);
	return ret;
}

/*
 * Write a list of register settings; ff/ff stops the process.
 */
static int siv121c_write_array(struct v4l2_subdev *sd,
				const struct regval_list *vals)
{
	while (vals->reg_num != 0xff || vals->value != 0xff) {
		int ret = siv121c_write_register(sd, vals->reg_num,
							vals->value);
		if (ret < 0)
			return ret;
		vals++;
	}
	return 0;
}

static int siv121c_g_chip_ident(struct v4l2_subdev *sd,
		struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SIV121C, 0);
}

#define SIN_STEP 5
/*
 * sine table is generated using the following formula:
 * round(sin(theta)*64), where theta = 0, 5, 10, ..., 90 degs
 *
 */
static const unsigned char siv121c_sin_table[] = {
	0, 6, 11, 17, 22,
	27, 32, 37, 41, 45,
	49, 52, 55, 58, 60,
	62, 63, 64, 64
};

static unsigned char siv121c_sine(int theta)
{
	unsigned char chs = 0;
	unsigned char sine;

	if (theta < 0) {
		theta = -theta;
		chs = 0x80;
	}
	if (theta <= 90)
		sine = siv121c_sin_table[theta/SIN_STEP];
	else {
		theta -= 90;
		sine = 64 - siv121c_sin_table[theta/SIN_STEP];
	}
	return sine ^ chs;
}

static unsigned char siv121c_cosine(int theta)
{
	theta = 90 - theta;
	if (theta > 180)
		theta -= 360;
	else if (theta < -180)
		theta += 360;
	return siv121c_sine(theta);
}

static int siv121c_s_sat(struct v4l2_subdev *sd, int value)
{
	struct siv121c_info *info = to_state(sd);
	int ret;

	if(value < 0 || value > 63)
		return -ERANGE;

	info->sat = value;

	ret = siv121c_write_register(sd, REG_CRGAIN, value);
	if(ret < 0)
		return ret;
	ret = siv121c_write_register(sd, REG_CBGAIN, value);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv121c_g_sat(struct v4l2_subdev *sd, __s32 *value)
{
	struct siv121c_info *info = to_state(sd);
	*value = info->sat;
	return 0;
}

static int siv121c_s_hue(struct v4l2_subdev *sd, int value)
{
	struct siv121c_info *info = to_state(sd);
	int ret;
	unsigned char sinth, costh;

	if (value < -180 || value > 180)
		return -ERANGE;

	info->hue = value;

	sinth = siv121c_sine(value);
	costh = siv121c_cosine(value);

	ret = siv121c_write_register(sd, REG_HUECOS, costh);
	if(ret < 0)
		return ret;
	ret = siv121c_write_register(sd, REG_HUESIN, sinth);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv121c_g_hue(struct v4l2_subdev *sd, __s32 *value)
{
	struct siv121c_info *info = to_state(sd);
	*value = info->hue;
	return 0;
}

static int siv121c_s_brightness(struct v4l2_subdev *sd, int value)
{
	int ret;

	if (value < -127 || value > 127)
		return -ERANGE;

	if(value < 0) {
		value = -value;
		value |= 0x80;
	}

	ret = siv121c_write_register(sd, REG_BRTCNT, (unsigned char)value);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv121c_g_brightness(struct v4l2_subdev *sd, __s32 *value)
{
	unsigned char tmp;
	int ret, v;

	ret = siv121c_read_register(sd, REG_BRTCNT, &tmp);
	if(ret < 0)
		return ret;

	v = tmp & BRTCNT_MASK;
	v = (tmp & BRTCNT_SIGN) ? -v : v;

	*value = v;

	return 0;
}

static int siv121c_g_hflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	unsigned char tmp;

	ret = siv121c_read_register(sd, REG_CNTR_B, &tmp);
	if(ret < 0)
		return ret;

	*value = (tmp & CNTR_B_HFLIP) == CNTR_B_HFLIP;

	return 0;
}


static int siv121c_s_hflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	unsigned char tmp;

	ret = siv121c_read_register(sd, REG_CNTR_B, &tmp);
	if(ret < 0)
		return ret;

	if(value)
		tmp |= CNTR_B_HFLIP;
	else
		tmp &= ~CNTR_B_HFLIP;

	ret = siv121c_write_register(sd, REG_CNTR_B, tmp);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv121c_g_vflip(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	unsigned char tmp;

	ret = siv121c_read_register(sd, REG_CNTR_B, &tmp);
	if(ret < 0)
		return ret;

	*value = (tmp & CNTR_B_VFLIP) == CNTR_B_VFLIP;

	return 0;
}

static int siv121c_s_vflip(struct v4l2_subdev *sd, int value)
{
	int ret;
	unsigned char tmp;

	ret = siv121c_read_register(sd, REG_CNTR_B, &tmp);
	if(ret < 0)
		return ret;

	if(value)
		tmp |= CNTR_B_VFLIP;
	else
		tmp &= ~CNTR_B_VFLIP;

	ret = siv121c_write_register(sd, REG_CNTR_B, tmp);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv121c_g_autogain(struct v4l2_subdev *sd, __s32 *value)
{
	/* Not feasible for siv121c? AE also handles gain control */
	*value = 0;
	return 0;
}

static int siv121c_s_autogain(struct v4l2_subdev *sd, int value)
{
	/* Not feasible for siv121c? AE also handles gain control */
	return 0;
}

static int siv121c_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct siv121c_info *info = to_state(sd);
	*value = info->ae_target;
	return 0;
}

static int siv121c_s_exp(struct v4l2_subdev *sd, int value)
{
	struct siv121c_info *info = to_state(sd);
	int ret;

	/* Exposure target values are input parameters for AE */
	if(!info->ae_enable)
		return -EBUSY;

	if (value < 0 || value > 0xff)
		return -ERANGE;

	info->ae_target = value;

	ret = siv121c_write_register(sd, REG_Y_TARGET_N, (unsigned char)value);
	if(ret < 0)
		return ret;
	ret = siv121c_write_register(sd, REG_Y_TARGET_CWF, (unsigned char)value);
	if(ret < 0)
		return ret;
	ret = siv121c_write_register(sd, REG_Y_TARGET_A, (unsigned char)value);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv121c_g_autoexp(struct v4l2_subdev *sd, __s32 *value)
{
	int ret;
	unsigned char tmp;

	ret = siv121c_read_register(sd, REG_AE_CNTR, &tmp);
	if(ret < 0)
		return ret;

	*value = ((tmp & AE_CNTR_AE_ENABLE) == AE_CNTR_AE_ENABLE) ?
			V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;

	return 0;
}

static int siv121c_s_autoexp(struct v4l2_subdev *sd,
		enum v4l2_exposure_auto_type value)
{
	struct siv121c_info *info = to_state(sd);
	int ret;
	unsigned char tmp;

	ret = siv121c_read_register(sd, REG_AE_CNTR, &tmp);
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

	ret = siv121c_write_register(sd, REG_AE_CNTR, tmp);
	if(ret < 0)
		return ret;

	return 0;
}

static int siv121c_queryctrl(struct v4l2_subdev *sd,
		struct v4l2_queryctrl *qc)
{
	struct siv121c_info *info = to_state(sd);
	int ret;

	/* Fill in min, max, step and default value for these controls. */
	switch (qc->id) {
	case V4L2_CID_BRIGHTNESS:
		return v4l2_ctrl_query_fill(qc, -127, 127, 1, 0);
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
		return v4l2_ctrl_query_fill(qc, 0, 1, 1, 0);
	case V4L2_CID_SATURATION:
		return v4l2_ctrl_query_fill(qc, 0, 63, 1, 32);
	case V4L2_CID_HUE:
		return v4l2_ctrl_query_fill(qc, -180, 180, 5, 0);
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

static int siv121c_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return siv121c_g_brightness(sd, &ctrl->value);
	case V4L2_CID_SATURATION:
		return siv121c_g_sat(sd, &ctrl->value);
	case V4L2_CID_HUE:
		return siv121c_g_hue(sd, &ctrl->value);
	case V4L2_CID_VFLIP:
		return siv121c_g_vflip(sd, &ctrl->value);
	case V4L2_CID_HFLIP:
		return siv121c_g_hflip(sd, &ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return siv121c_g_autogain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return siv121c_g_exp(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return siv121c_g_autoexp(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int siv121c_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return siv121c_s_brightness(sd, ctrl->value);
	case V4L2_CID_SATURATION:
		return siv121c_s_sat(sd, ctrl->value);
	case V4L2_CID_HUE:
		return siv121c_s_hue(sd, ctrl->value);
	case V4L2_CID_VFLIP:
		return siv121c_s_vflip(sd, ctrl->value);
	case V4L2_CID_HFLIP:
		return siv121c_s_hflip(sd, ctrl->value);
	case V4L2_CID_AUTOGAIN:
		return siv121c_s_autogain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return siv121c_s_exp(sd, ctrl->value);
	case V4L2_CID_EXPOSURE_AUTO:
		return siv121c_s_autoexp(sd,
				(enum v4l2_exposure_auto_type) ctrl->value);
	}
	return -EINVAL;
}

static int siv121c_reset(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	v4l_info(client, "reset...\n");
	/*
	 * siv121c doesn't have i2c reset command. The only way to reset
	 * the module is asserting the reset signal, which is done in
	 * mcam->plat_power_up().
	 */
	return 0;
}

static int siv121c_set_frame_rate(struct siv121c_info *info, int fps);

static int siv121c_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct siv121c_info *info = to_state(sd);
	int ret;

	v4l_info(client, "init...\n");

	/* force bank switching on first register access */
	info->bank = 0xff;

	/* Load default settings */
	ret = siv121c_write_array(sd, siv121c_default_regs);
	if (ret)
		return ret;

	/* Restore previous state */
	ret = siv121c_set_frame_rate(info, info->frame_rate->fps);
	if (ret)
		return ret;
	ret = siv121c_write_array(sd, info->fmt->regs);
	if (ret)
		return ret;
	ret = siv121c_write_array(sd, info->wsize->regs);
	if (ret)
		return ret;

	/* Start capturing */
	ret = siv121c_write_register(sd, REG_CNTR_A, 0x1);

	return ret;
}

static int siv121c_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned index,
					enum v4l2_mbus_pixelcode *code)
{
	if(index >= N_SIV121C_FMTS)
		return -EINVAL;

	*code = siv121c_formats[index].mbus_code;
	return 0;
}

static int siv121c_try_fmt_internal(struct v4l2_subdev *sd,
		struct v4l2_mbus_framefmt *fmt,
		const struct siv121c_format_struct **ret_fmt,
		const struct siv121c_win_size **ret_wsize)
{
	int index;
	const struct siv121c_win_size *wsize;

	for (index = 0; index < N_SIV121C_FMTS; index++)
		if (siv121c_formats[index].mbus_code == fmt->code)
			break;
	if (index >= N_SIV121C_FMTS) {
		/* default to first format */
		index = 0;
		fmt->code = siv121c_formats[0].mbus_code;
	}
	if (ret_fmt != NULL)
		*ret_fmt = siv121c_formats + index;
	fmt->field = V4L2_FIELD_NONE;
	/*
	 * Round requested image size down to the nearest
	 * we support, but not below the smallest.
	 */
	for (wsize = siv121c_win_sizes; wsize < siv121c_win_sizes + N_WIN_SIZES;
	     wsize++)
		if (fmt->width >= wsize->width && fmt->height >= wsize->height)
			break;
	if (wsize >= siv121c_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;
	fmt->height = wsize->height;
	fmt->colorspace = siv121c_formats[index].colorspace;
	return 0;
}

static int siv121c_try_mbus_fmt(struct v4l2_subdev *sd,
			    struct v4l2_mbus_framefmt *fmt)
{
	return siv121c_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int siv121c_s_mbus_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *fmt)
{
	const struct siv121c_format_struct *tfmt;
	const struct siv121c_win_size *wsize;
	struct siv121c_info *info = to_state(sd);
	int ret;

	ret = siv121c_try_fmt_internal(sd, fmt, &tfmt, &wsize);

	if (ret)
		return ret;

	siv121c_write_array(sd, tfmt->regs);
	siv121c_write_array(sd, wsize->regs);
	info->fmt = tfmt;

	return 0;
}

static int siv121c_set_frame_rate(struct siv121c_info *info, int fps)
{
	struct v4l2_subdev *sd = &info->sd;
	const struct siv121c_frame_rate *fr;
	int ret;
	int i;
	unsigned char tmp;

	if (info->wsize->width != 640 || info->wsize->height != 480)
		return -EINVAL;

	for (i = 0; i < N_SIV121C_FRAME_RATES; i++) {
		if (fps >= siv121c_frame_rates[i].fps) {
			info->frame_rate = &siv121c_frame_rates[i];
			break;
		}
	}
	if (i >= N_SIV121C_FRAME_RATES)
		return -EINVAL;

	fr = info->frame_rate;

	ret = siv121c_read_register(sd, REG_CNTR_B, &tmp);
	if (ret)
		return ret;
	tmp &= ~CNTR_B_PCLK_MASK;
	tmp |= ((info->mclkdiv + fr->clkdiv) << 2) & CNTR_B_PCLK_MASK;
	ret = siv121c_write_register(sd, REG_CNTR_B, tmp);
	if (ret)
		return ret;

	ret = siv121c_write_register(sd, REG_HBNKT, fr->hblank & 0xff);
	if (ret)
		return ret;
	ret = siv121c_write_register(sd, REG_VBNKT, fr->vblank & 0xff);
	if (ret)
		return ret;
	tmp = ((fr->hblank & 0xf00) >> 4) | ((fr->vblank & 0xf00) >> 8);
	ret = siv121c_write_register(sd, REG_BNKT, tmp);
	if (ret)
		return ret;

	ret = siv121c_write_register(sd, REG_STST_P, fr->shutter_step);
	if (ret)
		return ret;

	return 0;
}

static int siv121c_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct siv121c_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));

	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->timeperframe.numerator = 1;
	cp->timeperframe.denominator = info->frame_rate->fps;

	return 0;
}

static int siv121c_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct siv121c_info *info = to_state(sd);
	struct v4l2_fract *tpf = &cp->timeperframe;
	int fps;
	int ret;

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;
	if (cp->extendedmode != 0)
		return -EINVAL;

	if (tpf->numerator == 0 || tpf->denominator == 0)
		fps = siv121c_frame_rates[0].fps;
	else
		fps = tpf->denominator / tpf->numerator;

	ret = siv121c_set_frame_rate(info, fps);
	if (ret)
		return ret;

	return 0;
}

static int siv121c_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_frmivalenum *interval)
{
	if (interval->index >= N_SIV121C_FRAME_RATES)
		return -EINVAL;

	interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	interval->discrete.numerator = 1;
	interval->discrete.denominator =
		siv121c_frame_rates[interval->index].fps;

	return 0;
}

/*
 * Frame size enumeration
 */
static int siv121c_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_frmsizeenum *fsize)
{
	struct siv121c_info *info = to_state(sd);
	int i;
	int num_valid = -1;
	__u32 index = fsize->index;

	/*
	 * If a minimum width/height was requested, filter out the capture
	 * windows that fall outside that.
	 */
	for (i = 0; i < N_WIN_SIZES; i++) {
		const struct siv121c_win_size *win = &siv121c_win_sizes[index];
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

static int siv121c_detect(struct v4l2_subdev *sd)
{
	unsigned char v;
	int ret;

	ret = siv121c_read_register(sd, REG_CHIPID, &v);
	if (ret < 0)
		return ret;
	if (v != 0x95)
		return -ENODEV;
	ret = siv121c_read_register(sd, REG_INFO, &v);
	if (ret < 0)
		return ret;
	if (v != 0x10)
		return -ENODEV;

	return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int siv121c_g_register(struct v4l2_subdev *sd,
				struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char val = 0;
	int ret;

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	ret = siv121c_read_register(sd, reg->reg, &val);
	reg->val = val;
	reg->size = 1;

	return ret;
}

static int siv121c_s_register(struct v4l2_subdev *sd,
				struct v4l2_dbg_register *reg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!v4l2_chip_match_i2c_client(client, &reg->match))
		return -EINVAL;
	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	siv121c_write_register(sd, reg->reg, reg->val);

	return 0;
}
#endif

static const struct v4l2_subdev_core_ops siv121c_core_ops = {
	.g_chip_ident = siv121c_g_chip_ident,

	.g_ctrl = siv121c_g_ctrl,
	.s_ctrl = siv121c_s_ctrl,
	.queryctrl = siv121c_queryctrl,
	.reset = siv121c_reset,
	.init = siv121c_init,
#ifdef CONFIG_VIDEO_ADV_DEBUG
	.g_register = siv121c_g_register,
	.s_register = siv121c_s_register,
#endif

};

static const struct v4l2_subdev_video_ops siv121c_video_ops = {
	.enum_mbus_fmt = siv121c_enum_mbus_fmt,
	.try_mbus_fmt = siv121c_try_mbus_fmt,
	.s_mbus_fmt = siv121c_s_mbus_fmt,
	.s_parm = siv121c_s_parm,
	.g_parm = siv121c_g_parm,
	.enum_frameintervals = siv121c_enum_frameintervals,
	.enum_framesizes = siv121c_enum_framesizes,
};

static const struct v4l2_subdev_ops siv121c_ops = {
	.core = &siv121c_core_ops,
	.video = &siv121c_video_ops,
};

static int siv121c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct siv121c_info *info;
	int ret;

	info = kzalloc(sizeof(struct siv121c_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;

	/* Initialize private info */
	sd = &info->sd;
	v4l2_i2c_subdev_init(sd, client, &siv121c_ops);

	info->bank = 0xff; /* force bank switching on first register access */
	mutex_init(&info->reg_mutex);

	if (client->dev.platform_data) {
		struct siv121c_config *config = client->dev.platform_data;

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

	info->fmt = &siv121c_formats[0];
	info->sat = 32;
	info->hue = 0;
	info->ae_enable = 1;
	info->ae_target = 0x80;
	info->frame_rate = &siv121c_frame_rates[0];
	info->wsize = &siv121c_win_sizes[0];

	/* Make sure it's an siv121c */
	ret = siv121c_detect(sd);
	if (ret) {
		v4l_dbg(1, debug, client,
			"chip found @ 0x%x (%s) is not a siv121c.\n",
			client->addr << 1, client->adapter->name);
		v4l2_device_unregister_subdev(sd);
		kfree(info);
		return ret;
	}
	v4l_info(client, "chip found @ 0x%02x (%s)\n",
			client->addr << 1, client->adapter->name);

	return 0;
}

static int siv121c_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id siv121c_id[] = {
	{ "siv121c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, siv121c_id);

static struct i2c_driver siv121c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "siv121c",
	},
	.probe		= siv121c_probe,
	.remove		= siv121c_remove,
	.id_table	= siv121c_id,
};

static __init int init_siv121c(void)
{
	return i2c_add_driver(&siv121c_driver);
}

static __exit void exit_siv121c(void)
{
	i2c_del_driver(&siv121c_driver);
}

module_init(init_siv121c);
module_exit(exit_siv121c);

MODULE_AUTHOR("James Cameron <quozl@laptop.org>");
MODULE_DESCRIPTION("A low-level driver for SETi SIV121C sensors");
MODULE_LICENSE("GPL");
