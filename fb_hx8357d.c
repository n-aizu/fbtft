/*
 * FB driver for the HX8357D LCD Controller
 *
 * Copyright (C) 2013 Christian Vogelgsang
 *
 * Based on driver code found here: https://github.com/watterott/r61505u-Adapter
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>

#include "fbtft.h"

#define DRVNAME		"fb_hx8357d"
#define WIDTH		320
#define HEIGHT		480
#define DEFAULT_GAMMA	"0 0 0 0 0 0 0 0 0 0 0 0 0 0\n" \
			"0 0 0 0 0 0 0 0 0 0 0 0 0 0"

#define HX8357D 0xD
#define HX8357B 0xB

#define HX8357_TFTWIDTH  320
#define HX8357_TFTHEIGHT 480

#define HX8357B_NOP     0x00
#define HX8357B_SWRESET 0x01
#define HX8357B_RDDID   0x04
#define HX8357B_RDDST   0x09

#define HX8357B_RDPOWMODE  0x0A
#define HX8357B_RDMADCTL  0x0B
#define HX8357B_RDCOLMOD  0x0C
#define HX8357B_RDDIM  0x0D
#define HX8357B_RDDSDR  0x0F

#define HX8357_SLPIN   0x10
#define HX8357_SLPOUT  0x11
#define HX8357B_PTLON   0x12
#define HX8357B_NORON   0x13

#define HX8357_INVOFF  0x20
#define HX8357_INVON   0x21
#define HX8357_DISPOFF 0x28
#define HX8357_DISPON  0x29

#define HX8357_CASET   0x2A
#define HX8357_PASET   0x2B
#define HX8357_RAMWR   0x2C
#define HX8357_RAMRD   0x2E

#define HX8357B_PTLAR   0x30
#define HX8357_TEON  0x35
#define HX8357_TEARLINE  0x44
#define HX8357_MADCTL  0x36
#define HX8357_COLMOD  0x3A

#define HX8357_SETOSC 0xB0
#define HX8357_SETPWR1 0xB1
#define HX8357B_SETDISPLAY 0xB2
#define HX8357_SETRGB 0xB3
#define HX8357D_SETCOM  0xB6

#define HX8357B_SETDISPMODE  0xB4
#define HX8357D_SETCYC  0xB4
#define HX8357B_SETOTP 0xB7
#define HX8357D_SETC 0xB9

#define HX8357B_SET_PANEL_DRIVING 0xC0
#define HX8357D_SETSTBA 0xC0
#define HX8357B_SETDGC  0xC1
#define HX8357B_SETID  0xC3
#define HX8357B_SETDDB  0xC4
#define HX8357B_SETDISPLAYFRAME 0xC5
#define HX8357B_GAMMASET 0xC8
#define HX8357B_SETCABC  0xC9
#define HX8357_SETPANEL  0xCC

#define HX8357B_SETPOWER 0xD0
#define HX8357B_SETVCOM 0xD1
#define HX8357B_SETPWRNORMAL 0xD2

#define HX8357B_RDID1   0xDA
#define HX8357B_RDID2   0xDB
#define HX8357B_RDID3   0xDC
#define HX8357B_RDID4   0xDD

#define HX8357D_SETGAMMA 0xE0

#define HX8357B_SETGAMMA 0xC8
#define HX8357B_SETPANELRELATED  0xE9

// Color definitions
#define	HX8357_BLACK   0x0000
#define	HX8357_BLUE    0x001F
#define	HX8357_RED     0xF800
#define	HX8357_GREEN   0x07E0
#define HX8357_CYAN    0x07FF
#define HX8357_MAGENTA 0xF81F
#define HX8357_YELLOW  0xFFE0  
#define HX8357_WHITE   0xFFFF


static int init_display(struct fbtft_par *par)
{
	fbtft_par_dbg(DEBUG_INIT_DISPLAY, par, "%s()\n", __func__);

	par->fbtftops.reset(par);

	/* Reset things like Gamma */
	write_reg(par, HX8357B_SWRESET);

	/* setextc */
	write_reg(par, HX8357D_SETC, 0xFF, 0x83, 0x57);
	mdelay(300);

	/* setRGB which also enables SDO */
	write_reg(par, HX8357_SETRGB, 0x00, 0x00, 0x06, 0x06);

	/* -1.52V */
	write_reg(par, HX8357D_SETCOM, 0x25);

	/* Normal mode 70Hz, Idle mode 55 Hz */
	write_reg(par, HX8357_SETOSC, 0x68);

	/* Set Panel - BGR, Gate direction swapped */
	write_reg(par, HX8357_SETPANEL, 0x05);

	write_reg(par, HX8357_SETPWR1,
		0x00,  // Not deep standby
		0x15,  //BT
		0x1C,  //VSPR
		0x1C,  //VSNR
		0x83,  //AP
		0xAA);  //FS

	write_reg(par, HX8357D_SETSTBA,
		0x50,  //OPON normal
		0x50,  //OPON idle
		0x01,  //STBA
		0x3C,  //STBA
		0x1E,  //STBA
		0x08);  //GEN

	write_reg(par, HX8357D_SETCYC,
		0x02,  //NW 0x02
		0x40,  //RTN
		0x00,  //DIV
		0x2A,  //DUM
		0x2A,  //DUM
		0x0D,  //GDON
		0x78);  //GDOFF

	write_reg(par, HX8357D_SETGAMMA,
		0x02,
		0x0A,
		0x11,
		0x1d,
		0x23,
		0x35,
		0x41,
		0x4b,
		0x4b,
		0x42,
		0x3A,
		0x27,
		0x1B,
		0x08,
		0x09,
		0x03,
		0x02,
		0x0A,
		0x11,
		0x1d,
		0x23,
		0x35,
		0x41,
		0x4b,
		0x4b,
		0x42,
		0x3A,
		0x27,
		0x1B,
		0x08,
		0x09,
		0x03,
		0x00,
		0x01);

	/* 16 bit */
	write_reg(par, HX8357_COLMOD, 0x55);

	write_reg(par, HX8357_MADCTL, 0xC0);

	/* TE off */
	write_reg(par, HX8357_TEON, 0x00);

	/* tear line */
	write_reg(par, HX8357_TEARLINE, 0x00, 0x02);

	/* Exit Sleep */
	write_reg(par, HX8357_SLPOUT);
	mdelay(150);

	/* display on */
	write_reg(par, HX8357_DISPON);
	mdelay(50);

	return 0;
}

