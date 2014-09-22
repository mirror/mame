// license:BSD-3-Clause
// copyright-holders:Michael Zapf
/**************************************************************************

    HDC9234 Hard and Floppy Disk Controller
    Standard Microsystems Corporation (SMC)

    This controller handles MFM and FM encoded floppy disks and hard disks.
    A variant, the HDC9224, is used in some DEC systems.

    The HDC9234 is used in the Myarc HFDC card for the TI99/4A.

    References:
    [1] SMC HDC9234 preliminary data book (1988)

    The HDC9234 controller is also referred to as the "Universal Disk Controller" (UDC)
    by the data book

    Michael Zapf, September 2014

***************************************************************************/

#include "emu.h"
#include "hdc9234.h"

// Per-command debugging
#define TRACE_SELECT 0
#define TRACE_STEP 0
#define TRACE_RESTORE 0
#define TRACE_SUBSTATES 0
#define TRACE_READ 0
#define TRACE_WRITE 0
#define TRACE_READREG 0
#define TRACE_SETREG 0
#define TRACE_SETPTR 0
#define TRACE_FORMAT 0

// Common states
#define TRACE_READID 0
#define TRACE_VERIFY 0
#define TRACE_TRANSFER 0

// Live states debugging
#define TRACE_LIVE 0
#define TRACE_SHIFT 0
#define TRACE_SYNC 0

// Misc debugging
#define TRACE_DELAY 0
#define TRACE_INT 0
#define TRACE_LINES 0
#define TRACE_INDEX 0
#define TRACE_DMA 0
#define TRACE_DONE 0
#define TRACE_FAIL 0
#define TRACE_AUXBUS 0

#define TRACE_DETAIL 0

#define UNRELIABLE_MEDIA 0

// Not implemented:
// Poll drives
// Seek/Read ID
// Read track
// Write long (variant of the write operation, selectable by MODE register)
// Tape operations

// Untested:
// Multi-sector read/write
// Seek complete
// Read/write sectors physical

// TDF

/*
    Register names of the HDC. The left part is the set of write registers,
    while the right part are the read registers.

            +------+------+------+------+------+------+------+------+
    DHEAD:  |   0  | Sector size |  0   |   Desired head  (OUTPUT2) |  AT mode
            +------+------+------+------+------+------+------+------+
            |   0  |  Desired cylinder  |   Desired head  (OUTPUT2) |  SMC mode
            +------+------+------+------+------+------+------+------+
            +------+------+------+------+------+------+------+------+
    RETRY:  |  Retry count (ones comp!) |   Progr. output (OUTPUT1) |
            +------+------+------+------+------+------+------+------+
            +------+------+------+------+------+------+------+------+
    MODE:   | HD   | use CRC/ECC |  FM  |  0   |      step rate     |
            +------+------+------+------+------+------+------+------+
            +------+------+------+------+------+------+------+------+
    INTCOMM:|   1  |  0   | Done | DelD | User | WrPrt| Ready|Wfault|
            +------+------+------+------+------+------+------+------+
            +------+------+------+------+------+------+------+------+
    DDELAY: |   0  |  0   | Sector size |  0   |  0   |    Zone     | AT mode
            +------+------+------+------+------+------+------+------+
            |          Data to be written on disk                   | writing
            +------+------+------+------+------+------+------+------+
            |          Head load timer count                        | drselect
            +------+------+------+------+------+------+------+------+

    Read registers
            +------+------+------+------+------+------+------+------+
    CHIP_ST:| Retry|  ECC |  CRC | DelD | Sync | Comp | Current Drv |
            +------+------+------+------+------+------+------+------+
            +------+------+------+------+------+------+------+------+
    INT_ST: | Pend | DMARQ| Done |  Termcode   | RdyCh| Ovrun| BdSec|
            +------+------+------+------+------+------+------+------+
            +------+------+------+------+------+------+------+------+
    DRV_ST: | ECC  | Index| SeekC| Trk00| User | WrPrt| Ready|Wfault|
            +------+------+------+------+------+------+------+------+

*/
enum
{
	// Write registers   |   Read registers
	//--------------------------------------
	DMA7_0=0,
	DMA15_8=1,
	DMA23_16=2,
	DESIRED_SECTOR=3,       CURRENT_SECTOR=3,
	DESIRED_HEAD=4,         CURRENT_HEAD=4,
	DESIRED_CYLINDER=5,     CURRENT_CYLINDER=5,
	SECTOR_COUNT=6,         CURRENT_IDENT=6,
	RETRY_COUNT=7,          TEMP_STORAGE2=7,
	MODE=8,                 CHIP_STATUS=8,
	INT_COMM_TERM=9,        DRIVE_STATUS=9,
	DATA_DELAY=10,          DATA=10,
	COMMAND=11,             INT_STATUS=11,

	//======================
	// Internal registers
	CURRENT_SIZE=12,
	CURRENT_CRC1=13,
	CURRENT_CRC2=14
};

/*
    Definition of bits in the status register [1] p.7
*/
enum
{
	ST_INTPEND = 0x80,       // interrupt pending
	ST_DMAREQ  = 0x40,       // DMA request
	ST_DONE    = 0x20,       // command done
	ST_TERMCOD = 0x18,       // termination code (see below)
		TC_SUCCESS = 0x00,   // Successful completion
		TC_RDIDERR = 0x08,   // Error in READ-ID sequence
		TC_VRFYERR = 0x10,   // Error in VERIFY sequence
		TC_DATAERR = 0x18,   // Error in DATA-TRANSFER seq.
	ST_RDYCHNG = 0x04,       // ready change
	ST_OVRUN   = 0x02,       // overrun/underrun
	ST_BADSECT = 0x01        // bad sector
};

/*
    Definition of bits in the Termination-Conditions register
*/
enum
{
	TC_CRCPRE  = 0x80,       // CRC register preset, must be 1
	TC_UNUSED  = 0x40,       // bit 6 is not used and must be 0
	TC_INTDONE = 0x20,       // interrupt on command completion
	TC_TDELDAT = 0x10,       // terminate on deleted data mark detection
	TC_TDUSER  = 0x08,       // user-defined condition
	TC_TWPROT  = 0x04,       // terminate on write protection
	TC_INTRDCH = 0x02,       // interrupt on ready change
	TC_TWRFLT  = 0x01        // interrupt on write fault
};

/*
    Definition of bits in the chip status register.
*/
enum
{
	CS_RETREQ  = 0x80,        // retry required
	CS_ECCATT  = 0x40,        // ECC correction attempted
	CS_CRCERR  = 0x20,        // ECC/CRC error
	CS_DELDATA = 0x10,        // deleted data mark
	CS_SYNCERR = 0x08,        // synchronization error
	CS_COMPERR = 0x04,        // compare error
	CS_PRESDRV = 0x03         // present drive selected
};

/*
    Bits in the internal output registers. The registers are output via the
    auxiliary bus (AB)

    OUTPUT1
    AB7     drive select 3
    AB6     drive select 2
    AB5     drive select 1
    AB4     drive select 0
    AB3     programmable outputs
    AB2     programmable outputs
    AB1     programmable outputs
    AB0     programmable outputs

    OUTPUT2
    AB7     drive select 3* (active low, used for tape operations)
    AB6     reduce write current
    AB5     step direction       (0=towards TRK00)
    AB4     step pulse           (1=active)
    AB3     desired head 3
    AB2     desired head 2
    AB1     desired head 1
    AB0     desired head 0
*/
enum
{
	OUT1_DRVSEL3    = 0x80,
	OUT1_DRVSEL2    = 0x40,
	OUT1_DRVSEL1    = 0x20,
	OUT1_DRVSEL0    = 0x10,
	OUT2_DRVSEL3I   = 0x80,
	OUT2_REDWRT     = 0x40,
	OUT2_STEPDIR    = 0x20,
	OUT2_STEPPULSE  = 0x10,
	OUT2_HEADSEL    = 0x0f
};

#define NODRIVE -1

enum
{
	TYPE_AT = 0x00,
	TYPE_HD = 0x01,
	TYPE_FLOPPY8 = 0x02,
	TYPE_FLOPPY5 = 0x03,
	DRIVE_TYPE = 0x03
};

/*
    Timers
*/
enum
{
	GEN_TIMER = 1,
	COM_TIMER,
	LIVE_TIMER
};

/*
    Definition of bits in the Mode register
*/
enum {
	MO_TYPE     = 0x80,        // Hard disk (1) or floppy (0)
	MO_CRCECC   = 0x60,        // Values for CRC/ECC handling
	MO_DENSITY  = 0x10,        // FM = 1; MFM = 0
	MO_UNUSED   = 0x08,        // Unused, 0
	MO_STEPRATE = 0x07         // Step rates
};

/*
    Step rates in microseconds for MFM. This is set in the mode register,
    bits 0-2. FM mode doubles all values.
*/
static const int step_hd[]      = { 22, 50, 100, 200, 400, 800, 1600, 3200 };
static const int step_flop8[]   = { 218, 500, 1000, 2000, 4000, 8000, 16000, 32000 };
static const int step_flop5[]   = { 436, 1000, 2000, 4000, 8000, 16000, 32000, 64000 };

/*
    Head load timer increments in usec. Delay value is calculated from this value
    multiplied by the factor in the DATA/DELAY register. For FM mode all
    values are doubled. The values depend on the drive type.
*/
static const int head_load_timer_increment[] = { 200, 200, 2000, 4000 };

/*
    ID fields association to registers
*/
static const int id_field[] = { CURRENT_CYLINDER, CURRENT_HEAD, CURRENT_SECTOR, CURRENT_SIZE, CURRENT_CRC1, CURRENT_CRC2 };

/*
    Pulse widths for stepping in usec
*/
enum
{
	pulse_hd = 11,
	pulse_flop8 = 112,
	pulse_flop5 = 224
};

/*
    Times for UDC's acceptance of command and register write accesses (ns).
*/
enum
{
	REGISTER_COMMIT = 1000,
	COMMAND_COMMIT = 1000
};

enum
{
	UNDEF = 0x00,
	IDLE,
	DONE,
	COMMAND_INIT,
	REGISTER_ACCESS,

	STEP_ON,
	STEP_OFF,
	RESTORE_CHECK1,
	RESTORE_CHECK2,
	SEEK_COMPLETE,
	HEAD_DELAY,
	WAITINDEX0,
	WAITINDEX1,
	TRACKSTART,
	TRACKDONE,

	READ_ID = 0x40,
	READ_ID1,
	READ_ID_STEPON,
	READ_ID_STEPOFF,

	VERIFY = 0x50,
	VERIFY1,
	VERIFY2,
	VERIFY3,

	DATA_TRANSFER = 0x60,
	DATA_TRANSFER_READ,
	DATA_TRANSFER_WRITE,

	// Live states
	LIVE_STATES = 0x80,
	SEARCH_IDAM,
	SEARCH_IDAM_FAILED,
	READ_TWO_MORE_A1_IDAM,
	READ_ID_FIELDS_INTO_REGS,
	SEARCH_DAM,
	READ_TWO_MORE_A1_DAM,
	SEARCH_DAM_FAILED,
	READ_SECTOR_DATA,
	READ_SECTOR_DATA1,
	WRITE_DAM_AND_SECTOR,
	WRITE_SEC_SKIP_GAP2,
	WRITE_SEC_SKIP_GAP2_LOOP,
	WRITE_SEC_BYTE,
	WRITE_SEC_NEXT_BYTE,

	WRITE_TRACK_BYTE,
	WRITE_TRACK_NEXT_BYTE,

	READ_TRACK_BYTE,
	READ_TRACK_NEXT_BYTE,

	FORMAT_TRACK,
	WRITE_GAP0,
	WRITE_GAP1,
	WRITE_GAP2,
	WRITE_GAP3,
	WRITE_GAP4,
	WRITE_IXAM_SYNC,
	WRITE_IXAM,
	WRITE_FC,
	WRITE_IDAM_SYNC,
	WRITE_IDAM,
	WRITE_HEADER,
	WRITE_IDENT,

	WRITE_DAM_SYNC,
	WRITE_A1,
	WRITE_DATAMARK,
	WRITE_SECDATA,
	WRITE_DATA_CRC,
	WRITE_DONE,
	WRITE_HEADER_CRC,

	NO_DMA_ACK
};

/*
    Event lines
*/
enum
{
	INDEX_LINE = 1,
	READY_LINE,
	SEEKCOMP_LINE
};

/*
    State machine metastates.
*/
enum
{
	CONTINUE = 0,
	WAIT,
	NEXT,
	ERROR,
	SUCCESS
};

