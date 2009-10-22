/*
 * Copyright 1998-2008 VIA Technologies, Inc. All Rights Reserved.
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

#ifndef __VIAFBDEV_H__
#define __VIAFBDEV_H__

#include <linux/proc_fs.h>
#include <linux/fb.h>
#include <linux/spinlock.h>

#include "ioctl.h"
#include "share.h"
#include "chip.h"
#include "hw.h"

#define VERSION_MAJOR       2
#define VERSION_KERNEL      6	/* For kernel 2.6 */

#define VERSION_OS          0	/* 0: for 32 bits OS, 1: for 64 bits OS */
#define VERSION_MINOR       4

#define VIAFB_NUM_I2C		5

struct viafb_shared {
	u32 iga1_devices;
	u32 iga2_devices;

	struct proc_dir_entry *proc_entry;	/*viafb proc entry */
	struct proc_dir_entry *iga1_proc_entry;
	struct proc_dir_entry *iga2_proc_entry;
	struct viafb_dev *vdev;			/* Global dev info */

	/* All the information will be needed to set engine */
	struct tmds_setting_information tmds_setting_info;
	struct lvds_setting_information lvds_setting_info;
	struct lvds_setting_information lvds_setting_info2;
	struct chip_information chip_info;

	/* hardware acceleration stuff */
	u32 cursor_vram_addr;
	u32 vq_vram_addr;	/* virtual queue address in video ram */
	int (*hw_bitblt)(void __iomem *engine, u8 op, u32 width, u32 height,
		u8 dst_bpp, u32 dst_addr, u32 dst_pitch, u32 dst_x, u32 dst_y,
		u32 *src_mem, u32 src_addr, u32 src_pitch, u32 src_x, u32 src_y,
		u32 fg_color, u32 bg_color, u8 fill_rop);
};

/*
 * From VIA X Driver
 */
typedef struct{
	u32 interruptflag;         /* 200 */
	u32 ramtab;                /* 204 */
	u32 alphawin_hvstart;      /* 208 */
	u32 alphawin_size;         /* 20c */
	u32 alphawin_ctl;          /* 210 */
	u32 crt_startaddr;         /* 214 */
	u32 crt_startaddr_2;       /* 218 */
	u32 alphafb_stride ;       /* 21c */
	u32 color_key;             /* 220 */
	u32 alphafb_addr;          /* 224 */
	u32 chroma_low;            /* 228 */
	u32 chroma_up;             /* 22c */
	u32 video1_ctl;            /* 230 */
	u32 video1_fetch;          /* 234 */
	u32 video1y_addr1;         /* 238 */
	u32 video1_stride;         /* 23c */
	u32 video1_hvstart;        /* 240 */
	u32 video1_size;           /* 244 */
	u32 video1y_addr2;         /* 248 */
	u32 video1_zoom;           /* 24c */
	u32 video1_mictl;          /* 250 */
	u32 video1y_addr0;         /* 254 */
	u32 video1_fifo;           /* 258 */
	u32 video1y_addr3;         /* 25c */
	u32 hi_control;            /* 260 */
	u32 snd_color_key;         /* 264 */
	u32 v3alpha_prefifo;       /* 268 */
	u32 v1_source_w_h;         /* 26c */
	u32 hi_transparent_color;  /* 270 */
	u32 v_display_temp;        /* 274 :No use */
	u32 v3alpha_fifo;          /* 278 */
	u32 v3_source_width;       /* 27c */
	u32 dummy1;                /* 280 */
	u32 video1_CSC1;           /* 284 */
	u32 video1_CSC2;           /* 288 */
	u32 video1u_addr0;         /* 28c */
	u32 video1_opqctl;         /* 290 */
	u32 video3_opqctl;         /* 294 */
	u32 compose;               /* 298 */
	u32 dummy2;                /* 29c */
	u32 video3_ctl;            /* 2a0 */
	u32 video3_addr0;          /* 2a4 */
	u32 video3_addr1;          /* 2a8 */
	u32 video3_stride;         /* 2ac */
	u32 video3_hvstart;        /* 2b0 */
	u32 video3_size;           /* 2b4 */
	u32 v3alpha_fetch;         /* 2b8 */
	u32 video3_zoom;           /* 2bc */
	u32 video3_mictl;          /* 2c0 */
	u32 video3_CSC1;           /* 2c4 */
	u32 video3_CSC2;           /* 2c8 */
	u32 v3_display_temp;       /* 2cc */
	u32 cursor_mode;
	u32 cursor_pos;
	u32 cursor_org;
	u32 cursor_bg;
	u32 cursor_fg;
	u32 video1u_addr1;         /* 2e4 */
	u32 video1u_addr2;         /* 2e8 */
	u32 video1u_addr3;         /* 2ec */
	u32 video1v_addr0;         /* 2f0 */
	u32 video1v_addr1;         /* 2f4 */
	u32 video1v_addr2;         /* 2f8 */
	u32 video1v_addr3;         /* 2fc */
}  video_via_regs;

