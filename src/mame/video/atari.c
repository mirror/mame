/******************************************************************************
    Atari 400/800

    Video handler

    Juergen Buchmueller, June 1998
******************************************************************************/

#include "emu.h"
#include "includes/atari.h"
#include "video/gtia.h"

#define VERBOSE 0

#define LOG(x)  do { if (VERBOSE) logerror x; } while (0)


/************************************************************************
 * atari_vh_start
 * Initialize the ATARI800 video emulation
 ************************************************************************/

void atari_common_state::video_start()
{
	palette_device *m_palette = machine().first_screen()->palette();

	m_antic_render1 = 0;
	m_antic_render2 = 0;
	m_antic_render3 = 0;

	for (int i = 0; i < 256; i++)
		m_gtia->set_color_lookup(i, (m_palette->pen(0) << 8) + m_palette->pen(0));
	
	antic_vstart(machine());
}

/************************************************************************
 * atari_vh_screenrefresh
 * Refresh screen bitmap.
 * Note: Actual drawing is done scanline wise during atari_interrupt
 ************************************************************************/
UINT32 atari_common_state::screen_update_atari(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	UINT32 new_tv_artifacts;

	copybitmap(bitmap, *antic.bitmap, 0, 0, 0, 0, cliprect);

	new_tv_artifacts = screen.ioport("artifacts")->read_safe(0);
	if( tv_artifacts != new_tv_artifacts )
	{
		tv_artifacts = new_tv_artifacts;
	}

	return 0;
}

void atari_common_state::artifacts_gfx(UINT8 *src, UINT8 *dst, int width)
{
	UINT8 n, bits = 0;
	UINT8 b = m_gtia->get_w_colbk() & 0xf0;
	UINT8 c = m_gtia->get_w_colpf1() & 0x0f;
	UINT8 atari_A = ((b + 0x30) & 0xf0) + c;
	UINT8 atari_B = ((b + 0x70) & 0xf0) + c;
	UINT8 atari_C = b + c;
	UINT8 atari_D = m_gtia->get_w_colbk();
	UINT16 *color_lookup = m_gtia->get_color_lookup();

	for (int x = 0; x < width * 4; x++)
	{
		n = *src++;
		bits <<= 2;
		switch( n )
		{
		case G00:
			break;
		case G01:
			bits |= 1;
			break;
		case G10:
			bits |= 2;
			break;
		case G11:
			bits |= 3;
			break;
		default:
			*dst++ = color_lookup[n];
			*dst++ = color_lookup[n];
			continue;
		}
		switch ((bits >> 1) & 7)
		{
		case 0: /* 0 0 0 */
		case 1: /* 0 0 1 */
		case 4: /* 1 0 0 */
			*dst++ = atari_D;
			break;
		case 3: /* 0 1 1 */
		case 6: /* 1 1 0 */
		case 7: /* 1 1 1 */
			*dst++ = atari_C;
			break;
		case 2: /* 0 1 0 */
			*dst++ = atari_B;
			break;
		case 5: /* 1 0 1 */
			*dst++ = atari_A;
			break;
		}
		switch (bits & 7)
		{
		case 0: /* 0 0 0 */
		case 1: /* 0 0 1 */
		case 4: /* 1 0 0 */
			*dst++ = atari_D;
			break;
		case 3: /* 0 1 1 */
		case 6: /* 1 1 0 */
		case 7: /* 1 1 1 */
			*dst++ = atari_C;
			break;
		case 2: /* 0 1 0 */
			*dst++ = atari_A;
			break;
		case 5: /* 1 0 1 */
			*dst++ = atari_B;
			break;
		}
	}
}

void atari_common_state::artifacts_txt(UINT8 * src, UINT8 * dst, int width)
{
	UINT8 n, bits = 0;
	UINT8 b = m_gtia->get_w_colpf2() & 0xf0;
	UINT8 c = m_gtia->get_w_colpf1() & 0x0f;
	UINT8 atari_A = ((b+0x30)&0xf0)+c;
	UINT8 atari_B = ((b+0x70)&0xf0)+c;
	UINT8 atari_C = b+c;
	UINT8 atari_D = m_gtia->get_w_colpf2();
	UINT16 *color_lookup = m_gtia->get_color_lookup();

	for (int x = 0; x < width * 4; x++)
	{
		n = *src++;
		bits <<= 2;
		switch( n )
		{
		case T00:
			break;
		case T01:
			bits |= 1;
			break;
		case T10:
			bits |= 2;
			break;
		case T11:
			bits |= 3;
			break;
		default:
			*dst++ = color_lookup[n];
			*dst++ = color_lookup[n];
			continue;
		}
		switch( (bits >> 1) & 7 )
		{
		case 0: /* 0 0 0 */
		case 1: /* 0 0 1 */
		case 4: /* 1 0 0 */
			*dst++ = atari_D;
			break;
		case 3: /* 0 1 1 */
		case 6: /* 1 1 0 */
		case 7: /* 1 1 1 */
			*dst++ = atari_C;
			break;
		case 2: /* 0 1 0 */
			*dst++ = atari_A;
			break;
		case 5: /* 1 0 1 */
			*dst++ = atari_B;
			break;
		}
		switch( bits & 7 )
		{
		case 0:/* 0 0 0 */
		case 1:/* 0 0 1 */
		case 4:/* 1 0 0 */
			*dst++ = atari_D;
			break;
		case 3: /* 0 1 1 */
		case 6: /* 1 1 0 */
		case 7: /* 1 1 1 */
			*dst++ = atari_C;
			break;
		case 2: /* 0 1 0 */
			*dst++ = atari_B;
			break;
		case 5: /* 1 0 1 */
			*dst++ = atari_A;
			break;
		}
	}
}