const hdc9234_device::cmddef hdc9234_device::s_command[] =
{
	{ 0x00, 0xff, &hdc9234_device::reset_controller },
	{ 0x01, 0xff, &hdc9234_device::drive_deselect },
	{ 0x02, 0xfe, &hdc9234_device::restore_drive },
	{ 0x04, 0xfc, &hdc9234_device::step_drive },
	{ 0x08, 0xf8, &hdc9234_device::tape_backup },
	{ 0x10, 0xf0, &hdc9234_device::poll_drives },
	{ 0x20, 0xe0, &hdc9234_device::drive_select },
	{ 0x40, 0xf0, &hdc9234_device::set_register_pointer },
	{ 0x50, 0xf8, &hdc9234_device::seek_read_id },
	{ 0x58, 0xfe, &hdc9234_device::read_sectors },
	{ 0x5a, 0xfe, &hdc9234_device::read_track },
	{ 0x5c, 0xfc, &hdc9234_device::read_sectors },
	{ 0x60, 0xe0, &hdc9234_device::format_track },
	{ 0x80, 0x80, &hdc9234_device::write_sectors },
	{ 0, 0, 0 }
};

/*
    Standard constructor.
*/
hdc9234_device::hdc9234_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, HDC9234, "SMC HDC9234 Universal Disk Controller", tag, owner, clock, "hdc9234", __FILE__),
	m_out_intrq(*this),
	m_out_dmarq(*this),
	m_out_dip(*this),
	m_out_auxbus(*this),
	m_in_dma(*this),
	m_out_dma(*this),
	m_initialized(false)
{
}

/*
    Set or reset some bits.
*/
void hdc9234_device::set_bits(UINT8& byte, int mask, bool set)
{
	if (set) byte |= mask;
	else byte &= ~mask;
}

/*
    Tell whether the controller is in FM mode.
*/
bool hdc9234_device::fm_mode()
{
	return ((m_register_w[MODE]&MO_DENSITY)!=0);
}

/*
    Are we back on track 0?
*/
bool hdc9234_device::on_track00()
{
	return (m_register_r[DRIVE_STATUS] & HDC_DS_TRK00)!=0;
}

/*
    Accessor functions for specific parameters.
*/
int hdc9234_device::desired_head()
{
	return m_register_w[DESIRED_HEAD] & 0x0f;
}

int hdc9234_device::desired_cylinder()
{
	return (m_register_w[DESIRED_CYLINDER] & 0xff) | (m_register_w[DESIRED_HEAD] & 0x70);
}

int hdc9234_device::desired_sector()
{
	return m_register_w[DESIRED_SECTOR] & 0xff;
}

int hdc9234_device::current_head()
{
	return m_register_r[CURRENT_HEAD] & 0x0f;
}

int hdc9234_device::current_cylinder()
{
	return (m_register_r[CURRENT_CYLINDER] & 0xff) | (m_register_r[CURRENT_HEAD] & 0x70);
}

int hdc9234_device::current_sector()
{
	return m_register_r[CURRENT_SECTOR] & 0xff;
}

UINT8 hdc9234_device::current_command()
{
	return m_register_w[COMMAND];
}

/*
    Set/clear INT

    Interupts are generated in the following occasions:
    - when the DONE bit is set to 1 in the ISR and ST_DONE is set to 1
    - when the READY_CHANGE bit is set to 1 in the ISR and ST_RDYCHNG is set to 1
    (ready change: 1->0 or 0->1)
*/
void hdc9234_device::set_interrupt(line_state intr)
{
	if (intr == ASSERT_LINE)
	{
		// Only if there is not already a pending interrupt
		if ((m_register_r[INT_STATUS] & ST_INTPEND) == 0)
		{
			m_register_r[INT_STATUS] |= ST_INTPEND;
			m_out_intrq(intr);
		}
	}
	else
	{
		// if there is a pending interrupt
		if ((m_register_r[INT_STATUS] & ST_INTPEND) != 0)
			m_out_intrq(intr);
	}
}

/*
    Assert Command Done status bit, triggering interrupts as needed
*/
void hdc9234_device::set_command_done(int flags)
{
	set_bits(m_register_r[INT_STATUS], ST_DONE, true);

	if (flags != -1)
	{
		set_bits(m_register_r[INT_STATUS], ST_TERMCOD, false); // clear the previously set flags
		m_register_r[INT_STATUS] |= flags;
		if (TRACE_DONE) logerror("%s: command %02x done, flags=%02x\n", tag(), current_command(), flags);
	}
	else
	{
		if (TRACE_DONE) logerror("%s: command %02x done\n", tag(), current_command());
	}

	// [1], p. 6
	if (TRACE_INT) logerror("%s: Raise interrupt DONE\n", tag());
	set_interrupt(ASSERT_LINE);

	m_substate = UNDEF;
	m_executing = false;
}

/*
    Preserve previously set termination code
*/
void hdc9234_device::set_command_done()
{
	set_command_done(-1);
}

void hdc9234_device::wait_time(emu_timer *tm, int microsec, int next_substate)
{
	if (TRACE_DELAY) logerror("%s: Delay by %d microsec\n", tag(), microsec);
	tm->adjust(attotime::from_usec(microsec));
	m_substate = next_substate;
}

void hdc9234_device::wait_time(emu_timer *tm, attotime delay, int param)
{
	if (TRACE_DELAY) logerror("%s: [%s] Delaying by %4.2f microsecs\n", tag(), ttsn().cstr(), delay.as_double()*1000000);
	tm->adjust(delay);
	m_substate = param;
}

// ==================================================================
//     Common subroutines READ ID, VERIFY, DATA TRANSFER
//     called by all sector access commands
// ==================================================================
/*
    READ ID FIELD ([1] p. 9)
    The controller
    - scans for the next IDAM
    - reads the ID field values into the CURRENT_HEAD/CYLINDER/SECTOR registers
    - checks the CRC
    - calculates the number of steps and the direction towards DESIRED_CYLINDER
    (must have saved that value before!)
    - steps to that location during OUTPUT2 times
*/
void hdc9234_device::read_id(int& cont, bool implied_seek)
{
	cont = CONTINUE;

	while (cont==CONTINUE)
	{
		switch (m_substate)
		{
		case READ_ID:
			// Implied seek: Enter the READ_ID subprogram.
			if (TRACE_READID && TRACE_SUBSTATES) logerror("%s: substate READ_ID\n", tag());

			m_substate = implied_seek? READ_ID1 : VERIFY;

			// First step: Search the next IDAM, and if found, read the
			// ID values into the registers
			// Depending on the implied seek flag, continue with the read_id,
			// else switch to verify.
			m_live_state.bit_count_total = 0;
			live_start(SEARCH_IDAM);
			cont = WAIT;
			break;

		case READ_ID1:
			// If an error occured (no IDAM found), terminate the command
			if ((m_register_r[CHIP_STATUS] & CS_SYNCERR) != 0)
			{
				if (TRACE_FAIL) logerror("%s: READ_ID failed to find any IDAM\n", tag());
				cont = ERROR;
				break;
			}

			if (TRACE_READID && TRACE_SUBSTATES)
			{
				logerror("%s: substate READ_ID1\n", tag());
				logerror("%s: DESIRED_CYL = %d; CURRENT_CYL = %d\n", tag(), desired_cylinder(), current_cylinder());
			}

			// The CRC has been updated automatically with each read_one_bit during the live_run.
			// We just need to check whether it ended in 0000
			if (m_live_state.crc != 0)
			{
				if (TRACE_FAIL) logerror("%s: CRC error in sector header\n", tag());
				set_bits(m_register_r[CHIP_STATUS], CS_CRCERR, true);
				cont = ERROR;
				break;
			}

			// Calculate the direction and number of step pulses
			// positive -> towards inner cylinders
			// negative -> towards outer cylinders
			// zero -> we're already there
			m_track_delta = desired_cylinder() - current_cylinder();
			m_substate = READ_ID_STEPON;
			break;

		case READ_ID_STEPON:
			// Any more steps left?
			if (m_track_delta == 0)
			{
				m_substate = VERIFY;
				cont = NEXT;
				break;
			}

			if (TRACE_READID && TRACE_SUBSTATES) logerror("%s: substate STEP_ON\n", tag());
			// STEPDIR = 0 -> towards TRK00
			set_bits(m_output2, OUT2_STEPDIR, (m_track_delta>0));
			set_bits(m_output2, OUT2_STEPPULSE, true);
			auxbus_out();
			wait_time(m_timer, pulse_width(), READ_ID_STEPOFF);
			cont = WAIT;
			break;

		case READ_ID_STEPOFF:
			if (TRACE_READID && TRACE_SUBSTATES) logerror("%s: substate STEP_OFF\n", tag());
			set_bits(m_output2, OUT2_STEPPULSE, false);
			auxbus_out();
			m_track_delta += (m_track_delta<0)? 1 : -1;
			// Return to STEP_ON, check whether there are more steps
			wait_time(m_timer, step_time(), READ_ID_STEPON);
			cont = WAIT;
			break;

		default:
			logerror("%s: unknown substate %d in read_id\n", tag(), m_substate);
			cont = ERROR;
		}
	}

	//  When an error occurs, the COMMAND_TERMINATION bits are set to 01
	if (cont == ERROR)
	{
		live_abort();
		set_command_done(TC_RDIDERR);
	}
}

/*
    VERIFY ([1] p. 10)
    The controller
    - continues to read the next ID field until the current values match the
    contents of the DESIRED_HEAD/CYLINDER/SECTOR registers
    - checks the CRC
*/
void hdc9234_device::verify(int& cont, bool verify_all)
{
	cont = CONTINUE;

	while (cont==CONTINUE)
	{
		switch (m_substate)
		{
		case VERIFY:
			// After seeking (or immediately when implied seek has been disabled),
			// find the desired sector.

			if (TRACE_VERIFY && TRACE_SUBSTATES) logerror("%s: substate VERIFY\n", tag());

			// If an error occured (no IDAM found), terminate the command
			// (This test is only relevant when we did not have a seek phase before)
			if ((m_register_r[CHIP_STATUS] & CS_SYNCERR) != 0)
			{
				if (TRACE_FAIL) logerror("%s: VERIFY failed to find any IDAM\n", tag());
				cont = ERROR;
				break;
			}

			// Count from 0 again
			m_live_state.bit_count_total = 0;
			m_substate = VERIFY1;
			break;

		case VERIFY1:
			// Check whether we are already there
			if (desired_cylinder() == current_cylinder()
				&& desired_head() == current_head()
				&& desired_sector() == current_sector())
			{
				if (TRACE_VERIFY) logerror("%s: Found the desired sector CHS=(%d,%d,%d)\n", tag(),
					desired_cylinder(),
					desired_head(),
					desired_sector());
				m_substate = DATA_TRANSFER;
				cont = NEXT;
			}
			else
			{
				if (TRACE_VERIFY && TRACE_DETAIL) logerror("%s: Current CHS=(%d,%d,%d), desired CHS=(%d,%d,%d).\n", tag(),
					current_cylinder(),
					current_head(),
					current_sector(),
					desired_cylinder(),
					desired_head(),
					desired_sector());
				m_substate = VERIFY2;
			}
			break;

		case VERIFY2:
			// Search the next ID
			m_substate = VERIFY3;
			live_start(SEARCH_IDAM);
			cont = WAIT;
			break;

		case VERIFY3:
			if ((m_register_r[CHIP_STATUS] & CS_SYNCERR) != 0)
			{
				if (TRACE_FAIL) logerror("%s: VERIFY failed to find sector CHS=(%d,%d,%d)\n", tag(), desired_cylinder(), desired_head(), desired_sector());
				// live_run has set the sync error; clear it
				set_bits(m_register_r[CHIP_STATUS], CS_SYNCERR, false);
				// and set the compare error bit instead
				set_bits(m_register_r[CHIP_STATUS], CS_COMPERR, true);
				cont = ERROR;
				break;
			}

			// Continue with the loop
			if (verify_all)
			{
				// this is for the logical sector reading/writing
				m_substate = VERIFY1;
			}
			else
			{
				// this is for the physical sector reading/writing
				// do not verify the next ID field
				m_substate = DATA_TRANSFER;
				m_wait_for_index = true;
				cont = NEXT;
			}
			break;

		default:
			logerror("%s: unknown substate %d in verify\n", tag(), m_substate);
			cont = ERROR;
		}
	}

	// When an error occurs, the COMMAND_TERMINATION bits are set to 10
	if (cont == ERROR)
	{
		live_abort();
		set_command_done(TC_VRFYERR);
	}
}

