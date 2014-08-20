#include "emu.h"
#include "includes/dooyong.h"


inline void dooyong_state::dooyong_scroll8_w(offs_t offset, UINT8 data, UINT8 *scroll, tilemap_t *map)
{
	UINT8 old = scroll[offset];
	if (old != data)
	{
		scroll[offset] = data;
		if (map != NULL) switch (offset)
		{
		case 0: /* Low byte of x scroll - scroll tilemap */
			map->set_scrollx(0, data);
			break;
		case 1: /* High byte of x scroll - mark tilemap dirty so new tile gfx will be loaded */
			map->mark_all_dirty();
			break;
		case 3: /* Low byte of y scroll */
		case 4: /* High byte of y scroll */
			map->set_scrolly(0, (int)scroll[3] | ((int)scroll[4] << 8));
			break;
		case 6: /* Tilemap enable and mode control */
			map->enable(!(data & 0x10));
			if ((data & 0x20) != (old & 0x20))  // This sets the tilemap data format
				map->mark_all_dirty();
			break;
		default:    /* Other addresses are used but function is unknown */
			/* 0x05 and 0x07 are initialised on startup */
			/* 0x02 is initialised on startup by some games and written to continuously by others */
			/*{
			    const char *name;
			    if (scroll == m_bgscroll8)        name = "bg";
			    else if (scroll == m_bg2scroll8)  name = "bg2";
			    else if (scroll == m_fgscroll8)   name = "fg";
			    else if (scroll == m_fg2scroll8)  name = "fg2";
			    else                            name = "unknown";
			    printf("Unknown %s tilemap control: 0x%02x = 0x%02x\n", name, (unsigned)offset, (unsigned)data);
			}*/
			break;
		}
	}
}


/* These handle writes to the tilemap scroll registers in 8-bit machines.
   There is one per tilemap, wrapping the above function that does the work. */

WRITE8_MEMBER(dooyong_state::dooyong_bgscroll8_w)
{
	dooyong_scroll8_w(offset, data, m_bgscroll8, m_bg_tilemap);
}

WRITE8_MEMBER(dooyong_state::dooyong_bg2scroll8_w)
{
	dooyong_scroll8_w(offset, data, m_bg2scroll8, m_bg2_tilemap);
}

WRITE8_MEMBER(dooyong_state::dooyong_fgscroll8_w)
{
	dooyong_scroll8_w(offset, data, m_fgscroll8, m_fg_tilemap);
}

WRITE8_MEMBER(dooyong_state::dooyong_fg2scroll8_w)
{
	dooyong_scroll8_w(offset, data, m_fg2scroll8, m_fg2_tilemap);
}


/* These handle writes to the tilemap scroll registers in 16-bit machines.
   This is just an 8-bit peripheral in a 16-bit machine. */

WRITE16_MEMBER(dooyong_state::dooyong_bgscroll16_w)
{
	if (ACCESSING_BITS_0_7) dooyong_bgscroll8_w(space, offset, data & 0x00ff);
}

WRITE16_MEMBER(dooyong_state::dooyong_bg2scroll16_w)
{
	if (ACCESSING_BITS_0_7) dooyong_bg2scroll8_w(space, offset, data & 0x00ff);
}

WRITE16_MEMBER(dooyong_state::dooyong_fgscroll16_w)
{
	if (ACCESSING_BITS_0_7) dooyong_fgscroll8_w(space, offset, data & 0x00ff);
}

WRITE16_MEMBER(dooyong_state::dooyong_fg2scroll16_w)
{
	if (ACCESSING_BITS_0_7) dooyong_fg2scroll8_w(space, offset, data & 0x00ff);
}


WRITE8_MEMBER(dooyong_state::dooyong_txvideoram8_w)
{
	if (m_txvideoram[offset] != data)
	{
		m_txvideoram[offset] = data;
		if (m_tx_tilemap_mode == 0)
			m_tx_tilemap->mark_tile_dirty(offset & 0x07ff);
		else
			m_tx_tilemap->mark_tile_dirty(offset >> 1);
	}
}


/* Control registers seem to be different on every game */

