/*
 * Copyright 1998-2009 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2008 S3 Graphics, Inc. All Rights Reserved.

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTIES OR REPRESENTATIONS; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.See the GNU General Public License
 * for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <linux/via-core.h>

#define _MASTER_FILE
#include "global.h"

#ifdef CONFIG_PM
#include "via_suspend.h"

static void viafb_save_vga_reg(void)
{
	u8 i;

	/* Save Sequence regs: from SR00 to SRFF */
	for (i = 0; i <= 0x04; i++)
		viaparinfo->saved_seq_regs[i] = viafb_read_reg(VIASR, i);

	/* Save CRTC regs: from CR00 to CRFF */
	for (i = 0; i <= 0x18; i++)
		if ((i != 0x08) || (i != 0x09))
			viaparinfo->saved_crt_regs[i] = viafb_read_reg(VIACR, i);

	/* Save Graphics regs: from CR00 to CRFF */
	for (i = 0; i <= 0x08; i++)
		viaparinfo->saved_gr_regs[i] = viafb_read_reg(VIAGR, i);
	viaparinfo->saved_gr_regs[0x09] = viafb_read_reg(VIAGR, 0x20);
	viaparinfo->saved_gr_regs[0x0a] = viafb_read_reg(VIAGR, 0x21);
	viaparinfo->saved_gr_regs[0x0b] = viafb_read_reg(VIAGR, 0x22);

	/* Save Attibute regs: from CR00 to CRFF */
	for (i = 0; i <= 0x14; i++)
		viaparinfo->saved_ar_regs[i] = viafb_read_reg(VIAAR, i);
}

static void viafb_restore_vga_reg(void)
{
	u8 i;

	/* Save Sequence regs: from SR00 to SRFF */
	for (i = 0; i <= 0x04; i++)
		viafb_write_reg(i, VIASR, viaparinfo->saved_seq_regs[i]);

	/* Write CRTC regs: from CR00 to CR24 */
	for (i = 0; i <= 0x18; i++)
		if ((i < 0x07) || (i > 0x09))
			viafb_write_reg(i, VIACR, viaparinfo->saved_crt_regs[i]);

	/* Write Attribute regs: from AR00 to AR14 */
	for (i = 0; i <= 0x14; i++)
		viafb_write_reg(i, VIAAR, viaparinfo->saved_ar_regs[i]);

	/* Write Graphics regs: from GR00 to GR08, and GR20, GR21, GR22 */
	for (i = 0; i <= 0x08; i++)
		viafb_write_reg(i, VIAGR, viaparinfo->saved_gr_regs[i]);
	viafb_write_reg(0x20, VIAGR, viaparinfo->saved_gr_regs[0x09]);
	viafb_write_reg(0x21, VIAGR, viaparinfo->saved_gr_regs[0x0a]);
	viafb_write_reg(0x22, VIAGR, viaparinfo->saved_gr_regs[0x0b]);
}

static void viafb_save_seq_reg(void)
{
	u8 i;

	/* Save Sequence regs: from SR00 to SRFF */
	for (i = 0x08; i < 0xFF; i++)
		viaparinfo->saved_seq_regs[i] = viafb_read_reg(VIASR, i);
}

