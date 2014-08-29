/* machine/n64.c - contains N64 hardware emulation shared between MAME and MESS */

#include "emu.h"
#include "debugger.h"
#include "cpu/mips/mips3.h"
#include "cpu/mips/mips3com.h"
#include "includes/n64.h"
#include "video/n64.h"

UINT32 *n64_sram;
UINT32 *rdram;
UINT32 *rsp_imem;
UINT32 *rsp_dmem;

// device type definition
const device_type N64PERIPH = &device_creator<n64_periphs>;





n64_periphs::n64_periphs(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, N64PERIPH, "N64 Periphal Chips", tag, owner, clock, "n64_periphs", __FILE__)
	, device_video_interface(mconfig, *this)
	, m_nvram_image(NULL)
{
}

TIMER_CALLBACK_MEMBER(n64_periphs::reset_timer_callback)
{
	reset_tick();
}

void n64_periphs::reset_tick()
{
	reset_timer->adjust(attotime::never);
	maincpu->reset();
	maincpu->execute().set_input_line(INPUT_LINE_IRQ2, CLEAR_LINE);
	rspcpu->reset();
	rspcpu->execute().set_input_line(INPUT_LINE_HALT, ASSERT_LINE);
	rspcpu->state().set_state_int(RSP_SR, rspcpu->state().state_int(RSP_SR) | RSP_STATUS_HALT);
	reset_held = false;
	cic_status = 0;
	memset(pif_ram, 0, sizeof(pif_ram));
	switch(cic_type)
	{
		case 1:
			pif_ram[0x24] = 0x00;
			pif_ram[0x25] = 0x06;
			pif_ram[0x26] = 0x3f;
			pif_ram[0x27] = 0x3f;
			break;
		case 3:
			pif_ram[0x24] = 0x00;
			pif_ram[0x25] = 0x02;
			pif_ram[0x26] = 0x78;
			pif_ram[0x27] = 0x3f;
			break;
		case 5:
			pif_ram[0x24] = 0x00;
			pif_ram[0x25] = 0x02;
			pif_ram[0x26] = 0x91;
			pif_ram[0x27] = 0x3f;
			break;
		case 6:
			pif_ram[0x24] = 0x00;
			pif_ram[0x25] = 0x02;
			pif_ram[0x26] = 0x85;
			pif_ram[0x27] = 0x3f;
			break;
		case 0xd:
			pif_ram[0x24] = 0x00;
			pif_ram[0x25] = 0x0a;
			pif_ram[0x26] = 0xdd;
			pif_ram[0x27] = 0x3f;
			break;
		default:
			pif_ram[0x24] = 0x00;
			pif_ram[0x25] = 0x02;
			pif_ram[0x26] = 0x3f;
			pif_ram[0x27] = 0x3f;
			break;
	}
}

void n64_periphs::poll_reset_button(bool button)
{
	bool old_held = reset_held;
	reset_held = button;
	if(!old_held && reset_held)
	{
		machine().device("maincpu")->execute().set_input_line(INPUT_LINE_IRQ2, ASSERT_LINE);
	}
	else if(old_held && reset_held)
	{
		reset_timer->adjust(attotime::never);
	}
	else if(old_held && !reset_held)
	{
		reset_timer->adjust(attotime::from_hz(1));
	}
}

void n64_periphs::device_start()
{
	ai_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(n64_periphs::ai_timer_callback),this));
	pi_dma_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(n64_periphs::pi_dma_callback),this));
	vi_scanline_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(n64_periphs::vi_scanline_callback),this));
	reset_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(n64_periphs::reset_timer_callback),this));
}

void n64_periphs::device_reset()
{
	UINT32 *cart = (UINT32*)machine().root_device().memregion("user2")->base();

	maincpu = machine().device("maincpu");
	rspcpu = machine().device("rsp");
	mem_map = &maincpu->memory().space(AS_PROGRAM);

	mi_version = 0x01010101;
	mi_interrupt = 0;
	mi_intr_mask = 0;
	mi_mode = 0x80; // Skip RDRAM initialization

	sp_mem_addr = 0;
	sp_dram_addr = 0;
	sp_dma_length = 0;
	sp_dma_count = 0;
	sp_dma_skip = 0;
	sp_semaphore = 0;

	vi_width = 0;
	vi_origin = 0;
	vi_control = 0;
	vi_burst = 0;
	vi_vsync = 0;
	vi_hsync = 0;
	vi_leap = 0;
	vi_hstart = 0;
	vi_vstart = 0;
	vi_intr = 0;
	vi_vburst = 0;
	vi_xscale = 0;
	vi_yscale = 0;

	ai_dac[0] = machine().device<dmadac_sound_device>("dac1");
	ai_dac[1] = machine().device<dmadac_sound_device>("dac2");
	ai_timer->adjust(attotime::never);
	memset(ai_fifo, 0, sizeof(ai_fifo));
	ai_fifo_wpos = 0;
	ai_fifo_rpos = 0;
	ai_fifo_num = 0;
	ai_dram_addr = 0;
	ai_len = 0 ;
	ai_control = 0;
	ai_dacrate = 0;
	ai_bitrate = 0;
	ai_status = 0;

	pi_dma_timer->adjust(attotime::never);
	pi_rd_len = 0;
	pi_wr_len = 0;
	pi_status = 0;
	pi_bsd_dom1_lat = 0;
	pi_bsd_dom1_pwd = 0;
	pi_bsd_dom1_pgs = 0;
	pi_bsd_dom1_rls = 0;
	pi_bsd_dom2_lat = 0;
	pi_bsd_dom2_pwd = 0;
	pi_bsd_dom2_pgs = 0;
	pi_bsd_dom2_rls = 0;
	pi_dma_dir = 0;

	dd_int = 0;
	memset(dd_buffer, 0, sizeof(dd_buffer));
	memset(dd_sector_data, 0, sizeof(dd_sector_data));
	memset(dd_ram_seq_data, 0, sizeof(dd_ram_seq_data));
	dd_data_reg = 0;
	dd_status_reg = 0;
	dd_track_reg = 0;
	dd_buf_status_reg = 0;
	dd_sector_err_reg = 0;
	dd_seq_status_reg = 0;
	dd_seq_ctrl_reg = 0;
	dd_int = 0;

	memset(ri_regs, 0, sizeof(ri_regs));

	ri_regs[0] = 0xe; // Skip RDRAM initialization
	ri_regs[1] = 0x40; //
	ri_regs[3] = 0x14; //
	ri_regs[4] = 0x63634; //
	memset(pif_ram, 0, sizeof(pif_ram));
	memset(pif_cmd, 0, sizeof(pif_cmd));
	si_dram_addr = 0;
	si_pif_addr = 0;
	si_status = 0;

	memset(m_save_data.eeprom, 0, 2048);

	dp_clock = 0;

	cic_status = 0;

	reset_held = false;
	reset_timer->adjust(attotime::never);

	// bootcode differs between CIC-chips, so we can use its checksum to detect the CIC-chip
	UINT64 boot_checksum = 0;
	for(int i = 0x40; i < 0x1000; i+=4)
	{
		boot_checksum += cart[i/4]+i;
	}

	// CIC-NUS-6102 (default)
	pif_ram[0x24] = 0x00;
	pif_ram[0x25] = 0x00;
	pif_ram[0x26] = 0x3f;
	pif_ram[0x27] = 0x3f;
	dd_present = false;
	cic_type=2;
	mem_map->write_dword(0x00000318, 0x800000);

	if (boot_checksum == U64(0x00000000001ff230))
	{
		pif_ram[0x24] = 0x00;
		pif_ram[0x25] = 0x08;
		pif_ram[0x26] = 0xdd;
		pif_ram[0x27] = 0x3f;
		dd_present = true;
		cic_type=0xd;
	}
	else if (boot_checksum == U64(0x000000cffb830843) || boot_checksum == U64(0x000000d0027fdf31))
	{
		// CIC-NUS-6101
		pif_ram[0x24] = 0x00;
		pif_ram[0x25] = 0x04;
		pif_ram[0x26] = 0x3f;
		pif_ram[0x27] = 0x3f;
		cic_type=1;
	}
	else if (boot_checksum == U64(0x000000d6499e376b))
	{
		// CIC-NUS-6103
		pif_ram[0x24] = 0x00;
		pif_ram[0x25] = 0x00;
		pif_ram[0x26] = 0x78;
		pif_ram[0x27] = 0x3f;
		cic_type=3;
	}
	else if (boot_checksum == U64(0x0000011a4a1604b6))
	{
		// CIC-NUS-6105
		pif_ram[0x24] = 0x00;
		pif_ram[0x25] = 0x00;
		pif_ram[0x26] = 0x91;
		pif_ram[0x27] = 0x3f;
		cic_type=5;
		mem_map->write_dword(0x000003f0, 0x800000);
	}
	else if (boot_checksum == U64(0x000000d6d5de4ba0))
	{
		// CIC-NUS-6106
		pif_ram[0x24] = 0x00;
		pif_ram[0x25] = 0x00;
		pif_ram[0x26] = 0x85;
		pif_ram[0x27] = 0x3f;
		cic_type=6;
	}
	else
	{
		//printf("Unknown BootCode Checksum %08X%08X\n", (UINT32)(boot_checksum>>32),(UINT32)(boot_checksum));
	}
}

// Memory Interface (MI)