WRITE8_MEMBER(dooyong_state::lastday_ctrl_w)
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(machine(), 0, data & 0x01);
	coin_counter_w(machine(), 1, data & 0x02);

	/* bit 3 is used but unknown */

	/* bit 4 disables sprites */
	m_sprites_disabled = data & 0x10;

	/* bit 6 is flip screen */
	flip_screen_set(data & 0x40);
}

WRITE8_MEMBER(dooyong_state::pollux_ctrl_w)
{
	/* bit 0 is flip screen */
	flip_screen_set(data & 0x01);

	/* bits 6 and 7 are coin counters */
	coin_counter_w(machine(), 0, data & 0x80);
	coin_counter_w(machine(), 1, data & 0x40);

	/* bit 1 is used but unknown */

	/* bit 2 is continuously toggled (unknown) */
	/* bit 4 is used but unknown */
}

WRITE8_MEMBER(dooyong_state::primella_ctrl_w)
{
	/* bits 0-2 select ROM bank */
	membank("bank1")->set_entry(data & 0x07);

	/* bit 3 disables tx layer */
	m_tx_pri = data & 0x08;

	/* bit 4 flips screen */
	flip_screen_set(data & 0x10);

	/* bit 5 used but unknown */

//  logerror("%04x: bankswitch = %02x\n",space.device().safe_pc(),data&0xe0);
}

WRITE8_MEMBER(dooyong_state::paletteram_flytiger_w)
{
	if (m_flytiger_palette_bank)
	{
		UINT16 value;
		m_paletteram_flytiger[offset] = data;
		value = m_paletteram_flytiger[offset & ~1] | (m_paletteram_flytiger[offset | 1] << 8);
		m_palette->set_pen_color(offset/2, pal5bit(value >> 10), pal5bit(value >> 5), pal5bit(value >> 0));
	}
}

WRITE8_MEMBER(dooyong_state::flytiger_ctrl_w)
{
	/* bit 0 is flip screen */
	flip_screen_set(data & 0x01);

	/* bits 1, 2 used but unknown */

	/* bit 3 fg palette banking: trash protection? */
	m_flytiger_palette_bank = data & 0x08;

	/* bit 4 changes tilemaps priority */
	m_flytiger_pri = data & 0x10;
}

WRITE16_MEMBER(dooyong_state::rshark_ctrl_w)

{
	if (ACCESSING_BITS_0_7)
	{
		/* bit 0 flips screen */
		flip_screen_set(data & 0x0001);

		/* bit 4 changes tilemaps priority */
		m_rshark_pri = data & 0x0010;

		/* bit 5 used but unknown */
	}
}


/* These games all have ROM-based tilemaps for the backgrounds, title
   screens and sometimes "bosses" and special attacks. There are three
   schemes for tilemap encoding.  The scheme is chosen based on the
   contents of the tilemap control variables declared above.
   Note that although the tilemaps are arbitrarily wide (hundreds of
   thousands of pixels, depending on the size of the ROM), we only
   decode a 1024 pixel wide block at a time, and invalidate the tilemap
   when the x scroll moves out of range (trying to decode the whole lot
   at once uses hundreds of megabytes of RAM). */

inline void dooyong_state::lastday_get_tile_info(tile_data &tileinfo, int tile_index,
		const UINT8 *tilerom, UINT8 *scroll, int graphics)
{
	int offs = (tile_index + ((int)scroll[1] << 6)) * 2;
	int attr = tilerom[offs];
	int code, color, flags;
	if (scroll[6] & 0x20)
	{   /* lastday/gulfstrm/pollux/flytiger */
		/* Tiles take two bytes in ROM:
		                 MSB   LSB
		   [offs + 0x00] cCCC CYXc    (bit 9 of gfx code, bits 3-0 of color, Y flip, X flip, bit 8 of gfx code)
		   [offs + 0x01] cccc cccc    (bits 7-0 of gfx code)
		   c = gfx code
		   C = color code
		   X = x flip
		   Y = y flip */
		code = tilerom[offs + 1] | ((attr & 0x01) << 8) | ((attr & 0x80) << 2);
		color = (attr & 0x78) >> 3;
		flags = ((attr & 0x02) ? TILE_FLIPX : 0) | ((attr & 0x04) ? TILE_FLIPY : 0);
	}
	else
	{   

		/* primella */
		/* Tiles take two bytes in ROM:
		                 MSB   LSB
		   [offs + 0x00] YXCC CCcc    (Y flip, X flip, bits 3-0 of color code, bits 9-8 of gfx code)
		   [offs + 0x01] cccc cccc    (bits 7-0 of gfx code)
		   c = gfx code
		   C = color code
		   X = x flip
		   Y = y flip */
		int codemask = 0x03;
		int palmask  = 0x3c;
		
		if (m_gfxdecode->gfx(1)->depth() == 256) // pop bingo has an 8bpp layer and uses what would be the low palette bit to address more tiles (girls 4,5,6 in bonus game)
		{
			codemask = 0x07;
			palmask = 0x00;
		}

		code = tilerom[offs + 1] | ((attr & codemask) << 8);
		color = (attr & palmask) >> 2;
		flags = ((attr & 0x40) ? TILE_FLIPX : 0) | ((attr & 0x80) ? TILE_FLIPY : 0);
	}

	SET_TILE_INFO_MEMBER(graphics, code, color, flags);
}

