/*************************************************************************

   Run and Gun / Slam Dunk
   (c) 1993 Konami

   Driver by R. Belmont.

   This hardware uses the 55673 sprite chip like PreGX and System GX, but in a 4 bit
   per pixel layout.  There is also an all-TTL front overlay tilemap and a rotating
   scaling background done with the PSAC2 ('936).

   Status: Front tilemap should be complete, sprites are mostly correct, controls
   should be fine.


   Change Log:

   (AT070703)
   drivers\rungun.c (this file)
     - mem maps, device settings, component communications, I/O's, sound...etc.

   video\rungun.c
     - general clean-up, clipping, alignment

   video\konamiic.c
     - missing sprites and priority

   Known Issues:
     - no dual monitor support is broken
        - games seem to think they're in dual-monitor mode when they're not
        - speed in some sets is incorrect (for dual monitors I'm guessing it should
          output alternate frames to alternate monitors, but due to other bugs it
          just causes the game to run twice as fast as it should?)
        - synchronization and other oddities (rungunu doesn't show attract mode)
        - swapped P12 and P34 controls in 4-player mode team selectet (real puzzler)
        - P3 and P4 coin chutes not working in 4-player mode


     - sprite palettes are not entirely right

*************************************************************************/

#include "emu.h"


#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/eepromser.h"
#include "sound/k054539.h"
#include "includes/konamipt.h"
#include "includes/rungun.h"



READ16_MEMBER(rungun_state::rng_sysregs_r)
{
	UINT16 data = 0;

	switch (offset)
	{
		case 0x00/2:
			if (ioport("DSW")->read() & 0x20)
				return (ioport("P1")->read() | ioport("P3")->read() << 8);
			else
			{
				data = ioport("P1")->read() & ioport("P3")->read();
				return (data << 8 | data);
			}

		case 0x02/2:
			if (ioport("DSW")->read() & 0x20)
				return (ioport("P2")->read() | ioport("P4")->read() << 8);
			else
			{
				data = ioport("P2")->read() & ioport("P4")->read();
				return (data << 8 | data);
			}

		case 0x04/2:
			/*
			    bit0-7: coin mechs and services
			    bit8 : freeze
			    bit9 : joysticks layout(auto detect???)
			*/
			return ioport("SYSTEM")->read();

		case 0x06/2:
			if (ACCESSING_BITS_0_7)
			{
				data = ioport("DSW")->read();
			}
			return ((m_sysreg[0x06 / 2] & 0xff00) | data);
	}

	return m_sysreg[offset];
}

WRITE16_MEMBER(rungun_state::rng_sysregs_w)
{
	COMBINE_DATA(m_sysreg + offset);

	switch (offset)
	{
		case 0x08/2:
			/*
			    bit0  : eeprom_di_write
			    bit1  : eeprom_cs_write
			    bit2  : eeprom_clk_write
			    bit3  : coin counter?
			    bit7  : set before massive memory writes
			    bit10 : IRQ5 ACK
			*/
			if (ACCESSING_BITS_0_7)
				ioport("EEPROMOUT")->write(data, 0xff);

			if (!(data & 0x40))
				m_maincpu->set_input_line(M68K_IRQ_5, CLEAR_LINE);
		break;

		case 0x0c/2:
			/*
			    bit 0 : also enables IRQ???
			    bit 1 : disable PSAC2 input?
			    bit 2 : OBJCHA
			    bit 3 : enable IRQ 5
			*/
			m_k055673->k053246_set_objcha_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE);
		break;
	}
}

WRITE16_MEMBER(rungun_state::sound_cmd1_w)
{
	if (ACCESSING_BITS_8_15)
		soundlatch_byte_w(space, 0, data >> 8);
}

WRITE16_MEMBER(rungun_state::sound_cmd2_w)
{
	if (ACCESSING_BITS_8_15)
		soundlatch2_byte_w(space, 0, data >> 8);
}

WRITE16_MEMBER(rungun_state::sound_irq_w)
{
	if (ACCESSING_BITS_8_15)
		m_soundcpu->set_input_line(0, HOLD_LINE);
}

READ16_MEMBER(rungun_state::sound_status_msb_r)
{
	if (ACCESSING_BITS_8_15)
		return(m_sound_status << 8);

	return 0;
}

INTERRUPT_GEN_MEMBER(rungun_state::rng_interrupt)
{
	if (m_sysreg[0x0c / 2] & 0x09)
		device.execute().set_input_line(M68K_IRQ_5, ASSERT_LINE);
}

