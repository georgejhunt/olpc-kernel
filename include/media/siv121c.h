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

#ifndef SIV121C_H
#define SIV121C_H

struct siv121c_config {
	int min_width;			/* Filter out smaller sizes */
	int min_height;			/* Filter out smaller sizes */
	int clock_speed;		/* External clock speed (MHz) */
};

#define	g_bank(r)	((u8)(r >> 8))
#define	g_reg(r)	((u8)(r & 0xff))

/* --- Bank 0 pmu block --- */
#define	REG_BLK_SEL	0x0
#define	REG_CHIPID	0x1
#define	REG_INFO	0x2

/* --- Bank 1 control block --- */
#define	REG_CNTR_A	0x103
#define	REG_CNTR_B	0x104
#define	  CNTR_B_HFLIP	0x1
#define	  CNTR_B_VFLIP  0x2
#define	  CNTR_B_PCLK_MASK 0x0c
#define	  CNTR_B_PCLK_1	0x0	/* 1/1 pixel clock */
#define	  CNTR_B_PCLK_2	0x4	/* 1/2 pixel clock */
#define	  CNTR_B_PCLK_4	0x8	/* 1/4 pixel clock */
#define	  CNTR_B_PCLK_8	0xc	/* 1/8 pixel clock */
#define	REG_VMODE	0x106
#define	REG_BNKT	0x120
#define	REG_HBNKT	0x121
#define	REG_VBNKT	0x122

/* --- Bank 2 AE registers --- */
#define	REG_AE_CNTR	0x210
#define	  AE_CNTR_AE_ENABLE 0x80
#define	REG_Y_TARGET_N	0x212
#define	REG_Y_TARGET_CWF	0x213
#define	REG_Y_TARGET_A	0x214
#define	REG_STST_P	0x234

/* --- Bank 3 AWB registers --- */

/* --- Bank 4 IDP block --- */
#define	REG_OUTFMT	0x412
#define	  OUTFMT_WINDOW_MODE_ENABLE 0x8

#define	REG_CRGAIN	0x4A9
#define	REG_CBGAIN	0x4AA
#define	REG_BRTCNT	0x4AB
#define	  BRTCNT_SIGN	0x80
#define	  BRTCNT_MASK	0x7F
#define	REG_HUECOS	0x4AE
#define	REG_HUESIN	0x4AF
/*
 * Window mode registers could be used to crop image to user defined
 * output resolution.
 *
 * Horizontal start: REG_WDATH[7:6] << 8 | REG_WHSRTL[7:0]
 * Horizontal width: REG_WDATH[5:4] << 8 | REG_WHWIDL[7:0]
 * Vertival start: REG_WDATH[3] << 8 | REG_WVSRTL[7:0]
 * Vertical width: REG_WDATH[2] << 8 | REG_WVWIDL[7:0]
 */
#define	REG_WDATH	0x4C0
#define	REG_WHSRTL	0x4C1
#define	REG_WHWIDL	0x4C2
#define	REG_WVSRTL	0x4C3
#define	REG_WVWIDL	0x4C4

#endif /* SIV121C_H */
