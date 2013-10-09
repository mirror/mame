/***************************************************************************

        Unior

    2009-05-12 Skeleton driver.
    2013-10-09 Added DMA and CRTC

    Some info obtained from EMU-80.
    The schematic is difficult to read, and some code is guesswork.

The monitor will only allow certain characters to be typed, thus the
modifier keys appear to do nothing. There is no need to use the enter
key; using spacebar and the correct parameters is enough.

Monitor commands:
C
D
E - save
F
G
H - set register
I - load
J - modify memory
K
L - list registers
M

ToDo:
- Some devices
- Beeper (requires a timer chip)
- Cassette (requires a UART)
- Colour?

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/8257dma.h"
#include "video/i8275.h"


class unior_state : public driver_device
{
public:
	unior_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_dma(*this, "dma")
	{ }

	DECLARE_WRITE8_MEMBER(vram_w);
	DECLARE_WRITE8_MEMBER(unior_4c_w);
	DECLARE_READ8_MEMBER(unior_4c_r);
	DECLARE_READ8_MEMBER(unior_4d_r);
	DECLARE_WRITE8_MEMBER(unior_50_w);
	DECLARE_WRITE8_MEMBER(cpu_status_callback);
	DECLARE_PALETTE_INIT(unior);
	DECLARE_READ8_MEMBER(dma_r);
	UINT8 *m_p_vram;
	UINT8 *m_p_chargen;
private:
	UINT8 m_4c;
	virtual void machine_reset();
	virtual void video_start();
	required_device<cpu_device> m_maincpu;
	required_device<i8257_device> m_dma;
};

READ8_MEMBER( unior_state::unior_4c_r )
{
	return m_4c;
}

READ8_MEMBER( unior_state::unior_4d_r )
{
	char kbdrow[6];
	sprintf(kbdrow,"X%X", m_4c&15);
	return ioport(kbdrow)->read();
}

WRITE8_MEMBER( unior_state::unior_4c_w )
{
	m_4c = data;
}

WRITE8_MEMBER( unior_state::vram_w )
{
	m_p_vram[offset] = data;
}

WRITE8_MEMBER( unior_state::unior_50_w )
{
	if (data)
		memcpy(m_p_vram, m_p_vram+80, 24*80);
}

static ADDRESS_MAP_START( unior_mem, AS_PROGRAM, 8, unior_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xffff) AM_ROM AM_WRITE(vram_w) // main video
ADDRESS_MAP_END

static ADDRESS_MAP_START( unior_io, AS_IO, 8, unior_state )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x30, 0x38) AM_DEVREADWRITE("dma", i8257_device, i8257_r, i8257_w) // dma data
	AM_RANGE(0x3c, 0x3f) AM_NOP // cassette player control
	AM_RANGE(0x4c, 0x4c) AM_READWRITE(unior_4c_r,unior_4c_w)
	AM_RANGE(0x4d, 0x4d) AM_READ(unior_4d_r)
	AM_RANGE(0x4e, 0x4f) AM_NOP // possibly the control ports of a PIO
	AM_RANGE(0x50, 0x50) AM_WRITE(unior_50_w)
	AM_RANGE(0x60, 0x61) AM_DEVREADWRITE("crtc", i8275_device, read, write)
	AM_RANGE(0xdc, 0xdf) AM_NOP // timer chip + beeper
	AM_RANGE(0xec, 0xed) AM_NOP // cassette data to&from UART
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( unior )
	PORT_START("X0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED) // nothing
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED) // nothing
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)

	PORT_START("X1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC) PORT_CHAR(27) // cancel input
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB) PORT_CHAR(9) // Russian little M
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Rus/Lat") PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("NUMLK") PORT_CODE(KEYCODE_NUMLOCK) // switches 1st indicator between N and B - numlock? rctrl
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK)

	PORT_START("X2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') // ;
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS") PORT_CODE(KEYCODE_CAPSLOCK) // switches 2nd indicator between U and B - capslock? rshift
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD)

	PORT_START("X3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F1) // A
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('^') // ^ (')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)

	PORT_START("X4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F2) // B
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F7) // H
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F8) // I

	PORT_START("X5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F3) // C
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BSPACE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F6) // G
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LF") PORT_CODE(KEYCODE_HOME) PORT_CHAR(10) // line feed?

	PORT_START("X6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('\xA4')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('I') // I then hangs
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT) // Russian A
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) // ? (/)

	PORT_START("X7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4) // D
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN) // Russian bl
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP) // Russian signpost
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED) // } (<>)

	PORT_START("X8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F5) // E
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('>') // >
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= :") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(':') // :
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('=') // =

	PORT_START("X9")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT) // Russian U
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0')

	PORT_START("XA")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('@') // (`)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR('<') // <
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
INPUT_PORTS_END


void unior_state::machine_reset()
{
	m_maincpu->set_state_int(I8085_PC, 0xF800);
}

void unior_state::video_start()
{
	m_p_chargen = memregion("chargen")->base();
	m_p_vram = memregion("vram")->base();
}


/* F4 Character Displayer */
static const gfx_layout unior_charlayout =
{
	8, 8,                   /* 8 x 8 characters */
	256,                    /* 256 characters */
	1,                  /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8                 /* every char takes 8 bytes */
};