void atari_common_state::antic_linerefresh()
{
	int x, y;
	UINT8 *src;
	UINT32 *dst;
	UINT32 scanline[4 + (HCHARS * 2) + 4];
	UINT16 *color_lookup = m_gtia->get_color_lookup();

	/* increment the scanline */
	if( ++antic.scanline == machine().first_screen()->height() )
	{
		/* and return to the top if the frame was complete */
		antic.scanline = 0;
		antic.modelines = 0;
		/* count frames gone since last write to hitclr */
		m_gtia->count_hitclr_frames();
	}

	if( antic.scanline < MIN_Y || antic.scanline > MAX_Y )
		return;

	y = antic.scanline - MIN_Y;
	src = &antic.cclock[PMOFFSET - antic.hscrol_old + 12];
	dst = scanline;

	if( tv_artifacts )
	{
		if( (antic.cmd & 0x0f) == 2 || (antic.cmd & 0x0f) == 3 )
		{
			artifacts_txt(src, (UINT8*)(dst + 3), HCHARS);
			return;
		}
		else
		if( (antic.cmd & 0x0f) == 15 )
		{
			artifacts_gfx(src, (UINT8*)(dst + 3), HCHARS);
			return;
		}
	}
	dst[0] = color_lookup[PBK] | color_lookup[PBK] << 16;
	dst[1] = color_lookup[PBK] | color_lookup[PBK] << 16;
	dst[2] = color_lookup[PBK] | color_lookup[PBK] << 16;
	if ( (antic.cmd & ANTIC_HSCR) == 0  || (antic.pfwidth == 48) || (antic.pfwidth == 32))
	{
		/* no hscroll */
		dst[3] = color_lookup[src[BYTE_XOR_LE(0)]] | color_lookup[src[BYTE_XOR_LE(1)]] << 16;
		src += 2;
		dst += 4;
		for( x = 1; x < HCHARS-1; x++ )
		{
			*dst++ = color_lookup[src[BYTE_XOR_LE(0)]] | color_lookup[src[BYTE_XOR_LE(1)]] << 16;
			*dst++ = color_lookup[src[BYTE_XOR_LE(2)]] | color_lookup[src[BYTE_XOR_LE(3)]] << 16;
			src += 4;
		}
		dst[0] = color_lookup[src[BYTE_XOR_LE(0)]] | color_lookup[src[BYTE_XOR_LE(1)]] << 16;
	}
	else
	{
		/* if hscroll is enabled, more data are fetched by ANTIC, but it still renders playfield
		   of width defined by pfwidth. */
		switch( antic.pfwidth )
		{
			case 0:
				{
					dst[3] = color_lookup[PBK] | color_lookup[PBK] << 16;
					dst += 4;
					for ( x = 1; x < HCHARS-1; x++ )
					{
						*dst++ = color_lookup[PBK] | color_lookup[PBK] << 16;
						*dst++ = color_lookup[PBK] | color_lookup[PBK] << 16;
					}
					dst[0] = color_lookup[PBK] | color_lookup[PBK] << 16;
				}
				break;
			/* support for narrow playfield (32) with horizontal scrolling should be added here */
			case 40:
				{
					dst[3] = color_lookup[PBK] | color_lookup[PBK] << 16;
					dst += 4;
					for ( x = 1; x < HCHARS-2; x++ )
					{
						if ( x == 1 )
						{
							*dst++ = color_lookup[PBK] | color_lookup[PBK] << 16;
						}
						else
						{
							*dst++ = color_lookup[src[BYTE_XOR_LE(0)]] | color_lookup[src[BYTE_XOR_LE(1)]] << 16;
						}
						*dst++ = color_lookup[src[BYTE_XOR_LE(2)]] | color_lookup[src[BYTE_XOR_LE(3)]] << 16;
						src += 4;
					}
					for ( ; x < HCHARS-1; x++ )
					{
						*dst++ = color_lookup[PBK] | color_lookup[PBK] << 16;
						*dst++ = color_lookup[PBK] | color_lookup[PBK] << 16;
					}
					dst[0] = color_lookup[PBK] | color_lookup[PBK] << 16;
				}
				break;
		}
	}
	dst[1] = color_lookup[PBK] | color_lookup[PBK] << 16;
	dst[2] = color_lookup[PBK] | color_lookup[PBK] << 16;
	dst[3] = color_lookup[PBK] | color_lookup[PBK] << 16;

	draw_scanline8(*antic.bitmap, 12, y, MIN(antic.bitmap->width() - 12, sizeof(scanline)), (const UINT8 *) scanline, NULL);
}

