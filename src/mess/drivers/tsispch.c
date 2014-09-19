// license:MAME|LGPL-2.1+
// copyright-holders:Jonathan Gevaryahu
/******************************************************************************
*
*  Telesensory Systems Inc./Speech Plus
*  1500 and 2000 series
*  Prose 2020
*  Copyright (C) 2011-2013 Jonathan Gevaryahu AKA Lord Nightmare and Kevin 'kevtris' Horton
*
*  The Prose 2000 card is an IEEE 796 Multibus card, with additional connectors to facilitate power and serial input other than via multibus.
*  There are two hardware versions of the card:
*  - The 1981 Telesensory Systems Inc copyrighted version
*    (lacks U82, has some rework on the power input to add a bypass capacitor and an extra power line to the 8086)
*  - The 1986 Speech Plus copyrighted version
*    (adds U82 as an extra buffer for status (not sure)? integrates the greenwire fixes from the above board, minor reorganizations of passives)
*  Both versions encountered have been in non-multibus enclosures:
*  - The 1981 Version appeared on a 'Voice V4' Speech board scrapped from a "Kurzweil Reading Machine" Talking Scanner (predecessor to the TSI/Kurzweil/Xerox 'Reading Edge' scanner which is SPARC based) [I'm very sorry I didn't get the OCR computing guts of the scanner itself too :( ]
*  - The 1986 Version appeared in a 'Prose 2020' under-monitor RS232 speech unit.
*
*  DONE:
*  Skeleton Written
*  Load cpu and dsp roms and mapper proms
*  Successful compile
*  Successful run
*  Correctly Interleave 8086 CPU roms
*  Debug LEDs hooked to popmessage
*  Correctly load UPD7720 roms as UPD7725 data - done; this is utterly disgusting code, but appears to work.
*  Attached i8251a uart at u15
*  Added dipswitch array S4
*  Attached 8259 PIC
   * IR0 = upd7720 p0 pin masked by (probably peripheral bit 8)
   * IR1 = i8251 rxrdy
   * IR2 = i8251 txempty
   * IR3 = i8251 txrdy
   * IR4,5,6,7 unknown so far, not hooked up yet
*  Hooked the terminal to the i8251a uart at u15
*  Hooked up upd7720 reset line
*  Verified CPU and DSP clocks
*
*  TODO:
*  UPD7720: hook up serial output and SCK, and hook SO to the DAC; this requires fixing the upd7725 core to actually support SCK and serial output/SO!
*  Attach the other i8251a uart (assuming it is hooked to the main hardware at all!)
*  Correctly implement UPD7720 cpu core to avoid needing revolting conversion code; this probably involves overriding and duplicating much of the exec_xx sections of the 7725 core
*  Correct memory maps and io maps, and figure out what all the proms do - mostly done
*  8259 PIC: figure out where IR4-7 come from, if anywhere.
*  UPD7720 and 8259: hook up p0 and p1 as outputs, and figure out how 8259 IR0 is masked from 7720 p0.
*  Add other dipswitches and jumpers (these may actually just control clock dividers for the two 8251s)
*  Older v1.1 set gets stuck forever waiting for upd7720 status port to equal 0x20, which never happens.
*  Everything else
*
*  Notes:
*  Text in rom indicates there is a test mode 'activated by switch s4 dash 7'
*  When switch s4-7 is switched on, the hardware says, over and over:
*  "This is version 3.4.1 test mode, activated by switch s4 dash 7"
*
*  0x03400: the peripheral register
*    the low 8 bits (7-0) are Sw4
*    the high 8 bits:
*    Bit F E D C B A 9 8
*        | | | | | | | \- unknown but used, probably control (1=allow 0=mask?) 7720 p0 int to 8259 ir0?
*        | | | | | | \--- LED 6 control (0 = on)
*        | | | | | \----- LED 5 control (0 = on)
*        | | | | \------- LED 4 control (0 = on)
*        | | | \--------- LED 3 control (0 = on)
*        | | \----------- unknown, possibly unused?
*        | \------------- UPD7720 RESET line (0 = high/in reset, 1 = low/running)
*        \--------------- unknown, possibly unused? but might possibly related to 7720 p0->8259 as well?
*
*    When the unit is idle, leds 5 and 3 are on and upd7720 reset is low (write of 0b?1?0101?.
*    On all character writes from i8251, bit 8 is unset, then set again, possibly to avoid interrupt clashes?
*
*  Bootup notes v3.4.1:
*    D3109: checks if 0x80 (S4-8) is set: if set, continue, else jump to D3123
*    D3123: write 0x1C (0 0 0 [1 1 1 0] 0) to 3401
*    then jump to D32B0
*      D32B0: memory test routine:
*        This routine flood-fills memory from 0000-2FFF with 0xFF,
*        then, bytewise starting from 0000, shifts the value progresively
*        right by one, writes it and checks that it still matches,
*        i.e. read 0xFF, write 0x7f, read 0x7f, write 0x3f... etc.
*        Loop at D32E4.
*      D32E6: similar to D32B0, but rotates in 1 bits to 16 bit words,
*        though only the low byte is written, and only fills the 2BFF
*        down to 0000 region. (seems rather redundant, actually)
*        Loop at D3301.
*      D3311: write 0x0A (0 0 0 [0 1 0 1] 0) to 3401
*      then jump to D3330
*      D3330: jump back to D312E
*    D312E: this is some unknown conditional code, usually goes to D314E
*      if BP is not 1, go to D314E and don't update leds (usually taken?)
*      if BP is 1 and SI is 0, delay for 8*65536 cycles. no delay if si!=0
*      write 0x0C (0 0 0 [0 1 1 0] 0) to 3401
*    D314E: floodfill 0000-2BFF with 0x55 (rep at D315C)
*      check if bp was 1 and jump to D318F if it was
*      write 0x14 (0 0 0 [1 0 1 0] 0) to 3401
*      call E3987: initialize UPD7720, return
*    D33D2: checksum the roms in 5? passes, loop at D33DA, test at D33E6 (which passes)
*      if test DID fail: write 0x10 (0 0 0 [1 0 0 0] 0) to 3401
*        more stuff
*        write 0xFF to 3401
*        more stuff
*        set up word table? not sure what its doing here...
*      if test does NOT fail (and it doesn't):
*        D3414: write 0x08 (0 0 0 [0 1 0 0] 0) to 3400
*    D5E14: initialize PIC8259
*    <more stuff, wip>
*    D338A: write 0x12 0 0 0 [1 0 0 1] 0 to 3401
*
*  F44B4: general in-operation LED status write
******************************************************************************/