static ADDRESS_MAP_START( rungun_map, AS_PROGRAM, 16, rungun_state )
	AM_RANGE(0x000000, 0x2fffff) AM_ROM                                         // main program + data
	AM_RANGE(0x300000, 0x3007ff) AM_RAM_DEVWRITE("palette", palette_device, write) AM_SHARE("palette")
	AM_RANGE(0x380000, 0x39ffff) AM_RAM                                         // work RAM
	AM_RANGE(0x400000, 0x43ffff) AM_READNOP // AM_READ(K053936_0_rom_r )       // '936 ROM readback window
	AM_RANGE(0x480000, 0x48001f) AM_READWRITE(rng_sysregs_r, rng_sysregs_w) AM_SHARE("sysreg")
	AM_RANGE(0x4c0000, 0x4c001f) AM_DEVREADWRITE8("k053252", k053252_device, read, write, 0x00ff)                        // CCU (for scanline and vblank polling)
	AM_RANGE(0x540000, 0x540001) AM_WRITE(sound_irq_w)
	AM_RANGE(0x58000c, 0x58000d) AM_WRITE(sound_cmd1_w)
	AM_RANGE(0x58000e, 0x58000f) AM_WRITE(sound_cmd2_w)
	AM_RANGE(0x580014, 0x580015) AM_READ(sound_status_msb_r)
	AM_RANGE(0x580000, 0x58001f) AM_RAM                                         // sound regs read/write fall-through
	AM_RANGE(0x5c0000, 0x5c000f) AM_DEVREAD("k055673", k055673_device, k055673_rom_word_r)                       // 246A ROM readback window
	AM_RANGE(0x5c0010, 0x5c001f) AM_DEVWRITE("k055673", k055673_device, k055673_reg_word_w)
	AM_RANGE(0x600000, 0x600fff) AM_DEVREADWRITE("k055673", k055673_device, k053247_word_r, k053247_word_w)  // OBJ RAM
	AM_RANGE(0x601000, 0x601fff) AM_RAM                                         // communication? second monitor buffer?
	AM_RANGE(0x640000, 0x640007) AM_DEVWRITE("k055673", k055673_device, k053246_word_w)                      // '246A registers
	AM_RANGE(0x680000, 0x68001f) AM_DEVWRITE("k053936", k053936_device, ctrl_w)          // '936 registers
	AM_RANGE(0x6c0000, 0x6cffff) AM_RAM_WRITE(rng_936_videoram_w) AM_SHARE("936_videoram")  // PSAC2 ('936) RAM (34v + 35v)
	AM_RANGE(0x700000, 0x7007ff) AM_DEVREADWRITE("k053936", k053936_device, linectrl_r, linectrl_w)          // PSAC "Line RAM"
	AM_RANGE(0x740000, 0x741fff) AM_READWRITE(rng_ttl_ram_r, rng_ttl_ram_w)     // text plane RAM
	AM_RANGE(0x7c0000, 0x7c0001) AM_WRITENOP                                    // watchdog
ADDRESS_MAP_END


/**********************************************************************************/

WRITE8_MEMBER(rungun_state::sound_status_w)
{
	m_sound_status = data;
}

WRITE8_MEMBER(rungun_state::sound_ctrl_w)
{
	/*
	    .... xxxx - Z80 ROM bank
	    ...x .... - NMI enable/acknowledge
	    xx.. .... - BLT2/1 (?)
	*/

	membank("bank2")->set_entry(data & 0x07);

	if (!(data & 0x10))
		m_soundcpu->set_input_line(INPUT_LINE_NMI, CLEAR_LINE);

	m_sound_ctrl = data;
}

WRITE_LINE_MEMBER(rungun_state::k054539_nmi_gen)
{
	if (m_sound_ctrl & 0x10)
	{
		// Trigger an /NMI on the rising edge
		if (!m_sound_nmi_clk && state)
		{
			m_soundcpu->set_input_line(INPUT_LINE_NMI, ASSERT_LINE);
		}
	}

	m_sound_nmi_clk = state;
}

/* sound (this should be split into audio/xexex.c or pregx.c or so someday) */