inline void dooyong_state::rshark_get_tile_info(tile_data &tileinfo, int tile_index,
		const UINT8 *tilerom1, const UINT8 *tilerom2, UINT8 *scroll, int graphics)
{
		/* Tiles take two bytes in tile ROM 1:
		                 MSB   LSB
		   [offs + 0x00] YX?c cccc    (Y flip, X flip, bits 12-8 of gfx code)
		   [offs + 0x01] cccc cccc    (bits 7-0 of gfx code)
		   ? = unused/unknown
		   c = gfx code
		   X = x flip
		   Y = y flip */
	int offs = tile_index + ((int)scroll[1] << 9);
	int attr = tilerom1[offs * 2];
	int code = tilerom1[(offs * 2) + 1] | ((attr & 0x1f) << 8);
	int color = tilerom2[offs] & 0x0f;
	int flags = ((attr & 0x40) ? TILE_FLIPX : 0) | ((attr & 0x80) ? TILE_FLIPY : 0);

	SET_TILE_INFO_MEMBER(graphics, code, color, flags);
}

TILE_GET_INFO_MEMBER(dooyong_state::get_bg_tile_info)
{
	if (m_bg_tilerom2 != NULL)
		rshark_get_tile_info(tileinfo, tile_index, m_bg_tilerom, m_bg_tilerom2, m_bgscroll8, m_bg_gfx);
	else
		lastday_get_tile_info(tileinfo, tile_index, m_bg_tilerom, m_bgscroll8, m_bg_gfx);
}

TILE_GET_INFO_MEMBER(dooyong_state::get_bg2_tile_info)
{
	if (m_bg2_tilerom2 != NULL)
		rshark_get_tile_info(tileinfo, tile_index, m_bg2_tilerom, m_bg2_tilerom2, m_bg2scroll8, m_bg2_gfx);
	else
		lastday_get_tile_info(tileinfo, tile_index, m_bg2_tilerom, m_bg2scroll8, m_bg2_gfx);
}

TILE_GET_INFO_MEMBER(dooyong_state::get_fg_tile_info)
{
	if (m_fg_tilerom2 != NULL)
		rshark_get_tile_info(tileinfo, tile_index, m_fg_tilerom, m_fg_tilerom2, m_fgscroll8, m_fg_gfx);
	else
		lastday_get_tile_info(tileinfo, tile_index, m_fg_tilerom, m_fgscroll8, m_fg_gfx);
}

TILE_GET_INFO_MEMBER(dooyong_state::get_fg2_tile_info)
{
	if (m_fg2_tilerom2 != NULL)
		rshark_get_tile_info(tileinfo, tile_index, m_fg2_tilerom, m_fg2_tilerom2, m_fg2scroll8, m_fg2_gfx);
	else
		lastday_get_tile_info(tileinfo, tile_index, m_fg2_tilerom, m_fg2scroll8, m_fg2_gfx);
}

/* flytiger uses some palette banking technique or something maybe a trash protection */

TILE_GET_INFO_MEMBER(dooyong_state::flytiger_get_fg_tile_info)
{
	const UINT8 *tilerom = m_fg_tilerom;

	int offs = (tile_index + (m_fgscroll8[1] << 6)) * 2;
	int attr = tilerom[offs];
	int code = tilerom[offs + 1] | ((attr & 0x01) << 8) | ((attr & 0x80) << 2);
	int color = (attr & 0x78) >> 3;
	int flags = ((attr & 0x02) ? TILE_FLIPX : 0) | ((attr & 0x04) ? TILE_FLIPY : 0);

	SET_TILE_INFO_MEMBER(m_fg_gfx, code, color, flags);
}