static void viafb_restore_seq_reg(void)
{
	u8 i;

	for (i = 0x14; i <= 0x40; i++)
		if ((i != 0x1a) || (i != 0x1e) || (i != 0x2c) || (i != 0x3d))
			viafb_write_reg(i, VIASR, viaparinfo->saved_seq_regs[i]);

	viafb_write_reg(SR42, VIASR, viaparinfo->saved_seq_regs[0x42]);
	viafb_write_reg(SR4F, VIASR, viaparinfo->saved_seq_regs[0x4F]);
	viafb_write_reg(SR50, VIASR, viaparinfo->saved_seq_regs[0x50]);
	viafb_write_reg(SR57, VIASR, viaparinfo->saved_seq_regs[0x57]);
	viafb_write_reg(SR5D, VIASR, viaparinfo->saved_seq_regs[0x5D]);

	/* DVP1 Clock and Data Pads Driving: */
	viafb_write_reg(SR65, VIASR, viaparinfo->saved_seq_regs[0x65]);

	/*=* restore VCK, LCDCK and ECK *=*/
	/* VCK: */
	viafb_write_reg(SR44, VIASR, viaparinfo->saved_seq_regs[0x44]);
	viafb_write_reg(SR45, VIASR, viaparinfo->saved_seq_regs[0x45]);
	viafb_write_reg(SR46, VIASR, viaparinfo->saved_seq_regs[0x46]);

	/* Reset VCK PLL */
	viafb_write_reg_mask(SR40, VIASR, 0x02, BIT1);
	viafb_write_reg_mask(SR40, VIASR, 0x00, BIT1);

	/* LCK: */
	viafb_write_reg(SR4A, VIASR, viaparinfo->saved_seq_regs[0x4A]);
	viafb_write_reg(SR4B, VIASR, viaparinfo->saved_seq_regs[0x4B]);
	viafb_write_reg(SR4C, VIASR, viaparinfo->saved_seq_regs[0x4C]);

	/* Reset VCK PLL */
	viafb_write_reg_mask(SR40, VIASR, 0x04, BIT2);
	viafb_write_reg_mask(SR40, VIASR, 0x00, BIT2);

	/* ECK: */
	viafb_write_reg(SR47, VIASR, viaparinfo->saved_seq_regs[0x47]);
	viafb_write_reg(SR48, VIASR, viaparinfo->saved_seq_regs[0x48]);
	viafb_write_reg(SR49, VIASR, viaparinfo->saved_seq_regs[0x49]);

	/* Reset VCK PLL */
	viafb_write_reg_mask(SR40, VIASR, 0x01, BIT0);
	viafb_write_reg_mask(SR40, VIASR, 0x00, BIT0);
}

static void viafb_save_crt_reg(void)
{
	u8 i;

	/* Save CRTC regs: from CR00 to CRFF */
	for (i = 0x19; i < 0xFF; i++)
		viaparinfo->saved_crt_regs[i] = viafb_read_reg(VIACR, i);
}

static void viafb_restore_crt_reg(void)
{
	u8 i;

	/* Restore CRTC controller extended regs: */
	viafb_write_reg(CR08, VIACR, viaparinfo->saved_crt_regs[0x08]);
	viafb_write_reg(CR09, VIACR, viaparinfo->saved_crt_regs[0x09]);

	viafb_write_reg(CR13, VIACR, viaparinfo->saved_crt_regs[0x13]);
	viafb_write_reg(CR30, VIACR, viaparinfo->saved_crt_regs[0x30]);
	viafb_write_reg(CR31, VIACR, viaparinfo->saved_crt_regs[0x31]);
	viafb_write_reg(CR32, VIACR, viaparinfo->saved_crt_regs[0x32]);
	viafb_write_reg(CR33, VIACR, viaparinfo->saved_crt_regs[0x33]);

	for (i = 0x35; i <= 0x47; i++)
		viafb_write_reg(i, VIACR, viaparinfo->saved_crt_regs[i]);

	/* Starting Address: */
	viafb_write_reg(CR0C, VIACR, viaparinfo->saved_crt_regs[0x0C]);
	viafb_write_reg(CR0D, VIACR, viaparinfo->saved_crt_regs[0x0D]);
	viafb_write_reg(CR48, VIACR, viaparinfo->saved_crt_regs[0x48]);
	viafb_write_reg(CR34, VIACR, viaparinfo->saved_crt_regs[0x34]);

	for (i = 0x49; i < 0xFF; i++)
	{
		if ((i == 0x91) || (i == 0x6A))
		{
			/* we mask CR6A when we write it or it messes up the gamma */
			if (i == 0x6A)
				viafb_write_reg_mask(CR6A, VIACR, viaparinfo->saved_crt_regs[0x6A], 0xF4);
			continue;
		}
		viafb_write_reg(i, VIACR, viaparinfo->saved_crt_regs[i]);
	}
}

