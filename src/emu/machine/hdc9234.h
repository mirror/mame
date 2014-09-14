// license:BSD-3-Clause
// copyright-holders:Michael Zapf
/*
    HDC9234 Hard and Floppy Disk Controller
    For details see hdc9234.c
*/
#ifndef __HDC9234_H__
#define __HDC9234_H__

#include "emu.h"
#include "imagedev/floppy.h"
#include "fdc_pll.h"

extern const device_type HDC9234;

/*
    Enumeration of the latches outside of the controller
*/
enum
{
	HDC_INPUT_STATUS    = 0x00,
	HDC_OUTPUT_DMA_ADDR = 0x01,
	HDC_OUTPUT_1        = 0x02,
	HDC_OUTPUT_2        = 0x03
};


/*
    Definition of bits in the Disk-Status register
*/
enum
{
	HDC_DS_ECCERR  = 0x80,        // ECC error
	HDC_DS_INDEX   = 0x40,        // index hole
	HDC_DS_SKCOM   = 0x20,        // seek complete
	HDC_DS_TRK00   = 0x10,        // track 0
	HDC_DS_UDEF    = 0x08,        // user-defined
	HDC_DS_WRPROT  = 0x04,        // write-protected
	HDC_DS_READY   = 0x02,        // drive ready bit
	HDC_DS_WRFAULT = 0x01         // write fault
};

//===================================================================