TILE_GET_INFO_MEMBER(dooyong_state::get_tx_tile_info)
{
	/* Each tile takes two bytes of memory:
	                 MSB   LSB
	   [offs + 0x00] cccc cccc    (bits 7-0 of gfx code)
	   [offs + 0x01] CCCC cccc    (bits 3-0 of color code, bits 11-8 of gfx code)
	   c = gfx code
	   C = color code */
	int offs, attr, code, color;
	if (m_tx_tilemap_mode == 0)
	{   /* lastday/gulfstrm/pollux/flytiger */
		offs = tile_index;
		attr = m_txvideoram[offs | 0x0800];
	}
	else
	{   /* bluehawk/primella */
		offs = tile_index * 2;
		attr = m_txvideoram[offs + 1];
	}
	code = m_txvideoram[offs] | ((attr & 0x0f) << 8);
	color = (attr & 0xf0) >> 4;

	SET_TILE_INFO_MEMBER(0, code, color, 0);
}


void dooyong_state::draw_sprites(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, int pollux_extensions)
{
	/* Sprites take 32 bytes each in memory:
	                 MSB   LSB
	   [offs + 0x00] cccc cccc    (bits 7-0 of gfx code)
	   [offs + 0x01] cccx CCCC    (bits 10-8 of gfx code, bit 8 of x position, bits 3-0 of color code)
	   [offs + 0x02] yyyy yyyy
	   [offs + 0x03] xxxx xxxx    (bits 7-0 of x offset)
	   ...
	   [offs + 0x1c] ?hhh XY*c    (bits 2-0 of height, x flip, y flip, * see note, bit 11 of gfx code)
	   ...
	   ? = unused/unknown
	   E = enable
	   c = gfx code
	   x = x position
	   y = y position
	   C = color code
	   w = width
	   X = x flip
	   Y = y flip
	   * = alters y position in pollux and flytiger - see code below
	   bit 11 of gfx code only used by gulfstrm, pollux, bluehawk and flytiger
	   height only used by pollux, bluehawk and flytiger
	   x flip and y flip only used by pollux and flytiger */

	UINT8 *buffered_spriteram = m_spriteram->buffer();
	int offs;

	for (offs = 0; offs < m_spriteram->bytes(); offs += 32)
	{
		int sx, sy, code, color, pri;
		int flipx = 0, flipy = 0, height = 0, y;

		sx = buffered_spriteram[offs+3] | ((buffered_spriteram[offs+1] & 0x10) << 4);
		sy = buffered_spriteram[offs+2];
		code = buffered_spriteram[offs] | ((buffered_spriteram[offs+1] & 0xe0) << 3);
		color = buffered_spriteram[offs+1] & 0x0f;
		//TODO: This priority mechanism works for known games, but seems a bit strange.
		//Are we missing something?  (The obvious spare palette bit isn't it.)
		pri = (((color == 0x00) || (color == 0x0f)) ? 0xfc : 0xf0);

		if (pollux_extensions)
		{
			/* gulfstrm, pollux, bluehawk, flytiger */
			code |= ((buffered_spriteram[offs+0x1c] & 0x01) << 11);

			if (pollux_extensions >= 2)
			{
				/* pollux, bluehawk, flytiger */
				height = (buffered_spriteram[offs+0x1c] & 0x70) >> 4;
				code &= ~height;

				flipx = buffered_spriteram[offs+0x1c] & 0x08;
				flipy = buffered_spriteram[offs+0x1c] & 0x04;

				if (pollux_extensions == 3)
				{
					/* bluehawk */
					sy += 6 - ((~buffered_spriteram[offs+0x1c] & 0x02) << 7);
				}

				if (pollux_extensions == 4)
				{
					/* flytiger */
					sy -=(buffered_spriteram[offs+0x1c] & 0x02) << 7;
				}
			}
		}

		if (flip_screen())
		{
			sx = 498 - sx;
			sy = 240 - (16 * height) - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		for (y = 0; y <= height; y++)
		{
			m_gfxdecode->gfx(1)->prio_transpen(bitmap,cliprect,
					code + y,
					color,
					flipx, flipy,
					sx, sy + (16 * (flipy ? (height - y) : y)),
					screen.priority(),
					pri, 15);
		}
	}
}

void dooyong_state::rshark_draw_sprites(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	UINT16 *buffered_spriteram16 = m_spriteram16->buffer();

	/* Sprites take 8 16-bit words each in memory:
	              MSB             LSB
	   [offs + 0] ???? ???? ???? ???E
	   [offs + 1] ???? ???? hhhh wwww
	   [offs + 2] ???? ???? ???? ????
	   [offs + 3] cccc cccc cccc cccc
	   [offs + 4] ???? ???x xxxx xxxx
	   [offs + 5] ???? ???? ???? ????
	   [offs + 6] ???? ???y yyyy yyyy
	   [offs + 7] ???? ???? ???? CCCC
	   ? = unused/unknown
	   E = enable
	   c = gfx code
	   x = x offset
	   y = y offset (signed)
	   C = color code
	   w = width
	   h = height */

	int offs;

	for (offs = (m_spriteram16->bytes() / 2) - 8; offs >= 0; offs -= 8)
	{
		if (buffered_spriteram16[offs] & 0x0001)    /* enable */
		{
			int sx, sy, code, color, pri;
			int flipx = 0, flipy = 0, width, height, x, y;

			sx = buffered_spriteram16[offs+4] & 0x01ff;
			sy = (INT16)buffered_spriteram16[offs+6] & 0x01ff;
			if (sy & 0x0100) sy |= ~(int)0x01ff;    // Correctly sign-extend 9-bit number
			code = buffered_spriteram16[offs+3];
			color = buffered_spriteram16[offs+7] & 0x000f;
			//TODO: This priority mechanism works for known games, but seems a bit strange.
			//Are we missing something?  (The obvious spare palette bit isn't it.)
			pri = (((color == 0x00) || (color == 0x0f)) ? 0xfc : 0xf0);
			width = buffered_spriteram16[offs+1] & 0x000f;
			height = (buffered_spriteram16[offs+1] & 0x00f0) >> 4;

			if (flip_screen())
			{
				sx = 498 - (16 * width) - sx;
				sy = 240 - (16 * height) - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			for (y = 0; y <= height; y++)
			{
				int _y = sy + (16 * (flipy ? (height - y) : y));
				for (x = 0; x <= width; x++)
				{
					int _x = sx + (16 * (flipx ? (width - x) : x));
					m_gfxdecode->gfx(0)->prio_transpen(bitmap,cliprect,
							code,
							color,
							flipx, flipy,
							_x, _y,
							screen.priority(),
							pri, 15);
					code++;
				}
			}
		}
	}
}


UINT32 dooyong_state::screen_update_lastday(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->black_pen(), cliprect);
	screen.priority().fill(0, cliprect);

	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 1);
	m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 2);
	m_tx_tilemap->draw(screen, bitmap, cliprect, 0, 4);

	if (!m_sprites_disabled)
		draw_sprites(screen, bitmap, cliprect, 0);
	return 0;
}