/*
    DATA TRANSFER ([1], p. 10)
    only during READ/WRITE PHYSICAL/LOGICAL
    The controller
    - scans for the next DAM
    - initiates a DMA request and waits for ACK from the system processor
    - transfers the contents of the current sector into memory via DMA (read) or
      via DMA to the sector (write)
*/
void hdc9234_device::data_transfer(int& cont)
{
	cont = CONTINUE;

	while (cont==CONTINUE)
	{
		switch (m_substate)
		{
		case DATA_TRANSFER:
			if (TRACE_TRANSFER && TRACE_SUBSTATES) logerror("%s: substate DATA_TRANSFER\n", tag());

			// Count from 0 again
			m_live_state.bit_count_total = 0;

			if (m_transfer_enabled) dma_address_out(m_register_w[DMA23_16], m_register_w[DMA15_8], m_register_w[DMA7_0]);

			if (TRACE_TRANSFER && TRACE_DETAIL) logerror("%s: %s sector CHS=(%d,%d,%d)\n", tag(), m_write? "write" : "read",
					desired_cylinder(),
					desired_head(),
					desired_sector());

			if (m_write)
			{
				m_substate = DATA_TRANSFER_WRITE;
				live_start(WRITE_DAM_AND_SECTOR);
			}
			else
			{
				m_substate = DATA_TRANSFER_READ;
				live_start(SEARCH_DAM);
			}

			cont = WAIT;
			break;

		case DATA_TRANSFER_READ:
			// OK, sector has been read.
			// Check CRC
			if (m_live_state.crc != 0)
			{
				// Set Retry Required flag
				set_bits(m_register_r[CHIP_STATUS], CS_RETREQ, true);

				// Decrement the retry register (one's complemented value; 0000 = 15)
				int retry = 15-((m_register_w[RETRY_COUNT] >> 4)&0x0f);

				if (TRACE_FAIL) logerror("%s: DATA TRANSFER got CRC error in sector data, retries = %d\n", tag(), retry);
				m_register_w[RETRY_COUNT] = (m_register_w[RETRY_COUNT] & 0x0f) | ((15-(retry-1))<<4);

				if (retry == 0)
				{
					if (TRACE_FAIL) logerror("%s: CRC error; no retries left\n", tag());
					set_bits(m_register_r[CHIP_STATUS], CS_CRCERR, true);
					cont = ERROR;
				}
				else
				{
					// Go back to VERIFY and try again
					// Note that the specs recommend to set the retry to 0 (1111)
					// for physical reading; failing to do so will result in
					// unpredictable behavior.
					// We'll rely on the properly written software as well.
					m_live_state.bit_count_total = 0;
					m_substate = VERIFY2;
					cont = NEXT;
				}
			}
			else
			{
				if (TRACE_TRANSFER) logerror("%s: Sector successfully read\n", tag());

				// Update the DMA registers for multi-sector operations
				if (m_multi_sector)
				{
					int dma_address = (m_register_w[DMA23_16] & 0xff) << 16 |
						(m_register_w[DMA15_8] & 0xff) << 8 |
						(m_register_w[DMA7_0] & 0xff);

					dma_address = (dma_address + calc_sector_size()) & 0xffffff;

					m_register_w[DMA23_16] = m_register_r[DMA23_16] = (dma_address & 0xff0000) >> 16;
					m_register_w[DMA15_8] = m_register_r[DMA15_8] = (dma_address & 0x00ff00) >> 16;
					m_register_w[DMA7_0] = m_register_r[DMA7_0] = (dma_address & 0x0000ff) >> 16;
				}

				// Decrement the count
				m_register_w[SECTOR_COUNT] = (m_register_w[SECTOR_COUNT]-1) & 0xff;

				// Do we have more sectors to read?
				// Surprisingly, the manual does not say what happens when
				// the sector count is zero for the first access.
				// It explicitly states that the check is done after the access.
				// If we take it (and especially the state charts) seriously, zero means 256.
				// m_stop_after_index is important for physical reading
				if (m_register_w[SECTOR_COUNT] != 0 && !m_stop_after_index)
				{
					// Increment the sector number
					// What happens when we exceed the highest sector number
					// in the track? We have to assume that this is possible
					// and that in this case the VERIFY routine fails.
					m_register_w[DESIRED_SECTOR] = (desired_sector() + 1) & 0xff;
					m_substate = VERIFY1;
					cont = NEXT;
					m_live_state.bit_count_total = 0;
				}
				else
					cont = SUCCESS;
			}
			break;

		case DATA_TRANSFER_WRITE:
			if (TRACE_TRANSFER) logerror("%s: Sector successfully written\n", tag());

			// Update the DMA registers for multi-sector operations
			if (m_multi_sector)
			{
				int dma_address = (m_register_w[DMA23_16] & 0xff) << 16 |
				(m_register_w[DMA15_8] & 0xff) << 8 |
				(m_register_w[DMA7_0] & 0xff);

				dma_address = (dma_address + calc_sector_size()) & 0xffffff;

				m_register_w[DMA23_16] = m_register_r[DMA23_16] = (dma_address & 0xff0000) >> 16;
				m_register_w[DMA15_8] = m_register_r[DMA15_8] = (dma_address & 0x00ff00) >> 16;
				m_register_w[DMA7_0] = m_register_r[DMA7_0] = (dma_address & 0x0000ff) >> 16;
			}

			// Decrement the count
			m_register_w[SECTOR_COUNT] = (m_register_w[SECTOR_COUNT]-1) & 0xff;
			if (m_register_w[SECTOR_COUNT] != 0 && !m_stop_after_index)
			{
				m_register_w[DESIRED_SECTOR] = (desired_sector() + 1) & 0xff;
				m_substate = VERIFY1;
				cont = NEXT;
				m_live_state.bit_count_total = 0;
			}
			else
				cont = SUCCESS;

			break;

		default:
			logerror("%s: unknown substate %d in data_transfer\n", tag(), m_substate);
			cont = ERROR;
		}
	}

	if (cont==SUCCESS) set_command_done(TC_SUCCESS);

	//  When an error occurs, the COMMAND_TERMINATION bits are set to 11
	if (cont==ERROR)
	{
		live_abort();
		set_command_done(TC_DATAERR);
	}
}

// ===========================================================================
//     Commands
// ===========================================================================

/*
    RESET
*/
void hdc9234_device::reset_controller()
{
	logerror("%s: RESET command\n", tag());
	device_reset();
}
/*
    DESELECT DRIVE
*/
void hdc9234_device::drive_deselect()
{
	if (TRACE_SELECT) logerror("%s: DESELECT command\n", tag());
	m_selected_drive_number = NODRIVE;
	auxbus_out();
	set_command_done(TC_SUCCESS);
}

/*
    // RESTORE DRIVE
    // bit 0:
    // 0 -> command ends after last seek pulse,
    // 1 -> command ends when the drive asserts the seek complete pin
*/
void hdc9234_device::restore_drive()
{
	int cont = CONTINUE;

	// The substate is set to UNDEF when the command is started;
	// when we return here after a pause, the substate is set to some other value
	// In wd_fdc this is solved using two methods <command>_start and <command>_continue
	if (m_substate == UNDEF)
	{
		if (TRACE_RESTORE) logerror("%s: RESTORE command %02x\n", tag(), current_command());
		m_seek_count = 0;
		m_substate = RESTORE_CHECK1;
	}

	while (cont==CONTINUE)
	{
		switch (m_substate)
		{
		case RESTORE_CHECK1:
			if (TRACE_RESTORE && TRACE_SUBSTATES) logerror("%s: substate RESTORE_CHECK; seek count = %d\n", tag(), m_seek_count);
			// If the drive is on track 0 or not ready (no drive), terminate the command
			if (on_track00())
			{
				if (TRACE_RESTORE) logerror("%s: restore command TRK00 reached\n", tag());
				if (current_command() & 1)
				{
					// Buffered seek; wait for SEEK_COMPLETE
					wait_line(SEEKCOMP_LINE, ASSERT_LINE, SEEK_COMPLETE, false);
					cont = WAIT;
				}
				else
				{
					cont = SUCCESS;
				}
				break;
			}
			m_substate = RESTORE_CHECK2;
			break;

		case RESTORE_CHECK2:
			// Track 0 has not been reached yet
			if ((m_register_r[DRIVE_STATUS] & HDC_DS_READY)==0)
			{
				if (TRACE_RESTORE) logerror("%s: restore command: drive not ready\n", tag());
				// Does not look like a success, but this takes into account
				// that if a drive is not connected we do not want an error message
				cont = SUCCESS;
				break;
			}

			// Increase step count
			m_seek_count++;
			if (m_seek_count>=4096)
			{
				if (TRACE_FAIL) logerror("%s: restore command: failed to reach track 00\n", tag());
				set_command_done(TC_VRFYERR);
				cont = ERROR;
				break;
			}

			step_on(true, STEP_OFF);
			cont = WAIT;
			break;

		case STEP_OFF:
			step_off(RESTORE_CHECK1);
			cont = WAIT;
			break;

		case SEEK_COMPLETE:
			cont = SUCCESS;
			break;
		}
	}
	if (cont==SUCCESS) set_command_done(TC_SUCCESS);
}

/*
    STEP IN / OUT 1 CYLINDER
*/
void hdc9234_device::step_drive()
{
	int cont = CONTINUE;

	if (m_substate == UNDEF)
	{
		if (TRACE_STEP) logerror("%s: STEP IN/OUT command %02x\n", tag(), current_command());
		m_substate = STEP_ON;
	}

	while (cont==CONTINUE)
	{
		switch (m_substate)
		{
		case STEP_ON:
			step_on((current_command() & 0x02)!=0, STEP_OFF);
			cont = WAIT;
			break;
		case STEP_OFF:
			step_off(DONE);
			cont = WAIT;
			break;
		case DONE:
			cont = SUCCESS;
			break;
		}
	}
	if (cont==SUCCESS) set_command_done(TC_SUCCESS);
}

/*
    TAPE BACKUP
    Not implemented
*/
void hdc9234_device::tape_backup()
{
	logerror("%s: TAPE BACKUP command %02x not implemented\n", tag(), current_command());
	set_command_done(TC_SUCCESS);
}

/*
    POLL DRIVES
    Not implemented
*/
void hdc9234_device::poll_drives()
{
	logerror("%s: POLL DRIVES command %02x not implemented\n", tag(), current_command());
	set_command_done(TC_SUCCESS);
}

/*
    DRIVE SELECT

    Command word

            7     6     5     4     3     2     1     0
         +-----+-----+-----+-----+-----+-----+-----+-----+
         |  0  |  0  |  1  |Delay|    Type   |   Drive   |
         +-----+-----+-----+-----+-----+-----+-----+-----+

     [1] p.5: lower 4 bits of RETRY COUNT register (user programmable output) is put on OUTPUT1

     The HFDC controller board uses the user programmable output to
     select one of four floppy disk drives with Drive set to 00.
     Drive codes 01, 10, and 11 remain for three hard disk drives.
*/

void hdc9234_device::drive_select()
{
	int cont = CONTINUE;
	int head_load_delay = 0;

	if (m_substate == UNDEF)
	{
		int driveparm = current_command() & 0x1f;
		bool head_load_delay_enable = (driveparm & 0x10)!=0;

		// The drive type is used to configure DMA burst mode ([1], p.12)
		// and to select the timing parameters
		m_selected_drive_type = (driveparm>>2) & 0x03;
		m_selected_drive_number = driveparm & 0x03;

		// Calculate the head load delays
		head_load_delay = head_load_delay_enable? m_register_w[DATA] * head_load_timer_increment[m_selected_drive_type] : 0;
		if (fm_mode()) head_load_delay <<= 1;

		if (TRACE_SELECT) logerror("%s: DRIVE SELECT command (%02x): head load delay=%d, type=%d, drive=%d, pout=%02x, step_rate=%d\n", tag(), current_command(), head_load_delay, m_selected_drive_type, driveparm&3, m_register_w[RETRY_COUNT]&0x0f, pulse_width() + step_time());

		// Copy the DMA registers to registers CURRENT_HEAD, CURRENT_CYLINDER,
		// and CURRENT_IDENT. This is required during formatting ([1], p. 14)
		// as the format command reuses the registers for formatting parameters.
		m_register_r[CURRENT_HEAD] = m_register_r[DMA7_0];
		m_register_r[CURRENT_CYLINDER] = m_register_r[DMA15_8];
		m_register_r[CURRENT_IDENT] = m_register_r[DMA23_16];

		// Copy the selected drive number to the chip status register
		m_register_r[CHIP_STATUS] = (m_register_r[CHIP_STATUS] & 0xfc) | m_selected_drive_number;

		m_output1 = (m_selected_drive_number != NODRIVE)? (0x10 << m_selected_drive_number) : 0;
		m_output1 |= (m_register_w[RETRY_COUNT]&0x0f);
		if (TRACE_AUXBUS) logerror("%s: Setting OUTPUT1 to %02x\n", tag(), m_output1);

		auxbus_out();

		m_substate = (head_load_delay>0)? HEAD_DELAY : DONE;
	}

	// As for the head delay, the specs are not clear when it is applied.
	// There is no input line indicating whether the head is already loaded
	// (see WD17xx: HLT). Let's assume for now that the head is loaded with
	// this drive select operation, and that we have the delay here.
	switch (m_substate)
	{
	case HEAD_DELAY:
		wait_time(m_timer, head_load_delay, DONE);
		cont = WAIT;
		break;
	case DONE:
		cont = SUCCESS;
		break;
	}

	if (cont==SUCCESS) set_command_done(TC_SUCCESS);
}