static void viafb_save_video_reg(void)
{
	struct viafb_par *viapar = viafbinfo->par;
	void __iomem *engine = viapar->shared->vdev->engine_mmio;
	volatile video_via_regs  *viaVidEng = 
		(volatile video_via_regs *)(engine + 0x200);

	memcpy_fromio((void *)&viaparinfo->saved_video_regs, 
			engine + 0x200, sizeof(video_via_regs));

	/* fire video engine */
	viaVidEng->compose		   |= V1_COMMAND_FIRE;
	viaVidEng->compose		   |= V3_COMMAND_FIRE;
}

static void viafb_restore_video_reg(void)
{
	struct viafb_par *viapar = viafbinfo->par;
	void __iomem *engine = viapar->shared->vdev->engine_mmio;
	volatile video_via_regs  *viaVidEng = 
		(volatile video_via_regs *)(engine + 0x200);
	video_via_regs	*localVidEng = &viaparinfo->saved_video_regs;

	/*
	 * Following set of register restores is black magic takend
	 * from the VIA X driver. Most of it is from the LeaveVT() and
	 * EnterVT() path and some is gleaned by looking at other bits
	 * of code to figure out what registers to touch.
	 */
	viaVidEng->alphawin_size   = localVidEng->alphawin_size;
	viaVidEng->alphawin_ctl    = localVidEng->alphawin_ctl;
	viaVidEng->alphafb_stride  = localVidEng->alphafb_stride;
	viaVidEng->color_key	   = localVidEng->color_key;
	viaVidEng->alphafb_addr    = localVidEng->alphafb_addr;
	viaVidEng->chroma_low	   = localVidEng->chroma_low;
	viaVidEng->chroma_up	   = localVidEng->chroma_up;
	viaVidEng->interruptflag   = localVidEng->interruptflag;

	/*VT3314 only has V3*/
	viaVidEng->video1_ctl	   = localVidEng->video1_ctl;
	viaVidEng->video1_fetch    = localVidEng->video1_fetch;
	viaVidEng->video1y_addr1   = localVidEng->video1y_addr1;
	viaVidEng->video1_stride   = localVidEng->video1_stride;
	viaVidEng->video1_hvstart  = localVidEng->video1_hvstart;
	viaVidEng->video1_size	   = localVidEng->video1_size;
	viaVidEng->video1y_addr2   = localVidEng->video1y_addr2;
	viaVidEng->video1_zoom	   = localVidEng->video1_zoom;
	viaVidEng->video1_mictl    = localVidEng->video1_mictl;
	viaVidEng->video1y_addr0   = localVidEng->video1y_addr0;
	viaVidEng->video1_fifo	   = localVidEng->video1_fifo;
	viaVidEng->video1y_addr3   = localVidEng->video1y_addr3;
	viaVidEng->v1_source_w_h   = localVidEng->v1_source_w_h;
	viaVidEng->video1_CSC1	   = localVidEng->video1_CSC1;
	viaVidEng->video1_CSC2	   = localVidEng->video1_CSC2;

	viaVidEng->snd_color_key   = localVidEng->snd_color_key;
	viaVidEng->v3alpha_prefifo = localVidEng->v3alpha_prefifo;
	viaVidEng->v3alpha_fifo    = localVidEng->v3alpha_fifo;
	viaVidEng->video3_CSC2	   = localVidEng->video3_CSC2;
	viaVidEng->video3_CSC2	   = localVidEng->video3_CSC2;
	viaVidEng->v3_source_width = localVidEng->v3_source_width;
	viaVidEng->video3_ctl	   = localVidEng->video3_ctl;
	viaVidEng->video3_addr0    = localVidEng->video3_addr0;
	viaVidEng->video3_addr1    = localVidEng->video3_addr1;
	viaVidEng->video3_stride   = localVidEng->video3_stride;
	viaVidEng->video3_hvstart  = localVidEng->video3_hvstart;
	viaVidEng->video3_size	   = localVidEng->video3_size;
	viaVidEng->v3alpha_fetch   = localVidEng->v3alpha_fetch;
	viaVidEng->video3_zoom	   = localVidEng->video3_zoom;
	viaVidEng->video3_mictl    = localVidEng->video3_mictl;
	viaVidEng->video3_CSC1	   = localVidEng->video3_CSC1;
	viaVidEng->video3_CSC2	   = localVidEng->video3_CSC2;	  
	viaVidEng->compose		   = localVidEng->compose;

	/* this is an unused register we use it to tell
     * userspace to force an overlay update */
	viaVidEng->v_display_temp  |= OVL_FORCE_UPDATE;

	/* fire video engine */
	viaVidEng->compose		   |= V1_COMMAND_FIRE;
	viaVidEng->compose		   |= V3_COMMAND_FIRE;
}