UINT32 dooyong_state::screen_update_gulfstrm(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->black_pen(), cliprect);
	screen.priority().fill(0, cliprect);

	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 1);
	m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 2);
	m_tx_tilemap->draw(screen, bitmap, cliprect, 0, 4);

	draw_sprites(screen, bitmap, cliprect, 1);
	return 0;
}

UINT32 dooyong_state::screen_update_pollux(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->black_pen(), cliprect);
	screen.priority().fill(0, cliprect);

	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 1);
	m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 2);
	m_tx_tilemap->draw(screen, bitmap, cliprect, 0, 4);

	draw_sprites(screen, bitmap, cliprect, 2);
	return 0;
}

UINT32 dooyong_state::screen_update_flytiger(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->black_pen(), cliprect);
	screen.priority().fill(0, cliprect);

	if (m_flytiger_pri)
	{
		m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 1);
		m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 2);
	}
	else
	{
		m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 1);
		m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 2);
	}
	m_tx_tilemap->draw(screen, bitmap, cliprect, 0, 4);

	draw_sprites(screen, bitmap, cliprect, 4);
	return 0;
}


UINT32 dooyong_state::screen_update_bluehawk(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->black_pen(), cliprect);
	screen.priority().fill(0, cliprect);

	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 1);
	m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 2);
	m_fg2_tilemap->draw(screen, bitmap, cliprect, 0, 4);
	m_tx_tilemap->draw(screen, bitmap, cliprect, 0, 4);

	draw_sprites(screen, bitmap, cliprect, 3);
	return 0;
}