static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
	fbtft_par_dbg(DEBUG_SET_ADDR_WIN, par,
		"%s(xs=%d, ys=%d, xe=%d, ye=%d)\n", __func__, xs, ys, xe, ye);

	/* Column addr set */
	write_reg(par, HX8357_CASET,
		xs >> 8, xs & 0xff,  /* XSTART */
		xe >> 8, xe & 0xff); /* XEND */

	/* Row addr set */
	write_reg(par, HX8357_PASET,
		ys >> 8, ys & 0xff,  /* YSTART */
		ye >> 8, ye & 0xff); /* YEND */

	/* write to RAM */
	write_reg(par, HX8357_RAMWR);
}

#define HX8357D_MADCTL_MY  0x80
#define HX8357D_MADCTL_MX  0x40
#define HX8357D_MADCTL_MV  0x20
#define HX8357D_MADCTL_ML  0x10
#define HX8357D_MADCTL_RGB 0x00
#define HX8357D_MADCTL_BGR 0x08
#define HX8357D_MADCTL_MH  0x04
static int set_var(struct fbtft_par *par)
{
	u8 val;

	fbtft_par_dbg(DEBUG_INIT_DISPLAY, par, "%s()\n", __func__);

	switch (par->info->var.rotate) {
	case 270:
		val = HX8357D_MADCTL_MV | HX8357D_MADCTL_MX;
		break;
	case 180:
		val = 0;
		break;
	case 90:
		val = HX8357D_MADCTL_MV | HX8357D_MADCTL_MY;
		break;
	default:
		val = HX8357D_MADCTL_MX | HX8357D_MADCTL_MY;
		break;
	}

	val |= (par->bgr ? HX8357D_MADCTL_RGB : HX8357D_MADCTL_BGR);

	/* Memory Access Control */
	write_reg(par, HX8357_MADCTL, val);

	return 0;
}

static struct fbtft_display display = {
	.regwidth = 8,
	.width = WIDTH,
	.height = HEIGHT,
	.gamma_num = 2,
	.gamma_len = 14,
	.gamma = DEFAULT_GAMMA,
	.fbtftops = {
		.init_display = init_display,
		.set_addr_win = set_addr_win,
		.set_var = set_var,
	},
};

FBTFT_REGISTER_DRIVER(DRVNAME, "himax,hx8357d", &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:hx8357d");
MODULE_ALIAS("platform:hx8357d");

MODULE_DESCRIPTION("FB driver for the HX8357D LCD Controller");
MODULE_AUTHOR("Sean Cross <xobs@kosagi.com>");
MODULE_LICENSE("GPL");
