// license:MAME|LGPL-2.1+
// copyright-holders:Michael Zapf
/****************************************************************************

    Myarc Hard and Floppy Disk Controller ("HFDC")

    The HFDC is based on the HDC9234 controller chip from Standard
    Microsystems Corporation (SMC). It can work with up to three MFM hard disk
    drives and up to four floppy disk drives.

    Data flow is detached from the main CPU. The HDC transfers data to/from
    the drives using direct memory access to attached memory circuits. That
    is, to write a sector on a drive the CPU must set up the contents in the
    memory, then initiate a sector write operation.

    The advantage is a much higher data rate (in particular important when
    working with hard disks) with less load for the main CPU. Also, we do not
    need a READY line control (as seen with the WD17xx-based controllers).
    Any kinds of asynchronous events are propagated via INTA* (configurable
    to INTB*).

    Most of the control logic is hidden in the custom Gate Array chip. We do
    not have details on its contents, but the specifications in the HFDC manual
    and in the schematics are sufficient to create a (functionally) proper
    emulation.

    The HDC9234 can also control tape drives. In early HFDC controller card
    layouts, a socket for connecting a drive is available. However, there
    never was a support from the DSR (firmware), so this feature was eliminated
    in later releases.

    Components

    HDC 9234      - Universal Disk Controller
    FDC 9216      - Floppy disk data separator (8 MHz, divider is set by CD0 and CD1)
    HDC 92C26     - MFM hard disk data separator (10 MHz, also used for 9234)
    HDC 9223      - Analog data separator support
    DS 1000-50    - Delay circuit
    MM 58274BN    - Real time clock
    HM 6264-LP15  - SRAM 8K x 8   (may also be 32K x 8)
    27C128        - EPROM 16K x 8

    Author: Michael Zapf
    September 2014: Rewritten for modern floppy implementation

    WORK IN PROGRESS

*****************************************************************************/

/*
    FIXME: HFDC does not work at CRU addresses other than 1100
    (test shows
        hfdc: reado 41f5 (00): 00
        hfdc: reado 41f4 (00): 00 -> wrong rom page? (should be (02)))
*/

#include "emu.h"
#include "peribox.h"
#include "hfdc.h"
#include "machine/ti99_hd.h"
#include "formats/ti99_dsk.h"       // Format

#define BUFFER "ram"
#define FDC_TAG "hdc9234"
#define CLOCK_TAG "mm58274c"

#define MOTOR_TIMER 1

#define TAPE_ADDR   0x0fc0
#define HDC_R_ADDR  0x0fd0
#define HDC_W_ADDR  0x0fd2
#define CLK_ADDR    0x0fe0
#define RAM_ADDR    0x1000

#define TRACE_EMU 0
#define TRACE_CRU 0
#define TRACE_COMP 0
#define TRACE_RAM 0
#define TRACE_ROM 0
#define TRACE_LINES 0
#define TRACE_MOTOR 0
#define TRACE_DMA 0
#define TRACE_INT 0

// =========================================================================

/*
    Modern implementation.
*/
myarc_hfdc_device::myarc_hfdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: ti_expansion_card_device(mconfig, TI99_HFDC, "Myarc Hard and Floppy Disk Controller", tag, owner, clock, "ti99_hfdc_new", __FILE__),
		m_hdc9234(*this, FDC_TAG),
		m_clock(*this, CLOCK_TAG)
{
}


SETADDRESS_DBIN_MEMBER( myarc_hfdc_device::setaddress_dbin )
{
	// Selection login in the PAL and some circuits on the board

	// Is the card being selected?
	// Area = 4000-5fff
	// 010x xxxx xxxx xxxx
	m_address = offset;
	m_inDsrArea = ((m_address & m_select_mask)==m_select_value);

	if (!m_inDsrArea) return;

	// Is the HDC chip on the card being selected?
	// HDC9234: read: 4fd0,4 (mirror 8,c)
	// HDC9234: write: 4fd2,6 (mirror a,e)
	// read:  ...0 1111 1101 xx00
	// write: ...0 1111 1101 xx10

	m_HDCsel = m_inDsrArea &&
				((state==ASSERT_LINE && ((m_address & 0x1ff3)==HDC_R_ADDR))    // read
				|| (state==CLEAR_LINE && ((m_address & 0x1ff3)==HDC_W_ADDR)));  // write

	// Is the tape selected?
	// ...0 1111 1100 xxxx
	m_tapesel = m_inDsrArea && ((m_address & 0x1ff0)==TAPE_ADDR);

	// Is the RTC selected on the card? (even addr)
	// ...0 1111 111x xxx0
	m_RTCsel = m_inDsrArea && ((m_address & 0x1fe1)==CLK_ADDR);

	// Is RAM selected?
	// ...1 xxxx xxxx xxxx
	m_RAMsel = m_inDsrArea && ((m_address & 0x1000)==RAM_ADDR);

	// Is ROM selected?
	// not 0100 1111 11xx xxxx
	m_ROMsel = m_inDsrArea && !m_RAMsel && !((m_address & 0x0fc0)==0x0fc0);
}

/*
    Read a byte from the memory address space of the HFDC

    0x4000 - 0x4fbf one of four possible ROM pages
    0x4fc0 - 0x4fcf Tape control (only available in prototype HFDC models)
    0x4fd0 - 0x4fdf HDC 9234 ports
    0x4fe0 - 0x4fff RTC chip ports

    0x5000 - 0x53ff static RAM page 0x10
    0x5400 - 0x57ff static RAM page any of 32 pages
    0x5800 - 0x5bff static RAM page any of 32 pages
    0x5c00 - 0x5fff static RAM page any of 32 pages

    HFDC manual, p. 44
*/
READ8Z_MEMBER(myarc_hfdc_device::readz)
{
	if (m_inDsrArea && m_selected)
	{
		if (m_tapesel)
		{
			logerror("%s: Tape support not available on this HFDC version (access to address %04x)\n", tag(), m_address & 0xffff);
			return;
		}

		if (m_HDCsel)
		{
			if (!space.debugger_access()) *value = m_hdc9234->read(space, (m_address>>2)&1, 0xff);
			if (TRACE_COMP) logerror("%s: %04x[HDC] -> %02x\n", tag(), m_address & 0xffff, *value);
			return;
		}

		if (m_RTCsel)
		{
			if (!space.debugger_access()) *value = m_clock->read(space, (m_address & 0x001e) >> 1);
			if (TRACE_COMP) logerror("%s: %04x[CLK] -> %02x\n", tag(), m_address & 0xffff, *value);
			return;
		}

		if (m_RAMsel)
		{
			// 0101 00xx xxxx xxxx  static 0x08
			// 0101 01xx xxxx xxxx  bank 1
			// 0101 10xx xxxx xxxx  bank 2
			// 0101 11xx xxxx xxxx  bank 3
			int bank = (m_address & 0x0c00) >> 10;
			*value = m_buffer_ram[(m_ram_page[bank]<<10) | (m_address & 0x03ff)];
			if (TRACE_RAM)
			{
				if ((m_address & 1)==0)  // only show even addresses with words
				{
					int valword = (((*value) << 8) | m_buffer_ram[(m_ram_page[bank]<<10) | ((m_address+1) & 0x03ff)])&0xffff;
					logerror("%s: %04x[%02x] -> %04x\n", tag(), m_address & 0xffff, m_ram_page[bank], valword);
				}
			}
			return;
		}

		if (m_ROMsel)
		{
			*value = m_dsrrom[(m_rom_page << 12) | (m_address & 0x0fff)];
			if (TRACE_ROM)
			{
				if ((m_address & 1)==0)  // only show even addresses with words
				{
					int valword = (((*value) << 8) | m_dsrrom[(m_rom_page << 12) | ((m_address + 1) & 0x0fff)])&0xffff;
					logerror("%s: %04x[%02x] -> %04x\n", tag(), m_address & 0xffff, m_rom_page, valword);
				}
			}
			return;
		}
	}
}

