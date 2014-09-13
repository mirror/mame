// license:MAME
// copyright-holders:Miodrag Milanovic, Robbbert
/***************************************************************************

Samsung SPC-1000 driver by Miodrag Milanovic

    2009-05-10 Preliminary driver.
    2014-02-16 Added cassette, many games are playable

ToDo:
- Some games have keyboard problems (e.g. Invaders, Panzerspitze)
- Some games freeze at start (e.g. Super Xevious)
- Find out if any of the unconnected parts of 6000,4000,4001 are used


NOTE: 2014-09-13: added code from someone's modified MESS driver for floppy
                  disk. Since it is not to our coding standards, it is
                  commented out with #if 0/#endif and 3 slashes (///).
                  It is planned to be converted when time permits. The
                  author is unknown.

                  Hardware details of the fdc: Intelligent device, Z80 CPU,
                  XTAL_8MHz, PPI 8255, FDC uPD765C, 2 RAM chips, 28 other
                  small ics. And of course, no schematic.


****************************************************************************/


#include "emu.h"
#include "cpu/z80/z80.h"
#include "video/mc6847.h"
#include "sound/ay8910.h"
#include "sound/wave.h"
#include "imagedev/cassette.h"
#include "machine/ram.h"
#include "formats/spc1000_cas.h"


class spc1000_state : public driver_device
{
public:
	spc1000_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_p_videoram(*this, "videoram")
		, m_vdg(*this, "mc6847")
		, m_maincpu(*this, "maincpu")
		, m_ram(*this, RAM_TAG)
		, m_cass(*this, "cassette")
	{}

	DECLARE_WRITE8_MEMBER(spc1000_iplk_w);
	DECLARE_READ8_MEMBER(spc1000_iplk_r);
	DECLARE_WRITE_LINE_MEMBER(irq_w);
	DECLARE_WRITE8_MEMBER(spc1000_gmode_w);
	DECLARE_READ8_MEMBER(spc1000_gmode_r);
	DECLARE_READ8_MEMBER(porta_r);
	DECLARE_READ8_MEMBER(mc6847_videoram_r);
	DECLARE_WRITE8_MEMBER(cass_w);
	///DECLARE_WRITE8_MEMBER(spc1000_sd725_w);
	///DECLARE_READ8_MEMBER(spc1000_sd725_r);

	MC6847_GET_CHARROM_MEMBER(get_char_rom)
	{
		return m_p_videoram[0x1000 + (ch & 0x7f) * 16 + line];
	}

	required_shared_ptr<UINT8> m_p_videoram;
private:
	UINT8 m_IPLK;
	UINT8 m_GMODE;
	UINT16 m_page;
	virtual void machine_reset();
	///FloppyDisk fdd;
	///void initDisk(void);
	required_device<mc6847_base_device> m_vdg;
	required_device<cpu_device> m_maincpu;
	required_device<ram_device> m_ram;
	required_device<cassette_image_device> m_cass;
};



static ADDRESS_MAP_START(spc1000_mem, AS_PROGRAM, 8, spc1000_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x7fff ) AM_READ_BANK("bank1") AM_WRITE_BANK("bank2")
	AM_RANGE( 0x8000, 0xffff ) AM_READ_BANK("bank3") AM_WRITE_BANK("bank4")
ADDRESS_MAP_END

WRITE8_MEMBER(spc1000_state::spc1000_iplk_w)
{
	m_IPLK = m_IPLK ? 0 : 1;
	if (m_IPLK == 1) {
		UINT8 *mem = memregion("maincpu")->base();
		membank("bank1")->set_base(mem);
		membank("bank3")->set_base(mem);
	} else {
		UINT8 *ram = m_ram->pointer();
		membank("bank1")->set_base(ram);
		membank("bank3")->set_base(ram + 0x8000);
	}
}

READ8_MEMBER(spc1000_state::spc1000_iplk_r)
{
	m_IPLK = m_IPLK ? 0 : 1;
	if (m_IPLK == 1) {
		UINT8 *mem = memregion("maincpu")->base();
		membank("bank1")->set_base(mem);
		membank("bank3")->set_base(mem);
	} else {
		UINT8 *ram = m_ram->pointer();
		membank("bank1")->set_base(ram);
		membank("bank3")->set_base(ram + 0x8000);
	}
	return 0;
}

WRITE8_MEMBER( spc1000_state::cass_w )
{
	m_cass->output(BIT(data, 0) ? -1.0 : 1.0);
}