static void viafb_save_hqv_reg(void)
{
	struct viafb_par *viapar = viafbinfo->par;
	void __iomem *engine = viapar->shared->vdev->engine_mmio;

	viaparinfo->saved_hqv0_regs.subp_ctl = readl(engine + 0x3c0);
	viaparinfo->saved_hqv0_regs.subp_addr = readl(engine + 0x3c4);
	viaparinfo->saved_hqv0_regs.subp_palette = readl(engine + 0x3c8);
	viaparinfo->saved_hqv0_regs.src_data_offset = readl(engine + 0x3cc);
	viaparinfo->saved_hqv0_regs.src_y = readl(engine + 0x3d4);
	viaparinfo->saved_hqv0_regs.src_u = readl(engine + 0x3d8);
	viaparinfo->saved_hqv0_regs.misc_ctl = readl(engine + 0x3dc);
	viaparinfo->saved_hqv0_regs.src_linefetch = readl(engine + 0x3e0);
	viaparinfo->saved_hqv0_regs.mad_ctl = readl(engine + 0x3e4);
	viaparinfo->saved_hqv0_regs.scal_ctl = readl(engine + 0x3e8);
	viaparinfo->saved_hqv0_regs.dst_addr0 = readl(engine + 0x3ec);
	viaparinfo->saved_hqv0_regs.dst_addr1 = readl(engine + 0x3f0);
	viaparinfo->saved_hqv0_regs.dst_stride = readl(engine + 0x3f4);
	viaparinfo->saved_hqv0_regs.src_stride_flip = readl(engine + 0x3f8);
	viaparinfo->saved_hqv0_regs.dst_addr2 = readl(engine + 0x3fc);
	viaparinfo->saved_hqv0_regs.hqv_ctl = readl(engine + 0x3d0);

	viaparinfo->saved_hqv1_regs.subp_ctl = readl(engine + 0x13c0);
	viaparinfo->saved_hqv1_regs.subp_addr = readl(engine + 0x13c4);
	viaparinfo->saved_hqv1_regs.subp_palette = readl(engine + 0x13c8);
	viaparinfo->saved_hqv1_regs.src_data_offset = readl(engine + 0x13cc);
	viaparinfo->saved_hqv1_regs.src_y = readl(engine + 0x13d4);
	viaparinfo->saved_hqv1_regs.src_u = readl(engine + 0x13d8);
	viaparinfo->saved_hqv1_regs.misc_ctl = readl(engine + 0x13dc);
	viaparinfo->saved_hqv1_regs.src_linefetch = readl(engine + 0x13e0);
	viaparinfo->saved_hqv1_regs.mad_ctl = readl(engine + 0x13e4);
	viaparinfo->saved_hqv1_regs.scal_ctl = readl(engine + 0x13e8);
	viaparinfo->saved_hqv1_regs.dst_addr0 = readl(engine + 0x13ec);
	viaparinfo->saved_hqv1_regs.dst_addr1 = readl(engine + 0x13f0);
	viaparinfo->saved_hqv1_regs.dst_stride = readl(engine + 0x13f4);
	viaparinfo->saved_hqv1_regs.src_stride_flip = readl(engine + 0x13f8);
	viaparinfo->saved_hqv1_regs.dst_addr2 = readl(engine + 0x13fc);
	viaparinfo->saved_hqv1_regs.hqv_ctl = readl(engine + 0x13d0);
}