/*
    Write a byte to the memory address space of the HFDC

    0x4fc0 - 0x4fcf Tape control (only available in prototype HFDC models)
    0x4fd0 - 0x4fdf HDC 9234 ports
    0x4fe0 - 0x4fff RTC chip ports

    0x5000 - 0x53ff static RAM page 0x08
    0x5400 - 0x57ff static RAM page any of 32 pages
    0x5800 - 0x5bff static RAM page any of 32 pages
    0x5c00 - 0x5fff static RAM page any of 32 pages
*/
WRITE8_MEMBER( myarc_hfdc_device::write )
{
	if (m_inDsrArea && m_selected)
	{
		if (m_tapesel)
		{
			logerror("%s: Tape support not available on this HFDC version (write access to address %04x: %02x)\n", tag(), m_address & 0xffff, data);
			return;
		}

		if (m_HDCsel)
		{
			if (TRACE_COMP) logerror("%s: %04x[HDC] <- %02x\n", tag(), m_address & 0xffff, data);
			if (!space.debugger_access()) m_hdc9234->write(space, (m_address>>2)&1, data, 0xff);
			return;
		}

		if (m_RTCsel)
		{
			if (TRACE_COMP) logerror("%s: %04x[CLK] <- %02x\n", tag(), m_address & 0xffff, data);
			if (!space.debugger_access()) m_clock->write(space, (m_address & 0x001e) >> 1, data);
			return;
		}

		if (m_RAMsel)
		{
			// 0101 00xx xxxx xxxx  static 0x08
			// 0101 01xx xxxx xxxx  bank 1
			// 0101 10xx xxxx xxxx  bank 2
			// 0101 11xx xxxx xxxx  bank 3
			int bank = (m_address & 0x0c00) >> 10;
			if (TRACE_RAM) logerror("%s: %04x[%02x] <- %02x\n", tag(), m_address & 0xffff, m_ram_page[bank], data);
			m_buffer_ram[(m_ram_page[bank]<<10) | (m_address & 0x03ff)] = data;
			return;
		}
		// The rest is ROM
		if (m_ROMsel)
		{
			if (TRACE_ROM) logerror("%s: Ignoring write ROM %04x[%02x]: %02x\n", tag(), m_address & 0xffff, m_rom_page, data);
		}
	}
}

/*
    Read a set of 8 bits in the CRU space of the HFDC
    There are two banks, according to the state of m_see_switches

    m_see_switches == true:

       7     6     5     4     3     2     1     0
    +-----+-----+-----+-----+-----+-----+-----+-----+
    |DIP1*|DIP2*|DIP3*|DIP4*|DIP5*|DIP6*|DIP7*|DIP8*|
    +-----+-----+-----+-----+-----+-----+-----+-----+

    MZ: The setting 00 (all switches on) is a valid setting according to the
        HFDC manual and indicates 36 sectors/track, 80 tracks; however, this
        setting is intended "for possible future expansion" and cannot fall
        back to lower formats, hence, single density disks cannot be read.
    ---

    m_see_switches == false:

       7     6     5     4     3     2     1     0
    +-----+-----+-----+-----+-----+-----+-----+-----+
    |  0  |  0  |  0  |  0  |  0  | MON | DIP | IRQ |
    +-----+-----+-----+-----+-----+-----+-----+-----+

    MON = Motor on
    DIP = DMA in progress
    IRQ = Interrupt request
    ---
    0 on all other locations
*/
READ8Z_MEMBER(myarc_hfdc_device::crureadz)
{
	UINT8 reply;
	if ((offset & 0xff00)==m_cru_base)
	{
		if ((offset & 0x00ff)==0)  // CRU bits 0-7
		{
			if (m_see_switches)
			{
				// DIP switches.  Logic levels are inverted (on->0, off->1).  CRU
				// bit order is the reverse of DIP-switch order, too (dip 1 -> bit 7,
				// dip 8 -> bit 0).
				// MZ: 00 should not be used since there is a bug in the
				// DSR of the HFDC which causes problems with SD disks
				// (controller tries DD and then fails to fall back to SD) */
				reply = ~(ioport("HFDCDIP")->read());
			}
			else
			{
				reply = 0;
				if (m_irq == ASSERT_LINE)  reply |= 0x01;
				if (m_dip == ASSERT_LINE)  reply |= 0x02;
				if (m_motor_running) reply |= 0x04;
			}
			*value = reply;
		}
		else   // CRU bits 8+
		{
			*value = 0;
		}

		if (TRACE_CRU) logerror("%s: CRU %04x -> %02x\n", tag(), offset & 0xffff, *value);
	}
}