int atari_common_state::cycle()
{
	return machine().first_screen()->hpos() * CYCLES_PER_LINE / machine().first_screen()->width();
}

void atari_common_state::after(int cycles, timer_expired_delegate function)
{
	attotime duration = machine().first_screen()->scan_period() * cycles / CYCLES_PER_LINE;
	//(void)funcname;
	//LOG(("           after %3d (%5.1f us) %s\n", cycles, duration.as_double() * 1.0e6, funcname));
	machine().scheduler().timer_set(duration, function);
}

TIMER_CALLBACK_MEMBER( atari_common_state::antic_issue_dli )
{
	if( antic.w.nmien & DLI_NMI )
	{
		LOG(("           @cycle #%3d issue DLI\n", cycle()));
		antic.r.nmist |= DLI_NMI;
		machine().device("maincpu")->execute().set_input_line(INPUT_LINE_NMI, PULSE_LINE);
	}
	else
	{
		LOG(("           @cycle #%3d DLI not enabled\n", cycle()));
	}
}


/*****************************************************************************
 *
 *  Antic Line Done
 *
 *****************************************************************************/
TIMER_CALLBACK_MEMBER( atari_common_state::antic_line_done )
{
	LOG(("           @cycle #%3d antic_line_done\n", cycle()));
	if( antic.w.wsync )
	{
		LOG(("           @cycle #%3d release WSYNC\n", cycle()));
		/* release the CPU if it was actually waiting for HSYNC */
		machine().scheduler().trigger(TRIGGER_HSYNC);
		/* and turn off the 'wait for hsync' flag */
		antic.w.wsync = 0;
	}
	LOG(("           @cycle #%3d release CPU\n", cycle()));
	/* release the CPU (held for emulating cycles stolen by ANTIC DMA) */
	machine().scheduler().trigger(TRIGGER_STEAL);

	/* refresh the display (translate color clocks to pixels) */
	antic_linerefresh();
}

/*****************************************************************************
 *
 *  Antic Steal Cycles
 *  This is called once per scanline by a interrupt issued in the
 *  atari_scanline_render function. Set a new timer for the HSYNC
 *  position and release the CPU; but hold it again immediately until
 *  TRIGGER_HSYNC if WSYNC (D01A) was accessed
 *
 *****************************************************************************/
TIMER_CALLBACK_MEMBER( atari_common_state::antic_steal_cycles )
{
	LOG(("           @cycle #%3d steal %d cycles\n", cycle(), antic.steal_cycles));
	after(antic.steal_cycles, timer_expired_delegate(FUNC(atari_common_state::antic_line_done),this));
	antic.steal_cycles = 0;
	machine().device("maincpu")->execute().spin_until_trigger(TRIGGER_STEAL );
}


/*****************************************************************************
 *
 *  Antic Scan Line Render
 *  Render the scanline to the scrbitmap buffer.
 *  Also transport player/missile data to the grafp and grafm registers
 *  of the GTIA if enabled (DMA_PLAYER or DMA_MISSILE)
 *
 *****************************************************************************/