/* Core includes */
#include "emu.h"
#include "includes/tsispch.h"
#include "cpu/upd7725/upd7725.h"
#include "cpu/i86/i86.h"
#include "machine/i8251.h"
#include "machine/pic8259.h"
#include "machine/terminal.h"

// defines
#define DEBUG_PARAM 1
#undef DEBUG_DSP
#undef DEBUG_DSP_W

/*
   Devices and handlers
 */

/*****************************************************************************
 USART 8251 and Terminal stuff
*****************************************************************************/
WRITE_LINE_MEMBER(tsispch_state::i8251_rxrdy_int)
{
	machine().device<pic8259_device>("pic8259")->ir1_w(state);
}

WRITE_LINE_MEMBER(tsispch_state::i8251_txempty_int)
{
	machine().device<pic8259_device>("pic8259")->ir2_w(state);
}

WRITE_LINE_MEMBER(tsispch_state::i8251_txrdy_int)
{
	machine().device<pic8259_device>("pic8259")->ir3_w(state);
}

WRITE8_MEMBER( tsispch_state::i8251_rxd )
{
	i8251_device *uart = machine().device<i8251_device>("i8251a_u15");
	uart->receive_character(data);
}

/*****************************************************************************
 LED/dipswitch stuff
*****************************************************************************/
READ8_MEMBER( tsispch_state::dsw_r )
{
	/* the only dipswitch I'm really sure about is s4-7 which enables the test mode
	 * The switches are, for normal operation on my unit:
	 * 1  2  3   4   5   6   7   8
	 * ON ON OFF OFF OFF OFF OFF OFF
	 * which makes this register read 0xFC
	 * When s4-7 is turned on, it reads 0xBC
	 */
	return ioport("s4")->read();
}