/*
    SET REGISTER POINTER

    Sets the pointer to the read and write registers. On read or write accesses,
    the pointer is increased until it reaches the DATA register.
*/
void hdc9234_device::set_register_pointer()
{
	m_register_pointer = current_command() & 0xf;
	if (TRACE_SETPTR) logerror("%s: SET REGISTER POINTER command; start reg=%d\n", tag(), m_register_pointer);
	// The specification does not say anything about the effect of setting an
	// invalid value (only "care should be taken")
	if (m_register_pointer > 10)
	{
		logerror("%s: set register pointer: Invalid register number: %d. Setting to 10.\n", tag(), m_register_pointer);
		m_register_pointer = 10;
	}
	set_command_done(TC_SUCCESS);
}

/*
    SEEK / READ ID
    Not implemented
*/
void hdc9234_device::seek_read_id()
{
	logerror("%s: SEEK / READ ID command %02x not implemented\n", tag(), current_command());
	set_command_done(TC_SUCCESS);
}

/*
    READ SECTORS PHYSICAL / LOGICAL
    Read the desired sectors, maximum count being specified in SECTOR_COUNT

    Physical:
    For multiple sectors, read the sectors in the order as they appear on the track.
    The command terminates with the next index pulse or when all sectors have been read before.
    Implied seek (locate the correct track) is always true (opcodes 5a and 5b
    are used for READ TRACK).

    Logical:
    For multiple sectors, read the sectors in ascending order of their sector field (sector n, n+1, n+2 ...).

    Command flags
       [ 0 ]  [ 1 ]  [ 0 ]  [ 1 ]   [ 1 ] [ Logical ] [ NoImplSeek ] [ Transfer ]
*/
void hdc9234_device::read_sectors()
{
	bool logical = (current_command() & 0x04)!=0;

	if (m_substate == UNDEF)
	{
		// Command init
		if (TRACE_READ) logerror("%s: READ SECTORS %s command %02x, CHS=(%d,%d,%d)\n", tag(), logical? "LOGICAL": "PHYSICAL", current_command(), desired_cylinder(), desired_head(), desired_sector());
		m_retry_save = m_register_w[RETRY_COUNT];
		m_multi_sector = (m_register_w[SECTOR_COUNT] != 1);

		m_substate = READ_ID;
	}

	int cont = NEXT;
	bool implied_seek = (current_command() & 0x02)==0;
	m_transfer_enabled = (current_command() & 0x01)!=0;

	while (cont == NEXT)
	{
		switch (m_substate & 0xf0)
		{
		case READ_ID:
			read_id(cont, implied_seek);
			break;
		case VERIFY:
			verify(cont, logical);  // for physical, only verify the first sector
			break;
		case DATA_TRANSFER:
			m_write = false;
			data_transfer(cont);
			break;
		default:
			logerror("%s: unknown substate %d in read_sectors\n", tag(), m_substate);
			cont = ERROR;
		}
	}
}

/*
    READ TRACK
    Not implemented
*/
void hdc9234_device::read_track()
{
	logerror("%s: READ TRACK command %02x not implemented\n", tag(), current_command());
	set_command_done(TC_SUCCESS);
}

/*
    FORMAT TRACK
    Writes a track on the selected drive at the current cylinder. The write
    process starts with the falling edge of the index hole and stops with
    the rising edge of the next index hole.

    The formatting is done exclusively by the controller; user programs may
    set parameters for gaps and interleaving.

    1. Before starting the command, the user program must have set up a
    sector sequence table in the controller RAM (located on the PCB):
    (ident, cylinder, head, sector1, size)  (5 bytes)
    (ident, cylinder, head, sector2, size)
    (ident, cylinder, head, sector3, size)
    ...
    ident is not required for floppy FM operation. size is not required
    for IBM AT-compatible hard disks.

    2. The DMA registers must point to the beginning of the table

    3. DRIVE_SELECT must be executed (which moves DMA regs to CUR_HEAD ...)

    4. DESIRED_HEAD register must be loaded

    5. The following setup must be done:

    GAP 0 size            DMA7_0              (2s comp)
    GAP 1 size            DMA15_8             (2s comp)
    GAP 2 size            DMA23_16            (2s comp)
    GAP 3 size            DESIRED_SECTOR      (2s comp)
    Sync size             DESIRED_CYLINDER    (1s comp)
    Sector count          SECTOR_COUNT        (1s comp)
    Sector size multiple  RETRY_COUNT         (1s comp)

    GAP4 is variable and fills the rest of the track until the next
    index hole.

    6. The step rate and density must be loaded into the MODE register

    7. The drive must be stepped to the desired track.

    8. Now this command may be started.

    All data bytes of a sector are filled with 0xe5. The gaps will be filled
    with 0x4e (MFM) or 0xff (FM).

    To format another track, the sector id table must be updated, and steps
    7 and 8 must be repeated. If the DESIRED_HEAD register must be updated,
    the complete setup process must be done.

    Command flags
    [  0  ] [  1  ] [  1  ] [ NormalData ] [ ReducedWC ] [ PreC2, PreC1, PreC0 ]
*/
void hdc9234_device::format_track()
{
	if (m_substate == UNDEF)
	{
		if (TRACE_FORMAT) logerror("%s: FORMAT TRACK command %02x, head = %d\n", tag(), current_command(), desired_head());
		m_substate = WAITINDEX0;
		m_deleted = (current_command() & 0x10)==0;
		m_reduced_write_current = (current_command() & 0x08)!=0;
		m_precompensation = (current_command() & 0x07);

		m_gap0_size = -m_register_w[DMA7_0] & 0xff;
		m_gap1_size = -m_register_w[DMA15_8] & 0xff;
		m_gap2_size = -m_register_w[DMA23_16] & 0xff;
		m_gap3_size = -m_register_w[DESIRED_SECTOR] & 0xff;
		m_sync_size = ~m_register_w[DESIRED_CYLINDER] & 0xff;
		m_sector_count = ~m_register_w[SECTOR_COUNT] & 0xff;
		m_sector_size = (~m_register_w[RETRY_COUNT] & 0xff) * 128;

		if (TRACE_FORMAT && TRACE_DETAIL)
		{
			logerror("%s: GAP0 length  = %d\n", tag(), m_gap0_size);
			logerror("%s: GAP1 length  = %d\n", tag(), m_gap1_size);
			logerror("%s: GAP2 length  = %d\n", tag(), m_gap2_size);
			logerror("%s: GAP3 length  = %d\n", tag(), m_gap3_size);
			logerror("%s: Sync size    = %d\n", tag(), m_sync_size);
			logerror("%s: Sector count = %d\n", tag(), m_sector_count);
			logerror("%s: Sector size  = %d\n", tag(), m_sector_size);
		}

		dma_address_out(m_register_r[CURRENT_IDENT], m_register_r[CURRENT_CYLINDER], m_register_r[CURRENT_HEAD]);
	}

	int cont = NEXT;
	while (cont == NEXT)
	{
		switch (m_substate)
		{
		case WAITINDEX0:
			// Do we happen to have an index hole right now?
			if ((m_register_r[DRIVE_STATUS] & HDC_DS_INDEX)==0)
			{
				m_substate = WAITINDEX1;
				cont = NEXT;
			}
			else
			{
				// Waiting for the index line going down
				wait_line(INDEX_LINE, ASSERT_LINE, WAITINDEX1, false);
				cont = WAIT;
			}
			break;
		case WAITINDEX1:
			// Waiting for the next rising edge
			wait_line(INDEX_LINE, ASSERT_LINE, TRACKSTART, false);
			cont = WAIT;
			break;
		case TRACKSTART:
			live_start(FORMAT_TRACK);
			wait_line(INDEX_LINE, ASSERT_LINE, TRACKDONE, true);
			cont = WAIT;
			break;
		case TRACKDONE:
			if (FORMAT_TRACK && TRACE_SUBSTATES) logerror("%s: Track writing done\n", tag());
			cont = SUCCESS;
			break;
		}
	}

	if (cont==SUCCESS) set_command_done(TC_SUCCESS);
}

/*
    WRITE SECTORS PHYSICAL / LOGICAL

    Write the desired sectors, maximum count being specified in SECTOR_COUNT

    Physical:
    For multiple sectors, write sector contents into the data fields of
    the sectors as they are arranged on the track.
    The command terminates with the next index pulse or when all sectors have been written before.

    Logical:
    For multiple sectors, write the sectors in ascending order of their
    sector field (sector n, n+1, n+2 ...).

    Command flags
       [  1  ] [ NoImplSeek ] [  Logical  ]  [ NormalData ]  [ ReducedWC ]  [ PreC2, PreC1, PreC0 ]

*/
void hdc9234_device::write_sectors()
{
	bool logical = (current_command() & 0x20)!=0;

	if (m_substate == UNDEF)
	{
		if (TRACE_WRITE) logerror("%s: WRITE SECTORS %s command %02x, CHS=(%d,%d,%d)\n", tag(), logical? "LOGICAL" : "PHYSICAL", current_command(), desired_cylinder(), desired_head(), desired_sector());
		m_multi_sector = (m_register_w[SECTOR_COUNT] != 1);
		m_substate = READ_ID;

		m_deleted = (current_command() & 0x10)==0;
		m_reduced_write_current = (current_command() & 0x08)!=0;
		m_precompensation = (current_command() & 0x07);
		// Important for DATA TRANSFER
		m_transfer_enabled = true;
		m_write = true;
		m_sync_size = fm_mode()? 6 : 12;
		m_gap2_size = fm_mode()? 11 : 22;
	}

	int cont = NEXT;
	bool implied_seek = (current_command() & 0x40)==0;

	while (cont == NEXT)
	{
		// We're dispatching by substate value range
		switch (m_substate & 0xf0)
		{
		case READ_ID:
			read_id(cont, implied_seek);
			break;
		case VERIFY:
			verify(cont, logical);
			break;
		case DATA_TRANSFER:
			data_transfer(cont);
			break;
		default:
			logerror("%s: unknown substate %d in write_sectors\n", tag(), m_substate);
			cont = ERROR;
		}
	}
}

void hdc9234_device::reenter_command_processing()
{
	// Do we have a live run on the track?
	if (m_live_state.state != IDLE)
	{
		// Continue with it
		live_run();
		if (m_live_state.state != IDLE)  return;
	}

	// We're here when there is no live_run anymore
	// Where were we last time?
	// Take care not to restart commands because of the index callback
	if (m_executing && m_substate != UNDEF) (this->*m_command)();
}

// ===========================================================================

/*
    Step on / off; used by RESTORE and STEP IN/OUT
*/
void hdc9234_device::step_on(bool towards00, int next)
{
	if ((TRACE_RESTORE | TRACE_STEP) && TRACE_SUBSTATES) logerror("%s: substate STEP_ON\n", tag());

	// STEPDIR = 0 -> towards TRK00
	set_bits(m_output2, OUT2_STEPDIR, !towards00);

	// Raising edge (note that all signals must be inverted before leading them to the drive)
	set_bits(m_output2, OUT2_STEPPULSE, true);
	auxbus_out();
	wait_time(m_timer, pulse_width(), next);
}

void hdc9234_device::step_off(int next)
{
	if ((TRACE_RESTORE | TRACE_STEP) && TRACE_SUBSTATES) logerror("%s: substate STEP_OFF\n", tag());
	set_bits(m_output2, OUT2_STEPPULSE, false);
	auxbus_out();
	wait_time(m_timer, step_time(), next);
}

/*
    Delivers the step time (in microseconds) minus the pulse width
*/
int hdc9234_device::step_time()
{
	int time = 0;
	int index = m_register_w[MODE] & MO_STEPRATE;
	// Get seek time.
	if ((m_selected_drive_type & DRIVE_TYPE) == TYPE_FLOPPY8)
		time = step_flop8[index] - pulse_flop8;

	else if ((m_selected_drive_type & DRIVE_TYPE) == TYPE_FLOPPY5)
		time = step_flop5[index] - pulse_flop5;
	else
		time = step_hd[index] - pulse_hd;

	if (fm_mode()) time = time * 2;
	return time;
}

/*
    Delivers the pulse width time (in microseconds)
*/
int hdc9234_device::pulse_width()
{
	int time = 0;
	// Get seek time.
	if ((m_selected_drive_type & DRIVE_TYPE) == TYPE_FLOPPY8)
		time = pulse_flop8;

	else if ((m_selected_drive_type & DRIVE_TYPE) == TYPE_FLOPPY5)
		time = pulse_flop5;
	else
		time = pulse_hd;

	if (fm_mode()) time = time * 2;
	return time;
}

/*
    Delivers the sector size
*/
int hdc9234_device::calc_sector_size()
{
	return 128 << (m_register_r[CURRENT_SIZE] & 3);
}

/*
===========================================================================

    Live state machine

    We follow a very similar approach to track access like in wd_fdc. The live
    state machine attempts to find marks on the track, starting from the current
    position. When found, it waits for the machine to catch up. When an event
    happens in the meantime, the state machine is rolled back, and the actions
    are replayed until the position where the event occured.

    Lots of code is taken from wd_fdc, with some minor restructuring and renaming.
    Same ideas, though. More comments.

===========================================================================
*/