static ADDRESS_MAP_START( rungun_sound_map, AS_PROGRAM, 8, rungun_state )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK("bank2")
	AM_RANGE(0xc000, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xe22f) AM_DEVREADWRITE("k054539_1", k054539_device, read, write)
	AM_RANGE(0xe230, 0xe3ff) AM_RAM
	AM_RANGE(0xe400, 0xe62f) AM_DEVREADWRITE("k054539_2", k054539_device, read, write)
	AM_RANGE(0xe630, 0xe7ff) AM_RAM
	AM_RANGE(0xf000, 0xf000) AM_WRITE(sound_status_w)
	AM_RANGE(0xf002, 0xf002) AM_READ(soundlatch_byte_r)
	AM_RANGE(0xf003, 0xf003) AM_READ(soundlatch2_byte_r)
	AM_RANGE(0xf800, 0xf800) AM_WRITE(sound_ctrl_w)
	AM_RANGE(0xfff0, 0xfff3) AM_WRITENOP
ADDRESS_MAP_END


static INPUT_PORTS_START( rng )
	PORT_START("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )
	PORT_DIPNAME( 0x0100, 0x0000, "Freeze" )
	PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0100, DEF_STR( On ) )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("DSW")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE_MEMBER("eeprom", eeprom_serial_er5911_device, do_read)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_READ_LINE_DEVICE_MEMBER("eeprom", eeprom_serial_er5911_device, ready_read)
	PORT_SERVICE_NO_TOGGLE( 0x08, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x10, 0x00, "Monitors" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPNAME( 0x20, 0x00, "Number of players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x00, "Sound Output" )
	PORT_DIPSETTING(    0x40, DEF_STR( Mono ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Stereo ) )
	PORT_DIPNAME( 0x04, 0x04, "Bit2 (Unknown)" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Bit7 (Unknown)" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START( "EEPROMOUT" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_OUTPUT ) PORT_WRITE_LINE_DEVICE_MEMBER("eeprom", eeprom_serial_er5911_device, di_write)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_OUTPUT ) PORT_WRITE_LINE_DEVICE_MEMBER("eeprom", eeprom_serial_er5911_device, cs_write)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_OUTPUT ) PORT_WRITE_LINE_DEVICE_MEMBER("eeprom", eeprom_serial_er5911_device, clk_write)

	PORT_START("P1")
	KONAMI8_B123_START(1)

	PORT_START("P2")
	KONAMI8_B123_START(2)

	PORT_START("P3")
	KONAMI8_B123_START(3)

	PORT_START("P4")
	KONAMI8_B123_START(4)
INPUT_PORTS_END



/**********************************************************************************/

static const gfx_layout bglayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4,
		9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static GFXDECODE_START( rungun )
	GFXDECODE_ENTRY( "gfx1", 0, bglayout, 0x0000, 64 )
GFXDECODE_END


void rungun_state::machine_start()
{
	UINT8 *ROM = memregion("soundcpu")->base();

	membank("bank2")->configure_entries(0, 8, &ROM[0x10000], 0x4000);

	save_item(NAME(m_sound_ctrl));
	save_item(NAME(m_sound_status));
	save_item(NAME(m_sound_nmi_clk));
	save_item(NAME(m_ttl_vram));
}

void rungun_state::machine_reset()
{
	m_k054539_1->init_flags(k054539_device::REVERSE_STEREO);

	memset(m_sysreg, 0, 0x20);
	memset(m_ttl_vram, 0, 0x1000 * sizeof(UINT16));

	m_sound_ctrl = 0;
	m_sound_status = 0;
}

static MACHINE_CONFIG_START( rng, rungun_state )

	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, 16000000)
	MCFG_CPU_PROGRAM_MAP(rungun_map)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", rungun_state, rng_interrupt)

	MCFG_CPU_ADD("soundcpu", Z80, 8000000)
	MCFG_CPU_PROGRAM_MAP(rungun_sound_map)

	MCFG_QUANTUM_TIME(attotime::from_hz(6000)) // higher if sound stutters

	MCFG_GFXDECODE_ADD("gfxdecode", "palette", rungun)

	MCFG_EEPROM_SERIAL_ER5911_8BIT_ADD("eeprom")

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_VIDEO_ATTRIBUTES(VIDEO_UPDATE_BEFORE_VBLANK)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(64*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(88, 88+384-1, 24, 24+224-1)
	MCFG_SCREEN_UPDATE_DRIVER(rungun_state, screen_update_rng)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_PALETTE_ADD("palette", 1024)
	MCFG_PALETTE_FORMAT(xBBBBBGGGGGRRRRR)
	MCFG_PALETTE_ENABLE_SHADOWS()
	MCFG_PALETTE_ENABLE_HILIGHTS()

	MCFG_DEVICE_ADD("k053936", K053936, 0)
	MCFG_K053936_OFFSETS(34, 9)

	MCFG_DEVICE_ADD("k055673", K055673, 0)
	MCFG_K055673_CB(rungun_state, sprite_callback)
	MCFG_K055673_CONFIG("gfx2", 1, K055673_LAYOUT_RNG, -8, 15)
	MCFG_K055673_GFXDECODE("gfxdecode")
	MCFG_K055673_PALETTE("palette")

	MCFG_DEVICE_ADD("k053252", K053252, 16000000/2)
	MCFG_K053252_OFFSETS(9*8, 24)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_STEREO("lspeaker", "rspeaker")

	MCFG_DEVICE_ADD("k054539_1", K054539, XTAL_18_432MHz)
	MCFG_K054539_REGION_OVERRRIDE("shared")
	MCFG_K054539_TIMER_HANDLER(WRITELINE(rungun_state, k054539_nmi_gen))
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)

	MCFG_DEVICE_ADD("k054539_2", K054539, XTAL_18_432MHz)
	MCFG_K054539_REGION_OVERRRIDE("shared")
	MCFG_SOUND_ROUTE(0, "lspeaker", 1.0)
	MCFG_SOUND_ROUTE(1, "rspeaker", 1.0)
MACHINE_CONFIG_END


ROM_START( rungun )
	/* main program Europe Version AA  1993, 10.8 */
	ROM_REGION( 0x300000, "maincpu", 0)
	ROM_LOAD16_BYTE( "247eaa03.bin", 0x000000, 0x80000, CRC(f5c91ec0) SHA1(298926ea30472fa8d2c0578dfeaf9a93509747ef) )
	ROM_LOAD16_BYTE( "247eaa04.bin", 0x000001, 0x80000, CRC(0e62471f) SHA1(2861b7a4e78ff371358d318a1b13a6488c0ac364) )

	/* data (Guru 1 megabyte redump) */
	ROM_LOAD16_BYTE( "247b01.23n", 0x200000, 0x80000, CRC(2d774f27) SHA1(c48de9cb9daba25603b8278e672f269807aa0b20) )
	ROM_CONTINUE(                  0x100000, 0x80000)
	ROM_LOAD16_BYTE( "247b02.21n", 0x200001, 0x80000, CRC(d088c9de) SHA1(19d7ad4120f7cfed9cae862bb0c799fdad7ab15c) )
	ROM_CONTINUE(                  0x100001, 0x80000)

	/* sound program */
	ROM_REGION( 0x030000, "soundcpu", 0 )
	ROM_LOAD("247a05",  0x000000, 0x20000, CRC(64e85430) SHA1(542919c3be257c8f118fc21d3835d7b6426a22ed) )
	ROM_RELOAD(         0x010000, 0x20000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, "gfx1", 0)
	ROM_LOAD( "247-a13", 0x000000, 0x200000, CRC(cc194089) SHA1(b5af94f5f583d282ac1499b371bbaac8b2fedc03) )

	/* sprites */
	ROM_REGION( 0x800000, "gfx2", 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, CRC(c3f60854) SHA1(cbee7178ab9e5aa6a5aeed0511e370e29001fb01) )  // 5y
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, CRC(3e315eef) SHA1(898bc4d5ad244e5f91cbc87820b5d0be99ef6662) )  // 2u
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, CRC(5ca7bc06) SHA1(83c793c68227399f93bd1ed167dc9ed2aaac4167) )  // 2y
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, CRC(a5ccd243) SHA1(860b88ade1a69f8b6c5b8206424814b386343571) )  // 5u

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, "gfx3", 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, CRC(57a8d26e) SHA1(0431d10b76d77c26a1f6f2b55d9dbcfa959e1cd0) )

	/* sound data */
	ROM_REGION( 0x400000, "shared", 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, CRC(b8b2a67e) SHA1(a873d32f4b178c714743664fa53c0dca29cb3ce4) )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, CRC(0108142d) SHA1(4dc6a36d976dad9c0da5a5b1f01f2eb3b369c99d) )

	ROM_REGION( 0x80, "eeprom", 0 ) // default eeprom to prevent game booting upside down with error
	ROM_LOAD( "rungun.nv", 0x0000, 0x080, CRC(7bbf0e3c) SHA1(0fd3c9400e9b97a06517e0c8620f773a383100fd) )