UINT32 dooyong_state::screen_update_primella(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->black_pen(), cliprect);

	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	if (m_tx_pri) m_tx_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	if (!m_tx_pri) m_tx_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	return 0;
}

UINT32 dooyong_state::screen_update_rshark(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->black_pen(), cliprect);
	screen.priority().fill(0, cliprect);

	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 1);
	m_bg2_tilemap->draw(screen, bitmap, cliprect, 0, (m_rshark_pri ? 2 : 1));
	m_fg_tilemap->draw(screen, bitmap, cliprect, 0, 2);
	m_fg2_tilemap->draw(screen, bitmap, cliprect, 0, 2);

	rshark_draw_sprites(screen, bitmap, cliprect);
	return 0;
}

UINT32 dooyong_state::screen_update_popbingo(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->black_pen(), cliprect);
	screen.priority().fill(0, cliprect);

	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 1);

	rshark_draw_sprites(screen, bitmap, cliprect);
	return 0;
}


VIDEO_START_MEMBER(dooyong_state,lastday)
{
	/* Configure tilemap callbacks */
	m_bg_tilerom = memregion("gfx5")->base();
	m_fg_tilerom = memregion("gfx6")->base();
	m_bg_tilerom2 = NULL;
	m_fg_tilerom2 = NULL;
	m_bg_gfx = 2;
	m_fg_gfx = 3;
	m_tx_tilemap_mode = 0;

	/* Create tilemaps */
	m_bg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_bg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_fg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_fg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_tx_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_tx_tile_info),this), TILEMAP_SCAN_COLS,
			8, 8, 64, 32);

	/* Configure tilemap transparency */
	m_fg_tilemap->set_transparent_pen(15);
	m_tx_tilemap->set_transparent_pen(15);

	/* Text layer is offset on this machine */
	m_tx_tilemap->set_scrolly(0, 8);

	memset(m_bgscroll8, 0, 0x10);
	memset(m_bg2scroll8, 0, 0x10);
	memset(m_fgscroll8, 0, 0x10);
	memset(m_fg2scroll8, 0, 0x10);

	/* Register for save/restore */
	save_item(NAME(m_bgscroll8));
	save_item(NAME(m_fgscroll8));
	save_item(NAME(m_sprites_disabled));
	save_item(NAME(m_interrupt_line_1));
	save_item(NAME(m_interrupt_line_2));
}

VIDEO_START_MEMBER(dooyong_state,gulfstrm)
{
	/* Configure tilemap callbacks */
	m_bg_tilerom = memregion("gfx5")->base();
	m_fg_tilerom = memregion("gfx6")->base();
	m_bg_tilerom2 = NULL;
	m_fg_tilerom2 = NULL;
	m_bg_gfx = 2;
	m_fg_gfx = 3;
	m_tx_tilemap_mode = 0;

	/* Create tilemaps */
	m_bg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_bg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_fg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_fg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_tx_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_tx_tile_info),this), TILEMAP_SCAN_COLS,
			8, 8, 64, 32);

	/* Configure tilemap transparency */
	m_fg_tilemap->set_transparent_pen(15);
	m_tx_tilemap->set_transparent_pen(15);

	/* Text layer is offset on this machine */
	m_tx_tilemap->set_scrolly(0, 8);

	memset(m_bgscroll8, 0, 0x10);
	memset(m_bg2scroll8, 0, 0x10);
	memset(m_fgscroll8, 0, 0x10);
	memset(m_fg2scroll8, 0, 0x10);

	/* Register for save/restore */
	save_item(NAME(m_bgscroll8));
	save_item(NAME(m_fgscroll8));
	save_item(NAME(m_interrupt_line_1));
	save_item(NAME(m_interrupt_line_2));
}