static GFXDECODE_START( unior )
	GFXDECODE_ENTRY( "chargen", 0x0000, unior_charlayout, 0, 1 )
GFXDECODE_END

static I8275_DISPLAY_PIXELS(display_pixels)
{
	unior_state *state = device->machine().driver_data<unior_state>();
	const rgb_t *palette = palette_entry_list_raw(bitmap.palette());
	UINT8 gfx = state->m_p_chargen[(linecount & 7) | (charcode << 3)];
	if (vsp)
		gfx = 0;

	if (lten)
		gfx = 0xff;

	if (rvv)
		gfx ^= 0xff;

	for(UINT8 i=0;i<6;i++)
		bitmap.pix32(y, x + i) = palette[BIT(gfx, 5-i) ? (hlgt ? 2 : 1) : 0];
}

static const i8275_interface i8275_intf =
{
	6,
	0,
	DEVCB_DEVICE_LINE_MEMBER("dma", i8257_device, i8257_drq2_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	display_pixels
};

static const rgb_t unior_palette[3] =
{
	MAKE_RGB(0x00, 0x00, 0x00), // black
	MAKE_RGB(0xa0, 0xa0, 0xa0), // white
	MAKE_RGB(0xff, 0xff, 0xff)  // highlight
};

PALETTE_INIT_MEMBER(unior_state,unior)
{
	palette_set_colors(machine(), 0, unior_palette, ARRAY_LENGTH(unior_palette));
}

READ8_MEMBER(unior_state::dma_r)
{
	if (offset < 0xf800)
		return m_maincpu->space(AS_PROGRAM).read_byte(offset);
	else
		return m_p_vram[offset & 0x7ff];
}

static I8257_INTERFACE( i8257_intf )
{
	DEVCB_CPU_INPUT_LINE("maincpu", I8085_HALT),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_MEMBER("crtc", i8275_device, dack_w),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_DRIVER_MEMBER(unior_state, dma_r), DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
};

WRITE8_MEMBER( unior_state::cpu_status_callback )
{
	m_dma->i8257_hlda_w(BIT(data, 3));
}

static MACHINE_CONFIG_START( unior, unior_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",I8080, XTAL_20MHz / 9) // unknown clock
	MCFG_CPU_PROGRAM_MAP(unior_mem)
	MCFG_CPU_IO_MAP(unior_io)
	MCFG_I8085A_STATUS(WRITE8(unior_state, cpu_status_callback))

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 200)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)
	MCFG_SCREEN_UPDATE_DEVICE("crtc", i8275_device, screen_update)
	MCFG_GFXDECODE(unior)
	MCFG_PALETTE_LENGTH(3)
	MCFG_PALETTE_INIT_OVERRIDE(unior_state,unior)

	MCFG_I8257_ADD("dma", XTAL_20MHz / 9, i8257_intf) // unknown clock
	MCFG_I8275_ADD("crtc", i8275_intf)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( unior )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "unior.rom", 0xf800, 0x0800, CRC(23a347e8) SHA1(2ef3134e2f4a696c3b52a145fa5a2d4c3487194b))

	ROM_REGION( 0x0840, "chargen", ROMREGION_ERASEFF )
	ROM_LOAD( "unior.fnt",   0x0000, 0x0800, CRC(4f654828) SHA1(8c0ac11ea9679a439587952e4908940b67c4105e))
	ROM_LOAD( "palette.rom", 0x0800, 0x0040, CRC(b4574ceb) SHA1(f7a82c61ab137de8f6a99b0c5acf3ac79291f26a))

	ROM_REGION( 0x0800, "vram", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT    COMPAT   MACHINE    INPUT    INIT    COMPANY      FULLNAME       FLAGS */
COMP( 19??, unior,  radio86,  0,       unior,     unior, driver_device,   0,    "<unknown>",   "Unior", GAME_NOT_WORKING | GAME_NO_SOUND)