/*
    Set a bit in the CRU space of the HFDC

       7     6     5     4     3     2     1     0
    +-----+-----+-----+-----+-----+-----+-----+-----+
    |  0  | MON | DIP | ROM1| ROM0| MON | RES*| SEL |
    |     |     |     | CSEL| CD1 | CD0 |     |     |
    +-----+-----+-----+-----+-----+-----+-----+-----+

       17    16    15    14    13    12    11    10    F     E     D     C     B     A     9     8
    +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
    |    RAM page select @5C00    |    RAM page select @5800    |     RAM page select @5400   |  -  |
    +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+

    SEL  = Select card (and map ROM into address space)
    RES* = Reset controller
    MON  = Motor on / same line goes to CD0 input of floppy separator
    ROM bank select: bank 0..3; bit 3 = MSB, 4 = LSB
    RAM bank select: bank 0..31; bit 9 = LSB (accordingly for other two areas)
    CD0 and CD1 are Clock Divider selections for the Floppy Data Separator (FDC9216)
    CSEL = CRU input select (m_see_switches)

    HFDC manual p. 43
*/
WRITE8_MEMBER(myarc_hfdc_device::cruwrite)
{
	if ((offset & 0xff00)==m_cru_base)
	{
		if (TRACE_CRU) logerror("%s: CRU %04x <- %d\n", tag(), offset & 0xffff, data);

		int bit = (offset >> 1) & 0x1f;

		// Handle the page selects right here
		if (bit >= 0x09 && bit < 0x18)
		{
			if (data)
				// we leave index 0 unchanged; modify indices 1-3
				m_ram_page[(bit-4)/5] |= 1 << ((bit-9)%5);
			else
				m_ram_page[(bit-4)/5] &= ~(1 << ((bit-9)%5));

			if (TRACE_CRU)
			{
				if (bit==0x0d) logerror("%s: RAM page @5400 = %d\n", tag(), m_ram_page[1]);
				if (bit==0x12) logerror("%s: RAM page @5800 = %d\n", tag(), m_ram_page[2]);
				if (bit==0x17) logerror("%s: RAM page @5C00 = %d\n", tag(), m_ram_page[3]);
			}
			return;
		}

		switch (bit)
		{
		case 0:
			{
				bool turnOn = (data!=0);
				// Avoid too many meaningless log outputs
				if (TRACE_CRU) if (m_selected != turnOn) logerror("%s: card %s\n", tag(), turnOn? "selected" : "unselected");
				m_selected = turnOn;
				break;
			}
		case 1:
			if (TRACE_CRU) if (data==0) logerror("%s: trigger HDC reset\n", tag());
			m_hdc9234->reset((data == 0)? ASSERT_LINE : CLEAR_LINE);
			break;

		case 2:
			m_CD = (data != 0)? (m_CD | 1) : (m_CD & 0xfe);

			// Activate motor
			// When 1, let motor run continuously. When 0, a simple monoflop circuit keeps the line active for another 4 sec
			// We keep triggering the monoflop for data==1
			if (data==1)
			{
				if (TRACE_CRU) logerror("%s: trigger motor\n", tag());
				set_floppy_motors_running(true);
			}
			m_lastval = data;
			break;

		case 3:
			m_CD = (data != 0)? (m_CD | 2) : (m_CD & 0xfd);
			m_rom_page = (data != 0)? (m_rom_page | 2) : (m_rom_page & 0xfd);
			if (TRACE_CRU) logerror("%s: ROM page = %d, CD = %d\n", tag(), m_rom_page, m_CD);
			break;

		case 4:
			m_see_switches = (data != 0);
			m_rom_page = (data != 0)? (m_rom_page | 1) : (m_rom_page & 0xfe);
			if (TRACE_CRU) logerror("%s: ROM page = %d, see_switches = %d\n", tag(), m_rom_page, m_see_switches);
			break;

		default:
			logerror("%s: Attempt to set undefined CRU bit %d\n", tag(), bit);
		}
	}
}

/*
    Monoflop has gone back to the OFF state.
*/
void myarc_hfdc_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	set_floppy_motors_running(false);
}

/*
    This is called back from the floppy when an index hole is passing by.
*/
void myarc_hfdc_device::floppy_index_callback(floppy_image_device *floppy, int state)
{
	if (TRACE_LINES) if (state==1) logerror("%s: Index pulse\n", tag());
	// m_status_latch = (state==ASSERT_LINE)? (m_status_latch | HDC_DS_INDEX) :  (m_status_latch & ~HDC_DS_INDEX);
	set_bits(m_status_latch, HDC_DS_INDEX, (state==ASSERT_LINE));
	signal_drive_status();
}

void myarc_hfdc_device::set_bits(UINT8& byte, int mask, bool set)
{
	if (set) byte |= mask;
	else byte &= ~mask;
}

/*
    Calculate the logarithm. This is needed to determine the index of a drive.
    Returns -1 for value=0
*/
int myarc_hfdc_device::slog2(int value)
{
	int i=-1;
	while (value!=0)
	{
		value >>= 1;
		i++;
	}
	return i;
}

/*
    Notify the controller about the status change
*/
void myarc_hfdc_device::signal_drive_status()
{
	UINT8 reply = 0;
	// Status byte as defined by HDC9234
	// +------+------+------+------+------+------+------+------+
	// | ECC  |Index | SeekC| Tr00 | User | WrPrt| Ready|Fault |
	// +------+------+------+------+------+------+------+------+
	//
	// Set by HFDC
	// 74LS240 is used for driving the lines; it also inverts the inputs
	// If no hard drive or floppy is connected, all lines are 0
	// +------+------+------+------+------+------+------+------+
	// |  0   | Index| SeekC| Tr00 |   0  | WrPrt| Ready|Fault |
	// +------+------+------+------+------+------+------+------+
	//
	//  Ready = /WDS.ready* | DSK
	//  SeekComplete = /WDS.seekComplete* | DSK

	// If DSK is selected, set Ready and SeekComplete to 1
	if ((m_output1_latch & 0x10)!=0)
	{
		reply |= 0x22;

		// Check for TRK00*
		if ((m_current_floppy != NULL) && (!m_current_floppy->trk00_r()))
			reply |= 0x10;
	}

	// If WDS is selected but not connected, WDS.ready* and WDS.seekComplete* are 1, so Ready=SeekComplete=0
	reply |= m_status_latch;

	m_hdc9234->auxbus_in(reply);
}

/*
    When the HDC outputs a byte via its AB (auxiliary bus), we need to latch it.
    The target of the transfer is determined by two control lines (S1,S0).
    (0,0) = input drive status
    (0,1) = DMA address
    (1,0) = OUTPUT1
    (1,1) = OUTPUT2
*/
WRITE8_MEMBER( myarc_hfdc_device::auxbus_out )
{
	int index;
	switch (offset)
	{
	case HDC_INPUT_STATUS:
		logerror("%s: Invalid operation: S0=S1=0, but tried to write (expected: read drive status)\n", tag());
		break;

	case HDC_OUTPUT_DMA_ADDR:
		// Value is dma address byte. Shift previous contents to the left.
		// The value is latched inside the Gate Array.
		m_dma_address = ((m_dma_address << 8) + (data&0xff))&0xffffff;
		if (TRACE_DMA) logerror("%s: Setting DMA address; current value = %06x\n", tag(), m_dma_address);
		break;

	case HDC_OUTPUT_1:
		// value is output1
		// The HFDC interprets the value as follows:
		// WDS = Winchester Drive System, old name for hard disk
		// +------+------+------+------+------+------+------+------+
		// | WDS3 | WDS2 | WDS1 | DSKx | x=4  | x=3  | x=2  | x=1  |
		// +------+------+------+------+------+------+------+------+
		// Accordingly, drive 0 is always the floppy; selected by the low nibble

		m_output1_latch = data;

		if ((data & 0x10) != 0)
		{
			// Floppy selected
			connect_floppy_unit(slog2(data & 0x0f));
		}
		else
		{
			index = slog2((data>>4) & 0x0f);
			if (index == -1)
			{
				if (TRACE_LINES) logerror("%s: Unselect all HD drives\n", tag());
				connect_floppy_unit(index);
			}
			else
			{
				// HD selected
				if (TRACE_LINES) logerror("%s: Select hard disk WDS%d\n", tag(), index);
				//          if (index>=0) m_hdc9234->connect_hard_drive(m_harddisk_unit[index-1]);
			}
			if (m_current_harddisk==NULL)
			{
				set_bits(m_status_latch, HDC_DS_READY | HDC_DS_SKCOM, false);
			}
		}
		break;

	case HDC_OUTPUT_2:
		// value is output2
		// DS3* = /WDS3
		// WCur = Reduced Write Current
		// Dir = Step direction
		// Step = Step pulse
		// Head = Selected head number (floppy: 0000 or 0001)
		// +------+------+------+------+------+------+------+------+
		// | DS3* | WCur | Dir  | Step |           Head            |
		// +------+------+------+------+------+------+------+------+
		m_output2_latch = data;

		// Output the step pulse to the selected floppy drive
		if (m_current_floppy != NULL)
		{
			m_current_floppy->ss_w(data & 0x01);
			m_current_floppy->dir_w((data & 0x20)==0);
			m_current_floppy->stp_w((data & 0x10)==0);
		}

		// We are pushing the drive status after OUTPUT2
		signal_drive_status();
		break;
	}
}

