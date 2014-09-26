/*****************************************************************************
 *
 * includes/vector06.h
 *
 ****************************************************************************/

#ifndef VECTOR06_H_
#define VECTOR06_H_

#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/i8085/i8085.h"
#include "sound/wave.h"
#include "machine/i8255.h"
#include "machine/ram.h"
#include "machine/wd17xx.h"
#include "imagedev/cassette.h"
#include "imagedev/flopdrv.h"
#include "formats/basicdsk.h"
#include "bus/generic/slot.h"
#include "bus/generic/carts.h"


class vector06_state : public driver_device
{
public:
	vector06_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_cassette(*this, "cassette"),
	m_cart(*this, "cartslot"),
	m_fdc(*this, "wd1793"),
	m_ppi(*this, "ppi8255"),
	m_ppi2(*this, "ppi8255_2"),
	m_ram(*this, RAM_TAG),
	m_palette(*this, "palette")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<cassette_image_device> m_cassette;
	required_device<generic_slot_device> m_cart;
	required_device<fd1793_device> m_fdc;
	required_device<i8255_device> m_ppi;
	required_device<i8255_device> m_ppi2;
	required_device<ram_device> m_ram;
	required_device<palette_device> m_palette;
	DECLARE_READ8_MEMBER(vector06_8255_portb_r);
	DECLARE_READ8_MEMBER(vector06_8255_portc_r);
	DECLARE_WRITE8_MEMBER(vector06_8255_porta_w);
	DECLARE_WRITE8_MEMBER(vector06_8255_portb_w);
	DECLARE_WRITE8_MEMBER(vector06_color_set);
	DECLARE_READ8_MEMBER(vector06_romdisk_portb_r);
	DECLARE_WRITE8_MEMBER(vector06_romdisk_porta_w);
	DECLARE_WRITE8_MEMBER(vector06_romdisk_portc_w);
	DECLARE_READ8_MEMBER(vector06_8255_1_r);
	DECLARE_WRITE8_MEMBER(vector06_8255_1_w);
	DECLARE_READ8_MEMBER(vector06_8255_2_r);
	DECLARE_WRITE8_MEMBER(vector06_8255_2_w);
	DECLARE_WRITE8_MEMBER(vector06_disc_w);
	UINT8 m_keyboard_mask;
	UINT8 m_color_index;
	UINT8 m_video_mode;
	UINT8 m_romdisk_msb;
	UINT8 m_romdisk_lsb;
	UINT8 m_vblank_state;
	void vector06_set_video_mode(int width);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	DECLARE_PALETTE_INIT(vector06);
	UINT32 screen_update_vector06(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(vector06_interrupt);
	TIMER_CALLBACK_MEMBER(reset_check_callback);
	IRQ_CALLBACK_MEMBER(vector06_irq_callback);
};

#endif /* VECTOR06_H_ */