static void viafb_restore_hqv_reg(void)
{
	struct viafb_par *viapar = viafbinfo->par;
	void __iomem *engine = viapar->shared->vdev->engine_mmio;

	writel(viaparinfo->saved_hqv0_regs.subp_ctl, engine + 0x3c0);
	writel(viaparinfo->saved_hqv0_regs.subp_addr, engine + 0x3c4);
	writel(viaparinfo->saved_hqv0_regs.subp_palette, engine + 0x3c8);
	writel(viaparinfo->saved_hqv0_regs.src_data_offset, engine + 0x3cc);
	writel(viaparinfo->saved_hqv0_regs.src_y, engine + 0x3d4);
	writel(viaparinfo->saved_hqv0_regs.src_u, engine + 0x3d8);
	writel(viaparinfo->saved_hqv0_regs.misc_ctl, engine + 0x3dc);
	writel(viaparinfo->saved_hqv0_regs.src_linefetch, engine + 0x3e0);
	writel(viaparinfo->saved_hqv0_regs.mad_ctl, engine + 0x3e4);
	writel(viaparinfo->saved_hqv0_regs.scal_ctl, engine + 0x3e8);
	writel(viaparinfo->saved_hqv0_regs.dst_addr0, engine + 0x3ec);
	writel(viaparinfo->saved_hqv0_regs.dst_addr1, engine + 0x3f0);
	writel(viaparinfo->saved_hqv0_regs.dst_stride, engine + 0x3f4);
	writel(viaparinfo->saved_hqv0_regs.src_stride_flip, engine + 0x3f8);
	writel(viaparinfo->saved_hqv0_regs.dst_addr2, engine + 0x3fc);
	writel(viaparinfo->saved_hqv0_regs.hqv_ctl, engine + 0x3d0);

	writel(viaparinfo->saved_hqv1_regs.subp_ctl, engine + 0x13c0);
	writel(viaparinfo->saved_hqv1_regs.subp_addr, engine + 0x13c4);
	writel(viaparinfo->saved_hqv1_regs.subp_palette, engine + 0x13c8);
	writel(viaparinfo->saved_hqv1_regs.src_data_offset, engine + 0x13cc);
	writel(viaparinfo->saved_hqv1_regs.src_y, engine + 0x13d4);
	writel(viaparinfo->saved_hqv1_regs.src_u, engine + 0x13d8);
	writel(viaparinfo->saved_hqv1_regs.misc_ctl, engine + 0x13dc);
	writel(viaparinfo->saved_hqv1_regs.src_linefetch, engine + 0x13e0);
	writel(viaparinfo->saved_hqv1_regs.mad_ctl, engine + 0x13e4);
	writel(viaparinfo->saved_hqv1_regs.scal_ctl, engine + 0x13e8);
	writel(viaparinfo->saved_hqv1_regs.dst_addr0, engine + 0x13ec);
	writel(viaparinfo->saved_hqv1_regs.dst_addr1, engine + 0x13f0);
	writel(viaparinfo->saved_hqv1_regs.dst_stride, engine + 0x13f4);
	writel(viaparinfo->saved_hqv1_regs.src_stride_flip, engine + 0x13f8);
	writel(viaparinfo->saved_hqv1_regs.dst_addr2, engine + 0x13fc);
	writel(viaparinfo->saved_hqv1_regs.hqv_ctl, engine + 0x13d0);
}