ROM_END

ROM_START( runguna )
	/* main program Europe Version AA 1993, 10.4 */
	ROM_REGION( 0x300000, "maincpu", 0)
	ROM_LOAD16_BYTE( "247eaa03.rom", 0x000000, 0x80000, CRC(fec3e1d6) SHA1(cd89dc32ad06308134d277f343a7e8b5fe381f69) )
	ROM_LOAD16_BYTE( "247eaa04.rom", 0x000001, 0x80000, CRC(1b556af9) SHA1(c8351ebd595307d561d089c66cd6ed7f6111d996) )

	/* data (Guru 1 megabyte redump) */
	ROM_LOAD16_BYTE( "247b01.23n", 0x200000, 0x80000, CRC(2d774f27) SHA1(c48de9cb9daba25603b8278e672f269807aa0b20) )
	ROM_CONTINUE(                  0x100000, 0x80000)
	ROM_LOAD16_BYTE( "247b02.21n", 0x200001, 0x80000, CRC(d088c9de) SHA1(19d7ad4120f7cfed9cae862bb0c799fdad7ab15c) )
	ROM_CONTINUE(                  0x100001, 0x80000)

	/* sound program */
	ROM_REGION( 0x030000, "soundcpu", 0 )
	ROM_LOAD("1.13g",  0x000000, 0x20000, CRC(c0b35df9) SHA1(a0c73d993eb32bd0cd192351b5f86794efd91949) )
	ROM_RELOAD(         0x010000, 0x20000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, "gfx1", 0)
	ROM_LOAD( "247-a13", 0x000000, 0x200000, CRC(cc194089) SHA1(b5af94f5f583d282ac1499b371bbaac8b2fedc03) )

	/* sprites */
	ROM_REGION( 0x800000, "gfx2", 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, CRC(c3f60854) SHA1(cbee7178ab9e5aa6a5aeed0511e370e29001fb01) )  // 5y
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, CRC(3e315eef) SHA1(898bc4d5ad244e5f91cbc87820b5d0be99ef6662) )  // 2u
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, CRC(5ca7bc06) SHA1(83c793c68227399f93bd1ed167dc9ed2aaac4167) )  // 2y
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, CRC(a5ccd243) SHA1(860b88ade1a69f8b6c5b8206424814b386343571) )  // 5u

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, "gfx3", 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, CRC(57a8d26e) SHA1(0431d10b76d77c26a1f6f2b55d9dbcfa959e1cd0) )

	/* sound data */
	ROM_REGION( 0x400000, "shared", 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, CRC(b8b2a67e) SHA1(a873d32f4b178c714743664fa53c0dca29cb3ce4) )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, CRC(0108142d) SHA1(4dc6a36d976dad9c0da5a5b1f01f2eb3b369c99d) )

	ROM_REGION( 0x80, "eeprom", 0 ) // default eeprom to prevent game booting upside down with error
	ROM_LOAD( "runguna.nv", 0x0000, 0x080, CRC(7bbf0e3c) SHA1(0fd3c9400e9b97a06517e0c8620f773a383100fd) )