enum
{
	HFDC_FLOPPY = 1,
	HFDC_HARDDISK = 2
};

void myarc_hfdc_device::connect_floppy_unit(int index)
{
	// Check if we have a new floppy
	if (index>=0)
	{
		if (m_floppy_unit[index] != m_current_floppy)
		{
			if (TRACE_LINES) logerror("%s: Select floppy drive DSK%d\n", tag(), index+1);
			if (m_current_floppy != NULL)
			{
				// Disconnect old drive from index line
				if (TRACE_LINES) logerror("%s: Disconnect previous index callback DSK%d\n", tag(), index+1);
				m_current_floppy->setup_index_pulse_cb(floppy_image_device::index_pulse_cb());
			}
			// Connect new drive
			m_current_floppy = m_floppy_unit[index];

			// We don't use the READY line of floppy drives.
			// READY is asserted when DSKx = 1
			// The controller fetches the state with the auxbus access
			if (TRACE_LINES) logerror("%s: Connect index callback DSK%d\n", tag(), index+1);
			m_current_floppy->setup_index_pulse_cb(floppy_image_device::index_pulse_cb(FUNC(myarc_hfdc_device::floppy_index_callback), this));
			m_hdc9234->connect_floppy_drive(m_floppy_unit[index]);
		}
	}
	else
	{
		if (TRACE_LINES) logerror("%s: Unselect all floppy drives\n", tag());
		// Disconnect current floppy
		if (m_current_floppy != NULL)
		{
			m_current_floppy->setup_index_pulse_cb(floppy_image_device::index_pulse_cb());
			m_current_floppy = NULL;
		}
	}
	// The drive status is supposed to be sampled after OUTPUT2
	// signal_drive_status();
}

void myarc_hfdc_device::connect_harddisk_unit(int index)
{
//  if (index < 0)
//  {
		m_current_harddisk = NULL;
//  }
	// signal_drive_status();
}

/*
    All floppy motors are operated by the same line.
*/
void myarc_hfdc_device::set_floppy_motors_running(bool run)
{
	if (run)
	{
		if (TRACE_MOTOR)
			if (m_MOTOR_ON==CLEAR_LINE) logerror("%s: Motor START\n", tag());
		m_MOTOR_ON = ASSERT_LINE;
		m_motor_on_timer->adjust(attotime::from_msec(4230));
	}
	else
	{
		if (TRACE_MOTOR)
			if (m_MOTOR_ON==ASSERT_LINE) logerror("%s: Motor STOP\n", tag());
		m_MOTOR_ON = CLEAR_LINE;
	}

	// Set all motors
	for (int i=0; i < 4; i++)
		if (m_floppy_unit[i] != NULL) m_floppy_unit[i]->mon_w((run)? 0 : 1);
}

/*
    Called whenever the state of the HDC9234 interrupt pin changes.
*/
WRITE_LINE_MEMBER( myarc_hfdc_device::intrq_w )
{
	m_irq = (line_state)state;
	if (TRACE_INT) logerror("%s: INT pin from controller = %d, propagating to INTA*\n", tag(), state);

	// Set INTA*
	// Signal from SMC is active high, INTA* is active low; board inverts signal
	// Anyway, we stay with ASSERT_LINE and CLEAR_LINE
	m_slot->set_inta(state);
}

/*
    Called whenever the HDC9234 desires bus access to the buffer RAM. The
    controller expects a call to dmarq in 1 byte time.
*/
WRITE_LINE_MEMBER( myarc_hfdc_device::dmarq_w )
{
	if (TRACE_DMA) logerror("%s: DMARQ pin from controller = %d\n", tag(), state);
	if (state == ASSERT_LINE)
	{
		m_hdc9234->dmaack(ASSERT_LINE);
	}
}

/*
    Called whenever the state of the HDC9234 DMA in progress changes.
*/
WRITE_LINE_MEMBER( myarc_hfdc_device::dip_w )
{
	m_dip = (line_state)state;
}

/*
    Read a byte from the onboard SRAM
*/
READ8_MEMBER( myarc_hfdc_device::read_buffer )
{
	if (TRACE_DMA) logerror("%s: Read access to onboard SRAM at %04x\n", tag(), m_dma_address);
	UINT8 value = m_buffer_ram[m_dma_address & 0x7fff];
	m_dma_address = (m_dma_address+1) & 0x7fff;
	return value;
}

/*
    Write a byte to the onboard SRAM
*/
WRITE8_MEMBER( myarc_hfdc_device::write_buffer )
{
	if (TRACE_DMA) logerror("%s: Write access to onboard SRAM at %04x: %02x\n", tag(), m_dma_address, data);
	m_buffer_ram[m_dma_address & 0x7fff] = data;
	m_dma_address = (m_dma_address+1) & 0x7fff;
}

void myarc_hfdc_device::device_start()
{
	if (TRACE_EMU) logerror("%s: start\n", tag());
	m_dsrrom = memregion(DSRROM)->base();
	m_buffer_ram = memregion(BUFFER)->base();
	m_motor_on_timer = timer_alloc(MOTOR_TIMER);
	// The HFDC does not use READY; it has on-board RAM for DMA
	m_current_floppy = NULL;
}

