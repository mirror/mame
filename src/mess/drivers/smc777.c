// license:MAME
// copyright-holders:Angelo Salese
/*************************************************************************************

    SMC-777 (c) 1983 Sony

    driver by Angelo Salese

    TODO:
    - no real documentation, the entire driver is just a bunch of educated
      guesses ...
    - ROM/RAM bankswitch, it apparently happens after one instruction prefetching.
      We currently use an hackish implementation until the MAME/MESS framework can
      support that ...
    - keyboard input is very sluggish;
    - cursor stuck in Bird Crash;
    - add mc6845 features;
    - many other missing features;

**************************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/sn76496.h"
#include "sound/beep.h"
#include "video/mc6845.h"

#include "machine/wd17xx.h"
#include "formats/basicdsk.h"
#include "imagedev/flopdrv.h"

#define mc6845_h_char_total     (m_crtc_vreg[0]+1)
#define mc6845_h_display        (m_crtc_vreg[1])
#define mc6845_h_sync_pos       (m_crtc_vreg[2])
#define mc6845_sync_width       (m_crtc_vreg[3])
#define mc6845_v_char_total     (m_crtc_vreg[4]+1)
#define mc6845_v_total_adj      (m_crtc_vreg[5])
#define mc6845_v_display        (m_crtc_vreg[6])
#define mc6845_v_sync_pos       (m_crtc_vreg[7])
#define mc6845_mode_ctrl        (m_crtc_vreg[8])
#define mc6845_tile_height      (m_crtc_vreg[9]+1)
#define mc6845_cursor_y_start   (m_crtc_vreg[0x0a])
#define mc6845_cursor_y_end     (m_crtc_vreg[0x0b])
#define mc6845_start_addr       (((m_crtc_vreg[0x0c]<<8) & 0x3f00) | (m_crtc_vreg[0x0d] & 0xff))
#define mc6845_cursor_addr      (((m_crtc_vreg[0x0e]<<8) & 0x3f00) | (m_crtc_vreg[0x0f] & 0xff))
#define mc6845_light_pen_addr   (((m_crtc_vreg[0x10]<<8) & 0x3f00) | (m_crtc_vreg[0x11] & 0xff))
#define mc6845_update_addr      (((m_crtc_vreg[0x12]<<8) & 0x3f00) | (m_crtc_vreg[0x13] & 0xff))

class smc777_state : public driver_device
{
public:
	smc777_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_crtc(*this, "crtc"),
	m_fdc(*this, "fdc"),
	m_sn(*this, "sn1"),
	m_beeper(*this, "beeper"),
	m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<mc6845_device> m_crtc;
	required_device<mb8876_device> m_fdc;
	optional_device<sn76489a_device> m_sn;
	required_device<beep_device> m_beeper;

	UINT8 *m_ipl_rom;
	UINT8 *m_work_ram;
	UINT8 *m_vram;
	UINT8 *m_attr;
	UINT8 *m_gvram;
	UINT8 *m_pcg;

	UINT8 m_keyb_press;
	UINT8 m_keyb_press_flag;
	UINT8 m_shift_press_flag;
	UINT8 m_backdrop_pen;
	UINT8 m_display_reg;
	UINT8 m_fdc_irq_flag;
	UINT8 m_fdc_drq_flag;
	UINT8 m_system_data;
	struct { UINT8 r,g,b; } m_pal;
	UINT8 m_raminh,m_raminh_pending_change; //bankswitch
	UINT8 m_raminh_prefetch;
	UINT8 m_irq_mask;
	UINT8 m_keyb_direct;
	UINT8 m_pal_mode;
	UINT8 m_keyb_cmd;
	UINT8 m_crtc_vreg[0x20];
	UINT8 m_crtc_addr;
	DECLARE_WRITE8_MEMBER(smc777_6845_w);
	DECLARE_READ8_MEMBER(smc777_vram_r);
	DECLARE_READ8_MEMBER(smc777_attr_r);
	DECLARE_READ8_MEMBER(smc777_pcg_r);
	DECLARE_WRITE8_MEMBER(smc777_vram_w);
	DECLARE_WRITE8_MEMBER(smc777_attr_w);
	DECLARE_WRITE8_MEMBER(smc777_pcg_w);
	DECLARE_READ8_MEMBER(smc777_fbuf_r);
	DECLARE_WRITE8_MEMBER(smc777_fbuf_w);
	DECLARE_READ8_MEMBER(smc777_fdc1_r);
	DECLARE_WRITE8_MEMBER(smc777_fdc1_w);
	DECLARE_READ8_MEMBER(key_r);
	DECLARE_WRITE8_MEMBER(key_w);
	DECLARE_WRITE8_MEMBER(border_col_w);
	DECLARE_READ8_MEMBER(system_input_r);
	DECLARE_WRITE8_MEMBER(system_output_w);
	DECLARE_READ8_MEMBER(smc777_joystick_r);
	DECLARE_WRITE8_MEMBER(smc777_color_mode_w);
	DECLARE_WRITE8_MEMBER(smc777_ramdac_w);
	DECLARE_READ8_MEMBER(display_reg_r);
	DECLARE_WRITE8_MEMBER(display_reg_w);
	DECLARE_READ8_MEMBER(smc777_mem_r);
	DECLARE_WRITE8_MEMBER(smc777_mem_w);
	DECLARE_READ8_MEMBER(smc777_irq_mask_r);
	DECLARE_WRITE8_MEMBER(smc777_irq_mask_w);
	DECLARE_READ8_MEMBER(smc777_io_r);
	DECLARE_WRITE8_MEMBER(smc777_io_w);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	DECLARE_PALETTE_INIT(smc777);
	UINT32 screen_update_smc777(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(smc777_vblank_irq);
	TIMER_DEVICE_CALLBACK_MEMBER(keyboard_callback);
	DECLARE_WRITE_LINE_MEMBER(smc777_fdc_intrq_w);
	DECLARE_WRITE_LINE_MEMBER(smc777_fdc_drq_w);
	void check_floppy_inserted();
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;
};


/* TODO: correct calculation thru mc6845 regs */
#define CRTC_MIN_X 24*8
#define CRTC_MIN_Y 4*8