ROM_END

ROM_START( rungunu )
	/* main program US Version AB 1993 10.12 */
	ROM_REGION( 0x300000, "maincpu", 0)
	ROM_LOAD16_BYTE( "247uab03.bin", 0x000000, 0x80000, CRC(f259fd11) SHA1(60381a3fa7f78022dcb3e2f3d13ea32a10e4e36e) )
	ROM_LOAD16_BYTE( "247uab04.bin", 0x000001, 0x80000, CRC(b918cf5a) SHA1(4314c611ef600ec081f409c78218de1639f8b463) )

	/* data */
	ROM_LOAD16_BYTE( "247a01", 0x100000, 0x80000, CRC(8341cf7d) SHA1(372c147c4a5d54aed2a16b0ed258247e65dda563) )
	ROM_LOAD16_BYTE( "247a02", 0x100001, 0x80000, CRC(f5ef3f45) SHA1(2e1d8f672c130dbfac4365dc1301b47beee10161) )

	/* sound program */
	ROM_REGION( 0x030000, "soundcpu", 0 )
	ROM_LOAD("247a05", 0x000000, 0x20000, CRC(64e85430) SHA1(542919c3be257c8f118fc21d3835d7b6426a22ed) )
	ROM_RELOAD(        0x010000, 0x20000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, "gfx1", 0)
	ROM_LOAD( "247a13", 0x000000, 0x200000, CRC(c5a8ef29) SHA1(23938b8093bc0b9eef91f6d38127ca7acbdc06a6) )

	/* sprites */
	ROM_REGION( 0x800000, "gfx2", 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, CRC(c3f60854) SHA1(cbee7178ab9e5aa6a5aeed0511e370e29001fb01) )  // 5y
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, CRC(3e315eef) SHA1(898bc4d5ad244e5f91cbc87820b5d0be99ef6662) )  // 2u
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, CRC(5ca7bc06) SHA1(83c793c68227399f93bd1ed167dc9ed2aaac4167) )  // 2y
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, CRC(a5ccd243) SHA1(860b88ade1a69f8b6c5b8206424814b386343571) )  // 5u

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, "gfx3", 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, CRC(57a8d26e) SHA1(0431d10b76d77c26a1f6f2b55d9dbcfa959e1cd0) )

	/* sound data */
	ROM_REGION( 0x400000, "shared", 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, CRC(b8b2a67e) SHA1(a873d32f4b178c714743664fa53c0dca29cb3ce4) )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, CRC(0108142d) SHA1(4dc6a36d976dad9c0da5a5b1f01f2eb3b369c99d) )

	ROM_REGION( 0x80, "eeprom", 0 ) // default eeprom to prevent game booting upside down with error
	ROM_LOAD( "rungunu.nv", 0x0000, 0x080, CRC(d501f579) SHA1(9e01d9a6a8cdc782dd2a92fbf2295e8df732f892) )