WRITE8_MEMBER( tsispch_state::peripheral_w )
{
	/* This controls the four LEDS, the RESET line for the upd77p20,
	and (probably) the p0-to-ir0 masking of the upd77p20; there are two
	unknown and seemingly unused bits as well.
	see the top of the file for more info.
	*/
	m_paramReg = data;
	m_dsp->set_input_line(INPUT_LINE_RESET, BIT(data,6)?CLEAR_LINE:ASSERT_LINE);
#ifdef DEBUG_PARAM
	//fprintf(stderr,"8086: Parameter Reg written: UNK7: %d, DSPRST6: %d; UNK5: %d; LED4: %d; LED3: %d; LED2: %d; LED1: %d; DSPIRQMASK: %d\n", BIT(data,7), BIT(data,6), BIT(data,5), BIT(data,4), BIT(data,3), BIT(data,2), BIT(data,1), BIT(data,0));
	logerror("8086: Parameter Reg written: UNK7: %d, DSPRST6: %d; UNK5: %d; LED4: %d; LED3: %d; LED2: %d; LED1: %d; DSPIRQMASK: %d\n", BIT(data,7), BIT(data,6), BIT(data,5), BIT(data,4), BIT(data,3), BIT(data,2), BIT(data,1), BIT(data,0));
#endif
	popmessage("LEDS: 6/Talking:%d 5:%d 4:%d 3:%d\n", 1-BIT(data,1), 1-BIT(data,2), 1-BIT(data,3), 1-BIT(data,4));
}

/*****************************************************************************
 UPD77P20 stuff
*****************************************************************************/
READ16_MEMBER( tsispch_state::dsp_data_r )
{
	upd7725_device *upd7725 = machine().device<upd7725_device>("dsp");
#ifdef DEBUG_DSP
	UINT8 temp;
	temp = upd7725->snesdsp_read(true);
	fprintf(stderr, "dsp data read: %02x\n", temp);
	return temp;
#else
	return upd7725->snesdsp_read(true);
#endif
}

WRITE16_MEMBER( tsispch_state::dsp_data_w )
{
	upd7725_device *upd7725 = machine().device<upd7725_device>("dsp");
#ifdef DEBUG_DSP_W
	fprintf(stderr, "dsp data write: %02x\n", data);
#endif
	upd7725->snesdsp_write(true, data);
}

READ16_MEMBER( tsispch_state::dsp_status_r )
{
	upd7725_device *upd7725 = machine().device<upd7725_device>("dsp");
#ifdef DEBUG_DSP
	UINT8 temp;
	temp = upd7725->snesdsp_read(false);
	fprintf(stderr, "dsp status read: %02x\n", temp);
	return temp;
#else
	return upd7725->snesdsp_read(false);
#endif
}

WRITE16_MEMBER( tsispch_state::dsp_status_w )
{
	fprintf(stderr, "warning: upd772x status register should never be written to!\n");
	upd7725_device *upd7725 = machine().device<upd7725_device>("dsp");
	upd7725->snesdsp_write(false, data);
}

/*****************************************************************************
 Reset and Driver Init
*****************************************************************************/
void tsispch_state::machine_reset()
{
	// clear fifos (TODO: memset would work better here...)
	int i;
	for (i=0; i<32; i++) m_infifo[i] = 0;
	m_infifo_tail_ptr = m_infifo_head_ptr = 0;
	fprintf(stderr,"machine reset\n");
}

