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

#ifndef SIV120D_H
#define SIV120D_H

struct siv120d_config {
	int min_width;			/* Filter out smaller sizes */
	int min_height;			/* Filter out smaller sizes */
	int clock_speed;		/* External clock speed (MHz) */
	bool use_smbus;			/* Use smbus I/O instead of I2C */
};

#define	g_bank(r)	((u8)(r >> 8))
#define	g_reg(r)	((u8)(r & 0xff))

/* --- Bank 0 control registers --- */
#define	REG_BLK_SEL	0x0
#define	REG_CHIPID	0x1
#define	REG_INFO	0x2
#define	REG_CNTR_A	0x3
#define	REG_CNTR_B	0x4
#define	  CNTR_B_HFLIP	0x1
#define	  CNTR_B_VFLIP  0x2
/*
 * Input clock would be divided by 2^(CNTR_B_PCLK)
 * Maximum total configurable divider is 8.
 */
#define	  CNTR_B_PCLK_MASK 0x0c
#define	  CNTR_B_PCLK_1	0x0	/* 1/1 pixel clock */
#define	  CNTR_B_PCLK_2	0x4	/* 1/2 pixel clock */
#define	  CNTR_B_PCLK_4	0x8	/* 1/4 pixel clock */
#define	  CNTR_B_PCLK_8	0xc	/* 1/8 pixel clock */
#define	REG_VMODE	0x5
/* 0x6 Reserved */
#define	REG_CNTR_C	0x7
/* 0x8~0xf Reserved */
/* 0x10~0x16 Reserved */
#define	REG_LDO_CNTR	0x17
/* 0x18~0x1f Reserved */
#define	REG_P_BNKT	0x20
#define	REG_P_HBNKT	0x21
#define	REG_P_ROWFIL	0x22
#define	REG_P_VBNKT	0x23
#define	REG_S_BNKT	0x24
#define	REG_S_HBNKT	0x25
#define	REG_S_ROWFIL	0x26
#define	REG_S_VBNKT	0x27
/* 0x28~0xff Reserved */

/* --- Bank 1 AE registers --- */
#define	REG_AE_CNTR	0x110
#define	  AE_CNTR_AE_ENABLE 0x80
/*
 * Set REG_MAX_SHUTSTEP value to limit the maximum shutter time (minimum fps).
 * The formula for 24 MHz pixel clock is:
 * MAX_SHUTSTEP = round( 120 / (desired fps) )
 */
#define	REG_MAX_SHUTSTEP 0x111
#define	REG_Y_TARGET_N	0x112
#define	REG_Y_TARGET_CWF 0x113
#define	REG_Y_TARGET_A	0x114
#define	REG_YTGT_LEVEL	0x115
#define	REG_LOCK_LEVEL	0x116
#define	REG_SHUT_CNTR	0x117
#define	REG_AEADCT	0x118
#define	REG_DA2BR	0x119
#define	REG_D2B_RET_TIME 0x11A
#define	REG_BR2DA	0x11B
/* 0x1C Reserved */
#define	REG_INI_SHUTTIME 0x11D
#define	REG_INI_AGAIN	0x11E
/* 0x1f~0x2F Reserved */
#define	REG_SHTH	0x130
#define	REG_SHTL	0x131
#define	REG_AGAIN	0x132
/* 0x33 Reserved */
#define	REG_STST_P	0x134
#define	REG_STST_S	0x135
#define	REG_SF_P2S	0x136
/* 0x37~0x3F Reserved */
#define	REG_AG_MAX	0x140
#define	REG_AG_TOP1	0x141
#define	REG_AG_TOP0	0x142
#define	REG_AG_MIN1	0x143
#define	REG_AG_MIN0	0x144
#define	REG_G50_DEG	0x145
#define	REG_G33_DEC	0x146
#define	REG_G25_DEC	0x147
#define	REG_G20_DEC	0x148
#define	REG_G12_DEC	0x149
#define	REG_G09_DEC	0x14A
#define	REG_G06_DEC	0x14B
#define	REG_G03_DEC	0x14C
#define	REG_G100_INC	0x14D
#define	REG_G50_INC	0x14E
#define	REG_G33_INC	0x14F
#define	REG_G25_INC	0x150
#define	REG_G20_INC	0x151
#define	REG_G12_INC	0x152
#define	REG_G09_INC	0x153
#define	REG_G06_INC	0x154
#define	REG_G03_INC	0x155
/* 0x56~0x5F Reserved */
#define	REG_WINA_USE	0x160
#define	REG_WINB_USE	0x161
#define	REG_WINC_USE	0x162
#define	REG_WIND_USE	0x163
#define	REG_WINE_USE	0x164
#define	REG_WINF_USE	0x165
#define	REG_WINA_WEIGHT	0x166
#define	REG_WINB_WEIGHT	0x167
#define	REG_WINC_WEIGHT 0x168
#define	REG_WIND_WEIGHT	0x169
#define	REG_WINE_WEIGHT	0x16A
#define	REG_WINF_WEIGHT	0x16B
#define	REG_TOTAL_WEIGHT 0x16C
/* 0x6D~0xE9 Reserved */