void smc777_state::video_start()
{
}

UINT32 smc777_state::screen_update_smc777(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int x,y,yi;
	UINT16 count;
	int x_width;

//  popmessage("%d %d %d %d",mc6845_v_char_total,mc6845_v_total_adj,mc6845_v_display,mc6845_v_sync_pos);

	bitmap.fill(m_palette->pen(m_backdrop_pen), cliprect);

	x_width = ((m_display_reg & 0x80) >> 7);

	count = 0x0000;

	for(yi=0;yi<8;yi++)
	{
		for(y=0;y<200;y+=8)
		{
			for(x=0;x<160;x++)
			{
				UINT16 color;

				color = (m_gvram[count] & 0xf0) >> 4;
				/* todo: clean this up! */
				//if(x_width)
				{
					bitmap.pix16(y+yi+CRTC_MIN_Y, x*4+0+CRTC_MIN_X) = m_palette->pen(color);
					bitmap.pix16(y+yi+CRTC_MIN_Y, x*4+1+CRTC_MIN_X) = m_palette->pen(color);
				}
				//else
				//{
				//  bitmap.pix16(y+yi+CRTC_MIN_Y, x*2+0+CRTC_MIN_X) = m_palette->pen(color);
				//}

				color = (m_gvram[count] & 0x0f) >> 0;
				//if(x_width)
				{
					bitmap.pix16(y+yi+CRTC_MIN_Y, x*4+2+CRTC_MIN_X) = m_palette->pen(color);
					bitmap.pix16(y+yi+CRTC_MIN_Y, x*4+3+CRTC_MIN_X) = m_palette->pen(color);
				}
				//else
				//{
				//  bitmap.pix16(y+yi+CRTC_MIN_Y, x*2+1+CRTC_MIN_X) = m_palette->pen(color);
				//}

				count++;

			}
		}
		count+= 0x60;
	}

	count = 0x0000;

	for(y=0;y<25;y++)
	{
		for(x=0;x<80/(x_width+1);x++)
		{
			/*
			-x-- ---- blink
			--xx x--- bg color (00 transparent, 01 white, 10 black, 11 complementary to fg color
			---- -xxx fg color
			*/
			int tile = m_vram[count];
			int color = m_attr[count] & 7;
			int bk_color = (m_attr[count] & 0x18) >> 3;
			int blink = m_attr[count] & 0x40;
			int xi;
			int bk_pen;
			//int bk_struct[4] = { -1, 0x10, 0x11, (color & 7) ^ 8 };

			bk_pen = -1;
			switch(bk_color & 3)
			{
				case 0: bk_pen = -1; break; //transparent
				case 1: bk_pen = 0x17; break; //white
				case 2: bk_pen = 0x10; break; //black
				case 3: bk_pen = (color ^ 0xf); break; //complementary
			}

			if(blink && machine().first_screen()->frame_number() & 0x10) //blinking, used by Dragon's Alphabet
				color = bk_pen;

			for(yi=0;yi<8;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int pen;

					pen = ((m_pcg[tile*8+yi]>>(7-xi)) & 1) ? (color+m_pal_mode) : bk_pen;

					if (pen != -1)
					{
						if(x_width)
						{
							bitmap.pix16(y*8+CRTC_MIN_Y+yi, (x*8+xi)*2+0+CRTC_MIN_X) = m_palette->pen(pen);
							bitmap.pix16(y*8+CRTC_MIN_Y+yi, (x*8+xi)*2+1+CRTC_MIN_X) = m_palette->pen(pen);
						}
						else
							bitmap.pix16(y*8+CRTC_MIN_Y+yi, x*8+CRTC_MIN_X+xi) = m_palette->pen(pen);
					}
				}
			}

			// draw cursor
			if(mc6845_cursor_addr == count)
			{
				int xc,yc,cursor_on;

				cursor_on = 0;
				switch(mc6845_cursor_y_start & 0x60)
				{
					case 0x00: cursor_on = 1; break; //always on
					case 0x20: cursor_on = 0; break; //always off
					case 0x40: if(machine().first_screen()->frame_number() & 0x10) { cursor_on = 1; } break; //fast blink
					case 0x60: if(machine().first_screen()->frame_number() & 0x20) { cursor_on = 1; } break; //slow blink
				}

				if(cursor_on)
				{
					for(yc=0;yc<(8-(mc6845_cursor_y_start & 7));yc++)
					{
						for(xc=0;xc<8;xc++)
						{
							if(x_width)
							{
								bitmap.pix16(y*8+CRTC_MIN_Y-yc+7, (x*8+xc)*2+0+CRTC_MIN_X) = m_palette->pen(0x7);
								bitmap.pix16(y*8+CRTC_MIN_Y-yc+7, (x*8+xc)*2+1+CRTC_MIN_X) = m_palette->pen(0x7);
							}
							else
								bitmap.pix16(y*8+CRTC_MIN_Y-yc+7, x*8+CRTC_MIN_X+xc) = m_palette->pen(0x7);
						}
					}
				}
			}

			(m_display_reg & 0x80) ? count+=2 : count++;
		}
	}

	return 0;
}