static void viafb_init_3d_engine(void)
{
	struct viafb_par *viapar = viafbinfo->par;
	void __iomem *engine = viapar->shared->vdev->engine_mmio;
	u32 chip_name = viapar->shared->chip_info.gfx_chip_name;
	u32 StageOfTexture;
	int i;

	viafb_wait_engine_idle(viafbinfo);

	/* Init AGP and VQ regs */
	switch (chip_name) {
	case UNICHROME_K8M890:
	case UNICHROME_P4M900:
	case UNICHROME_VX800:
	case UNICHROME_VX855:
		printk(KERN_INFO "viafb_resume: Init 3d engine!\n");
		writel(0x00010000, engine + VIA_REG_TRANSET);
		for (i = 0; i <= 0x9A; i++)
			writel((u32) i << 24, engine + VIA_REG_TRANSPACE);

		/* Initial Texture Stage Setting*/
		for (StageOfTexture = 0; StageOfTexture <= 0xf; StageOfTexture++)
		{
			writel((0x00020000 | 0x00000000 | (StageOfTexture & 0xf) << 24),
					engine + VIA_REG_TRANSET);
			for (i = 0 ; i <= 0x30 ; i++ )
				writel((u32) i << 24, engine + VIA_REG_TRANSPACE);
		}

		/* Initial Texture Sampler Setting*/
		for (StageOfTexture = 0; StageOfTexture <= 0xf; StageOfTexture++)
		{
			writel((0x00020000 | 0x20000000 | (StageOfTexture & 0xf) << 24),
					engine + VIA_REG_TRANSET);
			for (i = 0 ; i <= 0x36 ; i++ )
				writel((u32) i << 24, engine + VIA_REG_TRANSPACE);
		}
			
		/* re-init 3d texture engine */
		writel((0x00020000 | 0xfe000000), engine + VIA_REG_TRANSET);
		for (i = 0; i <= 0x13; i++)
			writel((u32) i << 24, engine + VIA_REG_TRANSPACE);

		/* Initial Gamma Table Setting*/
		/* Initial Gamma Table Setting*/
		/* 5 + 4 = 9 (12) dwords*/
		/* sRGB texture is not directly support by H3 hardware.*/
		/* We have to set the deGamma table for texture sampling.*/

		/* degamma table*/
		writel(( 0x00030000 | 0x15000000), engine + VIA_REG_TRANSET);
		writel((0x40000000 | (30 << 20) | (15 << 10) | (5)), engine + VIA_REG_TRANSPACE);
		writel(((119 << 20) | (81 << 10) | (52)), engine + VIA_REG_TRANSPACE);
		writel(((283 << 20) | (219 << 10) | (165)), engine + VIA_REG_TRANSPACE);
		writel(((535 << 20) | (441 << 10) | (357)), engine + VIA_REG_TRANSPACE);
		writel(((119 << 20) | (884 << 20) | (757 << 10) | (640)), engine + VIA_REG_TRANSPACE);

		/* gamma table*/
		writel(( 0x00030000 | 0x17000000), engine + VIA_REG_TRANSET);
		writel((0x40000000 | (13 << 20) | (13 << 10) | (13)), engine + VIA_REG_TRANSPACE);
		writel((0x40000000 | (26 << 20) | (26 << 10) | (26)), engine + VIA_REG_TRANSPACE);
		writel((0x40000000 | (39 << 20) | (39 << 10) | (39)), engine + VIA_REG_TRANSPACE);
		writel(((51 << 20) | (51 << 10) | (51)), engine + VIA_REG_TRANSPACE);
		writel(((71 << 20) | (71 << 10) | (71)), engine + VIA_REG_TRANSPACE);
		writel((87 << 20) | (87 << 10) | (87), engine + VIA_REG_TRANSPACE);
		writel((113 << 20) | (113 << 10) | (113), engine + VIA_REG_TRANSPACE);
		writel((135 << 20) | (135 << 10) | (135), engine + VIA_REG_TRANSPACE);
		writel((170 << 20) | (170 << 10) | (170), engine + VIA_REG_TRANSPACE);
		writel((199 << 20) | (199 << 10) | (199), engine + VIA_REG_TRANSPACE);
		writel((246 << 20) | (246 << 10) | (246), engine + VIA_REG_TRANSPACE);
		writel((284 << 20) | (284 << 10) | (284), engine + VIA_REG_TRANSPACE);
		writel((317 << 20) | (317 << 10) | (317), engine + VIA_REG_TRANSPACE);
		writel((347 << 20) | (347 << 10) | (347), engine + VIA_REG_TRANSPACE);
		writel((373 << 20) | (373 << 10) | (373), engine + VIA_REG_TRANSPACE);
		writel((398 << 20) | (398 << 10) | (398), engine + VIA_REG_TRANSPACE);
		writel((442 << 20) | (442 << 10) | (442), engine + VIA_REG_TRANSPACE);
		writel((481 << 20) | (481 << 10) | (481), engine + VIA_REG_TRANSPACE);
		writel((517 << 20) | (517 << 10) | (517), engine + VIA_REG_TRANSPACE);
		writel((550 << 20) | (550 << 10) | (550), engine + VIA_REG_TRANSPACE);
		writel((609 << 20) | (609 << 10) | (609), engine + VIA_REG_TRANSPACE);
		writel((662 << 20) | (662 << 10) | (662), engine + VIA_REG_TRANSPACE);
		writel((709 << 20) | (709 << 10) | (709), engine + VIA_REG_TRANSPACE);
		writel((753 << 20) | (753 << 10) | (753), engine + VIA_REG_TRANSPACE);
		writel((794 << 20) | (794 << 10) | (794), engine + VIA_REG_TRANSPACE);
		writel((832 << 20) | (832 << 10) | (832), engine + VIA_REG_TRANSPACE);
		writel((868 << 20) | (868 << 10) | (868), engine + VIA_REG_TRANSPACE);
		writel((902 << 20) | (902 << 10) | (902), engine + VIA_REG_TRANSPACE);
		writel((934 << 20) | (934 << 10) | (934), engine + VIA_REG_TRANSPACE);
		writel((966 << 20) | (966 << 10) | (966), engine + VIA_REG_TRANSPACE);
		writel((996 << 20) | (996 << 10) | (996), engine + VIA_REG_TRANSPACE);

		/* For Interrupt Restore only
		   All types of write through regsiters should be write header data to
		   hardware at least before it can restore. H/W will automatically record
		   the header to write through state buffer for resture usage.
		   By Jaren:
		   HParaType = 8'h03, HParaSubType = 8'h00
											 8'h11
											 8'h12
											 8'h14
											 8'h15
											 8'h17
		   HParaSubType 8'h12, 8'h15 is initialized.
		  [HWLimit]
		   1. All these write through registers can't be partial update.
		   2. All these write through must be AGP command
		   16 entries : 4 128-bit data */

		/* Initialize INV_ParaSubType_TexPal */
		writel((0x00030000 | 0x00000000), engine + VIA_REG_TRANSET);
		for (i = 0; i < 16; i++)
			writel(0x00000000, engine + VIA_REG_TRANSPACE);

		/* Initialize INV_ParaSubType_4X4Cof */
		/* 32 entries : 8 128-bit data */
		writel((0x00030000 | 0x11000000), engine + VIA_REG_TRANSET);
		for (i = 0; i < 32; i++)
			writel(0x00000000, engine + VIA_REG_TRANSPACE);

		/* Initialize INV_ParaSubType_StipPal */
		/* 5 entries : 2 128-bit data */
		writel((0x00030000 | 0x14000000), engine + VIA_REG_TRANSET);
		for (i = 0; i < (5+3); i++)
			writel(0x00000000, engine + VIA_REG_TRANSPACE);

		/* primitive setting & vertex format */
		writel(0x00040000, engine + VIA_REG_TRANSET);
		for( i = 0; i <= 0x62; i++)
			writel(((u32) i << 24), engine + VIA_REG_TRANSPACE);

		/* ParaType 0xFE - Configure and Misc Setting */
		writel(0x00fe0000, engine + VIA_REG_TRANSET);
		for( i = 0; i <= 0x47; i++)
			writel(((u32) i << 24), engine + VIA_REG_TRANSPACE);

		/* ParaType 0x11 - Frame Buffer Auto-Swapping and
		Command Regulator Misc */
		writel(0x00110000, engine + VIA_REG_TRANSET);
		for( i = 0; i <= 0x20; i++)
			writel(((u32) i << 24), engine + VIA_REG_TRANSPACE);

		writel(0x00fe0000, engine + VIA_REG_TRANSET);
		writel(0x4000840f, engine + VIA_REG_TRANSPACE);
		writel(0x47000400, engine + VIA_REG_TRANSPACE);
		writel(0x44000000, engine + VIA_REG_TRANSPACE);
		writel(0x46000000, engine + VIA_REG_TRANSPACE);

		/* setting Misconfig*/
		writel(0x00fe0000, engine + VIA_REG_TRANSET);
		writel(0x00001004, engine + VIA_REG_TRANSPACE);
		writel(0x08000249, engine + VIA_REG_TRANSPACE);
		writel(0x0a0002c9, engine + VIA_REG_TRANSPACE);
		writel(0x0b0002fb, engine + VIA_REG_TRANSPACE);
		writel(0x0c000000, engine + VIA_REG_TRANSPACE);
		writel(0x0d0002cb, engine + VIA_REG_TRANSPACE);
		writel(0x0e000009, engine + VIA_REG_TRANSPACE);
		writel(0x10000049, engine + VIA_REG_TRANSPACE);
		writel(0x110002ff, engine + VIA_REG_TRANSPACE);
		writel(0x12000008, engine + VIA_REG_TRANSPACE);
		writel(0x130002db, engine + VIA_REG_TRANSPACE);
		break;
	default:
		break;
	}
}