DRIVER_INIT_MEMBER(tsispch_state,prose2k)
{
	UINT8 *dspsrc = (UINT8 *)(*memregion("dspprgload"));
	UINT32 *dspprg = (UINT32 *)(*memregion("dspprg"));
	fprintf(stderr,"driver init\n");
	// unpack 24 bit 7720 data into 32 bit space and shuffle it so it can run as 7725 code
	// data format as-is in dspsrc: (L = always 0, X = doesn't matter)
	// source upd7720                  dest upd7725
	// bit 7  6  5  4  3  2  1  0      bit 7  6  5  4  3  2  1  0
	// for OP/RT:
	// b1  15 16 17 18 19 20 21 22 ->      22 21 20 19 18 17 16 15
	// b2  L  8  9  10 11 12 13 14 ->      14 13 12 L  11 10 9  8
	// b3  0  1  2  3  4  5  6  7  ->      7  6  5  4  3  2  1  0
	// for JP:
	// b1  15 16 17 18 19 20 21 22 ->      22 21 20 19 18 17 16 15
	// b2  L  8  9  10 11 12 13 14 ->      14 13 L  L  L  12 11 10
	// b3  0  1  2  3  4  5  6  7  ->      9  8  7  6  5  4  X  X
	// for LD:
	// b1  15 16 17 18 19 20 21 22 ->      22 21 20 19 18 17 16 15
	// b2  L  8  9  10 11 12 13 14 ->      14 13 12 11 10 9  8  7
	// b3  0  1  2  3  4  5  6  7  ->      6  5  X  X  3  2  1  0
	UINT8 byte1t;
	UINT16 byte23t;
		for (int i = 0; i < 0x600; i+= 3)
		{
			byte1t = BITSWAP8(dspsrc[0+i], 0, 1, 2, 3, 4, 5, 6, 7);
			// here's where things get disgusting: if the first byte was an OP or RT, do the following:
			if ((byte1t&0x80) == 0x00) // op or rt instruction
			{
				byte23t = BITSWAP16( (((UINT16)dspsrc[1+i]<<8)|dspsrc[2+i]), 8, 9, 10, 15, 11, 12, 13, 14, 0, 1, 2, 3, 4, 5, 6, 7);
			}
			else if ((byte1t&0xC0) == 0x80) // jp instruction
			{
				byte23t = BITSWAP16( (((UINT16)dspsrc[1+i]<<8)|dspsrc[2+i]), 8, 9, 15, 15, 15, 10, 11, 12, 13, 14, 0, 1, 2, 3, 6, 7);
			}
			else // ld instruction
			{
				byte23t = BITSWAP16( (((UINT16)dspsrc[1+i]<<8)|dspsrc[2+i]), 8, 9, 10, 11, 12, 13, 14, 0, 1, 2, 3, 3, 4, 5, 6, 7);
			}

			*dspprg = byte1t<<24 | byte23t<<8;
			dspprg++;
		}
	m_paramReg = 0x00; // on power up, all leds on, reset to upd7720 is high
	m_dsp->set_input_line(INPUT_LINE_RESET, ASSERT_LINE); // starts in reset
}

/******************************************************************************
 Address Maps
******************************************************************************/
/* The address map of the prose 2020 is controlled by 2 proms, see the rom section
   for details on those.
   (x = ignored; * = selects address within this range; s = selects one of a pair of chips)
   A19 A18 A17 A16  A15 A14 A13 A12  A11 A10 A9 A8  A7 A6 A5 A4  A3 A2 A1 A0
     0   0   x   x    0   x   0   *    *   *  *  *   *  *  *  *   *  *  *  s  6264*2 SRAM first half
     0   0   x   x    0   x   1   0    *   *  *  *   *  *  *  *   *  *  *  s  6264*2 SRAM 3rd quarter
     0   0   x   x    0   x   1   1    0   0  0  x   x  x  x  x   x  x  *  x  iP8251A @ U15
     0   0   x   x    0   x   1   1    0   0  1  x   x  x  x  x   x  x  *  x  AMD P8259A PIC
     0   0   x   x    0   x   1   1    0   1  0  x   x  x  x  x   x  x  x  *  LEDS, dipswitches, and UPD77P20 control lines
     0   0   x   x    0   x   1   1    0   1  1  x   x  x  x  x   x  x  *  x  UPD77P20 data/status
     0   0   x   x    0   x   1   1    1   x  x                               Open bus, verified (returns 0x00EA)
     0   0   x   x    1   x                                                   Open bus? (or maybe communication with multibus connector?) (returns 0xFA,B,C,FFF)
     0   1                                                                    Open bus, verified (returns 0x00EA)
     1   0                                                                    Open bus, verified (returns 0x00EA)
     1   1   0   *    *   *   *   *    *   *  *  *   *  *  *  *   *  *  *  s  ROMs 2 and 3
     1   1   1   *    *   *   *   *    *   *  *  *   *  *  *  *   *  *  *  s  ROMs 0 and 1
*/
static ADDRESS_MAP_START(i8086_mem, AS_PROGRAM, 16, tsispch_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x02FFF) AM_MIRROR(0x34000) AM_RAM // verified; 6264*2 sram, only first 3/4 used
	AM_RANGE(0x03000, 0x03001) AM_MIRROR(0x341FC) AM_DEVREADWRITE8("i8251a_u15", i8251_device, data_r, data_w, 0x00FF)
	AM_RANGE(0x03002, 0x03003) AM_MIRROR(0x341FC) AM_DEVREADWRITE8("i8251a_u15", i8251_device, status_r, control_w, 0x00FF)
	AM_RANGE(0x03200, 0x03203) AM_MIRROR(0x341FC) AM_DEVREADWRITE8("pic8259", pic8259_device, read, write, 0x00FF) // AMD P8259 PIC @ U5 (reads as 04 and 7c, upper byte is open bus)
	AM_RANGE(0x03400, 0x03401) AM_MIRROR(0x341FE) AM_READ8(dsw_r, 0x00FF) // verified, read from dipswitch s4
	AM_RANGE(0x03400, 0x03401) AM_MIRROR(0x341FE) AM_WRITE8(peripheral_w, 0xFF00) // verified, write to the 4 leds, plus 4 control bits
	AM_RANGE(0x03600, 0x03601) AM_MIRROR(0x341FC) AM_READWRITE(dsp_data_r, dsp_data_w) // verified; UPD77P20 data reg r/w
	AM_RANGE(0x03602, 0x03603) AM_MIRROR(0x341FC) AM_READWRITE(dsp_status_r, dsp_status_w) // verified; UPD77P20 status reg r
	AM_RANGE(0xc0000, 0xfffff) AM_ROM // verified