void myarc_hfdc_device::device_reset()
{
	if (TRACE_EMU) logerror("%s: reset\n", tag());

	// The GenMOD mod; our implementation automagically adapts all cards
	if (m_genmod)
	{
		m_select_mask = 0x1fe000;
		m_select_value = 0x174000;
	}
	else
	{
		m_select_mask = 0x7e000;
		m_select_value = 0x74000;
	}

	m_cru_base = ioport("CRUHFDC")->read();

	// Resetting values
	m_rom_page = 0;

	m_ram_page[0] = 0x08;   // static page 0x08
	for (int i=1; i < 4; i++) m_ram_page[i] = 0;

	m_output1_latch = m_output2_latch = 0;

	m_status_latch = 0x00;

	m_dip = m_irq = CLEAR_LINE;
	m_see_switches = false;
	m_CD = 0;
	m_motor_running = false;
	m_selected = false;
	m_lastval = 0;
	m_readyflags = 0;

	for (int i=0; i < 4; i++)
	{
		if (m_floppy_unit[i] != NULL)
			logerror("%s: Connector %d with %s\n", tag(), i, m_floppy_unit[i]->name());
		else
			logerror("%s: Connector %d has no floppy attached\n", tag(), i);
	}

	// Disconnect all units
	connect_floppy_unit(-1);
	connect_harddisk_unit(-1);
}

void myarc_hfdc_device::device_config_complete()
{
	for (int i=0; i < 4; i++)
		m_floppy_unit[i] = NULL;

	// Seems to be null when doing a "-listslots"
	if (subdevice("0")!=NULL) m_floppy_unit[0] = static_cast<floppy_image_device*>(subdevice("0")->first_subdevice());
	if (subdevice("1")!=NULL) m_floppy_unit[1] = static_cast<floppy_image_device*>(subdevice("1")->first_subdevice());
	if (subdevice("2")!=NULL) m_floppy_unit[2] = static_cast<floppy_image_device*>(subdevice("2")->first_subdevice());
	if (subdevice("3")!=NULL) m_floppy_unit[3] = static_cast<floppy_image_device*>(subdevice("3")->first_subdevice());
}

INPUT_PORTS_START( ti99_hfdc )
	PORT_START( "CRUHFDC" )
	PORT_DIPNAME( 0x1f00, 0x1100, "HFDC CRU base" )
		PORT_DIPSETTING( 0x1000, "1000" )
		PORT_DIPSETTING( 0x1100, "1100" )
		PORT_DIPSETTING( 0x1200, "1200" )
		PORT_DIPSETTING( 0x1300, "1300" )
		PORT_DIPSETTING( 0x1400, "1400" )
		PORT_DIPSETTING( 0x1500, "1500" )
		PORT_DIPSETTING( 0x1600, "1600" )
		PORT_DIPSETTING( 0x1700, "1700" )
		PORT_DIPSETTING( 0x1800, "1800" )
		PORT_DIPSETTING( 0x1900, "1900" )
		PORT_DIPSETTING( 0x1a00, "1A00" )
		PORT_DIPSETTING( 0x1b00, "1B00" )
		PORT_DIPSETTING( 0x1c00, "1C00" )
		PORT_DIPSETTING( 0x1d00, "1D00" )
		PORT_DIPSETTING( 0x1e00, "1E00" )
		PORT_DIPSETTING( 0x1f00, "1F00" )

	PORT_START( "HFDCDIP" )
	PORT_DIPNAME( 0xff, 0x00, "HFDC drive config" )
		PORT_DIPSETTING( 0x00, "40 track, 16 ms")
		PORT_DIPSETTING( 0xaa, "40 track, 8 ms")
		PORT_DIPSETTING( 0x55, "80 track, 2 ms")
		PORT_DIPSETTING( 0xff, "80 track HD, 2 ms")
INPUT_PORTS_END

FLOPPY_FORMATS_MEMBER(myarc_hfdc_device::floppy_formats)
	FLOPPY_TI99_SDF_FORMAT,
	FLOPPY_TI99_TDF_FORMAT
FLOPPY_FORMATS_END

static SLOT_INTERFACE_START( hfdc_floppies )
	SLOT_INTERFACE( "525dd", FLOPPY_525_DD )        // 40 tracks
	SLOT_INTERFACE( "525qd", FLOPPY_525_QD )        // 80 tracks
	SLOT_INTERFACE( "35dd", FLOPPY_35_DD )          // 80 tracks
SLOT_INTERFACE_END