astring hdc9234_device::tts(const attotime &t)
{
	char buf[256];
	int nsec = t.attoseconds / ATTOSECONDS_PER_NANOSECOND;
	sprintf(buf, "%4d.%03d,%03d,%03d", int(t.seconds), nsec/1000000, (nsec/1000)%1000, nsec % 1000);
	return buf;
}

astring hdc9234_device::ttsn()
{
	return tts(machine().time());
}

/*
    The controller starts to read bits from the disk. This method takes an
    argument for the state machine called at the end.
*/
void hdc9234_device::live_start(int state)
{
	if (TRACE_LIVE) logerror("%s: [%s] Live start substate=%d\n", tag(), ttsn().cstr(), state);
	m_live_state.time = machine().time();
	m_live_state.state = state;
	m_live_state.next_state = -1;

	m_live_state.shift_reg = 0;
	m_live_state.crc = 0xffff;
	m_live_state.bit_counter = 0;
	m_live_state.byte_counter = 0;
	m_live_state.data_separator_phase = false;
	m_live_state.data_reg = 0;
	m_live_state.last_data_bit = false;

	pll_reset(m_live_state.time);
	m_checkpoint_state = m_live_state;

	// Save checkpoint
	m_checkpoint_pll = m_pll;

	live_run();
	m_last_live_state = UNDEF;
}

void hdc9234_device::live_run()
{
	live_run_until(attotime::never);
}