ROM_END

ROM_START( rungunua )
	/* main program US Version BA 1993 10.8 */
	ROM_REGION( 0x300000, "maincpu", 0)
	ROM_LOAD16_BYTE( "247uba03.bin", 0x000000, 0x80000, CRC(c24d7500) SHA1(38e6ae9fc00bf8f85549be4733992336c46fe1f3) )
	ROM_LOAD16_BYTE( "247uba04.bin", 0x000001, 0x80000, CRC(3f255a4a) SHA1(3a4d50ecec8546933ad8dabe21682ba0951eaad0) )

	/* data (Guru 1 megabyte redump) */
	ROM_LOAD16_BYTE( "247b01.23n", 0x200000, 0x80000, CRC(2d774f27) SHA1(c48de9cb9daba25603b8278e672f269807aa0b20) )
	ROM_CONTINUE(                  0x100000, 0x80000)
	ROM_LOAD16_BYTE( "247b02.21n", 0x200001, 0x80000, CRC(d088c9de) SHA1(19d7ad4120f7cfed9cae862bb0c799fdad7ab15c) )
	ROM_CONTINUE(                  0x100001, 0x80000)

	/* sound program */
	ROM_REGION( 0x030000, "soundcpu", 0 )
	ROM_LOAD("247a05", 0x000000, 0x20000, CRC(64e85430) SHA1(542919c3be257c8f118fc21d3835d7b6426a22ed) )
	ROM_RELOAD(        0x010000, 0x20000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, "gfx1", 0)
	ROM_LOAD( "247a13", 0x000000, 0x200000, CRC(c5a8ef29) SHA1(23938b8093bc0b9eef91f6d38127ca7acbdc06a6) )

	/* sprites */
	ROM_REGION( 0x800000, "gfx2", 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, CRC(c3f60854) SHA1(cbee7178ab9e5aa6a5aeed0511e370e29001fb01) )  // 5y
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, CRC(3e315eef) SHA1(898bc4d5ad244e5f91cbc87820b5d0be99ef6662) )  // 2u
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, CRC(5ca7bc06) SHA1(83c793c68227399f93bd1ed167dc9ed2aaac4167) )  // 2y
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, CRC(a5ccd243) SHA1(860b88ade1a69f8b6c5b8206424814b386343571) )  // 5u

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, "gfx3", 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, CRC(57a8d26e) SHA1(0431d10b76d77c26a1f6f2b55d9dbcfa959e1cd0) )

	/* sound data */
	ROM_REGION( 0x400000, "shared", 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, CRC(b8b2a67e) SHA1(a873d32f4b178c714743664fa53c0dca29cb3ce4) )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, CRC(0108142d) SHA1(4dc6a36d976dad9c0da5a5b1f01f2eb3b369c99d) )

	ROM_REGION( 0x80, "eeprom", 0 ) // default eeprom to prevent game booting upside down with error
	ROM_LOAD( "rungunua.nv", 0x0000, 0x080, CRC(9890d304) SHA1(c94a77d1d45e372350456cf8eaa7e7ebd3cdbb84) )