WRITE8_MEMBER(spc1000_state::spc1000_gmode_w)
{
	m_GMODE = data;

	// m_GMODE layout: CSS|NA|PS2|PS1|~A/G|GM0|GM1|NA
	//  [PS2,PS1] is used to set screen 0/1 pages
	m_vdg->gm1_w(BIT(data, 1));
	m_vdg->gm0_w(BIT(data, 2));
	m_vdg->ag_w(BIT(data, 3));
	m_vdg->css_w(BIT(data, 7));
	m_page = ( (BIT(data, 5) << 1) | BIT(data, 4) )*0x200;
}

READ8_MEMBER(spc1000_state::spc1000_gmode_r)
{
	return m_GMODE;
}

static ADDRESS_MAP_START( spc1000_io , AS_IO, 8, spc1000_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_RAM AM_SHARE("videoram")
	AM_RANGE(0x2000, 0x3fff) AM_READWRITE(spc1000_gmode_r, spc1000_gmode_w)
	AM_RANGE(0x4000, 0x4000) AM_DEVWRITE("ay8910", ay8910_device, address_w)
	AM_RANGE(0x4001, 0x4001) AM_DEVREADWRITE("ay8910", ay8910_device, data_r, data_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(cass_w)
	AM_RANGE(0x8000, 0x8000) AM_READ_PORT("LINE0")
	AM_RANGE(0x8001, 0x8001) AM_READ_PORT("LINE1")
	AM_RANGE(0x8002, 0x8002) AM_READ_PORT("LINE2")
	AM_RANGE(0x8003, 0x8003) AM_READ_PORT("LINE3")
	AM_RANGE(0x8004, 0x8004) AM_READ_PORT("LINE4")
	AM_RANGE(0x8005, 0x8005) AM_READ_PORT("LINE5")
	AM_RANGE(0x8006, 0x8006) AM_READ_PORT("LINE6")
	AM_RANGE(0x8007, 0x8007) AM_READ_PORT("LINE7")
	AM_RANGE(0x8008, 0x8008) AM_READ_PORT("LINE8")
	AM_RANGE(0x8009, 0x8009) AM_READ_PORT("LINE9")
	AM_RANGE(0xA000, 0xA000) AM_READWRITE(spc1000_iplk_r, spc1000_iplk_w)
	///AM_RANGE(0xC000, 0xC002) AM_READWRITE(spc1000_sd725_r, spc1000_sd725_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( spc1000 )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_RCONTROL) PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_PAUSE)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Graph") PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('^') PORT_CHAR('~') PORT_CHAR(0x1e)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR(0x03)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A') PORT_CHAR(0x01)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q') PORT_CHAR(0x16)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps") PORT_CODE(KEYCODE_CAPSLOCK)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR(0x1a)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}') PORT_CHAR(0x1d)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V') PORT_CHAR(0x16)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S') PORT_CHAR(0x13)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W') PORT_CHAR(0x17)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8) PORT_CHAR(0x12)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(0x1b)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{') PORT_CHAR(0x1b)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B') PORT_CHAR(0x02)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D') PORT_CHAR(0x04)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E') PORT_CHAR(0x05)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|') PORT_CHAR(0x1c)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N') PORT_CHAR(0x0e)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR(0x06)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR(0x12)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_START("LINE5")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M') PORT_CHAR(0x0d)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR(0x07)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR(0x14)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_START("LINE6")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('@') PORT_CHAR('`')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR(0x18)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H') PORT_CHAR(0x08)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y') PORT_CHAR(0x19)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_START("LINE7")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR(0x10)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J') PORT_CHAR(0x0a)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U') PORT_CHAR(0x15)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 '") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_START("LINE8")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K') PORT_CHAR(0x0b)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I') PORT_CHAR(0x09)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_START("LINE9")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED) ///	PORT_NAME("IPL") PORT_CODE(KEYCODE_F6)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L') PORT_CHAR(0x0c)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR(0x0e)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
INPUT_PORTS_END

#if 0
typedef union PC
{
    struct {
        unsigned char rDAV : 1;
        unsigned char rRFD : 1;
        unsigned char rDAC : 1;
        unsigned char rNON : 1;
        unsigned char wDAV : 1;
        unsigned char wRFD : 1;
        unsigned char wDAC : 1;
        unsigned char wATN : 1;
    } bits;
    unsigned char b;
} pc_t;

typedef char byte;

typedef struct
{
    byte rdVal;
    byte wrVal;
    byte rSize;
    UINT16 seq;
    byte isCmd;
    byte cmd;
    UINT8 sdata[6];
    pc_t PC;
    char diskfile[1024];
    char diskfile2[1024];
    UINT8 diskdata[80*16*256]; // 80 tracks 16 sectors 256 byte
    UINT8 diskdata2[80*16*256];
    byte modified;
    byte modified2;
    byte write;
    byte write2;
    byte *buffer;
    byte idx;
    int datasize;
    int dataidx;
} FloppyDisk;

void spc1000_state::initDisk(void)
{
    FILE *f;
	strcpy(fdd.diskfile, "system.dsk");
	strcpy(fdd.diskfile2, "disk.dsk");
	printf("ddd\n");
    if (strlen(fdd.diskfile) > 4)
    {
        f = fopen(fdd.diskfile, "rb");
		if (f > 0) {
        	fread(fdd.diskdata, 1, sizeof(fdd.diskdata), f);
	        fclose(f);
		}
    }
    if (strlen(fdd.diskfile2) > 4)
    {
        f = fopen(fdd.diskfile2, "rb");
		if (f > 0) {
        	fread(fdd.diskdata2, 1, sizeof(fdd.diskdata2), f);
			printf("disk.dsk\n");
	        fclose(f);
		}
    }
}

WRITE8_MEMBER(spc1000_state::spc1000_sd725_w)
{
	//printf("write 0x%04x=%d\n", (0xc000+offset), data);
	if (offset == 0) {
		fdd.wrVal = data;
	} else if (offset == 2) {
		if (data != 0)
		{
			fdd.PC.b = (data & 0xf0) | (fdd.PC.b & 0xf);
		}
		else
		{
			fdd.PC.b = fdd.PC.b & 0xf;
		}
		if (data & 0x10) // DAV=1
		{
			//printf("FDD Data Valid (c000 bit4 on)\n");
			if (fdd.isCmd == 1) // command
			{
				fdd.isCmd = 0;
				printf("FDD Command(Data) = 0h%02x\n", fdd.wrVal);
				fdd.cmd = fdd.wrVal;
				switch (fdd.wrVal)
				{
					case 0x00: // FDD Initialization
						printf("*FDD Initialization\n");
						break;
					case 0x01: // FDD Write
						printf("*FDD Write\n");
						fdd.rSize = 4;
						fdd.seq = 0;
						break;
					case 0x02: // FDD Read
						printf("*FDD Read\n");
//                            fdd.PC.bits.rRFD = 1;
						fdd.rSize = 4;
						fdd.seq = 0;
						break;
					case 0x03: // FDD Send Data
						//printf("*FDD Send Data\n");
						if (fdd.seq == 4)
						{
							printf("seq=%d,(%d,%d,%d,%d)\n", fdd.seq, fdd.sdata[0], fdd.sdata[1], fdd.sdata[2], fdd.sdata[3]);
							fdd.buffer = (byte*)(fdd.sdata[1] != 0 ? fdd.diskdata2 : fdd.diskdata);
							fdd.buffer += (fdd.sdata[2] * 16 + fdd.sdata[3]-1) * 256;
							fdd.datasize = fdd.sdata[0] * 256;
							fdd.dataidx = 0;

						}
						fdd.rdVal = 0;
						break;
					case 0x04: // FDD Copy
						printf("*FDD Copy\n");
						fdd.rSize = 7;
						fdd.seq = 0;
						break;
					case 0x05: // FDD Format
						printf("*FDD Format\n");
						break;
					case 0x06: // FDD Send Status
						printf("*FDD Send Status\n");
#define DATA_OK 0x40
						fdd.rdVal = 0x80 & DATA_OK;
						break;
					case 0x07: // FDD Send Drive State
						printf("*FDD Send Drive State\n");
#define DRIVE0 0x10
						fdd.rdVal = 0x0f | DRIVE0;
						break;
					case 0x08: // FDD RAM Test
						printf("*FDD RAM Test\n");
						fdd.rSize = 4;
						fdd.seq = 0;
						break;
					case 0x09: // FDD Transmit 2
						printf("*FDD Transmit 2\n");
						fdd.rSize = 4;
						fdd.seq = 0;
						break;
					case 0x0A: // FDD Action
						printf("*FDD No Action\n");
						break;
					case 0x0B: // FDD Transmit 1
						printf("*FDD Transmit 1\n");
						fdd.rSize = 4;
						fdd.seq = 0;
						break;
					case 0x0C: // FDD Receive
						printf("*FDD Receive\n");
						fdd.rSize = 4;
						fdd.seq = 0;
						break;
					case 0x0D: // FDD Go
						printf("*FDD Go\n");
						fdd.rSize = 2;
						fdd.seq = 0;
						break;
					case 0x0E: // FDD Load
						printf("*FDD Load\n");
						fdd.rSize = 6;
						fdd.seq = 0;
						break;
					case 0x0F: // FDD Save
						printf("FDD Save\n");
						fdd.rSize = 6;
						fdd.seq = 0;
						break;
					case 0x10: // FDD Load and Go
						printf("*FDD Load and Go\n");
						break;

				}
			}
			else
			{
				if (fdd.rSize-- > 0)
				{
					fdd.sdata[fdd.seq++] = (char) fdd.wrVal;
					printf("seq=%d, data = 0x%02x\n", fdd.seq-1, fdd.sdata[fdd.seq-1]);
					// printf("cmd=%d\n", fdd.cmd);
					if (fdd.rSize == 0)
					{
						printf("Fdd Command(%d) Fired\n", fdd.cmd);
						if (fdd.cmd == 0x0e)
						{
							int offset = (int)((int)fdd.sdata[2] * 16 + (int)fdd.sdata[3]-1) * 256;
							int size = fdd.sdata[0] * 256;
							printf("load(%d,%d,%d,%d),offset=%d, size=%d\n", fdd.sdata[0], fdd.sdata[1], fdd.sdata[2], fdd.sdata[3], offset, size);
							fdd.buffer = (byte*)(fdd.sdata[1] != 0 ? fdd.diskdata2 : fdd.diskdata);
							//fdd.buffer += offset;
							fdd.datasize = size;
							unsigned short addr = ((unsigned short)fdd.sdata[4]) * 0x100 + (unsigned short)fdd.sdata[5];
							printf("target addr=%04x, %02x, %02x, size=%d\n", addr, fdd.sdata[4], fdd.sdata[5], fdd.datasize);
							UINT8 *mem = m_ram->pointer();
							memcpy(&mem[addr], &fdd.buffer[offset], fdd.datasize);
						}
						else if (fdd.cmd == 0x0f)
						{
							int offset = (int)((int)fdd.sdata[2] * 16 + (int)fdd.sdata[3]-1) * 256;
							int size = fdd.sdata[0] * 256;
							printf("save(%d,%d,%d,%d),offset=%d, size=%d\n", fdd.sdata[0], fdd.sdata[1], fdd.sdata[2], fdd.sdata[3], offset, size);
							fdd.buffer = (byte*)(fdd.sdata[1] != 0 ? fdd.diskdata2 : fdd.diskdata);
							fdd.buffer += offset;
							fdd.datasize = size;
							unsigned short addr = ((unsigned short)fdd.sdata[4]) * 0x100 + (unsigned short)fdd.sdata[5];
							printf("target addr=%04x, %02x, %02x, size=%d\n", addr, fdd.sdata[4], fdd.sdata[5], fdd.datasize);
							UINT8 *mem = m_ram->pointer();
							memcpy(fdd.buffer, &mem[addr], fdd.datasize);
						}
					}
				}
			}
			fdd.PC.bits.rDAC = 1;

		}
		else if (fdd.PC.bits.rDAC == 1) // DAV=0
		{
			//printf("FDD Ouput Data Cleared (c000 bit4 off)\n");
			fdd.wrVal = 0;
//              printf("FDD_Read = 0h%02x (cleared)\n", fdd.wrVal);
			fdd.PC.bits.rDAC = 0;
		}
		if (data & 0x20) // RFD=1
		{
//                printf("FDD Ready for Data Read (c000 bit5 on)\n");
			fdd.PC.bits.rDAV = 1;
		}
		else if (fdd.PC.bits.rDAV == 1) // RFD=0
		{
			//fdd.rdVal = 0;
			//printf("FDD Input Data = 0h%02x\n", fdd.rdVal);
			fdd.PC.bits.rDAV = 0;
		}
		if (data & 0x40) // DAC=1
		{
//                printf("FDD Data accepted (c000 bit6 on)\n");
		}
		if (data & 0x80) // ATN=1
		{
//              printf("FDD Attention (c000 bit7 on)\n");
			//printf("Command = 0x%02x\n", fdd.rdVal);
			//printf("FDD Ready for Data\n", fdd.rdVal);
			fdd.PC.bits.rRFD = 1;
			fdd.isCmd = 1;
		}		
	}
	return;
}

READ8_MEMBER(spc1000_state::spc1000_sd725_r)
{
	//printf("read %04x\n", (0xc000+offset));
	if (offset == 1)
	{
		if (fdd.cmd == 3)
		{
			fdd.rdVal = *(fdd.buffer + fdd.dataidx++);
		}
		//printf("FDD_Data > 0h%02x\n", spcsys.fdd.rdVal);
		return fdd.rdVal;
	}
	else if (offset == 2)
	{
		//printf("FDD_PC > 0h%02x\n", spcsys.fdd.PC.b);
		return fdd.PC.b;
	}
	return 0;
}
#endif


void spc1000_state::machine_reset()
{
	address_space &space = m_maincpu->space(AS_PROGRAM);
	UINT8 *mem = memregion("maincpu")->base();
	UINT8 *ram = m_ram->pointer();

	space.install_read_bank(0x0000, 0x7fff, "bank1");
	space.install_read_bank(0x8000, 0xffff, "bank3");

	space.install_write_bank(0x0000, 0x7fff, "bank2");
	space.install_write_bank(0x8000, 0xffff, "bank4");

	membank("bank1")->set_base(mem);
	membank("bank2")->set_base(ram);
	membank("bank3")->set_base(mem);
	membank("bank4")->set_base(ram + 0x8000);
	///initDisk();

	m_IPLK = 1;
}

READ8_MEMBER(spc1000_state::mc6847_videoram_r)
{
	if (offset == ~0) return 0xff;

	// m_GMODE layout: CSS|NA|PS2|PS1|~A/G|GM0|GM1|NA
	if ( !BIT(m_GMODE, 3) )
	{   // text mode (~A/G set to A)
		UINT8 data = m_p_videoram[offset+m_page+0x800];
		m_vdg->inv_w(BIT(data, 0));
		m_vdg->css_w(BIT(data, 1));
		m_vdg->as_w (BIT(data, 2));
		m_vdg->intext_w(BIT(data, 3));
		return m_p_videoram[offset+m_page];
	}
	else
	{    // graphics mode: uses full 6KB of VRAM
		return m_p_videoram[offset];
	}
}

READ8_MEMBER( spc1000_state::porta_r )
{
	UINT8 data = 0;
	data |= (m_cass->input() > 0.0038) ? 0x80 : 0;
	data |= ((m_cass->get_state() & CASSETTE_MASK_UISTATE) == CASSETTE_PLAY) ? 0x00 : 0x40;

	return data;
}

// irq is inverted in emulation, so we need this trampoline
WRITE_LINE_MEMBER( spc1000_state::irq_w )
{
	m_maincpu->set_input_line(0, state ? CLEAR_LINE : HOLD_LINE);
}

static MACHINE_CONFIG_START( spc1000, spc1000_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(spc1000_mem)
	MCFG_CPU_IO_MAP(spc1000_io)

	/* video hardware */
	MCFG_SCREEN_MC6847_NTSC_ADD("screen", "mc6847")

	MCFG_DEVICE_ADD("mc6847", MC6847_NTSC, XTAL_3_579545MHz)
	MCFG_MC6847_FSYNC_CALLBACK(WRITELINE(spc1000_state, irq_w))
	MCFG_MC6847_INPUT_CALLBACK(READ8(spc1000_state, mc6847_videoram_r))
	MCFG_MC6847_CHARROM_CALLBACK(spc1000_state, get_char_rom)
	MCFG_MC6847_FIXED_MODE(MC6847_MODE_GM2)
	// other lines not connected

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("ay8910", AY8910, XTAL_4MHz / 1)
	MCFG_AY8910_PORT_A_READ_CB(READ8(spc1000_state, porta_r))
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_WAVE_ADD(WAVE_TAG, "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MCFG_CASSETTE_ADD( "cassette" )
	MCFG_CASSETTE_FORMATS(spc1000_cassette_formats)
	MCFG_CASSETTE_DEFAULT_STATE(CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( spc1000 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "spcall.rom", 0x0000, 0x8000, CRC(19638fc9) SHA1(489f1baa7aebf3c8c660325fb1fd790d84203284))
ROM_END

#if 0
ROM_START( spc1000 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "spcall.rom", 0x0000, 0x8000, CRC(2FBB6ECA) SHA1(cc9a076b0f00d54b2aec31f1f558b10f43ef61c8))
	/// more roms to come...
ROM_END
#endif


/* Driver */

/*    YEAR  NAME      PARENT  COMPAT   MACHINE    INPUT    CLASS         INIT    COMPANY    FULLNAME       FLAGS */
COMP( 1982, spc1000,  0,      0,       spc1000,   spc1000, driver_device,  0,   "Samsung", "SPC-1000", GAME_NOT_WORKING )