MACHINE_CONFIG_FRAGMENT( ti99_hfdc )
	MCFG_DEVICE_ADD(FDC_TAG, HDC9234, 0)
	MCFG_HDC9234_INTRQ_CALLBACK(WRITELINE(myarc_hfdc_device, intrq_w))
	MCFG_HDC9234_DIP_CALLBACK(WRITELINE(myarc_hfdc_device, dip_w))
	MCFG_HDC9234_AUXBUS_OUT_CALLBACK(WRITE8(myarc_hfdc_device, auxbus_out))
	MCFG_HDC9234_DMARQ_CALLBACK(WRITELINE(myarc_hfdc_device, dmarq_w))
	MCFG_HDC9234_DMA_IN_CALLBACK(READ8(myarc_hfdc_device, read_buffer))
	MCFG_HDC9234_DMA_OUT_CALLBACK(WRITE8(myarc_hfdc_device, write_buffer))

	MCFG_FLOPPY_DRIVE_ADD("0", hfdc_floppies, "525dd", myarc_hfdc_device::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("1", hfdc_floppies, "525dd", myarc_hfdc_device::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("2", hfdc_floppies, NULL, myarc_hfdc_device::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("3", hfdc_floppies, NULL, myarc_hfdc_device::floppy_formats)

	MCFG_DEVICE_ADD(CLOCK_TAG, MM58274C, 0)
	MCFG_MM58274C_MODE24(1) // 24 hour
	MCFG_MM58274C_DAY1(0)   // sunday
MACHINE_CONFIG_END

ROM_START( ti99_hfdc )
	ROM_REGION(0x4000, DSRROM, 0)
	ROM_LOAD("hfdc.bin", 0x0000, 0x4000, CRC(66fbe0ed) SHA1(11df2ecef51de6f543e4eaf8b2529d3e65d0bd59)) /* HFDC disk DSR ROM */
	ROM_REGION(0x8000, BUFFER, 0)  /* HFDC RAM buffer 32 KiB */
	ROM_FILL(0x0000, 0x8000, 0x00)
ROM_END


machine_config_constructor myarc_hfdc_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( ti99_hfdc );
}

const rom_entry *myarc_hfdc_device::device_rom_region() const
{
	return ROM_NAME( ti99_hfdc );
}

ioport_constructor myarc_hfdc_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( ti99_hfdc );
}

const device_type TI99_HFDC = &device_creator<myarc_hfdc_device>;

// =========================================================================

/*
    Legacy implementation.
*/
#define VERBOSE 1
#define LOG logerror

myarc_hfdc_legacy_device::myarc_hfdc_legacy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: ti_expansion_card_device(mconfig, TI99_HFDC_LEG, "Myarc Hard and Floppy Disk Controller LEGACY", tag, owner, clock, "ti99_hfdc", __FILE__),
		m_hdc9234(*this, FDC_TAG),
		m_clock(*this, CLOCK_TAG)
{
}

/*
    read a byte in disk DSR space
    HFDC manual, p. 44
    Memory map as seen by the 99/4A PEB

    0x4000 - 0x4fbf one of four possible ROM pages
    0x4fc0 - 0x4fcf tape select
    0x4fd0 - 0x4fdf disk controller select
    0x4fe0 - 0x4fff time chip select

    0x5000 - 0x53ff static RAM page 0x10
    0x5400 - 0x57ff static RAM page any of 32 pages
    0x5800 - 0x5bff static RAM page any of 32 pages
    0x5c00 - 0x5fff static RAM page any of 32 pages
*/
READ8Z_MEMBER(myarc_hfdc_legacy_device::readz)
{
	if (m_selected && ((offset & m_select_mask)==m_select_value))
	{
		/* DSR region */
		if ((offset & 0x1000)==0x0000)
		{
			if ((offset & 0x0fc0)==0x0fc0)
			{
				// Tape: 4fc0...4fcf
				if ((offset & 0x1ff0)==TAPE_ADDR)
				{
					if (VERBOSE>0) LOG("ti99: HFDC: Tape support not available (access to address %04x)\n", offset&0xffff);
					return;
				}

				// HDC9234: 4fd0..4fdf / read: 4fd0,4 (mirror 8,c)
				// read: 0100 1111 1101 xx00
				if ((offset & 0x1ff3)==HDC_R_ADDR)
				{
					if (!space.debugger_access()) *value = m_hdc9234->read(space, (offset>>2)&1, mem_mask);
					if (VERBOSE>7) LOG("hfdc: read %04x: %02x\n",  offset & 0xffff, *value);
					return;
				}

				if ((offset & 0x1fe1)==CLK_ADDR)
				{
					if (!space.debugger_access()) *value = m_clock->read(space, (offset & 0x001e) >> 1);
					if (VERBOSE>7) LOG("hfdc: read from clock address %04x: %02x\n", offset & 0xffff, *value);
					return;
				}
			}
			else
			{
				// ROM
				*value = m_dsrrom[(m_rom_page << 12) | (offset & 0x0fff)];
				if (VERBOSE>7) LOG("hfdc: reado %04x (%02x): %02x\n",  offset & 0xffff, m_rom_page, *value);
				return;
			}
		}

		// RAM: 0101 xxxx xxxx xxxx
		if ((offset & 0x1000)==RAM_ADDR)
		{
			// 0101 00xx xxxx xxxx  static 0x08
			// 0101 01xx xxxx xxxx  bank 1
			// 0101 10xx xxxx xxxx  bank 2
			// 0101 11xx xxxx xxxx  bank 3
			int bank = (offset & 0x0c00) >> 10;
			*value = m_buffer_ram[(m_ram_page[bank]<<10) | (offset & 0x03ff)];
			if (VERBOSE>7) LOG("hfdc: read %04x (%02x): %02x\n", offset & 0xffff, m_ram_page[bank], *value);
		}
	}
}

/*
    Write a byte to the controller.
*/
WRITE8_MEMBER( myarc_hfdc_legacy_device::write )
{
	if (m_selected && ((offset & m_select_mask)==m_select_value))
	{
		// Tape: 4fc0...4fcf
		if ((offset & 0x1ff0)==TAPE_ADDR)
		{
			if (VERBOSE>0) LOG("ti99: HFDC: Tape support not available (access to address %04x)\n", offset & 0xffff);
			return;
		}

		// HDC9234: 4fd0..4fdf / write: 4fd2,6 (mirror a,e)
		// write: 0100 1111 1101 xx10
		if ((offset & 0x1ff3)==HDC_W_ADDR)
		{
			if (VERBOSE>7) LOG("hfdc: write to controller address %04x: %02x\n", offset & 0xffff, data);
			if (!space.debugger_access()) m_hdc9234->write(space, (offset>>2)&1, data, mem_mask);
			return;
		}

		if ((offset & 0x1fe1)==CLK_ADDR)
		{
			if (VERBOSE>7) LOG("hfdc: write to clock address %04x: %02x\n", offset & 0xffff, data);
			if (!space.debugger_access()) m_clock->write(space, (offset & 0x001e) >> 1, data);
			return;
		}

		// RAM: 0101 xxxx xxxx xxxx
		if ((offset & 0x1000)==RAM_ADDR)
		{
			// 0101 00xx xxxx xxxx  static 0x08
			// 0101 01xx xxxx xxxx  bank 1
			// 0101 10xx xxxx xxxx  bank 2
			// 0101 11xx xxxx xxxx  bank 3
			int bank = (offset & 0x0c00) >> 10;
			if (VERBOSE>7) LOG("hfdc: WRITE %04x (%02x): %02x\n", (offset & 0xffff), m_ram_page[bank], data);
			m_buffer_ram[(m_ram_page[bank]<<10) | (offset & 0x03ff)] = data;
		}
	}
}

READ8Z_MEMBER(myarc_hfdc_legacy_device::crureadz)
{
	UINT8 reply;
	if ((offset & 0xff00)==m_cru_base)
	{
		if ((offset & 0x00ff)==0)  /* CRU bits 0-7 */
		{
			/* CRU bits */
			if (m_cru_select)
			{
				// DIP switches.  Logic levels are inverted (on->0, off->1).  CRU
				// bit order is the reverse of DIP-switch order, too (dip 1 -> bit 7,
				// dip 8 -> bit 0).
				// MZ: 00 should not be used since there is a bug in the
				// DSR of the HFDC which causes problems with SD disks
				// (controller tries DD and then fails to fall back to SD) */
				reply = ~(ioport("HFDCDIP")->read());
			}
			else
			{
				reply = 0;

				if (m_irq)          reply |= 0x01;
				if (m_dip)          reply |= 0x02;
				if (m_motor_running)    reply |= 0x04;
			}
			*value = reply;
		}
		else /* CRU bits 8+ */
			*value = 0;
		if (VERBOSE>7) LOG("hfdc: reading from CRU %04x: %02x\n", offset, *value);
	}
}

/*
    Write disk CRU interface
    HFDC manual p. 43

    CRU rel. address    Definition                      Active
    00                  Device Service Routine Select   HIGH
    02                  Reset to controller chip        LOW
    04                  Floppy Motor on / CD0           HIGH
    06                  ROM page select 0 / CD1
    08                  ROM page select 1 / CRU input select
    12/4/6/8/A          RAM page select at 0x5400
    1C/E/0/2/4          RAM page select at 0x5800
    26/8/A/C/E          RAM page select at 0x5C00

    RAM bank select: bank 0..31; 12 = LSB (accordingly for other two areas)
    ROM bank select: bank 0..3; 06 = MSB, 07 = LSB
    Bit number = (CRU_rel_address - base_address)/2
    CD0 and CD1 are Clock Divider selections for the Floppy Data Separator (FDC9216)
*/
WRITE8_MEMBER(myarc_hfdc_legacy_device::cruwrite)
{
	if ((offset & 0xff00)==m_cru_base)
	{
		if (VERBOSE>7) LOG("hfdc: writing to CRU %04x: %02x\n", offset, data);
		int bit = (offset >> 1) & 0x1f;

		if (bit >= 9 && bit < 24)
		{
			if (data)
				// we leave index 0 unchanged; modify indices 1-3
				m_ram_page[(bit-4)/5] |= 1 << ((bit-9)%5);
			else
				m_ram_page[(bit-4)/5] &= ~(1 << ((bit-9)%5));

			return;
		}

		switch (bit)
		{
		case 0:
			m_selected = (data!=0);
			if (VERBOSE>7) LOG("hfdc: selected = %d\n", m_selected);
			break;

		case 1:
			if (data==0) m_hdc9234->reset();  // active low
			break;

		case 2:
			m_CD0 = data;
			if (data != 0) m_motor_running = true;
			else
			{
				if (m_motor_running)
				{
					m_motor_on_timer->adjust(attotime::from_msec(4230));
				}
			}
			break;

		case 3:
			m_CD1 = data;
			if (data!=0) m_rom_page |= 2;
			else m_rom_page &= 0xfd;
			break;

		case 4:
			m_cru_select = (data!=0);
			if (data!=0) m_rom_page |= 1;
			else m_rom_page &= 0xfe;
			break;

		default:
			if (VERBOSE>1) LOG("ti99: HFDC: Attempt to set undefined CRU bit %d\n", bit);
		}
	}
}


int myarc_hfdc_legacy_device::slog2(int value)
{
	int i=-1;
	while (value!=0)
	{
		value >>= 1;
		i++;
	}
	return i;
}

READ8_MEMBER( myarc_hfdc_legacy_device::auxbus_in )
{
	UINT8 reply = 0;
	int index = 0;

	if ((m_output1_latch & 0xf0)==0)
	{
		if (VERBOSE>4) LOG("hfdc: no drive selected, returning 0\n");
		// No HD and no floppy
		return 0; /* is that the correct default value? */
	}

	// If a floppy is selected, we have one line set among the four programmable outputs.
	// Floppy selected <=> latch & 0x10 != 0; floppy number in last four bits
	if ((m_output1_latch & 0x10)!=0)
	{
		index = slog2(m_output1_latch & 0x0f);
		if (index==-1) return 0;

		/* Get floppy status. */
		if (m_floppy_unit[index]->floppy_drive_get_flag_state(FLOPPY_DRIVE_INDEX) == FLOPPY_DRIVE_INDEX)
			reply |= DS_INDEX;
		if (m_floppy_unit[index]->floppy_tk00_r() == CLEAR_LINE)
			reply |= DS_TRK00;
		if (m_floppy_unit[index]->floppy_wpt_r() == CLEAR_LINE)
			reply |= DS_WRPROT;

		/* if (image_exists(disk_img)) */

		reply |= DS_READY;  /* Floppies don't have a READY line; line is pulled up */
		reply |= DS_SKCOM;  /* Same here. */
		if (VERBOSE>4) LOG("hfdc: floppy selected, returning status %02x\n", reply);
	}
	else // one of the first three lines must be selected
	{
		UINT8 state;
		index = slog2((m_output1_latch>>4) & 0x0f)-1;
		mfm_harddisk_device *hd = m_harddisk_unit[index];
		state = hd->get_status();

		if (state & MFMHD_TRACK00)      reply |= DS_TRK00;
		if (state & MFMHD_SEEKCOMP)     reply |= DS_SKCOM;
		if (state & MFMHD_WRFAULT)      reply |= DS_WRFAULT;
		if (state & MFMHD_INDEX)        reply |= DS_INDEX;
		if (state & MFMHD_READY)        reply |= DS_READY;
		if (VERBOSE>4) LOG("hfdc: hd selected, returning status %02x\n", reply);
	}

	return reply;
}

WRITE8_MEMBER( myarc_hfdc_legacy_device::auxbus_out )
{
	int index;
	switch (offset)
	{
	case INPUT_STATUS:
		if (VERBOSE>1) LOG("ti99: HFDC: Invalid operation: S0=S1=0, but tried to write (expected: read drive status)\n");
		break;

	case OUTPUT_DMA_ADDR:
		/* Value is dma address byte. Shift previous contents to the left. */
		m_dma_address = ((m_dma_address << 8) + (data&0xff))&0xffffff;
		break;

	case OUTPUT_OUTPUT1:
		// value is output1
		// The HFDC interprets the value as follows:
		// WDS = Winchester Drive System, old name for hard disk
		// +------+------+------+------+------+------+------+------+
		// | WDS3 | WDS2 | WDS1 | DSKx | x=4  | x=3  | x=2  | x=1  |
		// +------+------+------+------+------+------+------+------+
		// Accordingly, drive 0 is always the floppy; selected by the low nibble
		if (data & 0x10)
		{
			// Floppy selected
			index = slog2(data & 0x0f);
			if (index>=0) m_hdc9234->connect_floppy_drive(m_floppy_unit[index]);
		}
		else
		{
			// HD selected
			index = slog2((data>>4) & 0x0f);
			if (index>=0) m_hdc9234->connect_hard_drive(m_harddisk_unit[index-1]);
		}

		m_output1_latch = data;
		break;

	case OUTPUT_OUTPUT2:
		/* value is output2 */
		m_output2_latch = data;
		break;
	}
}

/*
    Read a byte from buffer in DMA mode
*/
READ8_MEMBER( myarc_hfdc_legacy_device::read_buffer )
{
	UINT8 value = m_buffer_ram[m_dma_address & 0x7fff];
	m_dma_address++;
	return value;
}

/*
    Write a byte to buffer in DMA mode
*/
WRITE8_MEMBER( myarc_hfdc_legacy_device::write_buffer )
{
	m_buffer_ram[m_dma_address & 0x7fff] = data;
	m_dma_address++;
}

/*
    Called whenever the state of the sms9234 interrupt pin changes.
*/
WRITE_LINE_MEMBER( myarc_hfdc_legacy_device::intrq_w )
{
	m_irq = state;

	// Set INTA*
	// Signal from SMC is active high, INTA* is active low; board inverts signal
	// Anyway, we keep with ASSERT_LINE and CLEAR_LINE
	m_slot->set_inta(state);
}

/*
    Called whenever the state of the sms9234 DMA in progress changes.
*/
WRITE_LINE_MEMBER( myarc_hfdc_legacy_device::dip_w )
{
	m_dip = state;
}

/*
    Callback called at the end of DVENA pulse
*/
void myarc_hfdc_legacy_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	m_motor_running = false;
	if (VERBOSE>6) LOG("hfdc: motor off\n");
}