/* --- Bank 2 AWB registers --- */
#define	REG_AWBCNTR1	0x210
#define	REG_AWBCNTR2	0x211
#define	REG_AWBCNTR3	0x212
#define	REG_CRTARGET	0x213
#define	REG_CBTARGET	0x214
#define	REG_RGNTOP	0x215
#define	REG_RGNBOT	0x216
#define	REG_BGNTOP	0x217
#define	REG_BGNBOT	0x218
#define	REG_WHTCRTOP	0x219
#define	REG_WHTCRBOT	0x21A
#define	REG_WHTCBTOP	0x21B
#define	REG_WHTCBBOT	0x21C
#define	REG_WHTCXTOP	0x21D
#define	REG_WHTCXBOT	0x21E
/* 0x1F Reserved */
#define	REG_AWBLTOP	0x220
#define	REG_AWBLBOT	0x221
#define	REG_RBLINE	0x222
#define	REG_RBOVER	0x223
#define	REG_AWBGRNG	0x224
#define	REG_WHTCNTL	0x225
#define	REG_WHTCNTN	0x226
#define	REG_BRTSRT	0x227
#define	REG_BRTRGNTOP	0x228
#define	REG_BRTRGNBOT	0x229
#define	REG_BRTBGNTOP	0x22A
#define	REG_BRTBGNBOT	0x22B
#define	REG_RGAINCONT	0x22C
#define	REG_BGAINCONT	0x22D
/* 0x2E~0x2F Reserved */
#define	REG_AWBWINHSH	0x230
#define	REG_AWBWINHSL	0x231
#define	REG_AWBWINVSH	0x232
#define	REG_AWBWINVSL	0x233
#define	REG_AWBWINHEH	0x234
#define	REG_AWBWINHEL	0x235
#define	REG_AWBWINVEH	0x236
#define	REG_AWBWINVEL	0x237
/* 0x38~0x3F Reserved */
#define	REG_AWBLRNG	0x240
#define	REG_AWBMRNG1	0x241
#define	REG_AWBNRNG2	0x242
#define	REG_AWBMRNG3	0x243
#define	REG_AWBSTEP1	0x244
#define	REG_AWBSTEP2	0x245
#define	REG_WINLRNG	0x246
/* 0x47~0x5E Reserved */
#define	REG_AWBSTST	0x25F
#define	REG_RGAIN	0x260
#define	REG_BGAIN	0x261
#define	REG_GGAIN	0x262
#define	REG_D20RGNI	0x263
#define	REG_D20BGNI	0x264
#define	REG_D20RGNO	0x265
#define	REG_D20BGNO	0x266
#define	REG_D30RGNI	0x267
#define	REG_D30BGNI	0x268
#define	REG_D30RGNO	0x269
#define	REG_D30BGNO	0x26A
#define	REG_CRAVG	0x26B
#define	REG_CBAVG	0x26C
#define	REG_CRWHT	0x26D
#define	REG_CBWHT	0x26E
/* 0x6F~0x70 Reserved */
#define	REG_AWBTST	0x271
#define	REG_FRGAIN	0x272
#define	REG_FBGAIN	0x273
#define	REG_T1RGAINBOT	0x274
#define	REG_T1RGAINTOP	0x275
#define	REG_T1BGAINBOT	0x276
#define	REG_T1BGAINTOP	0x277
#define	REG_T2RGAINBOT	0x278
#define	REG_T2RGAINTOP	0x279
#define	REG_T2BGAINBOT	0x27A
#define	REG_T2BGAINTOP	0x27B
#define	REG_T3RGAINBOT	0x27C
#define	REG_T3RGAINTOP	0x27D
#define	REG_T3BGAINBOT	0x27E
#define	REG_T3BGAINTOP	0x27F
#define	REG_YDIFFTH	0x280
#define	REG_DNPEXPOTOP	0x281
#define	REG_DNPEXPOBOT	0x282
#define	REG_YDIFFTH2	0x283
#define	REG_SKYEXPOTOP	0x284
#define	REG_SKYEXPOBOT	0x285
/* 0x90~0x97 Reserved */