VIDEO_START_MEMBER(dooyong_state,pollux)
{
	/* Configure tilemap callbacks */
	m_bg_tilerom = memregion("gfx5")->base();
	m_fg_tilerom = memregion("gfx6")->base();
	m_bg_tilerom2 = NULL;
	m_fg_tilerom2 = NULL;
	m_bg_gfx = 2;
	m_fg_gfx = 3;
	m_tx_tilemap_mode = 0;

	/* Create tilemaps */
	m_bg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_bg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_fg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_fg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_tx_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_tx_tile_info),this), TILEMAP_SCAN_COLS,
			8, 8, 64, 32);

	/* Configure tilemap transparency */
	m_fg_tilemap->set_transparent_pen(15);
	m_tx_tilemap->set_transparent_pen(15);

	memset(m_bgscroll8, 0, 0x10);
	memset(m_bg2scroll8, 0, 0x10);
	memset(m_fgscroll8, 0, 0x10);
	memset(m_fg2scroll8, 0, 0x10);

	/* Register for save/restore */
	save_item(NAME(m_bgscroll8));
	save_item(NAME(m_fgscroll8));
	save_item(NAME(m_interrupt_line_1));
	save_item(NAME(m_interrupt_line_2));
}

VIDEO_START_MEMBER(dooyong_state,bluehawk)
{
	/* Configure tilemap callbacks */
	m_bg_tilerom = memregion("gfx3")->base() + 0x78000;
	m_fg_tilerom = memregion("gfx4")->base() + 0x78000;
	m_fg2_tilerom = memregion("gfx5")->base() + 0x38000;
	m_bg_tilerom2 = NULL;
	m_fg_tilerom2 = NULL;
	m_fg2_tilerom2 = NULL;
	m_bg_gfx = 2;
	m_fg_gfx = 3;
	m_fg2_gfx = 4;
	m_tx_tilemap_mode = 1;

	/* Create tilemaps */
	m_bg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_bg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_fg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_fg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_fg2_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_fg2_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_tx_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_tx_tile_info),this), TILEMAP_SCAN_COLS,
			8, 8, 64, 32);

	/* Configure tilemap transparency */
	m_fg_tilemap->set_transparent_pen(15);
	m_fg2_tilemap->set_transparent_pen(15);
	m_tx_tilemap->set_transparent_pen(15);

	memset(m_bgscroll8, 0, 0x10);
	memset(m_bg2scroll8, 0, 0x10);
	memset(m_fgscroll8, 0, 0x10);
	memset(m_fg2scroll8, 0, 0x10);

	/* Register for save/restore */
	save_item(NAME(m_bgscroll8));
	save_item(NAME(m_fgscroll8));
	save_item(NAME(m_fg2scroll8));
}

VIDEO_START_MEMBER(dooyong_state,flytiger)
{
	/* Configure tilemap callbacks */
	m_bg_tilerom = memregion("gfx3")->base() + 0x78000;
	m_fg_tilerom = memregion("gfx4")->base() + 0x78000;
	m_bg_tilerom2 = NULL;
	m_fg_tilerom2 = NULL;
	m_bg_gfx = 2;
	m_fg_gfx = 3;
	m_tx_tilemap_mode = 0;

	/* Create tilemaps */
	m_bg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_bg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_fg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::flytiger_get_fg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_tx_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_tx_tile_info),this), TILEMAP_SCAN_COLS,
			8, 8, 64, 32);

	/* Configure tilemap transparency */
	m_bg_tilemap->set_transparent_pen(15);
	m_fg_tilemap->set_transparent_pen(15);
	m_tx_tilemap->set_transparent_pen(15);

	memset(m_bgscroll8, 0, 0x10);
	memset(m_bg2scroll8, 0, 0x10);
	memset(m_fgscroll8, 0, 0x10);
	memset(m_fg2scroll8, 0, 0x10);

	/* Register for save/restore */
	save_item(NAME(m_bgscroll8));
	save_item(NAME(m_fgscroll8));
	save_item(NAME(m_flytiger_pri));
}