/*
    The main method of the live state machine. We stay in this method until
    the requested data are read.
    limit: if unlimited (attotime::never), run up to the end of the track and wait there
    otherwise, used to replay the read/write operation up to the point where the event happened
*/
void hdc9234_device::live_run_until(attotime limit)
{
	int slot = 0;

	if (m_live_state.state == IDLE || m_live_state.next_state != -1)
		return;

	if (TRACE_LIVE)
	{
		if (limit == attotime::never)
			logerror("%s: [%s] live_run, live_state=%d, mode=%s\n", tag(), tts(m_live_state.time).cstr(), m_live_state.state, fm_mode()? "FM":"MFM");
		else
			logerror("%s: [%s] live_run until %s, live_state=%d, mode=%s\n", tag(), tts(m_live_state.time).cstr(), tts(limit).cstr(), m_live_state.state, fm_mode()? "FM":"MFM");
	}

	if (limit == attotime::never)
	{
		// We did not specify an upper time bound, so we take the next index pulse
		if (m_floppy != NULL) limit = m_floppy->time_next_index();

		if (limit == attotime::never)
		{
			// We don't have an index pulse? (no disk?)
			// See wd_fdc: Force a sync from time to time in that case
			// so that the main cpu timeout isn't too painful.  Avoids
			// looping into infinity looking for data too.
			limit = machine().time() + attotime::from_msec(1);
			m_timer->adjust(attotime::from_msec(1));
		}
	}

	while (true)
	{
		switch (m_live_state.state)
		{
		case SEARCH_IDAM:

			// We're doing this complicated logerror check to avoid
			// repeated logging in the same state. This can be found for the
			// other live states as well. m_last_live_state is only used to
			// control this loggind.

			if (TRACE_LIVE && m_last_live_state != SEARCH_IDAM)
			{
				logerror("%s: [%s] SEARCH_IDAM [limit %s]\n", tag(),tts(m_live_state.time).cstr(), tts(limit).cstr());
				m_last_live_state = m_live_state.state;
			}

			// This bit will be set when the IDAM cannot be found
			set_bits(m_register_r[CHIP_STATUS], CS_SYNCERR, false);

			if (read_one_bit(limit))
			{
				if (TRACE_LIVE) logerror("%s: [%s] SEARCH_IDAM limit reached\n", tag(), tts(m_live_state.time).cstr());
				return;
			}
			// logerror("%s: SEARCH_IDAM\n", tts(m_live_state.time).cstr());
			if (TRACE_SHIFT) logerror("%s: shift = %04x data=%02x c=%d\n", tts(m_live_state.time).cstr(), m_live_state.shift_reg,
				get_data_from_encoding(m_live_state.shift_reg), m_live_state.bit_counter);

			// [1] p. 9: The ID field sync mark must be found within 33,792 byte times
			if (m_live_state.bit_count_total > 33792*16)
			{
				wait_for_realtime(SEARCH_IDAM_FAILED);
				return;
			}

			if (!fm_mode())
			{
				// MFM case
				if (m_live_state.shift_reg == 0x4489)
				{
					if (TRACE_LIVE) logerror("%s: [%s] Found an A1 mark\n", tag(),tts(m_live_state.time).cstr());
					m_live_state.crc = 0x443b;
					m_live_state.data_separator_phase = false;
					m_live_state.bit_counter = 0;
					// Next task: find the next two A1 marks
					m_live_state.state = READ_TWO_MORE_A1_IDAM;
				}
			}
			else
			{
				// FM case
				if (m_live_state.shift_reg == 0xf57e)
				{
					if (TRACE_LIVE) logerror("%s: SEARCH_IDAM: IDAM found\n", tag());
					m_live_state.crc = 0xef21;
					m_live_state.data_separator_phase = false;
					m_live_state.bit_counter = 0;
					m_live_state.state = READ_ID_FIELDS_INTO_REGS;
				}
			}
			break;

		case SEARCH_IDAM_FAILED:
			set_bits(m_register_r[CHIP_STATUS], CS_SYNCERR, true);
			m_live_state.state = IDLE;
			return;

		case READ_TWO_MORE_A1_IDAM:     // This state only applies for MFM mode.

			if (TRACE_LIVE && m_last_live_state != READ_TWO_MORE_A1_IDAM)
			{
				logerror("%s: [%s] READ_TWO_MORE_A1\n", tag(),tts(m_live_state.time).cstr());
				m_last_live_state = m_live_state.state;
			}

			// Beyond time limit?
			if (read_one_bit(limit)) return;

			if (TRACE_SHIFT) logerror("%s: shift = %04x data=%02x c=%d\n", tts(m_live_state.time).cstr(), m_live_state.shift_reg,
					get_data_from_encoding(m_live_state.shift_reg), m_live_state.bit_counter);

			if (m_live_state.bit_count_total > 33792*16)
			{
				wait_for_realtime(SEARCH_IDAM_FAILED);
				return;
			}

			// Repeat until we have collected 16 bits
			if (m_live_state.bit_counter & 15) break;

			// So we now got 16 bits. Fill this value into the next slot. We expect two more A1 values.
			slot = m_live_state.bit_counter >> 4;
			if (slot < 3)
			{
				if (m_live_state.shift_reg != 0x4489)
				{
					// This ain't A1. Step back into the previous state (look for the next IDAM).
					m_live_state.state = SEARCH_IDAM;
				}
				else
					if (TRACE_LIVE) logerror("%s: [%s] Found an A1 mark\n", tag(),tts(m_live_state.time).cstr());
				// Continue
				break;
			}

			if (TRACE_LIVE) logerror("%s: [%s] Found data value %02X\n", tag(),tts(m_live_state.time).cstr(), m_live_state.data_reg);

			// Check for ident field (fe, ff, fd, fc)
			if ((m_live_state.data_reg & 0xfc) != 0xfc)
			{
				// This may happen when we accidentally locked onto the DAM. Look for the next IDAM.
				if (TRACE_LIVE) logerror("%s: Missing ident data after A1A1A1\n", tag());
				m_live_state.state = SEARCH_IDAM;
				break;
			}

			m_register_r[CURRENT_IDENT] = m_live_state.data_reg;

			// We're here after we got the three A1 and FE
			m_live_state.bit_counter = 0;
			m_live_state.state = READ_ID_FIELDS_INTO_REGS;
			break;

		case READ_ID_FIELDS_INTO_REGS:
			if (TRACE_LIVE && m_last_live_state != READ_ID_FIELDS_INTO_REGS)
			{
				logerror("%s: [%s] READ_ID_FIELDS_INTO_REGS\n", tag(),tts(m_live_state.time).cstr());
				m_last_live_state = m_live_state.state;
			}

			if (read_one_bit(limit))
			{
				return;
			}
			// Already got 16 bits?
			if (m_live_state.bit_counter & 15) break;

			slot = (m_live_state.bit_counter >> 4)-1;

			if (TRACE_LIVE) logerror("%s: slot %d = %02x, crc=%04x\n", tag(), slot, m_live_state.data_reg, m_live_state.crc);

			// The id_field is an array of indexes into the chip registers.
			// Thus we get the values properly assigned to the registers.
			m_register_r[id_field[slot]] = m_live_state.data_reg;

			if(slot > 4)
			{
				// We successfully read the ID fields; let's wait for the machine time to catch up.
				// Live run is done here; it is the main state machine's turn again.
				m_live_state.bit_count_total = 0;
				wait_for_realtime(IDLE);
				return;
			}
			break;

		// ==================================================
		// Live states for sector read operations
		// ==================================================

		case SEARCH_DAM:
			if (TRACE_LIVE && m_last_live_state != SEARCH_DAM)
			{
				logerror("%s: [%s] SEARCH_DAM\n", tag(),tts(m_live_state.time).cstr());
				m_last_live_state = m_live_state.state;
			}

			set_bits(m_register_r[CHIP_STATUS], CS_DELDATA, false);

			if(read_one_bit(limit))
				return;

			if (TRACE_SHIFT) logerror("%s: shift = %04x data=%02x c=%d\n", tts(m_live_state.time).cstr(), m_live_state.shift_reg,
					get_data_from_encoding(m_live_state.shift_reg), m_live_state.bit_counter);

			if (!fm_mode())
			{   // MFM
				if(m_live_state.bit_counter > 43*16)
				{
					if (TRACE_FAIL) logerror("%s: SEARCH_DAM failed\n", tag());
					wait_for_realtime(SEARCH_DAM_FAILED);
					return;
				}

				if (m_live_state.bit_counter >= 28*16 && m_live_state.shift_reg == 0x4489)
				{
					if (TRACE_LIVE) logerror("%s: [%s] Found an A1 mark\n", tag(),tts(m_live_state.time).cstr());
					m_live_state.crc = 0x443b;
					m_live_state.data_separator_phase = false;
					m_live_state.bit_counter = 0;
					m_live_state.state = READ_TWO_MORE_A1_DAM;
				}
			}
			else
			{   // FM
				if (m_live_state.bit_counter > 23*16)
				{
					if (TRACE_FAIL) logerror("%s: SEARCH_DAM failed\n", tag());
					wait_for_realtime(SEARCH_DAM_FAILED);
					return;
				}

				if (m_live_state.bit_counter >= 11*16 && (m_live_state.shift_reg == 0xf56a || m_live_state.shift_reg == 0xf56b ||
														m_live_state.shift_reg == 0xf56e || m_live_state.shift_reg == 0xf56f)) {
					if (TRACE_LIVE) logerror("%s: SEARCH_DAM: found DAM = %04x\n", tag(), m_live_state.shift_reg);
					m_live_state.crc =
						m_live_state.shift_reg == 0xf56a ? 0x8fe7 :
						m_live_state.shift_reg == 0xf56b ? 0x9fc6 :
						m_live_state.shift_reg == 0xf56e ? 0xafa5 :
						0xbf84;
					m_live_state.data_separator_phase = false;
					m_live_state.bit_counter = 0;
					m_live_state.state = READ_SECTOR_DATA;
				}
			}
			break;

		case READ_TWO_MORE_A1_DAM: {
			if (TRACE_LIVE && m_last_live_state != READ_TWO_MORE_A1_DAM)
			{
				logerror("%s: [%s] READ_TWO_MORE_A1_DAM\n", tag(),tts(m_live_state.time).cstr());
				m_last_live_state = m_live_state.state;
			}

			if(read_one_bit(limit))
				return;

			if (TRACE_SHIFT) logerror("%s: shift = %04x data=%02x c=%d\n", tts(m_live_state.time).cstr(), m_live_state.shift_reg,
				get_data_from_encoding(m_live_state.shift_reg), m_live_state.bit_counter);

			// Repeat until we have collected 16 bits
			if (m_live_state.bit_counter & 15) break;

			// Fill this value into the next slot. We expect three A1 values.
			int slot = m_live_state.bit_counter >> 4;

			if (slot < 3)
			{
				if (m_live_state.shift_reg != 0x4489)
				{
					wait_for_realtime(SEARCH_DAM_FAILED);
					return;
				}
				else
					if (TRACE_LIVE) logerror("%s: [%s] Found an A1 mark\n", tag(),tts(m_live_state.time).cstr());
				// Continue
				break;
			}

			if (TRACE_LIVE) logerror("%s: [%s] Found data value %02X\n", tag(),tts(m_live_state.time).cstr(), m_live_state.data_reg);

			if ((m_live_state.data_reg & 0xff) == 0xf8)
			{
				if (TRACE_LIVE) logerror("%s: Found deleted data mark F8 after DAM sync\n", tag());
				set_bits(m_register_r[CHIP_STATUS], CS_DELDATA, true);
			}
			else
			{
				if ((m_live_state.data_reg & 0xff) != 0xfb)
				{
					if (TRACE_FAIL) logerror("%s: Missing FB/F8 data mark after DAM sync\n", tag());
					wait_for_realtime(SEARCH_DAM_FAILED);
					return;
				}
			}

			m_live_state.bit_counter = 0;
			m_live_state.state = READ_SECTOR_DATA;
			break;
		}
		case SEARCH_DAM_FAILED:
			if (TRACE_FAIL) logerror("%s: SEARCH_DAM failed\n", tag());
			m_live_state.state = IDLE;
			return;

		case READ_SECTOR_DATA:
		{
			if (TRACE_LIVE && m_last_live_state != READ_SECTOR_DATA)
			{
				logerror("%s: [%s] READ_SECTOR_DATA\n", tag(),tts(m_live_state.time).cstr());
				m_last_live_state = m_live_state.state;
			}

			if(read_one_bit(limit))
				return;

			// Request bus release at the first bit of each byte (floppy; [1], fig 5 and 6)
			if (m_transfer_enabled)
			{
				if ((m_live_state.bit_counter & 15)== 1)
				{
					set_bits(m_register_r[INT_STATUS], ST_OVRUN, true);
					m_out_dmarq(ASSERT_LINE);
				}
			}

			// Repeat until we have collected 16 bits
			if (m_live_state.bit_counter & 15) break;

			if (TRACE_LIVE) logerror("%s: [%s] Found data value %02X, CRC=%04x\n", tag(),tts(m_live_state.time).cstr(), m_live_state.data_reg, m_live_state.crc);
			int slot = (m_live_state.bit_counter >> 4)-1;

			if (slot < calc_sector_size())
			{
				// Sector data
				wait_for_realtime(READ_SECTOR_DATA1);
				return;
			}
			else if (slot < calc_sector_size()+2)
			{
				// CRC
				if (slot == calc_sector_size()+1)
				{
					if (TRACE_LIVE) logerror("%s: [%s] Sector read completed\n", tag(),tts(m_live_state.time).cstr());
					wait_for_realtime(IDLE);
					return;
				}
			}
			break;
		}

		case READ_SECTOR_DATA1:
			if (TRACE_LIVE && m_last_live_state != READ_SECTOR_DATA1)
			{
				logerror("%s: [%s] READ_SECTOR_DATA1\n", tag(),tts(m_live_state.time).cstr());
				m_last_live_state = m_live_state.state;
			}

			// Did the system CPU send the DMA ACK in the meantime?
			if ((m_register_r[INT_STATUS] & ST_OVRUN)!=0)
			{
				if (TRACE_FAIL) logerror("%s: No DMA ACK - buffer overrun\n", tag());
				set_bits(m_register_r[INT_STATUS], TC_DATAERR, true);
				m_live_state.state = IDLE;
				return;
			}

			if (m_transfer_enabled)
			{
				m_register_r[DATA] = m_register_w[DATA] = m_live_state.data_reg;
				m_out_dip(ASSERT_LINE);
				m_out_dma(0, m_register_r[DATA], 0xff);
				m_out_dip(CLEAR_LINE);

				m_out_dmarq(CLEAR_LINE);
			}

			m_live_state.state = READ_SECTOR_DATA;
			checkpoint();
			break;

		// ==================================================
		// Live states for sector write operations
		// ==================================================

		case WRITE_DAM_AND_SECTOR:
			// 1. Wait for 22*16 cells (MFM) or 11*16 cells (FM)  [704 usec, Gap 2]
			// 2. Write 12 (MFM) or 6 (FM) zeros
			// 3. Write 3*A1 sync plus the ident byte (MFM) or FB (FM) or F8 (deleted)
			// 4. Write the sector content and calculate the CRC on the fly
			// 5. Write the CRC bytes

			if (TRACE_LIVE)
				logerror("%s: [%s] WRITE_DAM_AND_SECTOR\n", tag(), tts(m_live_state.time).cstr());

			skip_on_track(m_gap2_size, WRITE_DAM_SYNC);
			break;

		case WRITE_DAM_SYNC:
			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Write sync zeros\n", tag());

			// Clear the overrun/underrun flag
			set_bits(m_register_r[INT_STATUS], ST_OVRUN, false);
			write_on_track(encode(0x00), m_sync_size, fm_mode()? WRITE_DATAMARK : WRITE_A1);
			break;

		case WRITE_A1:
			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Write three A1\n", tag());
			write_on_track(0x4489, 3, WRITE_DATAMARK);
			break;

		case WRITE_DATAMARK:
			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Write data mark and sector contents\n", tag());
			if (fm_mode())
			{
				// Init the CRC for the DAM and sector
				m_live_state.crc = 0xffff;

				// 1111 0101 0110 1010 = F8 deleted
				// 1111 0101 0110 1111 = FB normal
				write_on_track(m_deleted? 0xf56a : 0xf56f, 1, WRITE_SECDATA);
			}
			else
			{
				// Init the CRC for the ident byte and sector
				m_live_state.crc = 0xcdb4; // value for 3*A1
				write_on_track(encode(m_deleted? 0xf8 : 0xfb), 1, WRITE_SECDATA);
			}
			m_live_state.byte_counter = calc_sector_size();

			// Set the over/underrun flag and hope that it will be cleared before we start writing
			// (only for sector writing)
			if (m_substate == DATA_TRANSFER_WRITE)
			{
				set_bits(m_register_r[INT_STATUS], ST_OVRUN, true);
				m_out_dmarq(ASSERT_LINE);
			}
			break;

		case WRITE_SECDATA:
			if (m_substate == DATA_TRANSFER_WRITE)
			{
				// Check whether DMA has been acknowledged
				if ((m_register_r[INT_STATUS] & ST_OVRUN)!=0)
				{
					// No, then stop here
					m_live_state.state= NO_DMA_ACK;
				}
				else
				{
					m_out_dip(ASSERT_LINE);
					m_register_r[DATA] = m_register_w[DATA] = m_in_dma(0, 0xff);
					m_out_dip(CLEAR_LINE);
					m_out_dmarq(CLEAR_LINE);

					if (m_live_state.byte_counter > 0)
					{
						m_live_state.byte_counter--;
						write_on_track(encode(m_register_r[DATA]), 1, WRITE_SECDATA);
						m_out_dmarq(ASSERT_LINE);
					}
					else
					{
						m_live_state.state = WRITE_DATA_CRC;
						m_live_state.byte_counter = 2;
					}
				}
			}
			else
			{
				// We are here in the context of track formatting. Write a
				// blank sector
				write_on_track(encode(0xe5), m_sector_size, WRITE_DATA_CRC);
				m_live_state.byte_counter = 2;
			}
			break;

		case WRITE_DATA_CRC:
			// N.B.: when we write the first CRC byte, the value of the CRC will
			// change to the previous second byte, so we can write the first
			// byte in two iterations to get both
			if (m_live_state.byte_counter > 0)
			{
				if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Write CRC\n", tag());
				m_live_state.byte_counter--;
				write_on_track(encode((m_live_state.crc >> 8) & 0xff), 1, WRITE_DATA_CRC);
			}
			else
				m_live_state.state = WRITE_DONE;

			break;

		case WRITE_DONE:
			if (m_substate == DATA_TRANSFER_WRITE)
			{
				if (TRACE_WRITE) logerror("%s: Write sector complete\n", tag());
				m_pll.stop_writing(m_floppy, m_live_state.time);
				m_live_state.state = IDLE;
				return;
			}
			else
			{
				// Continue for track writing: Write GAP3
				m_live_state.state = WRITE_GAP3;
			}
			break;

// --------------------------------------------------------

		// ==================================================
		// Live states for track formatting
			// Write GAP 0
			// Write Sync+IXAM
			// Write GAP 1
			// Per sector
			//    Write Sync+IDAM
			//    Write Sector header+CRC
			//    Write GAP2
			//    Write Sync+DAM
			//    Write Sector data
			//    Write CRC bytes
			//    Write GAP3
			// Write GAP4 until the next pulse
		// ==================================================

		case FORMAT_TRACK:
			if (TRACE_LIVE) logerror("%s: FORMAT_TRACK\n", tag());
			m_live_state.state = WRITE_GAP0;
			m_pll.start_writing(m_live_state.time);
			break;

		case WRITE_GAP0:
			// GAP0 length is in DMA7_0 (negated, 2s comp)
			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Writing GAP0\n", tag());
			write_on_track(encode(fm_mode()? 0xff : 0x4e), m_gap0_size, WRITE_IXAM_SYNC);
			break;

		case WRITE_IXAM_SYNC:
			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Writing IXAM sync\n", tag());
			write_on_track(encode(0x00), m_sync_size, WRITE_IXAM);
			break;

		case WRITE_IXAM:
			// FM: FC with clock D7 = 1111 -111 -111 1010
			// MFM: C2 = 11000010
			// 0101 0010 -010 0100
			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Writing IXAM\n", tag());
			if (fm_mode())
				write_on_track(0xf77a, 1, WRITE_GAP1);
			else
				write_on_track(0x5224, 3, WRITE_FC);

			break;

		case WRITE_FC:
			// Only for MFM
			write_on_track(encode(0xfc), 1, WRITE_GAP1);
			break;

		case WRITE_GAP1:
			// GAP1 length is in DMA15_8
			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Writing GAP1\n", tag());
			write_on_track(encode(fm_mode()? 0xff : 0x4e), m_gap1_size, WRITE_IDAM_SYNC);
			break;

			// When does the HDC actually fetch the per-sector data? All data
			// at the beginning? Only the bytes for the next sector?
			// We assume it reads the bytes and writes them directly on the disk

		case WRITE_IDAM_SYNC:
			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Writing IDAM sync\n", tag());
			write_on_track(encode(0x00), m_sync_size, WRITE_IDAM);
			break;

		case WRITE_IDAM:
			// Set the over/underrun flag and hope that it will be cleared before we enter the next state (after writing)
			set_bits(m_register_r[INT_STATUS], ST_OVRUN, true);
			m_out_dmarq(ASSERT_LINE);

			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Writing IDAM and header\n", tag());
			if (fm_mode())
			{
				write_on_track(0xf57e, 1, WRITE_HEADER);
				m_live_state.byte_counter = 4;
			}
			else
			{
				write_on_track(0x4489, 3, WRITE_HEADER);
				m_live_state.byte_counter = 5;
			}
			m_live_state.crc = 0xffff;
			break;

		case WRITE_HEADER:
			if ((m_register_r[INT_STATUS] & ST_OVRUN)!=0)
				// No DMA (we do not get access to the ID table); exit
				m_live_state.state= NO_DMA_ACK;
			else
			{
				m_out_dip(ASSERT_LINE);
				m_live_state.byte_counter--;
				UINT8 headbyte = m_in_dma(0, 0xff);
				write_on_track(encode(headbyte), 1, (m_live_state.byte_counter>0)? WRITE_HEADER : WRITE_HEADER_CRC);
				m_out_dip(CLEAR_LINE);
				m_out_dmarq(CLEAR_LINE);
				// Writing will occur after the break; set the DMARQ again
				if (m_live_state.byte_counter>0)
					m_out_dmarq(ASSERT_LINE);
				else
					// we will go to WRITE_HEADER_CRC state; set the byte counter for CRC
					m_live_state.byte_counter = 2;
			}
			break;

		case WRITE_HEADER_CRC:
			if (m_live_state.byte_counter > 0)
			{
				UINT8 crct = (m_live_state.crc >> 8) & 0xff;
				if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Write CRC byte %02x\n", tag(), crct);
				m_live_state.byte_counter--;
				write_on_track(encode(crct), 1, WRITE_HEADER_CRC);
			}
			else
				m_live_state.state = WRITE_GAP2;

			break;

		case WRITE_GAP2:
			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Writing GAP2\n", tag());
			write_on_track(encode(fm_mode()? 0xff : 0x4e), m_gap2_size, WRITE_DAM_SYNC);
			break;

		case WRITE_GAP3:
			m_sector_count--;
			if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: Writing GAP3\n", tag());
			write_on_track(encode(fm_mode()? 0xff : 0x4e), m_gap3_size, (m_sector_count>0)? WRITE_IDAM_SYNC : WRITE_GAP4);
			break;

		case WRITE_GAP4:
			// Write bytes up to the end of the track
			if (TRACE_WRITE && TRACE_DETAIL && m_last_live_state != WRITE_GAP4)
			{
				logerror("%s: Writing GAP4\n", tag());
				m_last_live_state = WRITE_GAP4;
			}
			// Write a single byte; when the index hole shows up, the live run will be aborted
			write_on_track(encode(fm_mode()? 0xff : 0x4e), 1, WRITE_GAP4);
			break;

//  =================================================================

		case READ_TRACK_BYTE:
			// The pause is implemented by doing dummy reads on the floppy
			if (read_one_bit(limit))
			{
				if (TRACE_LIVE) logerror("%s: [%s] return; limit=%s\n", tag(), tts(m_live_state.time).cstr(), tts(limit).cstr());
				return;
			}

			// Repeat until we have collected 16 bits
			if ((m_live_state.bit_counter & 15)==0)
			{
				if (TRACE_READ && TRACE_DETAIL) logerror("%s: [%s] Read byte %02x, repeat = %d\n", tag(), tts(m_live_state.time).cstr(), m_live_state.data_reg, m_live_state.repeat);
				wait_for_realtime(READ_TRACK_NEXT_BYTE);
				return;
			}
			break;

		case READ_TRACK_NEXT_BYTE:
			m_live_state.state = READ_TRACK_BYTE;
			m_live_state.repeat--;
			if (m_live_state.repeat == 0)
			{
				// All bytes read
				m_live_state.state = m_live_state.return_state;
				checkpoint();
			}
			break;

		case WRITE_TRACK_BYTE:
			if (write_one_bit(limit))
				return;

			if (m_live_state.bit_counter == 0)
			{
				// All bits written; get the next byte into the shift register
				wait_for_realtime(WRITE_TRACK_NEXT_BYTE);
				return;
			}
			break;

		case WRITE_TRACK_NEXT_BYTE:
			m_live_state.state = WRITE_TRACK_BYTE;
			m_live_state.repeat--;

			// Write all bytes
			if (m_live_state.repeat == 0)
			{
				// All bytes written
				m_live_state.state = m_live_state.return_state;
				checkpoint();
			}
			else
				encode_again();

			break;

		case NO_DMA_ACK:
			if (TRACE_FAIL) logerror("%s: No DMA ACK - buffer underrun\n", tag());
			set_bits(m_register_r[INT_STATUS], TC_DATAERR, true);
			m_pll.stop_writing(m_floppy, m_live_state.time);
			m_live_state.state = IDLE;
			return;

		default:
			logerror("%s: Unknown live state: %02x\n", tag(), m_live_state.state);
			m_last_live_state = m_live_state.state;
			return;
		}
	}
	m_last_live_state = UNDEF;
}