typedef struct{
	u32 subp_ctl;			   /* 3c0 */
	u32 subp_addr;			   /* 3c4 */
	u32 subp_palette;          /* 3c8 */
	u32 src_data_offset;       /* 3cc */
	u32 src_y;          	   /* 3d4 */
	u32 src_u;           	   /* 3d8 */
	u32 misc_ctl;              /* 3dc */
	u32 src_linefetch;         /* 3e0 */
	u32 mad_ctl;		       /* 3e4 */
	u32 scal_ctl;			   /* 3e8 */
	u32 dst_addr0;			   /* 3ec */
	u32 dst_addr1;			   /* 3f0 */
	u32 dst_stride;			   /* 3f4 */
	u32 src_stride_flip;       /* 3f8 */
	u32 dst_addr2;         	   /* 3fc */
	u32 hqv_ctl;         	   /* 3d0 */
}  hqv_via_regs;

struct viafb_par {
	u8 depth;
	u32 vram_addr;

	unsigned int fbmem;	/*framebuffer physical memory address */
	unsigned int memsize;	/*size of fbmem */
	u32 fbmem_free;		/* Free FB memory */
	u32 fbmem_used;		/* Use FB memory size */
	u32 iga_path;

	struct viafb_shared *shared;

	/* All the information will be needed to set engine */
	/* depreciated, use the ones in shared directly */
	struct tmds_setting_information *tmds_setting_info;
	struct lvds_setting_information *lvds_setting_info;
	struct lvds_setting_information *lvds_setting_info2;
	struct chip_information *chip_info;

    	/* For suspend/resume */
	video_via_regs saved_video_regs;
	hqv_via_regs saved_hqv0_regs;
	hqv_via_regs saved_hqv1_regs;
	u8 saved_seq_regs[256];
	u8 saved_crt_regs[256];
	u8 saved_gr_regs[12];
	u8 saved_ar_regs[14];
	u32 saved_cur_regs[4];
	u32 saved_gamma_table[256];
};

extern int viafb_SAMM_ON;
extern int viafb_dual_fb;
extern int viafb_LCD2_ON;
extern int viafb_LCD_ON;
extern int viafb_DVI_ON;
extern int viafb_hotplug;

u8 viafb_gpio_i2c_read_lvds(struct lvds_setting_information
	*plvds_setting_info, struct lvds_chip_information
	*plvds_chip_info, u8 index);
void viafb_gpio_i2c_write_mask_lvds(struct lvds_setting_information
			      *plvds_setting_info, struct lvds_chip_information
			      *plvds_chip_info, struct IODATA io_data);
int via_fb_pci_probe(struct viafb_dev *vdev);
void via_fb_pci_remove(struct pci_dev *pdev);
int viafb_sync(struct fb_info *info);
/* Temporary */
int viafb_init(void);
void viafb_exit(void);
#endif /* __VIAFBDEV_H__ */