/* Interrupt line. To be connected with the controller PCB. */
#define MCFG_HDC9234_INTRQ_CALLBACK(_write) \
	devcb = &hdc9234_device::set_intrq_wr_callback(*device, DEVCB_##_write);

/* DMA request line. To be connected with the controller PCB. */
#define MCFG_HDC9234_DMARQ_CALLBACK(_write) \
	devcb = &hdc9234_device::set_dmarq_wr_callback(*device, DEVCB_##_write);

/* DMA in progress line. To be connected with the controller PCB. */
#define MCFG_HDC9234_DIP_CALLBACK(_write) \
	devcb = &hdc9234_device::set_dip_wr_callback(*device, DEVCB_##_write);

/* Auxiliary Bus. These 8 lines need to be connected to external latches
   and to a counter circuitry which works together with the external RAM.
   We use the S0/S1 lines as address lines. */
#define MCFG_HDC9234_AUXBUS_OUT_CALLBACK(_write) \
	devcb = &hdc9234_device::set_auxbus_wr_callback(*device, DEVCB_##_write);

/* Callback to read the contents of the external RAM via the data bus.
   Note that the address must be set and automatically increased
   by external circuitry. */
#define MCFG_HDC9234_DMA_IN_CALLBACK(_read) \
	devcb = &hdc9234_device::set_dma_rd_callback(*device, DEVCB_##_read);

/* Callback to write the contents of the external RAM via the data bus.
   Note that the address must be set and automatically increased
   by external circuitry. */
#define MCFG_HDC9234_DMA_OUT_CALLBACK(_write) \
	devcb = &hdc9234_device::set_dma_wr_callback(*device, DEVCB_##_write);

//===================================================================

class hdc9234_device : public device_t
{
public:
	hdc9234_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// Accesors from the CPU side
	DECLARE_READ8_MEMBER( read );
	DECLARE_WRITE8_MEMBER( write );
	DECLARE_WRITE_LINE_MEMBER( reset );
	DECLARE_WRITE_LINE_MEMBER( dmaack );

	// Callbacks
	template<class _Object> static devcb_base &set_intrq_wr_callback(device_t &device, _Object object) { return downcast<hdc9234_device &>(device).m_out_intrq.set_callback(object); }
	template<class _Object> static devcb_base &set_dmarq_wr_callback(device_t &device, _Object object) { return downcast<hdc9234_device &>(device).m_out_dmarq.set_callback(object); }
	template<class _Object> static devcb_base &set_dip_wr_callback(device_t &device, _Object object) { return downcast<hdc9234_device &>(device).m_out_dip.set_callback(object); }
	template<class _Object> static devcb_base &set_auxbus_wr_callback(device_t &device, _Object object) { return downcast<hdc9234_device &>(device).m_out_auxbus.set_callback(object); }
	template<class _Object> static devcb_base &set_dma_rd_callback(device_t &device, _Object object) { return downcast<hdc9234_device &>(device).m_in_dma.set_callback(object); }
	template<class _Object> static devcb_base &set_dma_wr_callback(device_t &device, _Object object) { return downcast<hdc9234_device &>(device).m_out_dma.set_callback(object); }

	// auxbus_in is intended to read events from the drives
	// In the real chip the status is polled; to avoid unnecessary load
	// we implement it as a push call
	void auxbus_in( UINT8 data );

	// Used to reconfigure the drive connections. Floppy drive selection is done
	// using the user-programmable outputs. Hence, the connection
	// is changed outside of the controller, and by this way we let it know.
	void connect_floppy_drive(floppy_image_device *floppy);

protected:
	void device_start();
	void device_reset();

private:
	devcb_write_line   m_out_intrq;    // INT line
	devcb_write_line   m_out_dmarq;    // DMA request line
	devcb_write_line   m_out_dip;      // DMA in progress line
	devcb_write8       m_out_auxbus;   // AB0-7 lines (using S0,S1 as address)
	devcb_read8        m_in_dma;       // DMA read access to the cache buffer
	devcb_write8       m_out_dma;      // DMA write access to the cache buffer

	// Internal register pointer used for sequential register loading
	int m_register_pointer;

	// Read and write registers
	UINT8 m_register_w[12];
	UINT8 m_register_r[15];

	// Interrupt management (outgoing INT pin)
	void set_interrupt(line_state intr);

	// Currently connected floppy
	floppy_image_device* m_floppy;

	// internal register OUTPUT1
	UINT8 m_output1;

	// internal register OUTPUT2
	UINT8 m_output2;

	// Write the output registers to the latches
	void auxbus_out();

	// Write the DMA address to the external latches
	void dma_address_out();

	// Intermediate storage for register
	UINT8 m_regvalue;

	// Drive type that has been selected in drive_select
	int m_selected_drive_type;

	// Drive numbere that has been selected in drive_select
	int m_selected_drive_number;

	// Indicates whether the device has completed initialization
	bool m_initialized;

	// Timers to delay execution/completion of commands */
	emu_timer *m_timer;
	emu_timer *m_cmd_timer;
	emu_timer *m_live_timer;

	// Timer callback
	void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

	// Callbacks
	void ready_callback(int level);
	void index_callback(int level);
	void seek_complete_callback(int level);

	// Wait for some time to pass or for a line to be raised
	void wait_time(emu_timer *tm, int microsec, int next_substate);
	void wait_time(emu_timer *tm, attotime delay, int param);
	void wait_line(int substate);

	// Converts attotime to a string
	astring tts(const attotime &t);

	// Current time
	astring ttsn();

	// Utility routine to set or reset bits
	void set_bits(UINT8& byte, int mask, bool set);

	// ==============================================
	//   Live state machine
	// ==============================================

	struct live_info
	{
		attotime time;
		UINT16 shift_reg;
		UINT16 crc;
		int bit_counter;
		int bit_count_total;    // used for timeout handling
		int byte_counter;
		bool data_separator_phase;
		bool last_data_bit;
		UINT8 data_reg;
		int state;
		int next_state;
	};

	live_info m_live_state, m_checkpoint_state;
	int m_last_live_state;

	// Starts the live run
	void live_start(int state);

	// Analyses the track until the given time
	void live_run_until(attotime limit);

	// Live run until next index pulse
	void live_run();

	// Control functions for syncing the track analyser with the machine time
	void wait_for_realtime(int state);
	void live_sync();
	void rollback();
	void checkpoint();

	// ==============================================
	//    PLL functions and interface to floppy
	// ==============================================

	// Phase-locked loops
	fdc_pll_t m_pll, m_checkpoint_pll;

	// Resets the PLL to the given time
	void pll_reset(const attotime &when);

	// Encodes the byte using FM or MFM. Changes the m_live_state members
	// shift_reg, data_reg, and last_data_bit
	void encode_byte(UINT8 byte);

	// Puts the word into the shift register directly. Changes the m_live_state members
	// shift_reg, and last_data_bit
	void encode_raw(UINT16 word);

	// Reads from the current position on the track
	bool read_one_bit(const attotime &limit);

	// Writes to the current position on the track
	bool write_one_bit(const attotime &limit);

	// ==============================================
	//   Command state machine
	// ==============================================

	int m_substate;
	int m_state_after_line;

	typedef void (hdc9234_device::*cmdfunc)(void);

	typedef struct
	{
		UINT8 baseval;
		UINT8 mask;
		cmdfunc command;
	} cmddef;

	static const cmddef s_command[];

	// Indicates whether a command is currently being executed
	bool m_executing;

	// Keeps the pointer to the function for later continuation
	cmdfunc m_command;

	// Invoked after the commit period for command initiation or register write access
	void process_command();

	// Re-enters the state machine after a delay
	void reenter_command_processing();

	// Command is done
	void set_command_done(int flags);
	void set_command_done();

	// Difference between current cylinder and desired cylinder
	int m_track_delta;

	// Used to restore the retry count for multi-sector operations
	int m_retry_save;

	// ==============================================
	//   Operation properties
	// ==============================================

	// Precompensation value
	int m_precompensation;

	// Do we have a multi-sector operation?
	bool m_multi_sector;

	// Shall we wait for the index hole?
	bool m_wait_for_index;

	// Shall we stop after the next index hole?
	bool m_stop_after_index;

	// Is data transfer enabled for read operations?
	bool m_transfer_enabled;

	// Is it a read or a write operation?
	bool m_write;

	// Have we found a deleted sector?
	bool m_deleted;

	// Do we apply a reduced write current?
	bool m_reduced_write_current;

	// Used in RESTORE to find out when to give up
	int m_seek_count;

	// Are we in FM mode?
	bool fm_mode();

	// Delivers the desired head
	int desired_head();

	// Delivers the desired sector
	int desired_sector();

	// Delivers the desired cylinder. The value is spread over two registers.
	int desired_cylinder();

	// Delivers the current head as read from the track
	int current_head();

	// Delivers the current sector as read from the track
	int current_sector();

	// Delivers the current cylinder as read from the track
	int current_cylinder();

	// Delivers the current command
	UINT8 current_command();

	// Step time (minus pulse width)
	int step_time();

	// Step pulse width
	int pulse_width();

	// Sector size as read from the track
	int calc_sector_size();

	// Are we on track 0?
	bool on_track00();

	// Common subprograms READ ID, VERIFY, and DATA TRANSFER
	void read_id(int& cont, bool implied_seek);
	void verify(int& cont, bool verify_all);
	void data_transfer(int& cont);

	// Activates the step line
	void step_on(bool towards00, int next);

	// Clears the step line (after the pulse length time)
	void step_off(int next);

	// ===================================================
	//   Commands
	// ===================================================

	void reset_controller();
	void drive_select();
	void drive_deselect();
	void restore_drive();
	void step_drive();
	void set_register_pointer();
	void read_sectors();
	void write_sector_logical();
};

#endif