VIDEO_START_MEMBER(dooyong_state,primella)
{
	/* Configure tilemap callbacks */
	m_bg_tilerom = memregion("gfx2")->base() + memregion("gfx2")->bytes() - 0x8000;
	m_fg_tilerom = memregion("gfx3")->base() + memregion("gfx3")->bytes() - 0x8000;
	m_bg_tilerom2 = NULL;
	m_fg_tilerom2 = NULL;
	m_bg_gfx = 1;
	m_fg_gfx = 2;
	m_tx_tilemap_mode = 1;

	/* Create tilemaps */
	m_bg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_bg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_fg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_fg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_tx_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_tx_tile_info),this), TILEMAP_SCAN_COLS,
			8, 8, 64, 32);

	/* Configure tilemap transparency */
	m_fg_tilemap->set_transparent_pen(15);
	m_tx_tilemap->set_transparent_pen(15);

	memset(m_bgscroll8, 0, 0x10);
	memset(m_bg2scroll8, 0, 0x10);
	memset(m_fgscroll8, 0, 0x10);
	memset(m_fg2scroll8, 0, 0x10);

	/* Register for save/restore */
	save_item(NAME(m_bgscroll8));
	save_item(NAME(m_fgscroll8));
	save_item(NAME(m_tx_pri));
}

VIDEO_START_MEMBER(dooyong_state,rshark)
{
	/* Configure tilemap callbacks */
	m_bg_tilerom = memregion("gfx5")->base();
	m_bg2_tilerom = memregion("gfx4")->base();
	m_fg_tilerom = memregion("gfx3")->base();
	m_fg2_tilerom = memregion("gfx2")->base();
	m_bg_tilerom2 = memregion("gfx6")->base() + 0x60000;
	m_bg2_tilerom2 = memregion("gfx6")->base() + 0x40000;
	m_fg_tilerom2 = memregion("gfx6")->base() + 0x20000;
	m_fg2_tilerom2 = memregion("gfx6")->base() + 0x00000;
	m_bg_gfx = 4;
	m_bg2_gfx = 3;
	m_fg_gfx = 2;
	m_fg2_gfx = 1;

	/* Create tilemaps */
	m_bg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_bg_tile_info),this), TILEMAP_SCAN_COLS,
			16, 16, 64, 32);
	m_bg2_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_bg2_tile_info),this), TILEMAP_SCAN_COLS,
			16, 16, 64, 32);
	m_fg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_fg_tile_info),this), TILEMAP_SCAN_COLS,
			16, 16, 64, 32);
	m_fg2_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_fg2_tile_info),this), TILEMAP_SCAN_COLS,
			16, 16, 64, 32);

	/* Configure tilemap transparency */
	m_bg2_tilemap->set_transparent_pen(15);
	m_fg_tilemap->set_transparent_pen(15);
	m_fg2_tilemap->set_transparent_pen(15);

	memset(m_bgscroll8, 0, 0x10);
	memset(m_bg2scroll8, 0, 0x10);
	memset(m_fgscroll8, 0, 0x10);
	memset(m_fg2scroll8, 0, 0x10);

	/* Register for save/restore */
	save_item(NAME(m_bgscroll8));
	save_item(NAME(m_bg2scroll8));
	save_item(NAME(m_fgscroll8));
	save_item(NAME(m_fg2scroll8));
	save_item(NAME(m_rshark_pri));
}

VIDEO_START_MEMBER(dooyong_state,popbingo)
{
	/* Configure tilemap callbacks */
	m_bg_tilerom = memregion("gfx2")->base();
	m_bg_gfx = 1;

	/* Create tilemaps */
	m_bg_tilemap = &machine().tilemap().create(m_gfxdecode, tilemap_get_info_delegate(FUNC(dooyong_state::get_bg_tile_info),this), TILEMAP_SCAN_COLS,
			32, 32, 32, 8);
	m_bg2_tilemap = m_fg_tilemap = m_fg2_tilemap = NULL;    /* Stop scroll handler from crashing on these */

	memset(m_bgscroll8, 0, 0x10);
	memset(m_bg2scroll8, 0, 0x10);
	memset(m_fgscroll8, 0, 0x10);
	memset(m_fg2scroll8, 0, 0x10);

	/* Register for save/restore */
	save_item(NAME(m_bgscroll8));
	save_item(NAME(m_bg2scroll8));  // Not used atm
	save_item(NAME(m_fgscroll8));   // Not used atm
	save_item(NAME(m_fg2scroll8));  // Not used atm
	save_item(NAME(m_rshark_pri));
}