WRITE8_MEMBER(smc777_state::smc777_6845_w)
{
	if(offset == 0)
	{
		m_crtc_addr = data;
		m_crtc->address_w(space, 0,data);
	}
	else
	{
		m_crtc_vreg[m_crtc_addr] = data;
		m_crtc->register_w(space, 0,data);
	}
}

READ8_MEMBER(smc777_state::smc777_vram_r)
{
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	return m_vram[vram_index];
}

READ8_MEMBER(smc777_state::smc777_attr_r)
{
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	return m_attr[vram_index];
}

READ8_MEMBER(smc777_state::smc777_pcg_r)
{
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	return m_pcg[vram_index];
}

WRITE8_MEMBER(smc777_state::smc777_vram_w)
{
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	m_vram[vram_index] = data;
}

WRITE8_MEMBER(smc777_state::smc777_attr_w)
{
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	m_attr[vram_index] = data;
}

WRITE8_MEMBER(smc777_state::smc777_pcg_w)
{
	UINT16 vram_index;

	vram_index  = ((offset & 0x0007) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	m_pcg[vram_index] = data;

	m_gfxdecode->gfx(0)->mark_dirty(vram_index >> 3);
}

READ8_MEMBER(smc777_state::smc777_fbuf_r)
{
	UINT16 vram_index;

	vram_index  = ((offset & 0x007f) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	return m_gvram[vram_index];
}

WRITE8_MEMBER(smc777_state::smc777_fbuf_w)
{
	UINT16 vram_index;

	vram_index  = ((offset & 0x00ff) << 8);
	vram_index |= ((offset & 0xff00) >> 8);

	m_gvram[vram_index] = data;
}


void smc777_state::check_floppy_inserted()
{
	int f_num;
	floppy_image_legacy *floppy;

	/* check if a floppy is there, automatically disconnect the ready line if so (HW doesn't control the ready line) */
	/* FIXME: floppy drive 1 doesn't work? */
	for(f_num=0;f_num<2;f_num++)
	{
		floppy = floppy_get_device(machine(), f_num)->flopimg_get_image();
		floppy_get_device(machine(), f_num)->floppy_mon_w((floppy != NULL) ? 0 : 1);
		floppy_get_device(machine(), f_num)->floppy_drive_set_ready_state((floppy != NULL) ? 1 : 0,0);
	}
}

READ8_MEMBER(smc777_state::smc777_fdc1_r)
{
	check_floppy_inserted();

	switch(offset)
	{
		case 0x00:
			return m_fdc->status_r(space, offset) ^ 0xff;
		case 0x01:
			return m_fdc->track_r(space, offset) ^ 0xff;
		case 0x02:
			return m_fdc->sector_r(space, offset) ^ 0xff;
		case 0x03:
			return m_fdc->data_r(space, offset) ^ 0xff;
		case 0x04: //irq / drq status
			//popmessage("%02x %02x\n",m_fdc_irq_flag,m_fdc_drq_flag);

			return (m_fdc_irq_flag ? 0x80 : 0x00) | (m_fdc_drq_flag ? 0x00 : 0x40);
	}

	return 0x00;
}

WRITE8_MEMBER(smc777_state::smc777_fdc1_w)
{
	check_floppy_inserted();

	switch(offset)
	{
		case 0x00:
			m_fdc->command_w(space, offset,data ^ 0xff);
			break;
		case 0x01:
			m_fdc->track_w(space, offset,data ^ 0xff);
			break;
		case 0x02:
			m_fdc->sector_w(space, offset,data ^ 0xff);
			break;
		case 0x03:
			m_fdc->data_w(space, offset,data ^ 0xff);
			break;
		case 0x04:
			// ---- xxxx select floppy drive (yes, 15 of them, A to P)
			m_fdc->set_drive(data & 0x01);
			//  m_fdc->set_side((data & 0x10)>>4);
			if(data & 0xf0)
				printf("floppy access %02x\n",data);
			break;
	}
}

WRITE_LINE_MEMBER(smc777_state::smc777_fdc_intrq_w)
{
	m_fdc_irq_flag = state;
}

WRITE_LINE_MEMBER(smc777_state::smc777_fdc_drq_w)
{
	m_fdc_drq_flag = state;
}

READ8_MEMBER(smc777_state::key_r)
{
	/*
	-x-- ---- shift key
	---- -x-- MCU data ready
	---- ---x handshake bit?
	*/

	switch(m_keyb_cmd)
	{
		case 0x00: //poll keyboard input
		{
			if(offset == 0)
				m_keyb_press_flag = 0;

			return (offset == 0) ? m_keyb_press : ((m_shift_press_flag << 6) | (m_keyb_press_flag << 2) | (m_keyb_press_flag));
		}
		default:
		{
			//if(offset == 1)
			//  printf("Unknown keyboard command %02x read-back\n",m_keyb_cmd);

			return (offset == 0) ? 0x00 : (machine().rand() & 0x5);
		}
	}

	// never executed
	//return 0x00;
}

/* TODO: the packet commands strikes me as something I've already seen before, don't remember where however ... */
WRITE8_MEMBER(smc777_state::key_w)
{
	if(offset == 1) //keyboard command
		m_keyb_cmd = data;
	else
	{
		// keyboard command param
	}
}

WRITE8_MEMBER(smc777_state::border_col_w)
{
	if(data & 0xf0)
		printf("Special border color enabled %02x\n",data);

	m_backdrop_pen = data & 0xf;
}


READ8_MEMBER(smc777_state::system_input_r)
{
	printf("System FF R %02x\n",m_system_data & 0x0f);

	switch(m_system_data & 0x0f)
	{
		case 0x00:
			return ((m_raminh & 1) << 4); //unknown bit, Dragon's Alphabet and Bird Crush relies on this for correct colors
	}

	return m_system_data;
}



WRITE8_MEMBER(smc777_state::system_output_w)
{
	/*
	---x 0000 ram inibit signal
	---x 1001 beep
	all the rest is unknown at current time
	*/
	m_system_data = data;
	switch(m_system_data & 0x0f)
	{
		case 0x00:
			m_raminh_pending_change = ((data & 0x10) >> 4) ^ 1;
			m_raminh_prefetch = (UINT8)(space.device().state().state_int(Z80_R)) & 0x7f;
			break;
		case 0x02: printf("Interlace %s\n",data & 0x10 ? "on" : "off"); break;
		case 0x05: m_beeper->set_state(data & 0x10); break;
		default: printf("System FF W %02x\n",data); break;
	}
}

/* presumably SMC-777 specific */
READ8_MEMBER(smc777_state::smc777_joystick_r)
{
	return ioport("JOY_1P")->read();
}

WRITE8_MEMBER(smc777_state::smc777_color_mode_w)
{
	switch(data & 0x0f)
	{
		case 0x06: m_pal_mode = (data & 0x10) ^ 0x10; break;
		default: printf("Color FF %02x\n",data); break;
	}
}

WRITE8_MEMBER(smc777_state::smc777_ramdac_w)
{
	UINT8 pal_index;
	pal_index = (offset & 0xf00) >> 8;

	if(data & 0x0f)
		printf("RAMdac used with data bits 0-3 set (%02x)\n",data);

	switch((offset & 0x3000) >> 12)
	{
		case 0: m_pal.r = (data & 0xf0) >> 4; m_palette->set_pen_color(pal_index, pal4bit(m_pal.r), pal4bit(m_pal.g), pal4bit(m_pal.b)); break;
		case 1: m_pal.g = (data & 0xf0) >> 4; m_palette->set_pen_color(pal_index, pal4bit(m_pal.r), pal4bit(m_pal.g), pal4bit(m_pal.b)); break;
		case 2: m_pal.b = (data & 0xf0) >> 4; m_palette->set_pen_color(pal_index, pal4bit(m_pal.r), pal4bit(m_pal.g), pal4bit(m_pal.b)); break;
		case 3: printf("RAMdac used with gradient index = 3! pal_index = %02x data = %02x\n",pal_index,data); break;
	}
}

READ8_MEMBER(smc777_state::display_reg_r)
{
	return m_display_reg;
}

/* x */
WRITE8_MEMBER(smc777_state::display_reg_w)
{
	/*
	x--- ---- width 80 / 40 switch (0 = 640 x 200 1 = 320 x 200)
	---- -x-- mode select?
	*/

	m_display_reg = data;
}

READ8_MEMBER(smc777_state::smc777_mem_r)
{
	UINT8 z80_r;

	if(m_raminh_prefetch != 0xff) //do the bankswitch AFTER that the prefetch instruction is executed (FIXME: this is an hackish implementation)
	{
		z80_r = (UINT8)space.device().state().state_int(Z80_R);

		if(z80_r == ((m_raminh_prefetch+2) & 0x7f))
		{
			m_raminh = m_raminh_pending_change;
			m_raminh_prefetch = 0xff;
		}
	}

	if(m_raminh == 1 && ((offset & 0xc000) == 0))
		return m_ipl_rom[offset];

	return m_work_ram[offset];
}

WRITE8_MEMBER(smc777_state::smc777_mem_w)
{
	m_work_ram[offset] = data;
}

READ8_MEMBER(smc777_state::smc777_irq_mask_r)
{
	return m_irq_mask;
}

WRITE8_MEMBER(smc777_state::smc777_irq_mask_w)
{
	if(data & 0xfe)
		printf("Irq mask = %02x\n",data & 0xfe);

	m_irq_mask = data & 1;
}

static ADDRESS_MAP_START(smc777_mem, AS_PROGRAM, 8, smc777_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(smc777_mem_r,smc777_mem_w)
ADDRESS_MAP_END

READ8_MEMBER(smc777_state::smc777_io_r)
{
	UINT8 low_offs;

	low_offs = offset & 0xff;

	if(low_offs <= 0x07)                          { return smc777_vram_r(space,offset & 0xff07); }
	else if(low_offs >= 0x08 && low_offs <= 0x0f) { return smc777_attr_r(space,offset & 0xff07); }
	else if(low_offs >= 0x10 && low_offs <= 0x17) { return smc777_pcg_r(space,offset & 0xff07); }
	else if(low_offs >= 0x18 && low_offs <= 0x19) { logerror("6845 read %02x",low_offs & 1); }
	else if(low_offs >= 0x1a && low_offs <= 0x1b) { return key_r(space,low_offs & 1); }
	else if(low_offs == 0x1c)                     { return system_input_r(space,0); }
	else if(low_offs == 0x1d)                     { logerror("System and control data R PC=%04x\n",space.device().safe_pc()); return 0xff; }
	else if(low_offs == 0x20)                     { return display_reg_r(space,0); }
	else if(low_offs == 0x21)                     { return smc777_irq_mask_r(space,0); }
	else if(low_offs == 0x25)                     { logerror("RTC read PC=%04x\n",space.device().safe_pc()); return 0xff; }
	else if(low_offs == 0x26)                     { logerror("RS-232c RX %04x\n",space.device().safe_pc()); return 0xff; }
	else if(low_offs >= 0x28 && low_offs <= 0x2c) { logerror("FDC 2 read %02x\n",low_offs & 7); return 0xff; }
	else if(low_offs >= 0x2d && low_offs <= 0x2f) { logerror("RS-232c no. 2 read %02x\n",low_offs & 3); return 0xff; }
	else if(low_offs >= 0x30 && low_offs <= 0x34) { return smc777_fdc1_r(space,low_offs & 7); }
	else if(low_offs >= 0x35 && low_offs <= 0x37) { logerror("RS-232c no. 3 read %02x\n",low_offs & 3); return 0xff; }
	else if(low_offs >= 0x38 && low_offs <= 0x3b) { logerror("Cache disk unit read %02x\n",low_offs & 7); return 0xff; }
	else if(low_offs >= 0x3c && low_offs <= 0x3d) { logerror("RGB superimposer read %02x\n",low_offs & 1); return 0xff; }
	else if(low_offs >= 0x40 && low_offs <= 0x47) { logerror("IEEE-488 interface unit read %02x\n",low_offs & 7); return 0xff; }
	else if(low_offs >= 0x48 && low_offs <= 0x4f) { logerror("HDD (Winchester) read %02x\n",low_offs & 1); return 0xff; } //might be 0x48 - 0x50
	else if(low_offs == 0x51)                     { return smc777_joystick_r(space,0); }
	else if(low_offs >= 0x54 && low_offs <= 0x59) { logerror("VTR Controller read %02x\n",low_offs & 7); return 0xff; }
	else if(low_offs == 0x5a || low_offs == 0x5b) { logerror("RAM Banking %02x\n",low_offs & 1); }
	else if(low_offs == 0x70)                     { logerror("Auto-start ROM read\n"); }
	else if(low_offs == 0x74)                     { logerror("IEEE-488 ROM read\n"); }
	else if(low_offs == 0x75)                     { logerror("VTR Controller ROM read\n"); }
	else if(low_offs == 0x7e || low_offs == 0x7f) { logerror("Kanji ROM read %02x\n",low_offs & 1); }
	else if(low_offs >= 0x80)                     { return smc777_fbuf_r(space,offset & 0xff7f); }

	logerror("Undefined read at %04x offset = %02x\n",space.device().safe_pc(),low_offs);
	return 0xff;
}

WRITE8_MEMBER(smc777_state::smc777_io_w)
{
	UINT8 low_offs;

	low_offs = offset & 0xff;

	if(low_offs <= 0x07)                          { smc777_vram_w(space,offset & 0xff07,data); }
	else if(low_offs >= 0x08 && low_offs <= 0x0f) { smc777_attr_w(space,offset & 0xff07,data); }
	else if(low_offs >= 0x10 && low_offs <= 0x17) { smc777_pcg_w(space,offset & 0xff07,data); }
	else if(low_offs >= 0x18 && low_offs <= 0x19) { smc777_6845_w(space,low_offs & 1,data); }
	else if(low_offs == 0x1a || low_offs == 0x1b) { key_w(space,low_offs & 1,data); }
	else if(low_offs == 0x1c)                     { system_output_w(space,0,data); }
	else if(low_offs == 0x1d)                     { logerror("Printer status / strobe write %02x\n",data); }
	else if(low_offs == 0x1e || low_offs == 0x1f) { logerror("RS-232C irq control [%02x] %02x\n",low_offs & 1,data); }
	else if(low_offs == 0x20)                     { display_reg_w(space,0,data); }
	else if(low_offs == 0x21)                     { smc777_irq_mask_w(space,0,data); }
	else if(low_offs == 0x22)                     { logerror("Printer output data %02x\n",data); }
	else if(low_offs == 0x23)                     { border_col_w(space,0,data); }
	else if(low_offs == 0x24)                     { logerror("RTC write / specify address %02x\n",data); }
	else if(low_offs == 0x26)                     { logerror("RS-232c TX %02x\n",data); }
	else if(low_offs >= 0x28 && low_offs <= 0x2c) { logerror("FDC 2 write %02x %02x\n",low_offs & 7,data); }
	else if(low_offs >= 0x2d && low_offs <= 0x2f) { logerror("RS-232c no. 2 write %02x %02x\n",low_offs & 3,data); }
	else if(low_offs >= 0x30 && low_offs <= 0x34) { smc777_fdc1_w(space,low_offs & 7,data); }
	else if(low_offs >= 0x35 && low_offs <= 0x37) { logerror("RS-232c no. 3 write %02x %02x\n",low_offs & 3,data); }
	else if(low_offs >= 0x38 && low_offs <= 0x3b) { logerror("Cache disk unit write %02x %02x\n",low_offs & 7,data); }
	else if(low_offs >= 0x3c && low_offs <= 0x3d) { logerror("RGB superimposer write %02x %02x\n",low_offs & 1,data); }
	else if(low_offs >= 0x40 && low_offs <= 0x47) { logerror("IEEE-488 interface unit write %02x %02x\n",low_offs & 7,data); }
	else if(low_offs >= 0x48 && low_offs <= 0x4f) { logerror("HDD (Winchester) write %02x %02x\n",low_offs & 1,data); } //might be 0x48 - 0x50
	else if(low_offs == 0x51)                     { smc777_color_mode_w(space,0,data); }
	else if(low_offs == 0x52)                     { smc777_ramdac_w(space,offset & 0xff00,data); }
	else if(low_offs == 0x53)                     { m_sn->write(space,0,data); }
	else if(low_offs >= 0x54 && low_offs <= 0x59) { logerror("VTR Controller write [%02x] %02x\n",low_offs & 7,data); }
	else if(low_offs == 0x5a || low_offs == 0x5b) { logerror("RAM Banking write [%02x] %02x\n",low_offs & 1,data); }
	else if(low_offs == 0x70)                     { logerror("Auto-start ROM write %02x\n",data); }
	else if(low_offs == 0x74)                     { logerror("IEEE-488 ROM write %02x\n",data); }
	else if(low_offs == 0x75)                     { logerror("VTR Controller ROM write %02x\n",data); }
	else if(low_offs == 0x7e || low_offs == 0x7f) { logerror("Kanji ROM write [%02x] %02x\n",low_offs & 1,data); }
	else if(low_offs >= 0x80)                     { smc777_fbuf_w(space,offset & 0xff7f,data); }
	else                                          { logerror("Undefined write at %04x offset = %02x data = %02x\n",space.device().safe_pc(),low_offs,data); }
}

static ADDRESS_MAP_START( smc777_io , AS_IO, 8, smc777_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(smc777_io_r,smc777_io_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( smc777 )
	PORT_START("key0") //0x00-0x07
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)      PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)      PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)      PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)      PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)      PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)      PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)      PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)      PORT_CHAR(UCHAR_MAMEKEY(7_PAD))

	PORT_START("key1") //0x08-0x0f
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)      PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT (0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)      PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ PAD") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("* PAD") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- PAD") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("+ PAD") PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ENTER PAD") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)
	PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". PAD") PORT_CODE(KEYCODE_DEL_PAD)

	PORT_START("key2") //0x10-0x17
	PORT_BIT (0x01, IP_ACTIVE_HIGH,IPT_UNUSED) // PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@')
	PORT_BIT (0x02, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT (0x04, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT (0x08, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT (0x10, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT (0x20, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT (0x40, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT (0x80, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START("key3") //0x18-0x1f
	PORT_BIT (0x01, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT (0x02, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT (0x04, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT (0x08, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT (0x10, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT (0x20, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT (0x40, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT (0x80, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START("key4") //0x20-0x27
	PORT_BIT (0x01, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT (0x02, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT (0x04, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT (0x08, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT (0x10, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT (0x20, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT (0x40, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT (0x80, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("key5") //0x28-0x2f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0-1") /* TODO: labels */
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0-2")
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1-1")
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1-2")

	PORT_START("key6") //0x30-0x37
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT(0x80,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')

	PORT_START("key7") //0x38-0x3f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2-1") /* TODO: labels */
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("2-3")
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3-1")
	PORT_BIT(0x20,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3-2")
	PORT_BIT(0x40,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("3-3")

	PORT_START("key8") //0x40-0x47
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(27)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("ESC") PORT_CHAR(27)
	PORT_BIT(0x20, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("INS") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x40, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)
	/* TODO: control inputs */

	PORT_START("key9") //0x40-0x47
	/* TODO: cursor inputs */
	PORT_BIT(0x01, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CURSOR UP") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x02, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CURSOR DOWN") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x04, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CURSOR LEFT") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x08, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("CURSOR RIGHT") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x10, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("HOME") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x20, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("END") PORT_CODE(KEYCODE_END)
	PORT_BIT(0x40, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PGUP") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x80, IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("PGDOWN") PORT_CODE(KEYCODE_PGDN)

	PORT_START("keya") //0x48-0x4f
	PORT_BIT(0x01,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x02,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x04,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x08,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x10,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)

	PORT_START("key_mod")
	PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KANA SHIFT") PORT_CODE(KEYCODE_LALT)


	#if 0
	PORT_BIT(0x00002000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')

	PORT_BIT(0x04000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(":") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':')
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';')
	PORT_BIT(0x08000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('[')
	PORT_BIT(0x20000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(']')
	PORT_BIT(0x40000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("^") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^')
	PORT_BIT(0x80000000,IP_ACTIVE_HIGH,IPT_KEYBOARD) PORT_NAME("_")
	#endif

	PORT_START("JOY_1P")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH,IPT_UNKNOWN ) //status?
INPUT_PORTS_END

/*
    Keyboard data:
    kana lock |= 0x80
    numpad 0 to 9 = 0x30 - 0x39
    / pad = 0x2f
    * pad = 0x2a
    - pad = 0x2d
    + pad = 0x2b
    ENTER pad = 0x0d
    . pad = 0x2e
    enter = 0x0d
    space = 0x20
    backspace = 0x08
    CTRL (TAB?) = 0x09
    cursor up = 0x17
    cursor down = 0x1c
    cursor left = 0x16
    cursor right = 0x19
    0 to 9 -> 0x30 to 0x39
    S + 0 = 0x29
    S + 1 = 0x21
    S + 2 = 0x40
    S + 3 = 0x23
    S + 4 = 0x24
    S + 5 = 0x25
    S + 6 = 0x5e
    S + 7 = 0x26
    S + 8 = 0x2a
    S + 9 = 0x28
    F1 = 0x01 / 0x15
    F2 = 0x02 / 0x18
    F3 = 0x04 / 0x12
    F4 = 0x06 / 0x05
    F5 = 0x0b / 0x03
    ESC = 0x1b
    INS = 0x0f
    DEL = 0x11
    PG UP = 0x12
    PG DOWN = 0x03
    HOME = 0x14
    END = 0x0e
    ' = 0x2d / 0x5f
    "P" row 1 ] = 0x5d / 0x7d
    "P" row 2 ' / ~ = 0x60 / 0x7e
    "L" row 1 ; / : = 0x3b / 0x3a
    "L" row 2 = (unused)
    "L" row 3 Yen / | = 0x5c / 0x7c
    "M" row 1 , / < = 0x2c / 0x3c
    "M" row 2 . / > = 0x2e / 0x3e
    "M" row 3 / / ? = 0x2f / 0x3f
*/

static const UINT8 smc777_keytable[2][0xa0] =
{
	/* normal*/
	{
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* numpad */
		0x38, 0x39, 0x2f, 0x2a, 0x2d, 0x2b, 0x0d, 0x2e,
		-1,   0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, /* A - G */
		0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, /* H - O */
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, /* P - W */
		0x78, 0x79, 0x7a, 0x2d, 0x5d, 0x60, -1, -1, /* X - Z */
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* 0 - 7*/
		0x38, 0x39, 0x3b, 0x5c, 0x2c, 0x2e, 0x2f, -1, /* 8 - 9 */
		0x0d, 0x20, 0x08, 0x09, 0x1b, 0x0f, 0x11, -1,
		0x17, 0x1c, 0x16, 0x19, 0x14, 0x0e, 0x12, 0x03,
		0x01, 0x02, 0x04, 0x06, 0x0b, -1, -1, -1,

	},
	/* shift */
	{
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, /* numpad */
		0x38, 0x39, 0x2f, 0x2a, 0x2d, 0x2b, 0x0d, 0x2e,
		-1,   0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
		0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, /* H - O */
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, /* P - W */
		0x58, 0x59, 0x5a, 0x5f, 0x7d, 0x7e,   -1,   -1, /* X - Z */
		0x29, 0x21, 0x40, 0x23, 0x24, 0x25, 0x5e, 0x26,
		0x2a, 0x28, 0x3a, 0x7c, 0x3c, 0x3e, 0x3f,   -1,
		0x0d, 0x20, 0x08, 0x09, 0x1b, 0x0f, 0x11,   -1,
		0x17, 0x1c, 0x16, 0x19, 0x14, 0x0e, 0x12, 0x03,
		0x15, 0x18, 0x12, 0x05, 0x03, -1, -1, -1, /* F1 - F5 */
	}
};

TIMER_DEVICE_CALLBACK_MEMBER(smc777_state::keyboard_callback)
{
	static const char *const portnames[11] = { "key0","key1","key2","key3","key4","key5","key6","key7", "key8", "key9", "keya" };
	int i,port_i,scancode;
	UINT8 shift_mod = ioport("key_mod")->read() & 1;
	UINT8 kana_mod = ioport("key_mod")->read() & 0x10;
	scancode = 0;

	for(port_i=0;port_i<11;port_i++)
	{
		for(i=0;i<8;i++)
		{
			if((ioport(portnames[port_i])->read()>>i) & 1)
			{
				m_keyb_press = smc777_keytable[shift_mod & 1][scancode];
				if(kana_mod) { m_keyb_press|=0x80; }
				m_keyb_press_flag = 1;
				m_shift_press_flag = shift_mod & 1;
				return;
			}
			scancode++;
		}
	}
}

static const gfx_layout smc777_charlayout =
{
	8, 8,
	0x800 / 8,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

void smc777_state::machine_start()
{
	m_ipl_rom = memregion("ipl")->base();
	m_work_ram = auto_alloc_array_clear(machine(), UINT8, 0x10000);
	m_vram = auto_alloc_array_clear(machine(), UINT8, 0x800);
	m_attr = auto_alloc_array_clear(machine(), UINT8, 0x800);
	m_gvram = auto_alloc_array_clear(machine(), UINT8, 0x8000);
	m_pcg = auto_alloc_array_clear(machine(), UINT8, 0x800);

	save_pointer(NAME(m_work_ram), 0x10000);
	save_pointer(NAME(m_vram), 0x800);
	save_pointer(NAME(m_attr), 0x800);
	save_pointer(NAME(m_gvram), 0x8000);
	save_pointer(NAME(m_pcg), 0x800);

	m_gfxdecode->set_gfx(0, global_alloc(gfx_element(m_palette, smc777_charlayout, (UINT8 *)m_pcg, 0, 8, 0)));
}

void smc777_state::machine_reset()
{
	m_raminh = 1;
	m_raminh_pending_change = 1;
	m_raminh_prefetch = 0xff;
	m_pal_mode = 0x10;

	m_beeper->set_frequency(300); //TODO: correct frequency
	m_beeper->set_state(0);
}


/* set-up SMC-70 mode colors */
PALETTE_INIT_MEMBER(smc777_state, smc777)
{
	int i;

	for(i=0x10;i<0x18;i++)
	{
		UINT8 r,g,b;

		r = (i & 4) >> 2;
		g = (i & 2) >> 1;
		b = (i & 1) >> 0;

		palette.set_pen_color(i, pal1bit(r),pal1bit(g),pal1bit(b));
		palette.set_pen_color(i+8, pal1bit(0),pal1bit(0),pal1bit(0));
	}
}

static LEGACY_FLOPPY_OPTIONS_START( smc777 )
	LEGACY_FLOPPY_OPTION( img, "img", "SMC70 disk image", basicdsk_identify_default, basicdsk_construct_default, NULL,
		HEADS([1])
		TRACKS([70])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
LEGACY_FLOPPY_OPTIONS_END

static const floppy_interface smc777_floppy_interface =
{
	FLOPPY_STANDARD_5_25_SSDD,
	LEGACY_FLOPPY_OPTIONS_NAME(smc777),
	"floppy_5_25"
};

INTERRUPT_GEN_MEMBER(smc777_state::smc777_vblank_irq)
{
	if(m_irq_mask)
		device.execute().set_input_line(0,HOLD_LINE);
}


#define MASTER_CLOCK XTAL_4_028MHz

static MACHINE_CONFIG_START( smc777, smc777_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, MASTER_CLOCK)
	MCFG_CPU_PROGRAM_MAP(smc777_mem)
	MCFG_CPU_IO_MAP(smc777_io)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", smc777_state, smc777_vblank_irq)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(0x400, 400)
	MCFG_SCREEN_VISIBLE_AREA(0, 660-1, 0, 220-1) //normal 640 x 200 + 20 pixels for border color
	MCFG_SCREEN_UPDATE_DRIVER(smc777_state, screen_update_smc777)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_PALETTE_ADD("palette", 0x20) // 16 + 8 colors (SMC-777 + SMC-70) + 8 empty entries (SMC-70)
	MCFG_PALETTE_INIT_OWNER(smc777_state, smc777)

	MCFG_GFXDECODE_ADD("gfxdecode", "palette", empty)

	MCFG_MC6845_ADD("crtc", H46505, "screen", MASTER_CLOCK/2)    /* unknown clock, hand tuned to get ~60 fps */
	MCFG_MC6845_SHOW_BORDER_AREA(true)
	MCFG_MC6845_CHAR_WIDTH(8)

	/* devices */
	MCFG_DEVICE_ADD("fdc", MB8876, 0)
	MCFG_WD17XX_DEFAULT_DRIVE2_TAGS
	MCFG_WD17XX_INTRQ_CALLBACK(WRITELINE(smc777_state, smc777_fdc_intrq_w))
	MCFG_WD17XX_DRQ_CALLBACK(WRITELINE(smc777_state, smc777_fdc_drq_w))

	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(smc777_floppy_interface)

	MCFG_SOFTWARE_LIST_ADD("flop_list","smc777")

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("sn1", SN76489A, MASTER_CLOCK) // unknown clock / divider
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_SOUND_ADD("beeper", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.50)

	MCFG_TIMER_DRIVER_ADD_PERIODIC("keyboard_timer", smc777_state, keyboard_callback, attotime::from_hz(240/32))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( smc777 )
	/* shadow ROM */
	ROM_REGION( 0x10000, "ipl", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "1st", "1st rev.")
	ROMX_LOAD( "smcrom.dat", 0x0000, 0x4000, CRC(b2520d31) SHA1(3c24b742c38bbaac85c0409652ba36e20f4687a1), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "2nd", "2nd rev.")
	ROMX_LOAD( "smcrom.v2",  0x0000, 0x4000, CRC(c1494b8f) SHA1(a7396f5c292f11639ffbf0b909e8473c5aa63518), ROM_BIOS(2))

	ROM_REGION( 0x800, "mcu", ROMREGION_ERASEFF )
	ROM_LOAD( "i80xx", 0x000, 0x800, NO_DUMP ) // keyboard mcu, needs decapping
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1983, smc777,  0,       0,    smc777,     smc777, driver_device,   0,  "Sony",   "SMC-777",       GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND)