/*
    Synchronize the live position on the track with the real time.
    Results in a new checkpoint and a live position at machine time or behind.
    As a side effect, portions of the track may be re-read
*/
void hdc9234_device::live_sync()
{
	// Do we have some time set?
	if (!m_live_state.time.is_never())
	{
		// Are we ahead of the machine time?
		if(m_live_state.time > machine().time())
		{
			// If so, we must roll back to the last checkpoint
			if (TRACE_SYNC) logerror("%s: [%s] Rolling back and replaying (%s)\n", tag(), ttsn().cstr(), tts(m_live_state.time).cstr());
			rollback();
			// and replay until we reach the machine time
			live_run_until(machine().time());
			// Caught up, write on floppy image
			m_pll.commit(m_floppy, m_live_state.time);
		}
		else
		{
			// We are behind machine time, so we will never get back to that
			// time, thus we can commit that position
			if (TRACE_SYNC) logerror("%s: [%s] Committing (%s)\n", tag(), ttsn().cstr(), tts(m_live_state.time).cstr());
			// Write on floppy image
			m_pll.commit(m_floppy, m_live_state.time);

			if (m_live_state.next_state != -1)
			{
				m_live_state.state = m_live_state.next_state;
			}

			if (m_live_state.state == IDLE)
			{
				m_pll.stop_writing(m_floppy, m_live_state.time);
				m_live_state.time = attotime::never;
			}
		}

		m_live_state.next_state = -1;
		checkpoint();
	}
}

void hdc9234_device::live_abort()
{
	if (!m_live_state.time.is_never() && m_live_state.time > machine().time())
	{
		if (TRACE_LIVE) logerror("%s: Abort; rolling back and replaying (%s)\n", ttsn().cstr(), tts(m_live_state.time).cstr());
		rollback();
		live_run_until(machine().time());
	}

	m_pll.stop_writing(m_floppy, m_live_state.time);
	m_live_state.time = attotime::never;
	m_live_state.state = IDLE;
	m_live_state.next_state = -1;
}

/*
    Brings the live state machine into the WRITE substate part
    comprised by WRITE_TRACK_(NEXT_)BYTE
    Arguments: byte to be written, number, state on return
*/
void hdc9234_device::write_on_track(UINT16 encoded, int repeat, int next_state)
{
	m_live_state.repeat = repeat;
	m_live_state.state = WRITE_TRACK_BYTE;
	m_live_state.return_state = next_state;
	encode_raw(encoded);
}

/*
    Brings the live state machine into the READ substate part. This is
    only intended for skipping bytes.
    Arguments: number, state on return
*/
void hdc9234_device::skip_on_track(int repeat, int next_state)
{
	m_live_state.bit_counter = 0;
	m_live_state.repeat = repeat;
	m_live_state.state = READ_TRACK_BYTE;
	m_live_state.return_state = next_state;
}

UINT8 hdc9234_device::get_data_from_encoding(UINT16 raw)
{
	return (raw & 0x4000 ? 0x80 : 0x00) |
			(raw & 0x1000 ? 0x40 : 0x00) |
			(raw & 0x0400 ? 0x20 : 0x00) |
			(raw & 0x0100 ? 0x10 : 0x00) |
			(raw & 0x0040 ? 0x08 : 0x00) |
			(raw & 0x0010 ? 0x04 : 0x00) |
			(raw & 0x0004 ? 0x02 : 0x00) |
			(raw & 0x0001 ? 0x01 : 0x00);
}

void hdc9234_device::rollback()
{
	m_live_state = m_checkpoint_state;
	m_pll = m_checkpoint_pll;
}

/*
    Wait for real time to catch up. This way we pretend that the last
    operation actually needed the real time.
*/
void hdc9234_device::wait_for_realtime(int state)
{
	m_live_state.next_state = state;
	m_timer->adjust(m_live_state.time - machine().time());
	if (TRACE_LIVE) logerror("%s: [%s] Waiting for real time [%s] to catch up\n", tag(), tts(m_live_state.time).cstr(), tts(machine().time()).cstr());
}

/*
    Read the next bit from the disk.
    Return true: the time limit has been reached
    Return false: The next bit is read into the shift register as the
    rightmost bit; the shift register is a member of m_live_state. Also,
    the CRC is updated.
*/
bool hdc9234_device::read_one_bit(const attotime &limit)
{
	// Get the next bit from the phase-locked loop.
	int bit = m_pll.get_next_bit(m_live_state.time, m_floppy, limit);

	// We have reached the time limit
	if (bit < 0) return true;

	// For test purposes: Drop a bit at some occasions
	// value > 1000: rare occasions
	// value = 500: can cope with
	// value < 100: big trouble for controller, will fail
	if (UNRELIABLE_MEDIA)
	{
		if ((machine().time().attoseconds % 1009)==0) bit = 0;
	}

	// Push into shift register
	m_live_state.shift_reg = (m_live_state.shift_reg << 1) | bit;
	m_live_state.bit_counter++;

	// Used for timeout handling
	m_live_state.bit_count_total++;

	// Clock bit (false) or data bit (true)?
	if (m_live_state.data_separator_phase==true)
	{
		m_live_state.data_reg = (m_live_state.data_reg << 1) | bit;
		// Update CRC
		if ((m_live_state.crc ^ (bit ? 0x8000 : 0x0000)) & 0x8000)
			m_live_state.crc = (m_live_state.crc << 1) ^ 0x1021;
		else
			m_live_state.crc = m_live_state.crc << 1;
	}

	m_live_state.data_separator_phase = !m_live_state.data_separator_phase;
	return false;
}

bool hdc9234_device::write_one_bit(const attotime &limit)
{
	bool bit = (m_live_state.shift_reg & 0x8000)!=0;

	bool over_limit = m_pll.write_next_bit(bit, m_live_state.time, m_floppy, limit);
	if (over_limit) return true;

	// Calculate the CRC from the data bits on the odd positions
	if (m_live_state.bit_counter & 1)
	{
		if ((m_live_state.crc ^ (bit ? 0x8000 : 0x0000)) & 0x8000)
			m_live_state.crc = (m_live_state.crc << 1) ^ 0x1021;
		else
			m_live_state.crc = m_live_state.crc << 1;
	}
	m_live_state.shift_reg = m_live_state.shift_reg << 1;
	m_live_state.bit_counter--;
	return false;
}

UINT16 hdc9234_device::encode(UINT8 byte)
{
	UINT16 raw;
	UINT8 check_pos;
	bool last_bit_set;
	check_pos =  0x80;

	if (fm_mode())
	{
		// Set all clock bits
		raw = 0xaaaa;

		// FM: data bit = 1 -> encode as 11
		//     data bit = 0 -> encode as 10
		for (int i=0; i<8; i++)
		{
			if (byte & check_pos) raw |= 0x4000 >> (2*i);
			check_pos >>= 1;
		}
		last_bit_set = ((byte & 1)!=0);
	}
	else
	{
		last_bit_set = m_live_state.last_data_bit;
		raw = 0;
		for (int i=0; i<8; i++)
		{
			bool bit_set = ((byte & check_pos)!=0);

			// MFM: data bit = 1 -> encode as 01
			//      data bit = 0 -> encode as x0 (x = !last_bit)
			if (bit_set)
				raw |= 0x4000 >> (2*i);
			else
				raw |= (last_bit_set? 0x0000 : 0x8000) >> (2*i);

			last_bit_set = bit_set;
			check_pos >>= 1;
		}
	}
	m_live_state.data_reg = byte;
	return raw;
}

/*
    Encode a byte for FM or MFM recording. Result is returned in the
    shift register of m_live_state.
*/
void hdc9234_device::encode_byte(UINT8 byte)
{
	UINT16 raw = encode(byte);
	m_live_state.bit_counter = 16;
	m_live_state.last_data_bit = raw & 1;
	m_live_state.shift_reg = m_live_state.shift_reg_save = raw;
	if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: [%s] Write %02x (%04x)\n", tag(), tts(m_live_state.time).cstr(), byte, raw);
	checkpoint();
}

void hdc9234_device::encode_again()
{
	encode_raw(m_live_state.shift_reg_save);
}

void hdc9234_device::encode_raw(UINT16 raw)
{
	m_live_state.bit_counter = 16;
	m_live_state.shift_reg = m_live_state.shift_reg_save = raw;
	m_live_state.last_data_bit = raw & 1;
	if (TRACE_WRITE && TRACE_DETAIL) logerror("%s: [%s] Write %02x (%04x)\n", tag(), tts(m_live_state.time).cstr(), get_data_from_encoding(raw), raw);
	checkpoint();
}

void hdc9234_device::pll_reset(const attotime &when)
{
	m_pll.reset(when);
	// In FM mode, cells are 4 usec long; in MFM they are 2 usec long.
	m_pll.set_clock(attotime::from_usec(fm_mode()? 4 : 2));
}

void hdc9234_device::checkpoint()
{
	// Write on floppy image
	m_pll.commit(m_floppy, m_live_state.time);
	m_checkpoint_state = m_live_state;
	m_checkpoint_pll = m_pll;
}

// ===========================================================================

/*
    This is pretty simple here, compared to wd17xx, because index and ready
    callbacks have to be tied to the controller board outside the chip.
*/
void hdc9234_device::connect_floppy_drive(floppy_image_device* floppy)
{
	m_floppy = floppy;
}

/*
    Read a byte of data from the controller
    The address (offset) encodes the C/D* line (command and /data)
*/
READ8_MEMBER( hdc9234_device::read )
{
	UINT8 reply = 0;

	if ((offset & 1) == 0)
	{
		// Data register
		reply = m_register_r[m_register_pointer];
		if (TRACE_READREG) logerror("%s: read register[%d] -> %02x\n", tag(), m_register_pointer, reply);

		// Autoincrement until DATA is reached.
		if (m_register_pointer < DATA)  m_register_pointer++;
	}
	else
	{
		// Status register
		reply = m_register_r[INT_STATUS];

		// "The interrupt pin is reset to its inactive state
		// when the UDC interrupt status register is read." [1] (p.3)
		if (TRACE_READREG) logerror("%s: read interrupt status register -> %02x\n", tag(), reply);
		set_interrupt(CLEAR_LINE);

		// Clear the bits due to interrupt status register read.
		m_register_r[INT_STATUS] &= ~(ST_INTPEND | ST_RDYCHNG);
	}
	return reply;
}

/*
    Write a byte to the controller
    The address (offset) encodes the C/D* line (command and /data), so there
    are only two addresses: 0 (register) and 1 (command).
    The operation terminates immediately, and the controller picks up the
    values stored in this phase at a later time.
*/
WRITE8_MEMBER( hdc9234_device::write )
{
	if ((offset & 1) == 0)
	{
		m_regvalue = data & 0xff;
		wait_time(m_cmd_timer, attotime::from_nsec(REGISTER_COMMIT), REGISTER_ACCESS);
	}
	else
	{
		if (m_executing)
		{
			logerror("%s: [%s] Error - previous command %02x not completed; new command %02x ignored\n", tag(), ttsn().cstr(), current_command(), data);
		}
		else
		{
			m_register_w[COMMAND] = data;
			wait_time(m_cmd_timer, attotime::from_nsec(COMMAND_COMMIT), COMMAND_INIT);
		}
	}
}