ADDRESS_MAP_END

// Technically the IO line of the i8086 is completely ignored (it is running in 8086 MIN mode,I believe, which may ignore IO)
static ADDRESS_MAP_START(i8086_io, AS_IO, 16, tsispch_state)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START(dsp_prg_map, AS_PROGRAM, 32, tsispch_state)
	AM_RANGE(0x0000, 0x01ff) AM_ROM AM_REGION("dspprg", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(dsp_data_map, AS_DATA, 16, tsispch_state)
	AM_RANGE(0x0000, 0x01ff) AM_ROM AM_REGION("dspdata", 0)
ADDRESS_MAP_END


/******************************************************************************
 Input Ports
******************************************************************************/
static INPUT_PORTS_START( prose2k )
PORT_START("s4") // dipswitch array s4
	PORT_DIPNAME( 0x01, 0x00, "S4-1") PORT_DIPLOCATION("SW4:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "S4-2") PORT_DIPLOCATION("SW4:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "S4-3") PORT_DIPLOCATION("SW4:3")
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "S4-4") PORT_DIPLOCATION("SW4:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "S4-5") PORT_DIPLOCATION("SW4:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "S4-6") PORT_DIPLOCATION("SW4:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "S4-7: Self Test") PORT_DIPLOCATION("SW4:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "S4-8") PORT_DIPLOCATION("SW4:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************
 Machine Drivers
******************************************************************************/
static MACHINE_CONFIG_START( prose2k, tsispch_state )
	/* basic machine hardware */
	/* There are two crystals on the board: a 24MHz xtal at Y2 and a 16MHz xtal at Y1 */
	MCFG_CPU_ADD("maincpu", I8086, 8000000) /* VERIFIED clock, unknown divider */
	MCFG_CPU_PROGRAM_MAP(i8086_mem)
	MCFG_CPU_IO_MAP(i8086_io)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DEVICE("pic8259", pic8259_device, inta_cb)

	/* TODO: the UPD7720 has a 10KHz clock to its INT pin */
	/* TODO: the UPD7720 has a 2MHz clock to its SCK pin */
	/* TODO: hook up p0, p1, int */
	MCFG_CPU_ADD("dsp", UPD7725, 8000000) /* VERIFIED clock, unknown divider; correct dsp type is UPD77P20 */
	MCFG_CPU_PROGRAM_MAP(dsp_prg_map)
	MCFG_CPU_DATA_MAP(dsp_data_map)

	/* PIC 8259 */
	MCFG_PIC8259_ADD("pic8259", INPUTLINE("maincpu", 0), VCC, NULL)

	/* uarts */
	MCFG_DEVICE_ADD("i8251a_u15", I8251, 0)
	// (todo: proper hookup, currently using hack w/i8251_receive_character())
	MCFG_I8251_RXRDY_HANDLER(WRITELINE(tsispch_state, i8251_rxrdy_int))
	MCFG_I8251_TXRDY_HANDLER(WRITELINE(tsispch_state, i8251_txrdy_int))
	MCFG_I8251_TXEMPTY_HANDLER(WRITELINE(tsispch_state, i8251_txempty_int))

	/* sound hardware */
	//MCFG_SPEAKER_STANDARD_MONO("mono")
	//MCFG_SOUND_ADD("dac", DAC, 0) /* TODO: correctly figure out how the DAC works; apparently it is connected to the serial output of the upd7720, which will be "fun" to connect up */
	//MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MCFG_DEVICE_ADD(TERMINAL_TAG, GENERIC_TERMINAL, 0)
	MCFG_GENERIC_TERMINAL_KEYBOARD_CB(WRITE8(tsispch_state, i8251_rxd))
MACHINE_CONFIG_END

/******************************************************************************
 ROM Definitions
******************************************************************************/
ROM_START( prose2k )
	ROM_REGION(0x100000,"maincpu", 0)
	// prose 2000/2020 firmware version 3.4.1
	ROMX_LOAD( "v3.4.1__2000__2.u22",   0xc0000, 0x10000, CRC(201D3114) SHA1(549EF1AA28D5664D4198CBC1826B31020D6C4870),ROM_SKIP(1))
	ROMX_LOAD( "v3.4.1__2000__3.u45",   0xc0001, 0x10000, CRC(190C77B6) SHA1(2B90B3C227012F2085719E6283DA08AFB36F394F),ROM_SKIP(1))
	ROMX_LOAD( "v3.4.1__2000__0.u21",   0xe0000, 0x10000, CRC(3FAE874A) SHA1(E1D3E7BA309B29A9C3EDBE3D22BECF82EAE50A31),ROM_SKIP(1))
	ROMX_LOAD( "v3.4.1__2000__1.u44",   0xe0001, 0x10000, CRC(BDBB0785) SHA1(6512A8C2641E032EF6BB0889490D82F5D4399575),ROM_SKIP(1))

	// TSI/Speech plus DSP firmware v3.12 8/9/88, NEC UPD77P20
	ROM_REGION( 0x600, "dspprgload", 0) // packed 24 bit data
	ROM_LOAD( "v3.12__8-9-88__dsp_prog.u29", 0x0000, 0x0600, CRC(9E46425A) SHA1(80A915D731F5B6863AEEB448261149FF15E5B786))
	ROM_REGION( 0x800, "dspprg", ROMREGION_ERASEFF) // for unpacking 24 bit data into 32 bit data which cpu core can understand
	ROM_REGION( 0x400, "dspdata", 0)
	ROM_LOAD( "v3.12__8-9-88__dsp_data.u29", 0x0000, 0x0400, CRC(F4E4DD16) SHA1(6E184747DB2F26E45D0E02907105FF192E51BABA))

	// mapping proms:
	// All are am27s19 32x8 TriState PROMs (equivalent to 82s123/6331)
	// L - always low; H - always high
	// U77: unknown (what does this do? likely as to do with multibus and possibly waitstates?)
	//      input is A19 for I4, A18 for I3, A15 for I2, A13 for I1, A12 for I0
	//      output bits 0bLLLLzyxH (TODO: recheck)
	//      bit - function
	//      7, 6, 5, 4 - seem unconnected?
	//      3 - connection unknown, low in the RAM, ROM, and Peripheral memory areas
	//      2 - connection unknown, low in the RAM and ROM memory areas
	//      1 - unknown, always high (TODO: recheck)
	//      0 - unknown, always high
	//
	// U79: SRAM and peripheral mapping:
	//      input is A19 for I4, A18 for I3, A15 for I2, A13 for I1, A12 for I0, same as U77
	//      On the Prose 2000 later board dumped, only bits 3 and 0 are used;
	//      bits 7-4 are always low, bits 2 and 1 are always high.
	//      SRAMS are only populated in U61 and U64.
	//      On the Prose 2000 earlier board dumped, bits 3,2,1,0 are all used;
	//      bits 7-4 are always low. sram is in 6 6116s, mapped the same as the 2 6264s on the later board.
	//      output bits 0bLLLL3210
	//      7,6,5,4 - seem unconnected?
	//      3 - to /EN3 (pin 4) of 74S138N at U80
	//          AND to EN1 (pin 6) of 74S138N at U78
	//          i.e. one is activated when pin is high and other when pin is low
	//          The 74S138N at U80: [*ENABLED ONLY WITHIN/CONTROLS THE 3000-3FFF AREA*]
	//              /EN2 - pulled to GND
	//              EN1 - pulled to VCC through resistor R5
	//              inputs: S0 - A9; S1 - A10; S2 - A11
	//              /Y0 - /CS (pin 11) of iP8251A at U15 [0x3000-0x31FF]
	//              /Y1 - /CS (pin 1) of AMD 8259A at U4 [0x3200-0x33FF]
	//              /Y2 - pins 1, 4, 9 (1A, 2A, 3A inputs) of 74HCT32 Quad OR gate at U58 [0x3400-0x35FF]
	//              /Y3 - pin 26 (/CS) of UPD77P20 at U29 [0x3600-0x37FF]
	//              /Y4 through /Y7 - seem unconnected? [0x3800-0x3FFF]
	//          The 74S138N at U78: [*ENABLED IN ALL AREAS EXCEPT 3000-3FFF*] <wip>
	//              /EN3 - ? (TODO: figure these out)
	//              /EN2 - ?
	//              inputs: S0 - A18; S1 - A19; S2 - Pulled to GND
	//              /Y0 - ?
	//              /Y1 - ?
	//              /Y2 - ?
	//              /Y3 - connects somewhere, only active when A18 and A19 are high, possibly a rom bus buffer enable? (TODO: figure out what this does)
	//              /Y4-/Y7 - never used since S2 is pulled to GND
	//      2 - to /CS1 on 6264 SRAMs at U63 and U66
	//      1 - to /CS1 on 6264 SRAMs at U62 and U65
	//      0 - to /CS1 on 6264 SRAMs at U61 and U64
	//
	// U81: (OPTIONAL) maps ROMS: input is A19-A15 for I4,3,2,1,0
	//      On the Prose 2000 board dumped, only bits 6 and 5 are used,
	//      the rest are always high; maps roms 0,1,2,3 to C0000-FFFFF.
	//      The Prose 2000 board has empty unpopulated sockets for roms 4-15;
	//      if present these would be driven by a different prom in this location.
	//      bit - function
	//      7 - to /CE of ROMs 14(U28) and 15(U51)
	//      6 - to /CE of ROMs 0(U21) and 1(U44)
	//      5 - to /CE of ROMs 2(U22) and 3(U45)
	//      4 - to /CE of ROMs 4(U23) and 5(U46)
	//      3 - to /CE of ROMs 6(U24) and 7(U47)
	//      2 - to /CE of ROMs 8(U25) and 9(U48)
	//      1 - to /CE of ROMs 10(U26) and 11(U49)
	//      0 - to /CE of ROMs 12(U27) and 13(U50)
	//
	// Note U81 is optional; it can be replaced by a 74s138 instead of a prom,
	// with A19, A18, A17 as inputs, for decoding the roms as:      
	//      7 - to /CE of ROMs 0(U21) and 1(U44)   (0xE0000-0xF1FFF)
	//      6 - to /CE of ROMs 2(U22) and 3(U45)   (0xE4000-0xF3FFF)
	//      5 - to /CE of ROMs 4(U23) and 5(U46)   (0xE8000-0xF5FFF)
	//      4 - to /CE of ROMs 6(U24) and 7(U47)   (0xEC000-0xF7FFF)
	//      3 - to /CE of ROMs 8(U25) and 9(U48)   (0xF0000-0xF9FFF)
	//      2 - to /CE of ROMs 10(U26) and 11(U49) (0xF4000-0xFBFFF)
	//      1 - to /CE of ROMs 12(U27) and 13(U50) (0xF8000-0xFDFFF)
	//      0 - to /CE of ROMs 14(U28) and 15(U51) (0xFC000-0xFFFFF)                                               
	
	ROM_REGION(0x1000, "proms", 0)
	ROM_LOAD( "am27s19.u77", 0x0000, 0x0020, CRC(A88757FC) SHA1(9066D6DBC009D7A126D75B8461CA464DDF134412))
	ROM_LOAD( "am27s19.u79", 0x0020, 0x0020, CRC(A165B090) SHA1(BFC413C79915C68906033741318C070AD5DD0F6B))
	ROM_LOAD( "am27s19.u81", 0x0040, 0x0020, CRC(62E1019B) SHA1(ACADE372EDB08FD0DCB1FA3AF806C22C47081880))
	ROM_END

ROM_START( prose2ko )
	// 'Older' prose2k set
	ROM_REGION(0x100000,"maincpu", 0)
	// prose 2000 firmware version 1.1
	ROMX_LOAD( "v1.1__6__speech__plus__(c)1983.am2764.6.u24",   0xec000, 0x2000, CRC(C881F92D) SHA1(2D4EB96360ADAC54D4F0110595BFAF682280C1CA),ROM_SKIP(1))
	ROMX_LOAD( "v1.1__7__speech__plus__(c)1983.am2764.7.u47",   0xec001, 0x2000, CRC(4D5771CB) SHA1(55ED59AD1CAD154804DBEEEBED98F062783C33C3),ROM_SKIP(1))
	ROMX_LOAD( "v1.1__8__speech__plus__(c)1983.am2764.8.u25",   0xf0000, 0x2000, CRC(ADF9BFB8) SHA1(0B73561B52B388B740FABF07ADA2D70A52F22037),ROM_SKIP(1))
	ROMX_LOAD( "v1.1__9__speech__plus__(c)1983.am2764.9.u48",   0xf0001, 0x2000, CRC(355F97D2) SHA1(7655FC55B577821E0BD8BF81FB74B8A20B1DF098),ROM_SKIP(1))
	ROMX_LOAD( "v1.1__10__speech__plus__(c)1983.am2764.10.u26", 0xf4000, 0x2000, CRC(949A0344) SHA1(8E33C69DFC413AEA95F166B08902AD97B1E3E980),ROM_SKIP(1))
	ROMX_LOAD( "v1.1__11__speech__plus__(c)1983.am2764.11.u49", 0xf4001, 0x2000, CRC(AD9A0670) SHA1(769F2F8696C7B6907706466AA9AB7A897ED9F889),ROM_SKIP(1))
	ROMX_LOAD( "v1.1__12__speech__plus__(c)1983.am2764.12.u27", 0xf8000, 0x2000, CRC(9EAF9378) SHA1(D296B1D347C03E6123C38C208EAD25B1F43B9859),ROM_SKIP(1))
	ROMX_LOAD( "v1.1__13__speech__plus__(c)1983.am2764.13.u50", 0xf8001, 0x2000, CRC(5E173667) SHA1(93230C2FEDE5095F56E10D20EA36A5A45A1E7356),ROM_SKIP(1))
	ROMX_LOAD( "v1.1__14__speech__plus__(c)1983.am2764.14.u28", 0xfc000, 0x2000, CRC(E616BD6E) SHA1(5DFAE2C5079D89F791C9D7166F9504231A464203),ROM_SKIP(1))
	ROMX_LOAD( "v1.1__15__speech__plus__(c)1983.am2764.15.u51", 0xfc001, 0x2000, CRC(BEB1FA19) SHA1(72130FE45C3FD3DE7CF794936DC68ED2D4193DAF),ROM_SKIP(1))

	// TSI/Speech plus DSP firmware v?.? (no sticker, but S140025 printed on chip), unlabeled chip, but clearly a NEC UPD77P20C ceramic
	// NOT DUMPED YET, using the 3.12 dsp firmware as a placeholder
	ROM_REGION( 0x600, "dspprgload", 0) // packed 24 bit data
	ROM_LOAD( "s140025__dsp_prog.u29", 0x0000, 0x0600, BAD_DUMP CRC(9E46425A) SHA1(80A915D731F5B6863AEEB448261149FF15E5B786))
	ROM_REGION( 0x800, "dspprg", ROMREGION_ERASEFF) // for unpacking 24 bit data into 32 bit data which cpu core can understand
	ROM_REGION( 0x400, "dspdata", 0)
	ROM_LOAD( "s140025__dsp_data.u29", 0x0000, 0x0400, BAD_DUMP CRC(F4E4DD16) SHA1(6E184747DB2F26E45D0E02907105FF192E51BABA))

	ROM_REGION(0x1000, "proms", 0)
	ROM_LOAD( "dm74s288n.u77", 0x0000, 0x0020, CRC(A88757FC) SHA1(9066D6DBC009D7A126D75B8461CA464DDF134412)) // == am27s19.u77
	ROM_LOAD( "dm74s288n.whitespot.u79", 0x0020, 0x0020, CRC(7FAEE6CB) SHA1(B6DD2A6909DAC9E89E7317C006A013FF0866382D))
	// no third prom in this set, a 74S138 is used instead for e0000-fffff rom mapping
	ROM_END

/******************************************************************************
 Drivers
******************************************************************************/

/*    YEAR  NAME    PARENT  COMPAT  MACHINE     INPUT   INIT    COMPANY     FULLNAME            FLAGS */
COMP( 1985, prose2k,    0,      0,      prose2k,        prose2k, tsispch_state, prose2k,    "Telesensory Systems Inc/Speech Plus",  "Prose 2000/2020 v3.4.1",  GAME_NOT_WORKING | GAME_NO_SOUND )
COMP( 1985, prose2ko, prose2k,      0,      prose2k,        prose2k, tsispch_state, prose2k,    "Telesensory Systems Inc/Speech Plus",  "Prose 2000/2020 v1.1",  GAME_NOT_WORKING | GAME_NO_SOUND )