TIMER_CALLBACK_MEMBER( atari_common_state::antic_scanline_render )
{
	address_space &space = machine().device("maincpu")->memory().space(AS_PROGRAM);

	LOG(("           @cycle #%3d render mode $%X lines to go #%d\n", cycle(), (antic.cmd & 0x0f), antic.modelines));

	antic_render(space, m_antic_render1, m_antic_render2, m_antic_render3);

	/* if player/missile graphics is enabled */
	if( antic.scanline < 256 && (antic.w.dmactl & (DMA_PLAYER|DMA_MISSILE)) )
	{
		/* new player/missile graphics data for every scanline ? */
		if( antic.w.dmactl & DMA_PM_DBLLINE )
		{
			/* transport missile data to GTIA ? */
			if( antic.w.dmactl & DMA_MISSILE )
			{
				antic.steal_cycles += 1;
				m_gtia->write(space, 0x11, RDPMGFXD(space, 3*256));
			}
			/* transport player data to GTIA ? */
			if( antic.w.dmactl & DMA_PLAYER )
			{
				antic.steal_cycles += 4;
				m_gtia->write(space, 0x0d, RDPMGFXD(space, 4*256));
				m_gtia->write(space, 0x0e, RDPMGFXD(space, 5*256));
				m_gtia->write(space, 0x0f, RDPMGFXD(space, 6*256));
				m_gtia->write(space, 0x10, RDPMGFXD(space, 7*256));
			}
		}
		else
		{
			/* transport missile data to GTIA ? */
			if( antic.w.dmactl & DMA_MISSILE )
			{
				if( (antic.scanline & 1) == 0 )      /* even line ? */
					antic.steal_cycles += 1;
				m_gtia->write(space, 0x11, RDPMGFXS(space, 3*128));
			}
			/* transport player data to GTIA ? */
			if( antic.w.dmactl & DMA_PLAYER )
			{
				if( (antic.scanline & 1) == 0 )      /* even line ? */
					antic.steal_cycles += 4;
				m_gtia->write(space, 0x0d, RDPMGFXS(space, 4*128));
				m_gtia->write(space, 0x0e, RDPMGFXS(space, 5*128));
				m_gtia->write(space, 0x0f, RDPMGFXS(space, 6*128));
				m_gtia->write(space, 0x10, RDPMGFXS(space, 7*128));
			}
		}
	}

	if (antic.scanline >= VBL_END && antic.scanline < 256)
		m_gtia->render((UINT8 *)antic.pmbits + PMOFFSET, (UINT8 *)antic.cclock + PMOFFSET - antic.hscrol_old, (UINT8 *)antic.prio_table[m_gtia->get_w_prior() & 0x3f], (UINT8 *)&antic.pmbits);

	antic.steal_cycles += CYCLES_REFRESH;
	LOG(("           run CPU for %d cycles\n", CYCLES_HSYNC - CYCLES_HSTART - antic.steal_cycles));
	after(CYCLES_HSYNC - CYCLES_HSTART - antic.steal_cycles, timer_expired_delegate(FUNC(atari_common_state::antic_steal_cycles),this));
}



void atari_common_state::LMS(int new_cmd)
{
	/**************************************************************
	 * If the LMS bit (load memory scan) of the current display
	 * list command is set, load the video source address from the
	 * following two bytes and split it up into video page/offset.
	 * Steal two more cycles from the CPU for fetching the address.
	 **************************************************************/
	if( new_cmd & ANTIC_LMS )
	{
		address_space &space = machine().device("maincpu")->memory().space(AS_PROGRAM);
		int addr = RDANTIC(space);
		antic.doffs = (antic.doffs + 1) & DOFFS;
		addr += 256 * RDANTIC(space);
		antic.doffs = (antic.doffs + 1) & DOFFS;
		antic.vpage = addr & VPAGE;
		antic.voffs = addr & VOFFS;
		LOG(("           LMS $%04x\n", addr));
		/* steal two more clock cycles from the cpu */
		antic.steal_cycles += 2;
	}
}

/*****************************************************************************
 *
 *  Antic Scan Line DMA
 *  This is called once per scanline from Atari Interrupt
 *  If the ANTIC DMA is active (DMA_ANTIC) and the scanline not inside
 *  the VBL range (VBL_START - TOTAL_LINES or 0 - VBL_END)
 *  check if all mode lines of the previous ANTIC command were done and
 *  if so, read a new command and set up the renderer function
 *
 *****************************************************************************/