/* --- Bank 3 IDP registers --- */
#define	REG_IPFUN	0x310
#define	REG_SIGCNT	0x311
#define	REG_OUTFMT	0x312
#define	  OUTFMT_WINDOW_MODE_ENABLE 0x8
#define	REG_IPFUN2	0x313
#define	REG_IPFUN3	0x314
#define	REG_VBTC	0x315
/* 0x16 Reserved */
#define	REG_DPCNRCTRL	0x317
#define	REG_DPTHR	0x318
#define	REG_DPTHRMIN	0x319
#define	REG_DPTHRMAX	0x31A
#define	REG_DPTHRSLP	0x31B
#define	REG_NRTHR	0x31C
#define	REG_NRTHRMIN	0x31D
#define	REG_NRTHRMAX	0x31E
#define	REG_NRTHRSLP	0x31F
#define	REG_STRTNOR	0x320
#define	REG_STRTDRK	0x321
#define	REG_HxBLCL	0x322
#define	REG_RxBLCL	0x323
#define	REG_GrBLCL	0x324
#define	REG_GbBLCL	0x325
#define	REG_BxBLCL	0x326
/* 0x27~0x2E Reserved */
#define	REG_BLCCTRL	0x32F
#define	REG_GMA0	0x330
#define	REG_GMA1	0x331
#define	REG_GMA2	0x332
#define	REG_GMA3	0x333
#define	REG_GMA4	0x334
#define	REG_GMA5	0x335
#define	REG_GMA6	0x336
#define	REG_GMA7	0x337
#define	REG_GMA8	0x338
#define	REG_GMA9	0x339
#define	REG_GMAA	0x33A
#define	REG_GMAB	0x33B
#define	REG_GMAC	0x33C
#define	REG_GMAD	0x33D
#define	REG_GMAE	0x33E
#define	REG_GMAF	0x33F
#define	REG_SPOSIA	0x340
#define	REG_SPOSIB	0x341
#define	REG_SPOSIC	0x342
#define	REG_SPOSID	0x343
#define	REG_SPOSIE	0x344
#define	REG_SPOSIF	0x345
#define	REG_SHDHORR	0x346
#define	REG_SHDVERR	0x347
#define	REG_SHDHORGR	0x348
#define	REG_SHDVERGR	0x349
#define	REG_SHDHORGB	0x34A
#define	REG_SHDVERGB	0x34B
#define	REG_SHDHORB	0x34C
#define	REG_SHDVERB	0x34D
#define	REG_SHDCNTH	0x34E
#define	REG_SHDCNTX	0x34F
#define	REG_SHDCNTY	0x350
#define	REG_SSRT	0x351
#define	REG_SHDOFSRR	0x352
#define	REG_SHDOFSGR	0x353
#define	REG_SHDOFSGB	0x354
#define	REG_SHDOFSBB	0x355
#define	REG_SHDDRKCTRL1	0x356
#define	REG_SHDDRKCTRL2	0x357
#define	REG_SHDDRKCTRL3	0x358
/* 0x59~0x5F Reserved */
#define	REG_INTNMCNT	0x360
/* 0x61 Reserved */
#define	REG_ASLPCTRL	0x362
#define	REG_YDTECTRL	0x363
#define	REG_GPEVCTRL	0x364
/* 0x65 Reserved */
#define	REG_SATHRMIN	0x366
#define	REG_SATHRMAX	0x367
#define	REG_SATHRSRT	0x368
#define	REG_SATHRSLP	0x369
/* 0x6A~0x70 Reserved */
#define	REG_CMA11	0x371
#define	REG_CMA12	0x372
#define	REG_CMA13	0x373
#define	REG_CMA21	0x374
#define	REG_CMA22	0x375
#define	REG_CMA23	0x376
#define	REG_CMA31	0x377
#define	REG_CMA32	0x378
#define	REG_CMA33	0x379
#define	REG_CMB11	0x37A
#define	REG_CMB12	0x37B
#define	REG_CMB13	0x37C
#define	REG_CMB21	0x37D
#define	REG_CMB22	0x37E
#define	REG_CMB23	0x37F
#define	REG_CMB31	0x380
#define	REG_CMB32	0x381
#define	REG_CMB33	0x382
#define	REG_CMC11	0x383
#define	REG_CMC12	0x384
#define	REG_CMC13	0x385
#define	REG_CMC21	0x386
#define	REG_CMC22	0x387
#define	REG_CMC23	0x388
#define	REG_CMC31	0x389
#define	REG_CMC32	0x38A
#define	REG_CMC33	0x38B
#define	REG_CMASEL	0x38C
#define	REG_GPROGECNT	0x38D
#define	REG_GPROGEVAL	0x38E
/* 0x8F Reserved */
#define	REG_Geugain	0x390
#define	REG_Gedgain	0x391
#define	REG_Geucore	0x392
#define	REG_GeucoreStrt	0x393
/* 0x94 Reserved */
#define	REG_GeucoreSlop	0x395
#define	REG_Gedcore	0x396
#define	REG_GedcoreStrt	0x397
/* 0x98 Reserved */
#define	REG_GedcoreSlop	0x399
#define	REG_Geuclip	0x39A
#define	REG_Gedclip	0x39B
#define	REG_GESTART	0x39C
#define	REG_GESLOP	0x39D
#define	REG_Gedglevel	0x39E
#define	REG_Yeugain	0x39F
#define	REG_Yedgain	0x3A0
#define	REG_Yeucore	0x3A1
/* 0xA2 Reserved */
#define	REG_Yeuclip	0x3A3
#define	REG_Yedclip	0x3A4
#define	REG_YESTART	0x3A5
#define	REG_YESLOP	0x3A6
#define	REG_Yedglevel	0x3A7
#define	REG_YGAIN	0x3A8
#define	REG_CRGAIN	0x3A9
#define	REG_CBGAIN	0x3AA
#define	REG_BRTCNT	0x3AB
#define	  BRTCNT_SIGN	0x80
#define	  BRTCNT_MASK	0x7F
#define	REG_CONTGAIN	0x3AC
#define	  CONTGAIN_ENABLE 0x80
#define	  CONTGAIN_MASK	0x3F
#define	REG_IMGYOFST	0x3AD
#define	REG_HUECOS	0x3AE
#define	REG_HUESIN	0x3AF
#define	REG_YTOP	0x3B0
#define	REG_YBOT	0x3B1
#define	REG_CRTOP	0x3B2
#define	REG_CRBOT	0x3B3
#define	REG_CBTOP	0x3B4
#define	REG_CBBOT	0x3B5
#define	REG_IEFCT	0x3B6
#define	REG_CBEFCT	0x3B7
#define	REG_CREFCT	0x3B8
#define	REG_GSTRT	0x3B9
#define	REG_GSLOP	0x3BA
#define	REG_YXOFFSET	0x3BB
#define	REG_CSGMTMN	0x3BC
#define	REG_CSGMTMX	0x3BD
#define	REG_CSGMTSRT	0x3BE
#define	REG_CSGMTSLP	0x3BF
/*
 * Window mode registers could be used to crop image to user defined
 * output resolution.
 *
 * Horizontal start: REG_WDATH[7:6] << 8 | REG_WHSRTL[7:0]
 * Horizontal width: REG_WDATH[5:4] << 8 | REG_WHWIDL[7:0]
 * Vertival start: REG_WDATH[3] << 8 | REG_WVSRTL[7:0]
 * Vertical width: REG_WDATH[2] << 8 | REG_WVWIDL[7:0]
 */