int viafb_suspend(void *unused)
{
	struct viafb_par *viapar = viafbinfo->par;
	void __iomem *engine = viapar->shared->vdev->engine_mmio;
	u32 *viafb_gamma_table;

	printk(KERN_WARNING "viafb_suspend!\n");

	console_lock();

	viafb_write_reg_mask(SR10, VIASR, 0x01, BIT0);

	viafb_save_vga_reg();
	viafb_save_seq_reg();
	viafb_save_crt_reg();

	viafb_gamma_table = viaparinfo->saved_gamma_table;
	viafb_get_gamma_table(viafb_gamma_table);

	viaparinfo->saved_cur_regs[0] = readl(engine + VIA_REG_HI_FBOFFSET);
	viaparinfo->saved_cur_regs[1] = readl(engine + VIA_REG_CURSOR_POS);
	viaparinfo->saved_cur_regs[2] = readl(engine + VIA_REG_HI_CENTEROFFSET);
	viaparinfo->saved_cur_regs[3] = readl(engine + VIA_REG_HI_CONTROL);

	viafb_save_hqv_reg();
	viafb_save_video_reg();

	fb_set_suspend(viafbinfo, 1);
	viafb_sync(viafbinfo);
	console_unlock();

	return 0;
}

int viafb_resume(void *unused)
{
	struct viafb_par *viapar = viafbinfo->par;
	void __iomem *engine = viapar->shared->vdev->engine_mmio;
	printk(KERN_INFO "viafb_resume!\n");
	
	console_lock();

	viafb_write_reg_mask(SR10, VIASR, 0x01, BIT0);
	viafb_unlock_crt();

	viafb_write_reg_mask(CR6A, VIACR, 0x00, BIT6);
	viafb_write_reg_mask(CR6A, VIACR, 0x00, BIT7);
	viafb_write_reg_mask(CR6A, VIACR, 0x40, BIT6);

	viafb_write_reg_mask(CR6B, VIACR, 0x00, 0xFF);
	viafb_write_reg_mask(CR6C, VIACR, 0x00, 0xFF);

	viafb_disable_gamma();

	viafb_restore_vga_reg();
	viafb_restore_seq_reg();
	viafb_restore_crt_reg();

	viafb_set_gamma_table(32, viaparinfo->saved_gamma_table);

	//viafb_write_reg(SR1A, VIASR, viafb_read_reg(VIASR, SR1A));

	via_write_misc_reg_mask(via_read_misc_reg(), 0xFF);

	viafb_lock_crt();

	/* This restores the ARGB Cursor for the XO 1.5 */
	writel(viaparinfo->saved_cur_regs[0], engine + VIA_REG_HI_FBOFFSET);
	writel(viaparinfo->saved_cur_regs[1], engine + VIA_REG_HI_POSSTART);
	writel(viaparinfo->saved_cur_regs[2], engine + VIA_REG_HI_CENTEROFFSET);
	writel(viaparinfo->saved_cur_regs[3], engine + VIA_REG_HI_CONTROL);

	viafb_reset_engine(viapar);

	/* We restore this Cursor register here as it was just reset
 	 * to 0x0 in viafb_init_engine
 	 */
	writel(viaparinfo->saved_cur_regs[1], engine + VIA_REG_CURSOR_POS);

	viafb_init_3d_engine();

	viafb_restore_hqv_reg();
	viafb_restore_video_reg();
 
	fb_set_suspend(viafbinfo, 0);

	console_unlock();
	return 0;
}
#endif /*CONFIG_PM*/