MACHINE_CONFIG_FRAGMENT( ti99_hfdc_legacy )
	MCFG_DEVICE_ADD(FDC_TAG, SMC92X4, 0)
	MCFG_SMC92X4_INTRQ_CALLBACK(WRITELINE(myarc_hfdc_legacy_device, intrq_w))
	MCFG_SMC92X4_DIP_CALLBACK(WRITELINE(myarc_hfdc_legacy_device, dip_w))
	MCFG_SMC92X4_AUXBUS_OUT_CALLBACK(WRITE8(myarc_hfdc_legacy_device, auxbus_out))
	MCFG_SMC92X4_AUXBUS_IN_CALLBACK(READ8(myarc_hfdc_legacy_device, auxbus_in))
	MCFG_SMC92X4_DMA_IN_CALLBACK(READ8(myarc_hfdc_legacy_device, read_buffer))
	MCFG_SMC92X4_DMA_OUT_CALLBACK(WRITE8(myarc_hfdc_legacy_device, write_buffer))
	MCFG_SMC92X4_FULL_TRACK_LAYOUT(FALSE)    /* do not use the full track layout */

	MCFG_DEVICE_ADD(CLOCK_TAG, MM58274C, 0)
	MCFG_MM58274C_MODE24(1) // 24 hour
	MCFG_MM58274C_DAY1(0)   // sunday