ROM_END

ROM_START( slmdunkj )
	/* main program Japan Version AA 1993 10.8 */
	ROM_REGION( 0x300000, "maincpu", 0)
	ROM_LOAD16_BYTE( "247jaa03.bin", 0x000000, 0x20000, CRC(87572078) SHA1(cfa784eb40ed8b3bda9d57abb6022bbe92056206) )
	ROM_LOAD16_BYTE( "247jaa04.bin", 0x000001, 0x20000, CRC(aa105e00) SHA1(617ac14535048b6e0da43cc98c4b67c8e306bef1) )

	/* data (Guru 1 megabyte redump) */
	ROM_LOAD16_BYTE( "247b01.23n", 0x200000, 0x80000, CRC(2d774f27) SHA1(c48de9cb9daba25603b8278e672f269807aa0b20) )
	ROM_CONTINUE(                  0x100000, 0x80000)
	ROM_LOAD16_BYTE( "247b02.21n", 0x200001, 0x80000, CRC(d088c9de) SHA1(19d7ad4120f7cfed9cae862bb0c799fdad7ab15c) )
	ROM_CONTINUE(                  0x100001, 0x80000)

	/* sound program */
	ROM_REGION( 0x030000, "soundcpu", 0 )
	ROM_LOAD("247a05",  0x000000, 0x20000, CRC(64e85430) SHA1(542919c3be257c8f118fc21d3835d7b6426a22ed) )
	ROM_RELOAD(         0x010000, 0x20000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, "gfx1", 0)
	ROM_LOAD( "247-a13", 0x000000, 0x200000, CRC(cc194089) SHA1(b5af94f5f583d282ac1499b371bbaac8b2fedc03) )

	/* sprites */
	ROM_REGION( 0x800000, "gfx2", 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, CRC(c3f60854) SHA1(cbee7178ab9e5aa6a5aeed0511e370e29001fb01) )  // 5y
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, CRC(3e315eef) SHA1(898bc4d5ad244e5f91cbc87820b5d0be99ef6662) )  // 2u
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, CRC(5ca7bc06) SHA1(83c793c68227399f93bd1ed167dc9ed2aaac4167) )  // 2y
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, CRC(a5ccd243) SHA1(860b88ade1a69f8b6c5b8206424814b386343571) )  // 5u

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, "gfx3", 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, CRC(57a8d26e) SHA1(0431d10b76d77c26a1f6f2b55d9dbcfa959e1cd0) )

	/* sound data */
	ROM_REGION( 0x400000, "shared", 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, CRC(b8b2a67e) SHA1(a873d32f4b178c714743664fa53c0dca29cb3ce4) )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, CRC(0108142d) SHA1(4dc6a36d976dad9c0da5a5b1f01f2eb3b369c99d) )

	ROM_REGION( 0x80, "eeprom", 0 ) // default eeprom to prevent game booting upside down with error
	ROM_LOAD( "slmdunkj.nv", 0x0000, 0x080, CRC(531d27bd) SHA1(42251272691c66c1f89f99e6e5e2f300c1a7d69d) )
ROM_END


GAME( 1993, rungun,   0,      rng, rng, driver_device, 0, ROT0, "Konami", "Run and Gun (ver EAA 1993 10.8)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
GAME( 1993, runguna,  rungun, rng, rng, driver_device, 0, ROT0, "Konami", "Run and Gun (ver EAA 1993 10.4)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
GAME( 1993, rungunu,  rungun, rng, rng, driver_device, 0, ROT0, "Konami", "Run and Gun (ver UAB 1993 10.12)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE | GAME_NOT_WORKING ) // runs twice as fast as it should, broken inputs!
GAME( 1993, rungunua, rungun, rng, rng, driver_device, 0, ROT0, "Konami", "Run and Gun (ver UBA 1993 10.8)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE | GAME_NOT_WORKING )  // runs twice as fast as it should, broken inputs! broken attract!
GAME( 1993, slmdunkj, rungun, rng, rng, driver_device, 0, ROT0, "Konami", "Slam Dunk (ver JAA 1993 10.8)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