/*
    When the commit period has passed, process the command or register access
*/
void hdc9234_device::process_command()
{
	if (m_substate == REGISTER_ACCESS)
	{
		// Writing data to registers
		// Data register
		if (TRACE_SETREG)
		{
			if (m_register_pointer == INT_COMM_TERM)
				logerror("%s: Setting interrupt trigger DONE=%d READY=%d\n", tag(), (m_regvalue & TC_INTDONE)? 1:0, (m_regvalue & TC_INTRDCH)? 1:0);
			else
				logerror("%s: register[%d] <- %02x\n", tag(), m_register_pointer, m_regvalue);
		}
		m_register_w[m_register_pointer] = m_regvalue;

		// Changes to these registers must be output via the auxbus
		if (m_register_pointer == DESIRED_HEAD || m_register_pointer == RETRY_COUNT)
			auxbus_out();

		// The DMA registers and the sector register for read and
		// write are identical, so in that case we copy the contents
		if (m_register_pointer < DESIRED_HEAD) m_register_r[m_register_pointer] = m_regvalue;

		// Autoincrement until DATA is reached.
		if (m_register_pointer < DATA)  m_register_pointer++;
	}
	else
	{
		// Reset DONE and BAD_SECTOR [1], p.7
		set_bits(m_register_r[INT_STATUS], ST_DONE | ST_BADSECT, false);

		// Reset interrupt line (not explicitly mentioned in spec, but seems reasonable
		set_interrupt(CLEAR_LINE);

		// Clear Interrupt Pending and Ready Change
		set_bits(m_register_r[INT_STATUS], ST_INTPEND | ST_RDYCHNG, false);

		int index = 0;
		bool found = false;

		while (s_command[index].mask!=0 && !found)
		{
			if ((m_register_w[COMMAND] & s_command[index].mask) == s_command[index].baseval)
			{
				found = true;

				m_stop_after_index = false;
				m_wait_for_index = false;
				m_substate = UNDEF;
				m_executing = true;
				m_command = s_command[index].command;
				// Invoke command
				(this->*m_command)();
			}
			else index++;
		}
		if (!found)
		{
			logerror("%s: Command %02x not defined\n", tag(), m_register_w[COMMAND]);
		}
	}
}

/*
    Auxiliary bus operation.

    The auxbus of the HDC9234 is used to poll the drive status of the cur-
    rently selected drive, to transmit DMA address bytes, to output the
    OUTPUT1 register, and to output the OUTPUT2 register.

    The specification is not really precise on the times when this bus is
    used, but at least we can rely on this information:

    - Whenever there is no output of data, the bus is sampled. ([1], p.8,
      Drive status register). Data is sampled at the rising edge of STB*.
      As the minimum STB* pulse is 800ns with min 100ns S0/S1 settling time
      and min 100ns hold time we can say that the bus is polled at a maximum
      rate of 1 MHz.

    - Data for the DMA address is output only when the address is initially
      set; also when the address must be set again on error ([1], p.5,
      DMA registers). The external memory system has to take care of the
      addressing for subsequent bytes. The address will be increased by the
      length of a sector during multiple sector read/write operations.

    We may assume that the OUTPUT1 and OUTPUT2 operations only occur on
    changes to the registers in the controller. The values showing up on the
    auxiliary bus must be latched anyway.

    For the sampling of drive status values, the emulation would have to
    invoke a callback to the hosting board at a rate of about 1 MHz. Since
    the devices like floppy or hard disks are pushing their status changes,
    it makes much more sense to allow for an incoming call to the controller
    instead of a polling. This also allows to raise interrupts as soon
    as the drive status changes. The difference to the real controller
    would be less than 3 microseconds (in the worst case when the auxbus is
    currently outputting data as the drive status change occurs).

    Drive status read
    S0 = 0, S1 = 0
    +------+------+------+------+------+------+------+------+
    | ECC  |Index | SeekC| Tr00 | User | WrPrt| Ready|Fault |
    +------+------+------+------+------+------+------+------+
*/

/*
    Read the drive status over the auxbus
    (as said, let the controller board push the values into the controller)
*/
void hdc9234_device::auxbus_in(UINT8 data)
{
	// Kill unwanted input via auxbus until we are initialized.
	if (!m_initialized)
		return;

	if (TRACE_AUXBUS) logerror("%s: Got value %02x via auxbus: ecc=%d index=%d seek_comp=%d tr00=%d user=%d writeprot=%d ready=%d fault=%d\n",
				tag(), data,
				(data&HDC_DS_ECCERR)? 1:0, (data&HDC_DS_INDEX)? 1:0,
				(data&HDC_DS_SKCOM)? 1:0, (data&HDC_DS_TRK00)? 1:0,
				(data&HDC_DS_UDEF)? 1:0, (data&HDC_DS_WRPROT)? 1:0,
				(data&HDC_DS_READY)? 1:0, (data&HDC_DS_WRFAULT)? 1:0);
	UINT8 prev = m_register_r[DRIVE_STATUS];
	m_register_r[DRIVE_STATUS] = data;

	if ((prev & HDC_DS_INDEX) != (data &  HDC_DS_INDEX))
	{
		// Check whether index value changed
		index_callback((data & HDC_DS_INDEX)? ASSERT_LINE : CLEAR_LINE);
	}

	if ((prev & HDC_DS_READY) != (data & HDC_DS_READY))
	{
		// Check whether ready value changed
		ready_callback((data & HDC_DS_READY)? ASSERT_LINE : CLEAR_LINE);
	}

	if ((prev & HDC_DS_SKCOM) != (data & HDC_DS_SKCOM))
	{
		// Check whether seek complete value changed
		seek_complete_callback((data & HDC_DS_SKCOM)? ASSERT_LINE : CLEAR_LINE);
	}
}

void hdc9234_device::index_callback(int level)
{
	if (TRACE_LINES) logerror("%s: [%s] Index callback level=%d\n", tag(), ttsn().cstr(), level);

	// Synchronize our position on the track
	live_sync();

	if (level==ASSERT_LINE)
	{
		if (TRACE_INDEX) logerror("%s: Index pulse\n", tag());
		if (m_wait_for_index) m_stop_after_index = true;
	}

	if (m_event_line == INDEX_LINE && level == m_line_level && m_state_after_line != UNDEF)
	{
		if (TRACE_LINES) logerror("%s: [%s] Index pulse level=%d triggers event\n", tag(), ttsn().cstr(), level);
		m_substate = m_state_after_line;
		m_state_after_line = UNDEF;
		if (m_stopwrite)
		{
			m_pll.stop_writing(m_floppy, m_live_state.time);
			m_live_state.state = IDLE;
		}
	}

	reenter_command_processing();
}

void hdc9234_device::ready_callback(int level)
{
	if (TRACE_LINES) logerror("%s: [%s] Ready callback level=%d\n", tag(), ttsn().cstr(), level);

	// Set the interrupt status flag
	set_bits(m_register_r[INT_STATUS], ST_RDYCHNG, true);

	// Synchronize our position on the track
	live_sync();

	// Raise an interrupt if desired
	if (m_register_w[INT_COMM_TERM] & TC_INTRDCH)
	{
		if (TRACE_INT) logerror("%s: Raise interrupt READY change\n", tag());
		set_interrupt(ASSERT_LINE);
	}

	if (m_event_line == READY_LINE && level == m_line_level && m_state_after_line != UNDEF)
	{
		m_substate = m_state_after_line;
		m_state_after_line = UNDEF;
		reenter_command_processing();
	}
}

void hdc9234_device::seek_complete_callback(int level)
{
	if (TRACE_LINES) logerror("%s: [%s] Seek complete callback level=%d\n", tag(), ttsn().cstr(), level);

	// Synchronize our position on the track
	live_sync();

	if (m_event_line == SEEKCOMP_LINE && level == m_line_level && m_state_after_line != UNDEF)
	{
		m_substate = m_state_after_line;
		m_state_after_line = UNDEF;
		reenter_command_processing();
	}
}

/*
    Set the hook for line level handling
*/
void hdc9234_device::wait_line(int line, line_state level, int substate, bool stopwrite)
{
	m_event_line = line;
	m_line_level = level;
	m_state_after_line = substate;
	m_stopwrite = stopwrite;
}

/*
    Push the output registers over the auxiliary bus. It is expected that
    the PCB contains latches to store the values.

    OUTPUT1 register contents
    S0 = 0, S1 = 1
    +------+------+------+------+------+------+------+------+
    | Drv3 | Drv2 | Drv1 | Drv0 |  PO3 |  PO2 |  PO1 |  PO0 |
    +------+------+------+------+------+------+------+------+

    DrvX = select Drive X (only one bit allowed)
    POX = Programmable output X (contents from low 4 bits of register RETRY_COUNT)


    OUTPUT2 register contents
    S0 = 1, S1 = 1
    +------+------+------+------+------+------+------+------+
    | Drv3*| WCur | Dir  | Step |           Head            |
    +------+------+------+------+------+------+------+------+

    Drv3* = inverted Drv3 signal of OUTPUT1
    WCur = Reduced write current
    Dir = Step direction (0 = towards track 0)
    Step = Step pulse
    Head = desired head
*/
void hdc9234_device::auxbus_out()
{
	m_out_auxbus((offs_t)HDC_OUTPUT_1, m_output1);

	// prepare output2
	set_bits(m_output2, OUT2_DRVSEL3I, (m_output1 & OUT1_DRVSEL3)==0);

	m_output2 = (m_output2 & 0xb0) | desired_head();
	if (m_reduced_write_current) m_output2 |= OUT2_REDWRT;

	if (TRACE_AUXBUS) logerror("%s: Setting OUTPUT2 to %02x\n", tag(), m_output2);
	m_out_auxbus((offs_t)HDC_OUTPUT_2, m_output2);
}

void hdc9234_device::dma_address_out(UINT8 addrub, UINT8 addrhb, UINT8 addrlb)
{
	if (TRACE_DMA) logerror("%s: Setting DMA address %06x\n", tag(), (addrub<<16 | addrhb<<8 | addrlb)&0xffffff);
	m_out_auxbus((offs_t)HDC_OUTPUT_DMA_ADDR, addrub);
	m_out_auxbus((offs_t)HDC_OUTPUT_DMA_ADDR, addrhb);
	m_out_auxbus((offs_t)HDC_OUTPUT_DMA_ADDR, addrlb);
}

/*
    This is reached when a timer has expired
*/
void hdc9234_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	live_sync();

	switch (id)
	{
	case GEN_TIMER:
		reenter_command_processing();
		break;
	case COM_TIMER:
		process_command();
		break;
	case LIVE_TIMER:
		live_run();
		break;
	}
}

/*
    DMA acknowledge line.
*/
WRITE_LINE_MEMBER( hdc9234_device::dmaack )
{
	if (state==ASSERT_LINE)
	{
		if (TRACE_DMA) logerror("%s: [%s] DMA acknowledged\n", tag(), ttsn().cstr());
		set_bits(m_register_r[INT_STATUS], ST_OVRUN, false);
	}
}

/*
    Reset the controller. Negative logic, but we use ASSERT_LINE.
*/
WRITE_LINE_MEMBER( hdc9234_device::reset )
{
	if (state == ASSERT_LINE)
	{
		if (TRACE_LINES) logerror("%s: Reset via RST line\n", tag());
		device_reset();
	}
}

void hdc9234_device::device_start()
{
	m_out_intrq.resolve_safe();
	m_out_dip.resolve_safe();
	m_out_auxbus.resolve_safe();
	m_out_dmarq.resolve_safe();
	m_out_dma.resolve_safe();
	m_in_dma.resolve_safe(0);

	// allocate timers
	m_timer = timer_alloc(GEN_TIMER);
	m_cmd_timer = timer_alloc(COM_TIMER);
	m_live_timer = timer_alloc(LIVE_TIMER);

	m_live_state.state = IDLE;
}

void hdc9234_device::device_reset()
{
	m_deleted = false;
	m_executing = false;
	m_event_line = UNDEF;
	m_initialized = true;
	m_line_level = CLEAR_LINE;
	m_live_state.state = IDLE;
	m_live_state.time = attotime::never;
	m_multi_sector = false;
	m_output1 = 0;
	m_output2 = 0x80;
	m_precompensation = 0;
	m_reduced_write_current = false;
	m_regvalue = 0;
	m_register_pointer = 0;
	m_retry_save = 0;
	m_seek_count = 0;
	m_selected_drive_number = NODRIVE;
	m_selected_drive_type = 0;
	m_state_after_line = UNDEF;
	m_stop_after_index = false;
	m_substate = UNDEF;
	m_track_delta = 0;
	m_transfer_enabled = true;
	m_wait_for_index = false;
	m_write = false;

	for (int i=0; i<=11; i++)
		m_register_r[i] = m_register_w[i] = 0;

	set_interrupt(CLEAR_LINE);
	m_out_dip(CLEAR_LINE);
	m_out_dmarq(CLEAR_LINE);
}

const device_type HDC9234 = &device_creator<hdc9234_device>;