void atari_common_state::antic_scanline_dma(int param)
{
	address_space &space = machine().device("maincpu")->memory().space(AS_PROGRAM);
	LOG(("           @cycle #%3d DMA fetch\n", cycle()));
	if (antic.scanline == VBL_END)
		antic.r.nmist &= ~VBL_NMI;
	if( antic.w.dmactl & DMA_ANTIC )
	{
		if( antic.scanline >= VBL_END && antic.scanline < VBL_START )
		{
			if( antic.modelines <= 0 )
			{
				m_antic_render1 = 0;
				m_antic_render3 = antic.w.dmactl & 3;
				UINT8 vscrol_subtract = 0;
				UINT8 new_cmd;

				new_cmd = RDANTIC(space);
				antic.doffs = (antic.doffs + 1) & DOFFS;
				/* steal at one clock cycle from the CPU for fetching the command */
				antic.steal_cycles += 1;
				LOG(("           ANTIC CMD $%02x\n", new_cmd));
				/* command 1 .. 15 ? */
				if (new_cmd & ANTIC_MODE)
				{
					antic.w.chbasl = 0;
					/* vertical scroll mode changed ? */
					if( (antic.cmd ^ new_cmd) & ANTIC_VSCR )
					{
						/* vertical scroll activate now ? */
						if( new_cmd & ANTIC_VSCR )
						{
							antic.vscrol_old =
							vscrol_subtract =
							antic.w.chbasl = antic.w.vscrol;
						}
						else
						{
							vscrol_subtract = ~antic.vscrol_old;
						}
					}
					/* does this command have horizontal scroll enabled ? */
					if( new_cmd & ANTIC_HSCR )
					{
						m_antic_render1 = 1;
						antic.hscrol_old = antic.w.hscrol;
					}
					else
					{
						antic.hscrol_old = 0;
					}
				}
				/* Set the ANTIC mode renderer function */
				m_antic_render2 = new_cmd & ANTIC_MODE;

				switch( new_cmd & ANTIC_MODE )
				{
				case 0x00:
					/* generate 1 .. 8 empty lines */
					antic.modelines = ((new_cmd >> 4) & 7) + 1;
					/* did the last ANTIC command have vertical scroll enabled ? */
					if( antic.cmd & ANTIC_VSCR )
					{
						/* yes, generate vscrol_old additional empty lines */
						antic.modelines += antic.vscrol_old;
					}
					/* leave only bit 7 (DLI) set in ANTIC command */
					new_cmd &= ANTIC_DLI;
					break;
				case 0x01:
					/* ANTIC "jump" with DLI: issue interrupt immediately */
					if( new_cmd & ANTIC_DLI )
					{
						/* remove the DLI bit */
						new_cmd &= ~ANTIC_DLI;
						after(CYCLES_DLI_NMI, timer_expired_delegate(FUNC(atari_common_state::antic_issue_dli),this));
					}
					/* load memory scan bit set ? */
					if( new_cmd & ANTIC_LMS )
					{
						int addr = RDANTIC(space);
						antic.doffs = (antic.doffs + 1) & DOFFS;
						addr += 256 * RDANTIC(space);
						antic.dpage = addr & DPAGE;
						antic.doffs = addr & DOFFS;
						/* produce empty scanlines until vblank start */
						antic.modelines = VBL_START + 1 - antic.scanline;
						if( antic.modelines < 0 )
							antic.modelines = machine().first_screen()->height() - antic.scanline;
						LOG(("           JVB $%04x\n", antic.dpage|antic.doffs));
					}
					else
					{
						int addr = RDANTIC(space);
						antic.doffs = (antic.doffs + 1) & DOFFS;
						addr += 256 * RDANTIC(space);
						antic.dpage = addr & DPAGE;
						antic.doffs = addr & DOFFS;
						/* produce a single empty scanline */
						antic.modelines = 1;
						LOG(("           JMP $%04x\n", antic.dpage|antic.doffs));
					}
					break;
				case 0x02:
					LMS(new_cmd);
					antic.chbase = (antic.w.chbash & 0xfc) << 8;
					antic.modelines = 8 - (vscrol_subtract & 7);
					if( antic.w.chactl & 4 )    /* decrement chbasl? */
						antic.w.chbasl = antic.modelines - 1;
					break;
				case 0x03:
					LMS(new_cmd);
					antic.chbase = (antic.w.chbash & 0xfc) << 8;
					antic.modelines = 10 - (vscrol_subtract & 9);
					if( antic.w.chactl & 4 )    /* decrement chbasl? */
						antic.w.chbasl = antic.modelines - 1;
					break;
				case 0x04:
					LMS(new_cmd);
					antic.chbase = (antic.w.chbash & 0xfc) << 8;
					antic.modelines = 8 - (vscrol_subtract & 7);
					if( antic.w.chactl & 4 )    /* decrement chbasl? */
						antic.w.chbasl = antic.modelines - 1;
					break;
				case 0x05:
					LMS(new_cmd);
					antic.chbase = (antic.w.chbash & 0xfc) << 8;
					antic.modelines = 16 - (vscrol_subtract & 15);
					if( antic.w.chactl & 4 )    /* decrement chbasl? */
						antic.w.chbasl = antic.modelines - 1;
					break;
				case 0x06:
					LMS(new_cmd);
					antic.chbase = (antic.w.chbash & 0xfe) << 8;
					antic.modelines = 8 - (vscrol_subtract & 7);
					if( antic.w.chactl & 4 )    /* decrement chbasl? */
						antic.w.chbasl = antic.modelines - 1;
					break;
				case 0x07:
					LMS(new_cmd);
					antic.chbase = (antic.w.chbash & 0xfe) << 8;
					antic.modelines = 16 - (vscrol_subtract & 15);
					if( antic.w.chactl & 4 )    /* decrement chbasl? */
						antic.w.chbasl = antic.modelines - 1;
					break;
				case 0x08:
					LMS(new_cmd);
					antic.modelines = 8 - (vscrol_subtract & 7);
					break;
				case 0x09:
					LMS(new_cmd);
					antic.modelines = 4 - (vscrol_subtract & 3);
					break;
				case 0x0a:
					LMS(new_cmd);
					antic.modelines = 4 - (vscrol_subtract & 3);
					break;
				case 0x0b:
					LMS(new_cmd);
					antic.modelines = 2 - (vscrol_subtract & 1);
					break;
				case 0x0c:
					LMS(new_cmd);
					antic.modelines = 1;
					break;
				case 0x0d:
					LMS(new_cmd);
					antic.modelines = 2 - (vscrol_subtract & 1);
					break;
				case 0x0e:
					LMS(new_cmd);
					antic.modelines = 1;
					break;
				case 0x0f:
					LMS(new_cmd);
					/* bits 6+7 of the priority select register determine */
					/* if newer GTIA or plain graphics modes are used */
					switch (m_gtia->get_w_prior() >> 6)
					{
						case 0: break;
						case 1: m_antic_render2 = 16;  break;
						case 2: m_antic_render2 = 17;  break;
						case 3: m_antic_render2 = 18;  break;
					}
					antic.modelines = 1;
					break;
				}
				/* set new (current) antic command */
				antic.cmd = new_cmd;
			}
		}
		else
		{
			LOG(("           out of visible range\n"));
			antic.cmd = 0x00;
			m_antic_render1 = 0;
			m_antic_render2 = 0;
			m_antic_render3 = 0;
		}
	}
	else
	{
		LOG(("           DMA is off\n"));
		antic.cmd = 0x00;
		m_antic_render1 = 0;
		m_antic_render2 = 0;
		m_antic_render3 = 0;
	}

	antic.r.nmist &= ~DLI_NMI;
	if( antic.modelines == 1 && (antic.cmd & antic.w.nmien & DLI_NMI) )
		after(CYCLES_DLI_NMI, timer_expired_delegate(FUNC(atari_common_state::antic_issue_dli),this));

	after(CYCLES_HSTART, timer_expired_delegate(FUNC(atari_common_state::antic_scanline_render),this));
}