MACHINE_CONFIG_END

ROM_START( ti99_hfdc_legacy )
	ROM_REGION(0x4000, DSRROM, 0)
	ROM_LOAD("hfdc.bin", 0x0000, 0x4000, CRC(66fbe0ed) SHA1(11df2ecef51de6f543e4eaf8b2529d3e65d0bd59)) /* HFDC disk DSR ROM */
	ROM_REGION(0x8000, BUFFER, 0)  /* HFDC RAM buffer 32 KiB */
	ROM_FILL(0x0000, 0x8000, 0x00)
ROM_END

INPUT_PORTS_START( ti99_hfdc_legacy )
	PORT_START( "CRUHFDC" )
	PORT_DIPNAME( 0x1f00, 0x1100, "HFDC CRU base" )
		PORT_DIPSETTING( 0x1000, "1000" )
		PORT_DIPSETTING( 0x1100, "1100" )
		PORT_DIPSETTING( 0x1200, "1200" )
		PORT_DIPSETTING( 0x1300, "1300" )
		PORT_DIPSETTING( 0x1400, "1400" )
		PORT_DIPSETTING( 0x1500, "1500" )
		PORT_DIPSETTING( 0x1600, "1600" )
		PORT_DIPSETTING( 0x1700, "1700" )
		PORT_DIPSETTING( 0x1800, "1800" )
		PORT_DIPSETTING( 0x1900, "1900" )
		PORT_DIPSETTING( 0x1a00, "1A00" )
		PORT_DIPSETTING( 0x1b00, "1B00" )
		PORT_DIPSETTING( 0x1c00, "1C00" )
		PORT_DIPSETTING( 0x1d00, "1D00" )
		PORT_DIPSETTING( 0x1e00, "1E00" )
		PORT_DIPSETTING( 0x1f00, "1F00" )

	PORT_START( "HFDCDIP" )
	PORT_DIPNAME( 0xff, 0x55, "HFDC drive config" )
		PORT_DIPSETTING( 0x00, "40 track, 16 ms")
		PORT_DIPSETTING( 0xaa, "40 track, 8 ms")
		PORT_DIPSETTING( 0x55, "80 track, 2 ms")
		PORT_DIPSETTING( 0xff, "80 track HD, 2 ms")

	PORT_START( "DRVSPD" )
	PORT_CONFNAME( 0x01, 0x01, "Floppy and HD speed" )
		PORT_CONFSETTING( 0x00, "No delay")
		PORT_CONFSETTING( 0x01, "Realistic")
INPUT_PORTS_END

void myarc_hfdc_legacy_device::device_start()
{
	if (VERBOSE>5) LOG("hfdc: start\n");
	m_dsrrom = memregion(DSRROM)->base();
	m_buffer_ram = memregion(BUFFER)->base();

	m_motor_on_timer = timer_alloc(MOTOR_TIMER);

	// The HFDC does not use READY; it has on-board RAM for DMA
}

void myarc_hfdc_legacy_device::device_reset()
{
	if (VERBOSE>5) LOG("hfdc: reset\n");
	if (m_genmod)
	{
		m_select_mask = 0x1fe000;
		m_select_value = 0x174000;
	}
	else
	{
		m_select_mask = 0x7e000;
		m_select_value = 0x74000;
	}

	m_cru_base = ioport("CRUHFDC")->read();

	// Resetting values
	m_rom_page = 0;

	m_ram_page[0] = 0x08;   // static page 0x08
	for (int i=1; i < 4; i++) m_ram_page[i] = 0;
	m_dma_address = 0;
	m_output1_latch = m_output2_latch = 0;
	m_dip = m_irq = 0;
	m_cru_select = false;
	m_CD0 = m_CD1 = 0;
	m_motor_running = false;
	m_selected = false;

	// Find the floppies and hard disks
	m_floppy_unit[0] = static_cast<legacy_floppy_image_device *>(m_slot->get_drive(FLOPPY_0));
	m_floppy_unit[1] = static_cast<legacy_floppy_image_device *>(m_slot->get_drive(FLOPPY_1));
	m_floppy_unit[2] = static_cast<legacy_floppy_image_device *>(m_slot->get_drive(FLOPPY_2));
	m_floppy_unit[3] = static_cast<legacy_floppy_image_device *>(m_slot->get_drive(FLOPPY_3));

	m_harddisk_unit[0] = static_cast<mfm_harddisk_device *>(m_slot->get_drive(MFMHD_0));
	m_harddisk_unit[1] = static_cast<mfm_harddisk_device *>(m_slot->get_drive(MFMHD_1));
	m_harddisk_unit[2] = static_cast<mfm_harddisk_device *>(m_slot->get_drive(MFMHD_2));

	if (ioport("HFDCDIP")->read()&0x55)
		ti99_set_80_track_drives(TRUE);
	else
		ti99_set_80_track_drives(FALSE);

	m_hdc9234->set_timing(ioport("DRVSPD")->read()!=0);

	floppy_type_t floptype = FLOPPY_STANDARD_5_25_DSHD;
	// Again, need to re-adjust the floppy geometries
	for (int i=0; i < HFDC_MAX_FLOPPY; i++)
	{
		if (m_floppy_unit[i] != NULL)
		{
			//  floppy_drive_set_controller(card->floppy_unit[i], device);
			//  floppy_drive_set_index_pulse_callback(floppy_unit[i], smc92x4_index_pulse_callback);
			m_floppy_unit[i]->floppy_drive_set_geometry(floptype);
		}
	}

	if (VERBOSE>2) LOG("hfdc: CRU base = %04x\n", m_cru_base);
	// TODO: Check how to make use of   floppy_mon_w(w->drive, CLEAR_LINE);
}

machine_config_constructor myarc_hfdc_legacy_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( ti99_hfdc_legacy );
}

const rom_entry *myarc_hfdc_legacy_device::device_rom_region() const
{
	return ROM_NAME( ti99_hfdc_legacy );
}

ioport_constructor myarc_hfdc_legacy_device::device_input_ports() const
{
	return INPUT_PORTS_NAME(ti99_hfdc_legacy);
}

const device_type TI99_HFDC_LEG = &device_creator<myarc_hfdc_legacy_device>;
