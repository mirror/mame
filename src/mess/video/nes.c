/***************************************************************************

  video/nes.c

  Routines to control the unique NES video hardware/PPU.

***************************************************************************/

#include "emu.h"
#include "includes/nes.h"

void nes_state::video_reset()
{
	m_ppu->set_vidaccess_callback(ppu2c0x_vidaccess_delegate(FUNC(nes_state::nes_ppu_vidaccess),this));
}

void nes_state::video_start()
{
	m_last_frame_flip =  0;
}

PALETTE_INIT_MEMBER(nes_state, nes)
{
	m_ppu->init_palette(palette, 0);
}


/***************************************************************************

  Display refresh

***************************************************************************/

UINT32 nes_state::screen_update_nes(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// render the ppu
	m_ppu->render(bitmap, 0, 0, 0, 0);

	// if this is a disk system game, check for the flip-disk key
	if (m_cartslot && 
		 (m_cartslot->get_pcb_id() == STD_DISKSYS	// first scenario = disksys in m_cartslot (= famicom)
			|| !m_cartslot->cart_mounted()))	// second scenario = disk via fixed internal option (= fds)
	{
		// latch this input so it doesn't go at warp speed
		if ((m_io_disksel->read_safe(0) & 0x01) && (!m_last_frame_flip))
		{
			m_cartslot->disk_flip_side();
			m_last_frame_flip = 1;
		}

		if (!m_io_disksel->read_safe(1) & 0x01)
			m_last_frame_flip = 0;
	}
	return 0;
}

// Alternative version to support "Disk flip hack" also in the Famicom Twin System (to be removed soonish!)
UINT32 nes_state::screen_update_famitwin(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	// render the ppu
	m_ppu->render(bitmap, 0, 0, 0, 0);

	if (m_cartslot2 && !m_cartslot2->cart_mounted())
	{
		// latch this input so it doesn't go at warp speed
		if ((m_io_disksel->read_safe(0) & 0x01) && (!m_last_frame_flip))
		{
			m_cartslot2->disk_flip_side();
			m_last_frame_flip = 1;
		}
		
		if (!m_io_disksel->read_safe(1) & 0x01)
			m_last_frame_flip = 0;
	}
	return 0;
}