/*****************************************************************************
 *
 *  Generic Atari Interrupt Dispatcher
 *  This is called once per scanline and handles:
 *  vertical blank interrupt
 *  ANTIC DMA to possibly access the next display list command
 *
 *****************************************************************************/

void atari_common_state::generic_atari_interrupt(int button_count)
{
	LOG(("ANTIC #%3d @cycle #%d scanline interrupt\n", antic.scanline, cycle()));

	if( antic.scanline < VBL_START )
	{
		antic_scanline_dma(0);
		return;
	}

	if( antic.scanline == VBL_START )
	{
		/* specify buttons relevant to this Atari variant */
		m_gtia->button_interrupt(button_count, machine().root_device().ioport("djoy_b")->read_safe(0));

		/* do nothing new for the rest of the frame */
		antic.modelines = machine().first_screen()->height() - VBL_START;
		m_antic_render1 = 0;
		m_antic_render2 = 0;
		m_antic_render3 = 0;
		
		/* if the CPU want's to be interrupted at vertical blank... */
		if( antic.w.nmien & VBL_NMI )
		{
			LOG(("           cause VBL NMI\n"));
			/* set the VBL NMI status bit */
			antic.r.nmist |= VBL_NMI;
			machine().device("maincpu")->execute().set_input_line(INPUT_LINE_NMI, PULSE_LINE);
		}
	}

	/* refresh the display (translate color clocks to pixels) */
	antic_linerefresh();
}



TIMER_DEVICE_CALLBACK_MEMBER( atari_common_state::a400_interrupt )
{
	generic_atari_interrupt(4);
}

TIMER_DEVICE_CALLBACK_MEMBER( atari_common_state::a800_interrupt )
{
	generic_atari_interrupt(4);
}

TIMER_DEVICE_CALLBACK_MEMBER( atari_common_state::a800xl_interrupt )
{
	generic_atari_interrupt(2);
}

TIMER_DEVICE_CALLBACK_MEMBER( atari_common_state::a5200_interrupt )
{
	generic_atari_interrupt(4);
}

/**************************************************************
 *
 * Palette
 *
 **************************************************************/