#define	REG_WDATH	0x3C0
#define	REG_WHSRTL	0x3C1
#define	REG_WHWIDL	0x3C2
#define	REG_WVSRTL	0x3C3
#define	REG_WVWIDL	0x3C4
/* 0xC5~0xCB Reserved */
#define	REG_CSC11	0x3CC
#define	REG_CSC12	0x3CD
#define	REG_CSC13	0x3CE
#define	REG_CSC21	0x3CF
#define	REG_CSC22	0x3D0
#define	REG_CSC23	0x3D1
#define	REG_CSC31	0x3D2
#define	REG_CSC32	0x3D3
#define	REG_CSC33	0x3D4
/* 0xD5~0xD8 Reserved */
#define	REG_YSGMTMN	0x3D9
#define	REG_YSGMTMX	0x3DA
#define	REG_YSGMTSRT	0x3DB
#define	REG_YSGMTSLP	0x3DC
/* 0xDD Reserved */
#define	REG_NOIZCTRL	0x3DE
#define	REG_DRKSTART	0x3DF
#define	REG_DRKOFSYTARGET 0x3E0
#define	REG_DRKOFSSTART	0x3E1
#define	REG_DRKOFSSLOP	0x3E2
#define	REG_DRKOFSMAX	0x3E3
#define	REG_DRKOFSLVL	0x3E4
/* 0xE5~0xFF Reserved */

#endif /* SIV120D_H */