#define MI_CLR_INIT             0x0080      /* Bit  7: clear init mode */
#define MI_SET_INIT             0x0100      /* Bit  8: set init mode */
#define MI_CLR_EBUS             0x0200      /* Bit  9: clear ebus test */
#define MI_SET_EBUS             0x0400      /* Bit 10: set ebus test mode */
#define MI_CLR_DP_INTR          0x0800      /* Bit 11: clear dp interrupt */
#define MI_CLR_RDRAM            0x1000      /* Bit 12: clear RDRAM reg */
#define MI_SET_RDRAM            0x2000      /* Bit 13: set RDRAM reg mode */
#define MI_MODE_INIT            0x0080      /* Bit  7: init mode */
#define MI_MODE_EBUS            0x0100      /* Bit  8: ebus test mode */
#define MI_MODE_RDRAM           0x0200      /* Bit  9: RDRAM reg mode */

READ32_MEMBER( n64_periphs::mi_reg_r )
{
	UINT32 ret = 0;
	switch (offset)
	{
		case 0x00/4:            // MI_MODE_REG
			ret = mi_mode;
			break;

		case 0x04/4:            // MI_VERSION_REG
			ret = mi_version;
			break;

		case 0x08/4:            // MI_INTR_REG
			ret = mi_interrupt;
			break;

		case 0x0c/4:            // MI_INTR_MASK_REG
			ret = mi_intr_mask;
			break;

		default:
			logerror("mi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, mem_map->device().safe_pc());
			break;
	}

	//printf("mi_reg_r %08x = %08x\n", offset * 4, ret); fflush(stdout);
	return ret;
}

WRITE32_MEMBER( n64_periphs::mi_reg_w )
{
	//printf("mi_reg_w %08x %08x %08x\n", offset * 4, data, mem_mask); fflush(stdout);
	switch (offset)
	{
		case 0x00/4:        // MI_INIT_MODE_REG
			if (data & MI_CLR_INIT)     mi_mode &= ~MI_MODE_INIT;
			if (data & MI_SET_INIT)     mi_mode |=  MI_MODE_INIT;
			if (data & MI_CLR_EBUS)     mi_mode &= ~MI_MODE_EBUS;
			if (data & MI_SET_EBUS)     mi_mode |=  MI_MODE_EBUS;
			if (data & MI_CLR_RDRAM)    mi_mode &= ~MI_MODE_RDRAM;
			if (data & MI_SET_RDRAM)    mi_mode |=  MI_MODE_RDRAM;
			if (data & MI_CLR_DP_INTR)
			{
				clear_rcp_interrupt(DP_INTERRUPT);
			}
			mi_mode &= ~0x7f;
			mi_mode |= data & 0x7f;
			break;

		case 0x04/4:        // MI_VERSION_REG
			mi_version = data;
			break;

		case 0x0c/4:        // MI_INTR_MASK_REG
		{
			if (data & 0x0001)
			{
				mi_intr_mask &= ~0x1;      // clear SP mask
			}
			if (data & 0x0002)
			{
				mi_intr_mask |= 0x1;           // set SP mask
			}
			if (data & 0x0004)
			{
				mi_intr_mask &= ~0x2;      // clear SI mask
			}
			if (data & 0x0008)
			{
				mi_intr_mask |= 0x2;           // set SI mask
			}
			if (data & 0x0010)
			{
				mi_intr_mask &= ~0x4;      // clear AI mask
			}
			if (data & 0x0020)
			{
				mi_intr_mask |= 0x4;           // set AI mask
			}
			if (data & 0x0040)
			{
				mi_intr_mask &= ~0x8;      // clear VI mask
			}
			if (data & 0x0080)
			{
				mi_intr_mask |= 0x8;           // set VI mask
			}
			if (data & 0x0100)
			{
				mi_intr_mask &= ~0x10;     // clear PI mask
			}
			if (data & 0x0200)
			{
				mi_intr_mask |= 0x10;      // set PI mask
			}
			if (data & 0x0400)
			{
				mi_intr_mask &= ~0x20;     // clear DP mask
			}
			if (data & 0x0800)
			{
				mi_intr_mask |= 0x20;      // set DP mask
			}
			check_interrupts();
			break;
		}

		default:
			logerror("mi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, maincpu->safe_pc());
			break;
	}
}

void signal_rcp_interrupt(running_machine &machine, int interrupt)
{
	machine.device<n64_periphs>("rcp")->signal_rcp_interrupt(interrupt);
}

void n64_periphs::check_interrupts()
{
	if (mi_intr_mask & mi_interrupt)
	{
		//printf("Asserting IRQ, %02x : %02x\n", mi_intr_mask, mi_interrupt); fflush(stdout);
		machine().device("maincpu")->execute().set_input_line(INPUT_LINE_IRQ0, ASSERT_LINE);
	}
	else
	{
		//printf("Deasserting IRQ, %02x : %02x\n", mi_intr_mask, mi_interrupt); fflush(stdout);
		machine().device("maincpu")->execute().set_input_line(INPUT_LINE_IRQ0, CLEAR_LINE);
	}
}

void n64_periphs::signal_rcp_interrupt(int interrupt)
{
	mi_interrupt |= interrupt;

	check_interrupts();
}

void n64_periphs::clear_rcp_interrupt(int interrupt)
{
	mi_interrupt &= ~interrupt;

	check_interrupts();
}

READ32_MEMBER( n64_periphs::is64_r )
{
	switch(offset)
	{
		case 0x0000/4:
			return 0x49533634;

		case 0x0004/4:
		case 0x0008/4:
		case 0x000c/4:
		case 0x0010/4:
		case 0x0014/4:
		case 0x0018/4:
		case 0x001c/4:
			return 0;

		default:
			return ( is64_buffer[(offset << 2) + 0] << 24 ) |
					( is64_buffer[(offset << 2) + 1] << 16 ) |
					( is64_buffer[(offset << 2) + 2] <<  8 ) |
					( is64_buffer[(offset << 2) + 3] <<  0 );
	}
}

WRITE32_MEMBER( n64_periphs::is64_w )
{
	int i = 0;

	switch(offset)
	{
		case 0x0014/4:
			for(i = 0x20; i < (0x20 + data); i++)
			{
				//printf( "%c", is64_buffer[i] );
				if(is64_buffer[i] == 0x0a)
				{
					//printf( "%c", 0x0d );
				}
				is64_buffer[i] = 0;
			}
			break;

		default:
			is64_buffer[(offset << 2) + 0] = (data >> 24) & 0x000000ff;
			is64_buffer[(offset << 2) + 1] = (data >> 16) & 0x000000ff;
			is64_buffer[(offset << 2) + 2] = (data >>  8) & 0x000000ff;
			is64_buffer[(offset << 2) + 3] = (data >>  0) & 0x000000ff;
			break;
	}
}

READ32_MEMBER( n64_periphs::open_r )
{
	UINT32 retval = (offset << 2) & 0x0000ffff;
	retval = ((retval + 2) << 16) | retval;
	return retval;
}

WRITE32_MEMBER( n64_periphs::open_w )
{
	// Do nothing
}

// RDRAM Interface (RI)

#define RDRAM_CONFIG        (0)
#define RDRAM_DEVICE_ID     (1)
#define RDRAM_DELAY         (2)
#define RDRAM_MODE          (3)
#define RDRAM_REF_INTERVAL  (4)
#define RDRAM_REF_ROW       (5)
#define RDRAM_RAS_INTERVAL  (6)
#define RDRAM_MIN_INTERVAL  (7)
#define RDRAM_ADDR_SELECT   (8)
#define RDRAM_DEVICE_MANUF  (9)

READ32_MEMBER( n64_periphs::rdram_reg_r )
{
	//printf("rdram_reg_r %08x = %08x\n", offset * 4, rdram_regs[offset]); fflush(stdout);
	if(offset > 0x24/4)
	{
		logerror("rdram_reg_r: %08X, %08X at %08X\n", offset, mem_mask, maincpu->safe_pc());
		return 0;
	}
	return rdram_regs[offset];
}

WRITE32_MEMBER( n64_periphs::rdram_reg_w )
{
	//printf("rdram_reg_w %08x %08x %08x\n", offset * 4, data, mem_mask); fflush(stdout);
	if(offset > 0x24/4)
	{
		logerror("rdram_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, maincpu->safe_pc());
		return;
	}
	COMBINE_DATA(&rdram_regs[offset]);
}

// RSP Interface

void n64_periphs::sp_dma(int direction)
{
	UINT32 length = sp_dma_length + 1;

	if ((length & 7) != 0)
	{
		length = (length + 7) & ~7;
	}

	//printf("Length %08x Skip %08x Count %08x\n", length, sp_dma_skip, sp_dma_count); fflush(stdout);
	if (sp_mem_addr & 0x3)
	{
		sp_mem_addr = sp_mem_addr & ~3;
	}
	if (sp_dram_addr & 0x7)
	{
		sp_dram_addr = sp_dram_addr & ~7;
	}

	if ((sp_mem_addr & 0xfff) + (length) > 0x1000)
	{
		//printf("sp_dma: dma out of memory area: %08X, %08X, %08X\n", sp_mem_addr, sp_dram_addr, length);
		//fatalerror("sp_dma: dma out of memory area: %08X, %08X\n", sp_mem_addr, length);
		length = 0x1000 - (sp_mem_addr & 0xfff);
	}

	UINT32 *sp_mem[2] = { rsp_dmem, rsp_imem };

	int sp_mem_page = (sp_mem_addr >> 12) & 1;
	if(direction == 0)// RDRAM -> I/DMEM
	{
		for(int c = 0; c <= sp_dma_count; c++)
		{
			UINT32 src = (sp_dram_addr & 0x007fffff) >> 2;
			UINT32 dst = (sp_mem_addr & 0xfff) >> 2;

			for(int i = 0; i < length / 4; i++)
			{
				sp_mem[sp_mem_page][(dst + i) & 0x3ff] = rdram[src + i];
			}

			sp_mem_addr += length;
			sp_dram_addr += length;

			sp_mem_addr += sp_dma_skip;
		}
	}
	else                    // I/DMEM -> RDRAM
	{
		for(int c = 0; c <= sp_dma_count; c++)
		{
			UINT32 src = (sp_mem_addr & 0xfff) >> 2;
			UINT32 dst = (sp_dram_addr & 0x007fffff) >> 2;

			for(int i = 0; i < length / 4; i++)
			{
				rdram[dst + i] = sp_mem[sp_mem_page][(src + i) & 0x3ff];
			}

			sp_mem_addr += length;
			sp_dram_addr += length;

			sp_dram_addr += sp_dma_skip;
		}
	}
}

WRITE32_MEMBER(n64_periphs::sp_set_status)
{
	//printf("sp_set_status: %08x\n", data);
	if (data & 0x1)
	{
		rspcpu->execute().set_input_line(INPUT_LINE_HALT, ASSERT_LINE);
		rspcpu->state().set_state_int(RSP_SR, rspcpu->state().state_int(RSP_SR) | RSP_STATUS_HALT);
	}

	if (data & 0x2)
	{
		rspcpu->state().set_state_int(RSP_SR, rspcpu->state().state_int(RSP_SR) | RSP_STATUS_BROKE);

		if (rspcpu->state().state_int(RSP_SR) & RSP_STATUS_INTR_BREAK)
		{
			signal_rcp_interrupt(SP_INTERRUPT);
		}
	}
}

READ32_MEMBER(n64_periphs::sp_reg_r)
{
	UINT32 ret = 0;
	switch (offset)
	{
		case 0x00/4:        // SP_MEM_ADDR_REG
			ret = sp_mem_addr;
			break;

		case 0x04/4:        // SP_DRAM_ADDR_REG
			ret = sp_dram_addr;
			break;

		case 0x08/4:        // SP_RD_LEN_REG
			ret = (sp_dma_skip << 20) | (sp_dma_count << 12) | sp_dma_length;
			break;

		case 0x10/4:        // SP_STATUS_REG
			//machine().scheduler().synchronize();
			//machine().scheduler().boost_interleave(attotime::from_msec(1), attotime::from_msec(m));
			ret = rspcpu->state().state_int(RSP_SR);
			break;

		case 0x14/4:        // SP_DMA_FULL_REG
			ret = 0;
			break;

		case 0x18/4:        // SP_DMA_BUSY_REG
			ret = 0;
			break;

		case 0x1c/4:        // SP_SEMAPHORE_REG
			//machine().scheduler().boost_interleave(attotime::from_usec(1), attotime::from_usec(1));
			machine().device("maincpu")->execute().yield();
			if( sp_semaphore )
			{
				ret = 1;
			}
			else
			{
				//printf("Semaphore is now acquired, returning 0\n");
				sp_semaphore = 1;
				ret = 0;
			}
			break;

		case 0x20/4:        // DP_CMD_START
		{
			n64_state *state = machine().driver_data<n64_state>();
			ret = state->m_rdp->GetStartReg();
			break;
		}

		case 0x24/4:        // DP_CMD_END
		{
			n64_state *state = machine().driver_data<n64_state>();
			ret = state->m_rdp->GetEndReg();
			break;
		}

		case 0x28/4:        // DP_CMD_CURRENT
		{
			n64_state *state = machine().driver_data<n64_state>();
			ret = state->m_rdp->GetCurrentReg();
			break;
		}

		case 0x34/4:        // DP_CMD_BUSY
		case 0x38/4:        // DP_CMD_PIPE_BUSY
		case 0x3c/4:        // DP_CMD_TMEM_BUSY
			break;

		case 0x2c/4:        // DP_CMD_STATUS
		{
			n64_state *state = machine().driver_data<n64_state>();
			ret = state->m_rdp->GetStatusReg();
			break;
		}

		case 0x30/4:        // DP_CMD_CLOCK
		{
			if(!(machine().driver_data<n64_state>()->m_rdp->GetStatusReg() & DP_STATUS_FREEZE))
			{
				dp_clock += 13;
				ret = dp_clock;
			}
			break;
		}

		case 0x40000/4:     // PC
			ret = rspcpu->state().state_int(RSP_PC) & 0x00000fff;
			break;

		default:
			logerror("sp_reg_r: %08X at %08X\n", offset, maincpu->safe_pc());
			break;
	}

	//printf("%08x sp_reg_r %08x = %08x\n", (UINT32)maincpu->state().state_int(MIPS3_PC), offset * 4, ret); fflush(stdout);
	return ret;
}


WRITE32_MEMBER(n64_periphs::sp_reg_w )
{
	//printf("%08x sp_reg_w %08x %08x %08x\n", (UINT32)maincpu->state().state_int(MIPS3_PC), offset * 4, data, mem_mask); fflush(stdout);

	if ((offset & 0x10000) == 0)
	{
		switch (offset & 0xffff)
		{
			case 0x00/4:        // SP_MEM_ADDR_REG
				sp_mem_addr = data;
				break;

			case 0x04/4:        // SP_DRAM_ADDR_REG
				sp_dram_addr = data & 0xffffff;
				break;

			case 0x08/4:        // SP_RD_LEN_REG
				sp_dma_length = data & 0xfff;
				sp_dma_count = (data >> 12) & 0xff;
				sp_dma_skip = (data >> 20) & 0xfff;
				sp_dma(0);
				break;

			case 0x0c/4:        // SP_WR_LEN_REG
				sp_dma_length = data & 0xfff;
				sp_dma_count = (data >> 12) & 0xff;
				sp_dma_skip = (data >> 20) & 0xfff;
				sp_dma(1);
				break;

			case 0x10/4:        // RSP_STATUS_REG
			{
				UINT32 oldstatus = rspcpu->state().state_int(RSP_SR);
				UINT32 newstatus = oldstatus;

				// printf( "RSP_STATUS_REG Write; %08x\n", data );
				if (data & 0x00000001)      // clear halt
				{
					rspcpu->execute().set_input_line(INPUT_LINE_HALT, CLEAR_LINE);
					newstatus &= ~RSP_STATUS_HALT;
					//printf("***SP HALT CLR***\n"); fflush(stdout);
				}
				if (data & 0x00000002)      // set halt
				{
					rspcpu->execute().set_input_line(INPUT_LINE_HALT, ASSERT_LINE);
					newstatus |= RSP_STATUS_HALT;
					//printf("***SP HALT SET***\n"); fflush(stdout);
				}
				if (data & 0x00000004)
				{
					newstatus &= ~RSP_STATUS_BROKE;
					//printf("***SP BROKE CLR***\n"); fflush(stdout);
				}
				if (data & 0x00000008)      // clear interrupt
				{
					//printf("***SP INT CLR***\n"); fflush(stdout);
					clear_rcp_interrupt(SP_INTERRUPT);
				}
				if (data & 0x00000010)      // set interrupt
				{
					//printf("***SP INT SET***\n"); fflush(stdout);
					signal_rcp_interrupt(SP_INTERRUPT);
				}
				if (data & 0x00000020)
				{
					newstatus &= ~RSP_STATUS_SSTEP;
					//printf("***SP SSTEP CLR***\n"); fflush(stdout);
				}
				if (data & 0x00000040)
				{
					newstatus |= RSP_STATUS_SSTEP;  // set single step
					//printf("***SP SSTEP SET***\n"); fflush(stdout);
					if(!(oldstatus & (RSP_STATUS_BROKE | RSP_STATUS_HALT)))
					{
						rspcpu->state().set_state_int(RSP_STEPCNT, 1 );
						machine().device("rsp")->execute().yield();
					}
				}
				if (data & 0x00000080)
				{
					newstatus &= ~RSP_STATUS_INTR_BREAK;    // clear interrupt on break
					//printf("***SP INTRBRK CLR***\n"); fflush(stdout);
				}
				if (data & 0x00000100)
				{
					newstatus |= RSP_STATUS_INTR_BREAK;     // set interrupt on break
					//printf("***SP INTRBRK SET***\n"); fflush(stdout);
				}
				if (data & 0x00000200)
				{
					newstatus &= ~RSP_STATUS_SIGNAL0;       // clear signal 0
					//printf("***SP YIELD CLR***\n"); fflush(stdout);
				}
				if (data & 0x00000400)
				{
					newstatus |= RSP_STATUS_SIGNAL0;        // set signal 0
					//printf("***SP YIELD SET***\n"); fflush(stdout);
				}
				if (data & 0x00000800)
				{
					newstatus &= ~RSP_STATUS_SIGNAL1;       // clear signal 1
					//printf("***SP YIELDED CLR***\n"); fflush(stdout);
				}
				if (data & 0x00001000)
				{
					newstatus |= RSP_STATUS_SIGNAL1;        // set signal 1
					//printf("***SP YIELDED SET***\n"); fflush(stdout);
				}
				if (data & 0x00002000)
				{
					newstatus &= ~RSP_STATUS_SIGNAL2 ;      // clear signal 2
					//printf("***SP TASKDONE CLR***\n"); fflush(stdout);
				}
				if (data & 0x00004000)
				{
					newstatus |= RSP_STATUS_SIGNAL2;        // set signal 2
					//printf("***SP TASKDONE SET***\n"); fflush(stdout);
				}
				if (data & 0x00008000)
				{
					newstatus &= ~RSP_STATUS_SIGNAL3;       // clear signal 3
					//printf("***SP SIG3 CLR***\n"); fflush(stdout);
				}
				if (data & 0x00010000)
				{
					newstatus |= RSP_STATUS_SIGNAL3;        // set signal 3
					//printf("***SP SIG3 SET***\n"); fflush(stdout);
				}
				if (data & 0x00020000)
				{
					newstatus &= ~RSP_STATUS_SIGNAL4;       // clear signal 4
					//printf("***SP SIG4 CLR***\n"); fflush(stdout);
				}
				if (data & 0x00040000)
				{
					newstatus |= RSP_STATUS_SIGNAL4;        // set signal 4
					//printf("***SP SIG4 SET***\n"); fflush(stdout);
				}
				if (data & 0x00080000)
				{
					newstatus &= ~RSP_STATUS_SIGNAL5;       // clear signal 5
					//printf("***SP SIG5 CLR***\n"); fflush(stdout);
				}
				if (data & 0x00100000)
				{
					newstatus |= RSP_STATUS_SIGNAL5;        // set signal 5
					//printf("***SP SIG5 SET***\n"); fflush(stdout);
				}
				if (data & 0x00200000)
				{
					newstatus &= ~RSP_STATUS_SIGNAL6;       // clear signal 6
					//printf("***SP SIG6 CLR***\n"); fflush(stdout);
				}
				if (data & 0x00400000)
				{
					newstatus |= RSP_STATUS_SIGNAL6;        // set signal 6
					//printf("***SP SIG6 SET***\n"); fflush(stdout);
				}
				if (data & 0x00800000)
				{
					newstatus &= ~RSP_STATUS_SIGNAL7;       // clear signal 7
					//printf("***SP SIG7 CLR***\n"); fflush(stdout);
				}
				if (data & 0x01000000)
				{
					newstatus |= RSP_STATUS_SIGNAL7;        // set signal 7
					//printf("***SP SIG7 SET***\n"); fflush(stdout);
				}
				rspcpu->state().set_state_int(RSP_SR, newstatus);
				break;
			}

			case 0x1c/4:        // SP_SEMAPHORE_REG
				//machine().scheduler().boost_interleave(attotime::from_usec(1), attotime::from_usec(1));
				//printf("Semaphore is being released\n");
				if(data == 0)
				{
					sp_semaphore = 0;
				}
				break;

			default:
				logerror("sp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, maincpu->safe_pc());
				break;
		}
	}
	else
	{
		switch (offset & 0xffff)
		{
			case 0x00/4:        // SP_PC_REG
				if( rspcpu->state().state_int(RSP_NEXTPC) != 0xffffffff )
				{
					rspcpu->state().set_state_int(RSP_NEXTPC, 0x1000 | (data & 0xfff));
				}
				else
				{
					rspcpu->state().set_state_int(RSP_PC, 0x1000 | (data & 0xfff));
				}
				break;

			default:
				logerror("sp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, maincpu->safe_pc());
				break;
		}
	}
}


// RDP Interface

void dp_full_sync(running_machine &machine)
{
	signal_rcp_interrupt(machine, DP_INTERRUPT);
}

READ32_MEMBER( n64_periphs::dp_reg_r )
{
	n64_state *state = space.machine().driver_data<n64_state>();
	UINT32 ret = 0;
	switch (offset)
	{
		case 0x00/4:        // DP_START_REG
			ret = state->m_rdp->GetStartReg();
			break;

		case 0x04/4:        // DP_END_REG
			ret = state->m_rdp->GetEndReg();
			break;

		case 0x08/4:        // DP_CURRENT_REG
			ret = state->m_rdp->GetCurrentReg();
			break;

		case 0x0c/4:        // DP_STATUS_REG
			ret = state->m_rdp->GetStatusReg();
			break;

		case 0x10/4:        // DP_CLOCK_REG
		{
			if(!(state->m_rdp->GetStatusReg() & DP_STATUS_FREEZE))
			{
				dp_clock += 13;
				ret = dp_clock;
			}
			break;
		}

		default:
			logerror("dp_reg_r: %08X, %08X at %08X\n", offset, mem_mask, safe_pc());
			break;
	}

	//printf("%08x dp_reg_r %08x = %08x\n", (UINT32)space.machine().device("rsp")->state().state_int(RSP_PC), offset, ret); fflush(stdout);
	return ret;
}

WRITE32_MEMBER( n64_periphs::dp_reg_w )
{
	n64_state *state = space.machine().driver_data<n64_state>();

	//printf("%08x dp_reg_w %08x %08x %08x\n", (UINT32)space.machine().device("rsp")->state().state_int(RSP_PC), offset, data, mem_mask); fflush(stdout);
	switch (offset)
	{
		case 0x00/4:        // DP_START_REG
			state->m_rdp->SetStartReg(data);
			state->m_rdp->SetCurrentReg(state->m_rdp->GetStartReg());
			break;

		case 0x04/4:        // DP_END_REG
			//printf("dp_end_reg %08x\n", data);
			state->m_rdp->SetEndReg(data);
			g_profiler.start(PROFILER_USER1);
			state->m_rdp->ProcessList();
			g_profiler.stop();
			break;

		case 0x0c/4:        // DP_STATUS_REG
		{
			UINT32 current_status = state->m_rdp->GetStatusReg();
			if (data & 0x00000001)  current_status &= ~DP_STATUS_XBUS_DMA;
			if (data & 0x00000002)  current_status |= DP_STATUS_XBUS_DMA;
			if (data & 0x00000004)  current_status &= ~DP_STATUS_FREEZE;
			if (data & 0x00000008)  current_status |= DP_STATUS_FREEZE;
			if (data & 0x00000010)  current_status &= ~DP_STATUS_FLUSH;
			if (data & 0x00000020)  current_status |= DP_STATUS_FLUSH;
			if (data & 0x00000200)  dp_clock = 0;
			state->m_rdp->SetStatusReg(current_status);
			break;
		}

		default:
			logerror("dp_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, safe_pc());
			break;
	}
}

TIMER_CALLBACK_MEMBER(n64_periphs::vi_scanline_callback)
{
	machine().device<n64_periphs>("rcp")->vi_scanline_tick();
}

void n64_periphs::vi_scanline_tick()
{
	signal_rcp_interrupt(VI_INTERRUPT);
	vi_scanline_timer->adjust(m_screen->time_until_pos(vi_intr >> 1));
}

// Video Interface
void n64_periphs::vi_recalculate_resolution()
{
	n64_state *state = machine().driver_data<n64_state>();

	int x_start = (vi_hstart & 0x03ff0000) >> 16;
	int x_end = vi_hstart & 0x000003ff;
	int y_start = ((vi_vstart & 0x03ff0000) >> 16) / 2;
	int y_end = (vi_vstart & 0x000003ff) / 2;
	int width = ((vi_xscale & 0x00000fff) * (x_end - x_start)) / 0x400;
	int height = ((vi_yscale & 0x00000fff) * (y_end - y_start)) / 0x400;
	//printf("%04x | %02x | ", vi_xscale >> 16, vi_burst & 0x000000ff);
	rectangle visarea = m_screen->visible_area();
	attoseconds_t period = m_screen->frame_period().attoseconds;

	if (width == 0 || height == 0)
	{
		vi_blank = 1;
		/*
		FIXME: MAME doesn't handle well a h/w res of zero (otherwise it hardlocks the emu, seen especially in Aleck 64 games
		that sets the res after a longer delay than n64), guess that this just disables drawing?
		*/
		return;
	}
	else
	{
		vi_blank = 0;
	}

	if (width > 640)
		width = 640;

	if (height > 480)
		height = 480;

	state->m_rdp->MiscState.FBHeight = height;

	visarea.max_x = width - 1;
	visarea.max_y = height - 1;
	//printf("Reconfig %d, %d (%d - %d), %08x, %08x, %08x, %08x, %08x\n", width, height, x_start, x_end, vi_width, vi_xscale, vi_hsync, vi_hstart, vi_burst);
	m_screen->configure(width, 525, visarea, period);
}

READ32_MEMBER( n64_periphs::vi_reg_r )
{
	UINT32 ret = 0;
	switch (offset)
	{
		case 0x00/4:        // VI_CONTROL_REG
			ret = vi_control;
			break;

		case 0x04/4:        // VI_ORIGIN_REG
			ret = vi_origin;
			break;

		case 0x08/4:        // VI_WIDTH_REG
			ret = vi_width;
			break;

		case 0x0c/4:
			ret = vi_intr;
			break;

		case 0x10/4:        // VI_CURRENT_REG
			ret = m_screen->vpos() << 1;
			break;

		case 0x14/4:        // VI_BURST_REG
			ret = vi_burst;
			break;

		case 0x18/4:        // VI_V_SYNC_REG
			ret = vi_vsync;
			break;

		case 0x1c/4:        // VI_H_SYNC_REG
			ret = vi_hsync;
			break;

		case 0x20/4:        // VI_LEAP_REG
			ret = vi_leap;
			break;

		case 0x24/4:        // VI_H_START_REG
			ret = vi_hstart;
			break;

		case 0x28/4:        // VI_V_START_REG
			ret = vi_vstart;
			break;

		case 0x2c/4:        // VI_V_BURST_REG
			ret = vi_vburst;
			break;

		case 0x30/4:        // VI_X_SCALE_REG
			ret = vi_xscale;
			break;

		case 0x34/4:        // VI_Y_SCALE_REG
			ret = vi_yscale;
			break;

		default:
			logerror("vi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, maincpu->safe_pc());
			break;
	}

	//printf("vi_reg_r %08x = %08x\n", offset * 4, ret);
	return ret;
}

WRITE32_MEMBER( n64_periphs::vi_reg_w )
{
	//printf("vi_reg_w %08x %08x %08x\n", offset * 4, data, mem_mask);
	n64_state *state = machine().driver_data<n64_state>();

	switch (offset)
	{
		case 0x00/4:        // VI_CONTROL_REG
			vi_control = data;
			vi_recalculate_resolution();
			break;

		case 0x04/4:        // VI_ORIGIN_REG
			vi_origin = data & 0xffffff;
			break;

		case 0x08/4:        // VI_WIDTH_REG
			if (vi_width != data && data > 0)
			{
				vi_recalculate_resolution();
			}
			vi_width = data;
			state->m_rdp->MiscState.FBWidth = data;
			break;

		case 0x0c/4:        // VI_INTR_REG
			vi_intr = data;
			vi_scanline_timer->adjust(m_screen->time_until_pos(vi_intr >> 1));
			break;

		case 0x10/4:        // VI_CURRENT_REG
			clear_rcp_interrupt(VI_INTERRUPT);
			break;

		case 0x14/4:        // VI_BURST_REG
			vi_burst = data;
			break;

		case 0x18/4:        // VI_V_SYNC_REG
			vi_vsync = data;
			break;

		case 0x1c/4:        // VI_H_SYNC_REG
			vi_hsync = data;
			break;

		case 0x20/4:        // VI_LEAP_REG
			vi_leap = data;
			break;

		case 0x24/4:        // VI_H_START_REG
			vi_hstart = data;
			vi_recalculate_resolution();
			break;

		case 0x28/4:        // VI_V_START_REG
			vi_vstart = data;
			vi_recalculate_resolution();
			break;

		case 0x2c/4:        // VI_V_BURST_REG
			vi_vburst = data;
			break;

		case 0x30/4:        // VI_X_SCALE_REG
			vi_xscale = data;
			vi_recalculate_resolution();
			break;

		case 0x34/4:        // VI_Y_SCALE_REG
			vi_yscale = data;
			vi_recalculate_resolution();
			break;

		/*
		Uncomment this for convenient homebrew debugging
		case 0x44/4:        // TEMP DEBUG
		    printf( "E Ping: %08x\n", data );
		    break;
		*/

		default:
			logerror("vi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, maincpu->safe_pc());
			break;
	}
}


// Audio Interface
void n64_periphs::ai_fifo_push(UINT32 address, UINT32 length)
{
	if (ai_fifo_num == AUDIO_DMA_DEPTH)
	{
		osd_printf_debug("ai_fifo_push: tried to push to full DMA FIFO!!!\n");
	}

	ai_fifo[ai_fifo_wpos].address = address;
	ai_fifo[ai_fifo_wpos].length = length;

	ai_fifo_wpos++;
	ai_fifo_num++;

	if (ai_fifo_wpos >= AUDIO_DMA_DEPTH)
	{
		ai_fifo_wpos = 0;
	}

	if (ai_fifo_num >= AUDIO_DMA_DEPTH)
	{
		ai_status |= 0x80000001;    // FIFO full
	}

	if (! (ai_status & 0x40000000))
	{
		signal_rcp_interrupt(AI_INTERRUPT);
		ai_dma();
	}
}

void n64_periphs::ai_fifo_pop()
{
	ai_fifo_rpos++;
	ai_fifo_num--;

	if (ai_fifo_num < 0)
	{
		fatalerror("ai_fifo_pop: FIFO underflow!\n");
	}

	if (ai_fifo_rpos >= AUDIO_DMA_DEPTH)
	{
		ai_fifo_rpos = 0;
	}

	if (ai_fifo_num < AUDIO_DMA_DEPTH)
	{
		ai_status &= ~0x80000001;   // FIFO not full
		signal_rcp_interrupt(AI_INTERRUPT);
	}
}

n64_periphs::AUDIO_DMA *n64_periphs::ai_fifo_get_top()
{
	if (ai_fifo_num > 0)
	{
		return &ai_fifo[ai_fifo_rpos];
	}
	else
	{
		return NULL;
	}
}

#define N64_ATTOTIME_NORMALIZE(a)   do { while ((a).attoseconds >= ATTOSECONDS_PER_SECOND) { (a).seconds++; (a).attoseconds -= ATTOSECONDS_PER_SECOND; } } while (0)

void n64_periphs::ai_dma()
{
	INT16 *ram = (INT16*)rdram;
	AUDIO_DMA *current = ai_fifo_get_top();
	attotime period;

	//static FILE * audio_dump = NULL;
	//
	//if (audio_dump == NULL)
	//    audio_dump = fopen("audio_dump.raw","wb");
	//
	//fwrite(&ram[current->address/2],current->length,1,audio_dump);

	ram = &ram[current->address/2];

	//osd_printf_debug("DACDMA: %x for %x bytes\n", current->address, current->length);

	dmadac_transfer(&ai_dac[0], 2, 1, 2, current->length/4, ram);

	ai_status |= 0x40000000;

	// adjust the timer
	period = attotime::from_hz(DACRATE_NTSC) * ((ai_dacrate + 1) * (current->length / 4));
	ai_timer->adjust(period);
}

TIMER_CALLBACK_MEMBER(n64_periphs::ai_timer_callback)
{
	machine().device<n64_periphs>("rcp")->ai_timer_tick();
}

void n64_periphs::ai_timer_tick()
{
	ai_fifo_pop();

	// keep playing if there's another DMA queued
	if (ai_fifo_get_top() != NULL)
	{
		ai_dma();
		signal_rcp_interrupt(AI_INTERRUPT);
	}
	else
	{
		ai_status &= ~0x40000000;
	}
}

READ32_MEMBER( n64_periphs::ai_reg_r )
{
	UINT32 ret = 0;
	switch (offset)
	{
		case 0x04/4:        // AI_LEN_REG
		{
			if (ai_status & 0x80000001)
			{
				ret = ai_len;
			}
			else if (ai_status & 0x40000000)
			{
				double secs_left = (ai_timer->expire() - machine().time()).as_double();
				unsigned int samples_left = (UINT32)(secs_left * (double)DACRATE_NTSC / (double)(ai_dacrate + 1));
				ret = samples_left * 4;
			}
			else
			{
				ret = 0;
			}
			break;
		}

		case 0x0c/4:        // AI_STATUS_REG
			ret = ai_status;
			break;

		default:
			logerror("ai_reg_r: %08X, %08X at %08X\n", offset, mem_mask, maincpu->safe_pc());
			break;
	}

	//printf("ai_reg_r %08x = %08x\n", offset * 4, ret);
	return ret;
}

WRITE32_MEMBER( n64_periphs::ai_reg_w )
{
	//printf("ai_reg_w %08x %08x %08x\n", offset * 4, data, mem_mask);
	switch (offset)
	{
		case 0x00/4:        // AI_DRAM_ADDR_REG
			ai_dram_addr = data & 0xfffff8;
			break;

		case 0x04/4:        // AI_LEN_REG
			ai_len = data & 0x3ffff;        // Hardware v2.0 has 18 bits, v1.0 has 15 bits
			ai_fifo_push(ai_dram_addr, ai_len);
			break;

		case 0x08/4:        // AI_CONTROL_REG
			ai_control = data;
			break;

		case 0x0c/4:
			clear_rcp_interrupt(AI_INTERRUPT);
			break;

		case 0x10/4:        // AI_DACRATE_REG
			ai_dacrate = data & 0x3fff;
			dmadac_set_frequency(&ai_dac[0], 2, (double)DACRATE_NTSC / (double)(ai_dacrate+1));
			//printf( "frequency: %f\n", (double)DACRATE_NTSC / (double)(ai_dacrate+1) );
			dmadac_enable(&ai_dac[0], 2, 1);
			break;

		case 0x14/4:        // AI_BITRATE_REG
			ai_bitrate = data & 0xf;
			break;

		default:
			logerror("ai_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, maincpu->safe_pc());
			break;
	}
}


// Peripheral Interface

TIMER_CALLBACK_MEMBER(n64_periphs::pi_dma_callback)
{
	machine().device<n64_periphs>("rcp")->pi_dma_tick();
	machine().device("rsp")->execute().yield();
}

void n64_periphs::pi_dma_tick()
{
	UINT16 *cart16;
	UINT16 *dram16 = (UINT16*)rdram;

	UINT32 cart_addr = (pi_cart_addr & 0x0fffffff) >> 1;
	UINT32 dram_addr = (pi_dram_addr & 0x007fffff) >> 1;

	if((cart_addr & 0x04000000) == 0x04000000)
	{
		cart16 = (UINT16*)n64_sram;
		cart_addr = (pi_cart_addr & 0x0001ffff) >> 1;
	}
	else if((cart_addr & 0x03000000) == 0x03000000 && dd_present)
	{
		cart16 = (UINT16*)machine().root_device().memregion("ddipl")->base();
		cart_addr = (pi_cart_addr & 0x003fffff) >> 1;
	}
	else
	{
		cart16 = (UINT16*)machine().root_device().memregion("user2")->base();
		cart_addr &= ((machine().root_device().memregion("user2")->bytes() >> 1) - 1);
	}

	//printf("%08x Cart, %08x Dram\n", cart_addr << 1, dram_addr << 1); fflush(stdout);

	if(pi_dma_dir == 1)
	{
		UINT32 dma_length = pi_wr_len + 1;
		if (dma_length & 7)
		{
			dma_length = (dma_length + 7) & ~7;
		}

		if (pi_dram_addr != 0xffffffff)
		{
			for(int i = 0; i < dma_length / 2; i++)
			{
				dram16[BYTE_XOR_BE(dram_addr + i)] = cart16[BYTE_XOR_BE(cart_addr + i)];
			}

			pi_cart_addr += dma_length;
			pi_dram_addr += dma_length;
		}
	}
	else
	{
		UINT32 dma_length = pi_rd_len + 1;
		if (dma_length & 7)
		{
			dma_length = (dma_length + 7) & ~7;
		}

		if (pi_dram_addr != 0xffffffff)
		{
			for(int i = 0; i < dma_length / 2; i++)
			{
				cart16[BYTE_XOR_BE(cart_addr + i)] = dram16[BYTE_XOR_BE(dram_addr + i)];
			}

			pi_cart_addr += dma_length;
			pi_dram_addr += dma_length;
		}
	}

	pi_status &= ~1; // Clear DMA_BUSY
	pi_status |= 8; // Set INTERRUPT

	signal_rcp_interrupt(PI_INTERRUPT);

	pi_dma_timer->adjust(attotime::never);
}

READ32_MEMBER( n64_periphs::pi_reg_r )
{
	UINT32 ret = 0;
	switch (offset)
	{
		case 0x00/4:        // PI_DRAM_ADDR_REG
			ret = pi_dram_addr;
			break;

		case 0x04/4:        // PI_CART_ADDR_REG
			ret = pi_cart_addr;
			break;

		case 0x10/4:        // PI_STATUS_REG
			ret = pi_status;
			break;

		case 0x14/4:        // PI_BSD_DOM1_LAT
			ret = pi_bsd_dom1_lat;
			break;

		case 0x18/4:        // PI_BSD_DOM1_PWD
			ret = pi_bsd_dom1_pwd;
			break;

		case 0x1c/4:        // PI_BSD_DOM1_PGS
			ret = pi_bsd_dom1_pgs;
			break;

		case 0x20/4:        // PI_BSD_DOM1_RLS
			ret = pi_bsd_dom1_rls;
			break;

		case 0x24/4:        // PI_BSD_DOM2_LAT
			ret = pi_bsd_dom2_lat;
			break;

		case 0x28/4:        // PI_BSD_DOM2_PWD
			ret = pi_bsd_dom2_pwd;
			break;

		case 0x2c/4:        // PI_BSD_DOM2_PGS
			ret = pi_bsd_dom2_pgs;
			break;

		case 0x30/4:        // PI_BSD_DOM2_RLS
			ret = pi_bsd_dom2_rls;
			break;

		default:
			logerror("pi_reg_r: %08X, %08X at %08X\n", offset, mem_mask, maincpu->safe_pc());
			break;
	}

	//printf("pi_reg_r %08x = %08x\n", offset * 4, ret);
	return ret;
}

WRITE32_MEMBER( n64_periphs::pi_reg_w )
{
	//printf("pi_reg_w %08x %08x %08x\n", offset * 4, data, mem_mask); fflush(stdout);
	switch (offset)
	{
		case 0x00/4:        // PI_DRAM_ADDR_REG
		{
			pi_dram_addr = data;
			break;
		}

		case 0x04/4:        // PI_CART_ADDR_REG
		{
			pi_cart_addr = data;
			break;
		}

		case 0x08/4:        // PI_RD_LEN_REG
		{
			pi_rd_len = data;
			pi_dma_dir = 0;
			pi_status |= 1;

			attotime dma_period = attotime::from_hz(93750000) * (int)((float)(pi_rd_len + 1) * 5.08f); // Measured as between 2.53 cycles per byte and 2.55 cycles per byte
			//printf("want read dma in %d\n", (pi_rd_len + 1));
			pi_dma_timer->adjust(dma_period);
			//pi_dma_tick();
			break;
		}

		case 0x0c/4:        // PI_WR_LEN_REG
		{
			pi_wr_len = data;
			pi_dma_dir = 1;
			pi_status |= 1;

			attotime dma_period = attotime::from_hz(93750000) * (int)((float)(pi_wr_len + 1) * 5.08f); // Measured as between 2.53 cycles per byte and 2.55 cycles per byte
			//printf("want write dma in %d\n", (pi_wr_len + 1));
			pi_dma_timer->adjust(dma_period);

			//pi_dma_tick();
			break;
		}

		case 0x10/4:        // PI_STATUS_REG
		{
			if (data & 0x2)
			{
				pi_status &= ~8; // Clear INTERRUPT
				clear_rcp_interrupt(PI_INTERRUPT);
			}
			break;
		}

		case 0x14/4:        // PI_BSD_DOM1_LAT
			pi_bsd_dom1_lat = data;
			break;

		case 0x18/4:        // PI_BSD_DOM1_PWD
			pi_bsd_dom1_pwd = data;
			break;

		case 0x1c/4:        // PI_BSD_DOM1_PGS
			pi_bsd_dom1_pgs = data;
			break;

		case 0x20/4:        // PI_BSD_DOM1_RLS
			pi_bsd_dom1_rls = data;
			break;

		case 0x24/4:        // PI_BSD_DOM2_LAT
			pi_bsd_dom2_lat = data;
			break;

		case 0x28/4:        // PI_BSD_DOM2_PWD
			pi_bsd_dom2_pwd = data;
			break;

		case 0x2c/4:        // PI_BSD_DOM2_PGS
			pi_bsd_dom2_pgs = data;
			break;

		case 0x30/4:        // PI_BSD_DOM2_RLS
			pi_bsd_dom2_rls = data;
			break;

		default:
			logerror("pi_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, maincpu->safe_pc());
			break;
	}
}

// RDRAM Interface

READ32_MEMBER( n64_periphs::ri_reg_r )
{
	//printf("ri_reg_r %08x = %08x\n", offset * 4, ri_regs[offset]);
	if(offset > 0x1c/4)
	{
		logerror("ri_reg_r: %08X, %08X at %08X\n", offset, mem_mask, maincpu->safe_pc());
		return 0;
	}
	return ri_regs[offset];
}

WRITE32_MEMBER( n64_periphs::ri_reg_w )
{
	//printf("ri_reg_w %08x %08x %08x\n", offset * 4, data, mem_mask);
	if(offset > 0x1c/4)
	{
		logerror("ri_reg_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, maincpu->safe_pc());
		return;
	}
	COMBINE_DATA(&ri_regs[offset]);
}

// Serial Interface
UINT8 n64_periphs::calc_mempak_crc(UINT8 *buffer, int length)
{
	UINT32 crc = 0;
	UINT32 temp2 = 0;

	for(int i = 0; i <= length; i++)
	{
		for(int j = 7; j >= 0; j--)
		{
			if ((crc & 0x80) != 0)
			{
				temp2 = 0x85;
			}
			else
			{
				temp2 = 0;
			}

			crc <<= 1;

			if (i == length)
			{
				crc &= 0xff;
			}
			else
			{
				if ((buffer[i] & (1 << j)) != 0)
				{
					crc |= 0x1;
				}
			}

			crc ^= temp2;
		}
	}

	return crc;
}

INLINE UINT8 convert_to_bcd(int val)
{
	return ((val / 10) << 4) | (val % 10);
}

int n64_periphs::pif_channel_handle_command(int channel, int slength, UINT8 *sdata, int rlength, UINT8 *rdata)
{
	UINT8 command = sdata[0];

	switch (command)
	{
		case 0x00:      // Read status
		case 0xff:      // Reset
		{
			if(command == 0)
			{
				//printf("Read status\n");
			}
			else
			{
				//printf("Reset\n");
			}
			switch (channel)
			{
				case 0:
				case 1:
				{
					//printf("Read controller %d status\n", channel + 1);
					rdata[0] = 0x05;
					rdata[1] = 0x00;
					rdata[2] = 0x01;
					return 0;
				}
				case 2:
				case 3:
				{
					//printf("Read controller %d status (NC)\n", channel + 1);
					// not connected
					return 1;
				}
				case 4:
				{
					//printf("Read EEPROM status\n");
					rdata[0] = 0x00;
					rdata[1] = 0x80;
					rdata[2] = 0x00;

					return 0;
				}
				case 5:
				{
					osd_printf_debug("EEPROM2? read status\n");
					return 1;
				}
			}

			break;
		}

		case 0x01:      // Read button values
		{
			UINT16 buttons = 0;
			INT8 x = 0, y = 0;
			/* add here tags for P3 and P4 when implemented */
			static const char *const portnames[] = { "P1", "P1_ANALOG_X", "P1_ANALOG_Y", "P2", "P2_ANALOG_X", "P2_ANALOG_Y" };

			if (slength != 1 || rlength != 4)
			{
				fatalerror("handle_pif: read button values (bytes to send %d, bytes to receive %d)\n", slength, rlength);
			}

			switch (channel)
			{
				case 0: //p1 inputs
				case 1: //p2 inputs
				{
					//printf("Read p%d inputs\n", channel + 1);
					buttons = machine().root_device().ioport(portnames[(channel*3) + 0])->read();
					x = machine().root_device().ioport(portnames[(channel*3) + 1])->read() - 128;
					y = machine().root_device().ioport(portnames[(channel*3) + 2])->read() - 128;

					rdata[0] = (buttons >> 8) & 0xff;
					rdata[1] = (buttons >> 0) & 0xff;
					rdata[2] = (UINT8)(x);
					rdata[3] = (UINT8)(y);
					return 0;
				}
				case 2:
				case 3:
				{
					//printf("Controller %d not connected\n", channel + 1);
					// not connected
					return 1;
				}
			}

			break;
		}

		case 0x02: // Read mempak
		{
			UINT32 address;

			address = (sdata[1] << 8) | (sdata[2]);
			address &= ~0x1f;

			////printf("Read mempak at %04x\n", address);

			if(address == 0x8000)
			{
				for(int i = 0; i < rlength-1; i++)
				{
					rdata[i] = 0x00;
				}

				rdata[rlength-1] = calc_mempak_crc(rdata, rlength-1);
				return 0;
			}
			else if(address < 0x7fe0)
			{
				for(int i = 0; i < rlength-1; i++)
				{
					rdata[i] = m_save_data.mempak[channel & 1][address+i];
				}

				rdata[rlength-1] = calc_mempak_crc(rdata, rlength-1);
				return 0;
			}

			return 1;
		}
		case 0x03: // Write mempak
		{
			UINT32 address = (sdata[1] << 8) | (sdata[2]);
			address &= ~0x1f;

			////printf("Write mempak at %04x\n", address);
			if (address >= 0x8000)
			{
			}
			else
			{
				for(int i = 3; i < slength; i++)
				{
					m_save_data.mempak[channel & 1][address++] = sdata[i];
				}
			}

			rdata[0] = calc_mempak_crc(&sdata[3], slength-3);

			return 0;
		}

		case 0x04:      // Read from EEPROM
		{
			if (channel != 4)
			{
				return 1;
			}

			if (slength != 2 || rlength != 8)
			{
				fatalerror("handle_pif: write EEPROM (bytes to send %d, bytes to receive %d)\n", slength, rlength);
			}

			UINT16 block_offset = sdata[1] * 8;

			//printf("Read EEPROM at %04x\n", block_offset);

			for(int i=0; i < 8; i++)
			{
				rdata[i] = m_save_data.eeprom[block_offset+i];
			}

			return 0;
		}

		case 0x05:      // Write to EEPROM
		{
			if (channel != 4)
			{
				return 1;
			}

			if (slength != 10 || rlength != 1)
			{
				fatalerror("handle_pif: write EEPROM (bytes to send %d, bytes to receive %d)\n", slength, rlength);
			}

			UINT16 block_offset = sdata[1] * 8;

			//printf("Write EEPROM at %04x\n", block_offset);

			for(int i = 0; i < 8; i++)
			{
				m_save_data.eeprom[block_offset+i] = sdata[2+i];
			}

			return 0;
		}

		case 0x06:      // Read RTC Status
		{
			//printf("Read RTC Status\n");
			rdata[0] = 0x00;
			rdata[1] = 0x10;
			rdata[2] = 0x00;
			return 0;
		}

		case 0x07:      // Read RTC Block
		{
			switch(sdata[1])
			{
				case 0:
					//printf("Read RTC Block Header\n");
					rdata[0] = 0x00;
					rdata[1] = 0x02;
					rdata[8] = 0x00;
					return 0;

				case 1:
					return 0;

				case 2:
					system_time systime;
					machine().base_datetime(systime);
					rdata[0] = convert_to_bcd(systime.local_time.second); // Seconds
					rdata[1] = convert_to_bcd(systime.local_time.minute); // Minutes
					rdata[2] = 0x80 | convert_to_bcd(systime.local_time.hour); // Hours
					rdata[3] = convert_to_bcd(systime.local_time.mday); // Day of month
					rdata[4] = convert_to_bcd(systime.local_time.weekday); // Day of week
					rdata[5] = convert_to_bcd(systime.local_time.month + 1); // Month
					rdata[6] = convert_to_bcd(systime.local_time.year % 100); // Year
					rdata[7] = convert_to_bcd(systime.local_time.year / 100); // Century
					rdata[8] = 0x00;
					//printf("Read RTC Time\n");
					return 0;
			}
			return 1;
		}

		default:
		{
			osd_printf_debug("handle_pif: unknown/unimplemented command %02X\n", command);
			return 1;
		}
	}

	return 0;
}

void n64_periphs::handle_pif()
{
	UINT8 *ram = (UINT8*)pif_ram;
	if(pif_cmd[0x3f] == 0x1)        // only handle the command if the last byte is 1
	{
		int channel = 0;
		int end = 0;
		int cmd_ptr = 0;

		while(cmd_ptr < 0x3f && !end)
		{
			INT8 bytes_to_send = (INT8)pif_cmd[cmd_ptr++];
			//printf("bytes to send: 0x%02x\n", bytes_to_send);

			if (bytes_to_send == -2)
			{
				end = 1;
				//printf("end\n");
			}
			else if (bytes_to_send < 0)
			{
				//printf("do nothing\n");
				// do nothing
			}
			else
			{
				if(bytes_to_send > 0 && (bytes_to_send & 0xc0) == 0)
				{
					UINT8 recv_buffer[0x40];
					UINT8 send_buffer[0x40];

					INT8 bytes_to_recv = pif_cmd[cmd_ptr++];
					//printf("bytes to receive: 0x%02x\n", bytes_to_recv);

					if (bytes_to_recv == -2)
					{
						break; // Hack, shouldn't need to do this
					}

					for(int j = 0; j < bytes_to_send; j++)
					{
						send_buffer[j] = pif_cmd[cmd_ptr++];
					}

					int res = pif_channel_handle_command(channel, bytes_to_send, send_buffer, bytes_to_recv, recv_buffer);
					//printf("result: %d\n", res);

					if (res == 0)
					{
						//printf("cmd_ptr (%d) + bytes_to_recv (%d) = %d\n", cmd_ptr, bytes_to_recv, cmd_ptr + bytes_to_recv);
						if (cmd_ptr + bytes_to_recv > 0x3f)
						{
							fatalerror("cmd_ptr overflow\n");
						}
						for(int j = 0; j < bytes_to_recv; j++)
						{
							pif_ram[cmd_ptr++] = recv_buffer[j];
						}
					}
					else if (res == 1)
					{
						int offset = 0;//bytes_to_send;
						pif_ram[cmd_ptr - offset - 2] |= 0x80;
					}
				}

				channel++;
			}
		}

		ram[0x3f ^ BYTE4_XOR_BE(0)] = 0;
	}

	/*printf("After:\n"); fflush(stdout);
	for(int i = 0; i < 0x40; i++)
	{
	    printf("%02x ", pif_ram[i]); fflush(stdout);
	    if((i & 0xf) == 0xf)
	    {
	        printf("\n"); fflush(stdout);
	    }
	}*/
}

void n64_periphs::pif_dma(int direction)
{
	if (si_dram_addr & 0x3)
	{
		fatalerror("pif_dma: si_dram_addr unaligned: %08X\n", si_dram_addr);
	}

	if (direction)      // RDRAM -> PIF RAM
	{
		UINT32 *src = (UINT32*)&rdram[(si_dram_addr & 0x1fffffff) / 4];

		for(int i = 0; i < 64; i+=4)
		{
			UINT32 d = *src++;
			pif_ram[i+0] = (d >> 24) & 0xff;
			pif_ram[i+1] = (d >> 16) & 0xff;
			pif_ram[i+2] = (d >>  8) & 0xff;
			pif_ram[i+3] = (d >>  0) & 0xff;
		}

		memcpy(pif_cmd, pif_ram, 0x40);
	}
	else                // PIF RAM -> RDRAM
	{
		handle_pif();

		UINT32 *dst = (UINT32*)&rdram[(si_dram_addr & 0x1fffffff) / 4];

		for(int i = 0; i < 64; i+=4)
		{
			UINT32 d = 0;
			d |= pif_ram[i+0] << 24;
			d |= pif_ram[i+1] << 16;
			d |= pif_ram[i+2] <<  8;
			d |= pif_ram[i+3] <<  0;

			*dst++ = d;
		}
	}

	si_status |= 0x1000;
	signal_rcp_interrupt(SI_INTERRUPT);
}

READ32_MEMBER( n64_periphs::si_reg_r )
{
	UINT32 ret = 0;
	switch (offset)
	{
		//case 0x00/4:      // SI_DRAM_ADDR_REG
			//return si_dram_addr;

		case 0x18/4:        // SI_STATUS_REG
			ret = si_status;
	}

	//printf("si_reg_r %08x = %08x\n", offset * 4, ret); fflush(stdout);
	return ret;
}

WRITE32_MEMBER( n64_periphs::si_reg_w )
{
	//printf("si_reg_w %08x %08x %08x\n", offset * 4, data, mem_mask); fflush(stdout);
	switch (offset)
	{
		case 0x00/4:        // SI_DRAM_ADDR_REG
			si_dram_addr = data;
			break;

		case 0x04/4:        // SI_PIF_ADDR_RD64B_REG
			// PIF RAM -> RDRAM
			si_pif_addr = data;
			si_pif_addr_rd64b = data;
			pif_dma(0);
			break;

		case 0x10/4:        // SI_PIF_ADDR_WR64B_REG
			// RDRAM -> PIF RAM
			si_pif_addr = data;
			si_pif_addr_wr64b = data;
			pif_dma(1);
			break;

		case 0x18/4:        // SI_STATUS_REG
			si_status &= ~0x1000;
			clear_rcp_interrupt(SI_INTERRUPT);
			break;

		default:
			logerror("si_reg_w: %08X, %08X, %08X\n", data, offset, mem_mask);
			break;
	}
}

#define DD_STATUS_INTR      (1 << 25)

READ32_MEMBER( n64_periphs::dd_reg_r )
{
	if(offset < 0x400/4)
	{
		return dd_buffer[offset];
	}

	if(offset < 0x480/4)
	{
		return dd_sector_data[(offset - 0x400/4) / 4];
	}

	if(offset < 0x500/4)
	{
		return dd_ram_seq_data[(offset - 0x480/4) / 4];
	}

	offset -= 0x500/4;

	UINT32 ret = 0;
	switch(offset)
	{
		case 0x00/4: // DD Data
			ret = dd_data_reg;
			break;

		case 0x04/4: // ??
			break;

		case 0x08/4: // DD Status
			ret = dd_status_reg;
			break;

		case 0x0c/4: // Current Track
			ret = dd_track_reg;
			break;

		case 0x10/4: // Transfer Buffer Status
			ret = dd_buf_status_reg;
			break;

		case 0x14/4: // Sector Error
			ret = dd_sector_err_reg;
			break;

		case 0x18/4: // Sequence Status
			ret = dd_seq_status_reg;
			break;

		case 0x1c/4: // Sequence Control
			ret = dd_seq_ctrl_reg;
			break;
	}

	//logerror("dd_reg_r: %08x (%08x)\n", offset << 2, ret);
	return ret;
}

WRITE32_MEMBER( n64_periphs::dd_reg_w )
{
	//logerror("dd_reg_w: %08X, %08X, %08X\n", data, offset << 2, mem_mask);

	if(offset < 0x400/4)
	{
		COMBINE_DATA(&dd_buffer[offset]);
		return;
	}

	if(offset < 0x480/4)
	{
		COMBINE_DATA(&dd_sector_data[(offset - 0x400/4) / 4]);
		return;
	}

	if(offset < 0x500/4)
	{
		COMBINE_DATA(&dd_ram_seq_data[(offset - 0x480/4) / 4]);
		return;
	}

	offset -= 0x500/4;

	switch(offset)
	{
		case 0x00/4: // DD Data
			dd_data_reg = data;
			break;

		case 0x08/4: // DD Command
			switch((data >> 16) & 0xff)
			{
				case 0x01: // Seek Read
					logerror("dd command: Seek Read\n");
					break;
				case 0x02: // Seek Write
					logerror("dd command: Seek Write\n");
					break;
				case 0x03: // Re-Zero / Recalibrate
					logerror("dd command: Re-Zero\n");
					break;
				case 0x04: // Engage Brake
					logerror("dd command: Engage Brake\n");
					break;
				case 0x05: // Start Motor
					logerror("dd command: Start Motor\n");
					break;
				case 0x06: // Standby
					logerror("dd command: Standby\n");
					break;
				case 0x07: // Set Sleep Mode
					logerror("dd command: Set Sleep Mode\n");
					break;
				case 0x08: // Unknown
					logerror("dd command: Unknown\n");
					break;
				case 0x09: // Initialize Drive(?)
					logerror("dd command: Initialize Drive\n");
					break;
				case 0x0B: // Select Disk Type
					logerror("dd command: Select Disk Type\n");
					break;
				case 0x0C: // ASIC Command Inquiry
					logerror("dd command: ASIC Commadn Inquiry\n");
					break;
				case 0x0D: // Standby Mode (?)
					logerror("dd command: Standby Mode(?)\n");
					break;
				case 0x0E: // Detect Disk Index
					logerror("dd command: Detect Disk Index\n");
					break;
				case 0x0F: // Set RTC Year / Month
					logerror("dd command: Set RTC Year / Month\n");
					break;
				case 0x10: // Set RTC Day / Hour
					logerror("dd command: Set RTC Day / Hour\n");
					break;
				case 0x11: // Set RTC Minute / Second
					logerror("dd command: Set RTC Minute / Second\n");
					break;
				case 0x12: // Read RTC Month / Year
				{
					logerror("dd command: Read RTC Month / Year\n");

					system_time systime;
					machine().base_datetime(systime);

					dd_data_reg = (convert_to_bcd(systime.local_time.year % 100) << 24) | (convert_to_bcd(systime.local_time.month + 1) << 16);

					machine().device("maincpu")->execute().set_input_line(INPUT_LINE_IRQ1, ASSERT_LINE);
					dd_status_reg |= DD_STATUS_INTR;
					break;
				}

				case 0x13: // Read RTC Hour / Day
				{
					logerror("dd command: Read RTC Hour / Day\n");

					system_time systime;
					machine().base_datetime(systime);

					dd_data_reg = (convert_to_bcd(systime.local_time.mday) << 24) | (convert_to_bcd(systime.local_time.hour) << 16);

					machine().device("maincpu")->execute().set_input_line(INPUT_LINE_IRQ1, ASSERT_LINE);
					dd_status_reg |= DD_STATUS_INTR;
					break;
				}

				case 0x14: // Read RTC Minute / Second
				{
					logerror("dd command: Read RTC Minute / Second\n");

					system_time systime;
					machine().base_datetime(systime);

					dd_data_reg = (convert_to_bcd(systime.local_time.minute) << 24) | (convert_to_bcd(systime.local_time.second) << 16);

					machine().device("maincpu")->execute().set_input_line(INPUT_LINE_IRQ1, ASSERT_LINE);
					dd_status_reg |= DD_STATUS_INTR;
					break;
				}

				case 0x1B: // Disk Inquiry
					logerror("dd command: Disk Inquiry\n");
					break;
			}
			// Do something here
			break;

		case 0x10/4: // Interrupt Clear
			logerror("dd interrupt clear\n");
			machine().device("maincpu")->execute().set_input_line(INPUT_LINE_IRQ1, CLEAR_LINE);
			dd_status_reg &= ~DD_STATUS_INTR;
			break;

		case 0x1c/4: // Sequence Control
			dd_seq_ctrl_reg = data;
			break;
	}
}

READ32_MEMBER( n64_periphs::pif_ram_r )
{
	if(!space.debugger_access())
	{
		if( offset == ( 0x24 / 4 ) )
		{
			cic_status = 0x00000080;
		}
		if( offset == ( 0x3C / 4 ) )
		{
			return cic_status;
		}
	}
	return ( ( pif_ram[offset*4+0] << 24 ) | ( pif_ram[offset*4+1] << 16 ) | ( pif_ram[offset*4+2] <<  8 ) | ( pif_ram[offset*4+3] <<  0 ) ) & mem_mask;
}

WRITE32_MEMBER( n64_periphs::pif_ram_w )
{
	if( mem_mask & 0xff000000 )
	{
		pif_ram[offset*4+0] = ( data >> 24 ) & 0x000000ff;
	}
	if( mem_mask & 0x00ff0000 )
	{
		pif_ram[offset*4+1] = ( data >> 16 ) & 0x000000ff;
	}
	if( mem_mask & 0x0000ff00 )
	{
		pif_ram[offset*4+2] = ( data >>  8 ) & 0x000000ff;
	}
	if( mem_mask & 0x000000ff )
	{
		pif_ram[offset*4+3] = ( data >>  0 ) & 0x000000ff;
	}

	signal_rcp_interrupt(SI_INTERRUPT);
}

void n64_state::n64_machine_stop()
{
	n64_periphs *periphs = machine().device<n64_periphs>("rcp");

	if( periphs->m_nvram_image == NULL )
		return;

	device_image_interface *image = dynamic_cast<device_image_interface *>(periphs->m_nvram_image);
	//printf("Saving stuff\n");
	UINT8 data[0x30800];
	memcpy(data, n64_sram, 0x20000);
	memcpy(data + 0x20000, periphs->m_save_data.eeprom, 0x800);
	memcpy(data + 0x20800, periphs->m_save_data.mempak[0], 0x8000);
	memcpy(data + 0x28800, periphs->m_save_data.mempak[1], 0x8000);
	image->battery_save(data, 0x30800);
}

void n64_state::machine_start()
{
	rdram = reinterpret_cast<UINT32 *>(memshare("rdram")->ptr());
	n64_sram = reinterpret_cast<UINT32 *>(memshare("sram")->ptr());
	rsp_imem = reinterpret_cast<UINT32 *>(memshare("rsp_imem")->ptr());
	rsp_dmem = reinterpret_cast<UINT32 *>(memshare("rsp_dmem")->ptr());

	dynamic_cast<mips3_device *>(machine().device("maincpu"))->mips3drc_set_options(MIPS3DRC_COMPATIBLE_OPTIONS);

	/* configure fast RAM regions for DRC */
	dynamic_cast<mips3_device *>(machine().device("maincpu"))->mips3drc_add_fastram(0x00000000, 0x007fffff, FALSE, rdram);

	rsp_device *rsp = machine().device<rsp_device>("rsp");
	rsp->rspdrc_set_options(RSPDRC_STRICT_VERIFY);
	rsp->rspdrc_flush_drc_cache();
	rsp->rspdrc_add_dmem(rsp_dmem);
	rsp->rspdrc_add_imem(rsp_imem);

	/* add a hook for battery save */
	machine().add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(FUNC(n64_state::n64_machine_stop),this));
}

void n64_state::machine_reset()
{
	machine().device("rsp")->execute().set_input_line(INPUT_LINE_HALT, ASSERT_LINE);
}