static const UINT8 atari_palette[256*3] =
{
	/* Grey */
	0x00,0x00,0x00, 0x25,0x25,0x25, 0x34,0x34,0x34, 0x4e,0x4e,0x4e,
	0x68,0x68,0x68, 0x75,0x75,0x75, 0x8e,0x8e,0x8e, 0xa4,0xa4,0xa4,
	0xb8,0xb8,0xb8, 0xc5,0xc5,0xc5, 0xd0,0xd0,0xd0, 0xd7,0xd7,0xd7,
	0xe1,0xe1,0xe1, 0xea,0xea,0xea, 0xf4,0xf4,0xf4, 0xff,0xff,0xff,
	/* Gold */
	0x41,0x20,0x00, 0x54,0x28,0x00, 0x76,0x37,0x00, 0x9a,0x50,0x00,
	0xc3,0x68,0x06, 0xe4,0x7b,0x07, 0xff,0x91,0x1a, 0xff,0xab,0x1d,
	0xff,0xc5,0x1f, 0xff,0xd0,0x3b, 0xff,0xd8,0x4c, 0xff,0xe6,0x51,
	0xff,0xf4,0x56, 0xff,0xf9,0x70, 0xff,0xff,0x90, 0xff,0xff,0xaa,
	/* Orange */
	0x45,0x19,0x04, 0x72,0x1e,0x11, 0x9f,0x24,0x1e, 0xb3,0x3a,0x20,
	0xc8,0x51,0x20, 0xe3,0x69,0x20, 0xfc,0x81,0x20, 0xfd,0x8c,0x25,
	0xfe,0x98,0x2c, 0xff,0xae,0x38, 0xff,0xb9,0x46, 0xff,0xbf,0x51,
	0xff,0xc6,0x6d, 0xff,0xd5,0x87, 0xff,0xe4,0x98, 0xff,0xe6,0xab,
	/* Red-Orange */
	0x5d,0x1f,0x0c, 0x7a,0x24,0x0d, 0x98,0x2c,0x0e, 0xb0,0x2f,0x0f,
	0xbf,0x36,0x24, 0xd3,0x4e,0x2a, 0xe7,0x62,0x3e, 0xf3,0x6e,0x4a,
	0xfd,0x78,0x54, 0xff,0x8a,0x6a, 0xff,0x98,0x7c, 0xff,0xa4,0x8b,
	0xff,0xb3,0x9e, 0xff,0xc2,0xb2, 0xff,0xd0,0xc3, 0xff,0xda,0xd0,
	/* Pink */
	0x4a,0x17,0x00, 0x72,0x1f,0x00, 0xa8,0x13,0x00, 0xc8,0x21,0x0a,
	0xdf,0x25,0x12, 0xec,0x3b,0x24, 0xfa,0x52,0x36, 0xfc,0x61,0x48,
	0xff,0x70,0x5f, 0xff,0x7e,0x7e, 0xff,0x8f,0x8f, 0xff,0x9d,0x9e,
	0xff,0xab,0xad, 0xff,0xb9,0xbd, 0xff,0xc7,0xce, 0xff,0xca,0xde,
	/* Purple */
	0x49,0x00,0x36, 0x66,0x00,0x4b, 0x80,0x03,0x5f, 0x95,0x0f,0x74,
	0xaa,0x22,0x88, 0xba,0x3d,0x99, 0xca,0x4d,0xa9, 0xd7,0x5a,0xb6,
	0xe4,0x67,0xc3, 0xef,0x72,0xce, 0xfb,0x7e,0xda, 0xff,0x8d,0xe1,
	0xff,0x9d,0xe5, 0xff,0xa5,0xe7, 0xff,0xaf,0xea, 0xff,0xb8,0xec,
	/* Purple-Blue */
	0x48,0x03,0x6c, 0x5c,0x04,0x88, 0x65,0x0d,0x90, 0x7b,0x23,0xa7,
	0x93,0x3b,0xbf, 0x9d,0x45,0xc9, 0xa7,0x4f,0xd3, 0xb2,0x5a,0xde,
	0xbd,0x65,0xe9, 0xc5,0x6d,0xf1, 0xce,0x76,0xfa, 0xd5,0x83,0xff,
	0xda,0x90,0xff, 0xde,0x9c,0xff, 0xe2,0xa9,0xff, 0xe6,0xb6,0xff,
	/* Blue 1 */
	0x05,0x1e,0x81, 0x06,0x26,0xa5, 0x08,0x2f,0xca, 0x26,0x3d,0xd4,
	0x44,0x4c,0xde, 0x4f,0x5a,0xec, 0x5a,0x68,0xff, 0x65,0x75,0xff,
	0x71,0x83,0xff, 0x80,0x91,0xff, 0x90,0xa0,0xff, 0x97,0xa9,0xff,
	0x9f,0xb2,0xff, 0xaf,0xbe,0xff, 0xc0,0xcb,0xff, 0xcd,0xd3,0xff,
	/* Blue 2 */
	0x0b,0x07,0x79, 0x20,0x1c,0x8e, 0x35,0x31,0xa3, 0x46,0x42,0xb4,
	0x57,0x53,0xc5, 0x61,0x5d,0xcf, 0x6d,0x69,0xdb, 0x7b,0x77,0xe9,
	0x89,0x85,0xf7, 0x91,0x8d,0xff, 0x9c,0x98,0xff, 0xa7,0xa4,0xff,
	0xb2,0xaf,0xff, 0xbb,0xb8,0xff, 0xc3,0xc1,0xff, 0xd3,0xd1,0xff,
	/* Light-Blue */
	0x1d,0x29,0x5a, 0x1d,0x38,0x76, 0x1d,0x48,0x92, 0x1d,0x5c,0xac,
	0x1d,0x71,0xc6, 0x32,0x86,0xcf, 0x48,0x9b,0xd9, 0x4e,0xa8,0xec,
	0x55,0xb6,0xff, 0x69,0xca,0xff, 0x74,0xcb,0xff, 0x82,0xd3,0xff,
	0x8d,0xda,0xff, 0x9f,0xd4,0xff, 0xb4,0xe2,0xff, 0xc0,0xeb,0xff,
	/* Turquoise */
	0x00,0x4b,0x59, 0x00,0x5d,0x6e, 0x00,0x6f,0x84, 0x00,0x84,0x9c,
	0x00,0x99,0xbf, 0x00,0xab,0xca, 0x00,0xbc,0xde, 0x00,0xd0,0xf5,
	0x10,0xdc,0xff, 0x3e,0xe1,0xff, 0x64,0xe7,0xff, 0x76,0xea,0xff,
	0x8b,0xed,0xff, 0x9a,0xef,0xff, 0xb1,0xf3,0xff, 0xc7,0xf6,0xff,
	/* Green-Blue */
	0x00,0x48,0x00, 0x00,0x54,0x00, 0x03,0x6b,0x03, 0x0e,0x76,0x0e,
	0x18,0x80,0x18, 0x27,0x92,0x27, 0x36,0xa4,0x36, 0x4e,0xb9,0x4e,
	0x51,0xcd,0x51, 0x72,0xda,0x72, 0x7c,0xe4,0x7c, 0x85,0xed,0x85,
	0x99,0xf2,0x99, 0xb3,0xf7,0xb3, 0xc3,0xf9,0xc3, 0xcd,0xfc,0xcd,
	/* Green */
	0x16,0x40,0x00, 0x1c,0x53,0x00, 0x23,0x66,0x00, 0x28,0x78,0x00,
	0x2e,0x8c,0x00, 0x3a,0x98,0x0c, 0x47,0xa5,0x19, 0x51,0xaf,0x23,
	0x5c,0xba,0x2e, 0x71,0xcf,0x43, 0x85,0xe3,0x57, 0x8d,0xeb,0x5f,
	0x97,0xf5,0x69, 0xa0,0xfe,0x72, 0xb1,0xff,0x8a, 0xbc,0xff,0x9a,
	/* Yellow-Green */
	0x2c,0x35,0x00, 0x38,0x44,0x00, 0x44,0x52,0x00, 0x49,0x56,0x00,
	0x60,0x71,0x00, 0x6c,0x7f,0x00, 0x79,0x8d,0x0a, 0x8b,0x9f,0x1c,
	0x9e,0xb2,0x2f, 0xab,0xbf,0x3c, 0xb8,0xcc,0x49, 0xc2,0xd6,0x53,
	0xcd,0xe1,0x53, 0xdb,0xef,0x6c, 0xe8,0xfc,0x79, 0xf2,0xff,0xab,
	/* Orange-Green */
	0x46,0x3a,0x09, 0x4d,0x3f,0x09, 0x54,0x45,0x09, 0x6c,0x58,0x09,
	0x90,0x76,0x09, 0xab,0x8b,0x0a, 0xc1,0xa1,0x20, 0xd0,0xb0,0x2f,
	0xde,0xbe,0x3d, 0xe6,0xc6,0x45, 0xed,0xcd,0x4c, 0xf5,0xd8,0x62,
	0xfb,0xe2,0x76, 0xfc,0xee,0x98, 0xfd,0xf3,0xa9, 0xfd,0xf3,0xbe,
	/* Light-Orange */
	0x40,0x1a,0x02, 0x58,0x1f,0x05, 0x70,0x24,0x08, 0x8d,0x3a,0x13,
	0xab,0x51,0x1f, 0xb5,0x64,0x27, 0xbf,0x77,0x30, 0xd0,0x85,0x3a,
	0xe1,0x93,0x44, 0xed,0xa0,0x4e, 0xf9,0xad,0x58, 0xfc,0xb7,0x5c,
	0xff,0xc1,0x60, 0xff,0xca,0x69, 0xff,0xcf,0x7e, 0xff,0xda,0x96
};


/* Initialise the palette */
PALETTE_INIT_MEMBER(atari_common_state, atari)
{
	int i;

	for ( i = 0; i < sizeof(atari_palette) / 3; i++ )
	{
		palette.set_pen_color(i, atari_palette[i*3], atari_palette[i*3+1], atari_palette[i*3+2]);
	}
}
