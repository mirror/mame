/*****************************************************************************
 *
 * includes/svi318.h
 *
 * Spectravideo SVI-318 and SVI-328
 *
 ****************************************************************************/

#ifndef SVI318_H_
#define SVI318_H_

#include "cpu/z80/z80.h"
#include "machine/i8255.h"
#include "machine/ins8250.h"
#include "machine/wd17xx.h"
#include "machine/ram.h"
#include "machine/buffer.h"
#include "imagedev/cassette.h"
#include "sound/dac.h"
#include "sound/ay8910.h"
#include "sound/wave.h"
#include "video/mc6845.h"
#include "video/tms9928a.h"
#include "bus/centronics/ctronics.h"
#include "bus/generic/slot.h"
#include "bus/generic/carts.h"


struct SVI_318
{
	/* general */
	UINT8   svi318;     /* Are we dealing with an SVI-318 or a SVI-328 model. 0 = 328, 1 = 318 */
	/* memory */
	UINT8   *empty_bank;
	UINT8   bank_switch;
	UINT8   bankLow;
	UINT8   bankHigh1;
	UINT8   *bankLow_ptr;
	UINT8   bankLow_read_only;
	UINT8   *bankHigh1_ptr;
	UINT8   bankHigh1_read_only;
	UINT8   *bankHigh2_ptr;
	UINT8   bankHigh2_read_only;
	/* keyboard */
	UINT8   keyboard_row;
	/* SVI-806 80 column card */
	UINT8   svi806_present;
	UINT8   svi806_ram_enabled;
	memory_region   *svi806_ram;
	UINT8   *svi806_gfx;
};

struct SVI318_FDC_STRUCT
{
	UINT8 driveselect;
	int drq;
	int irq;
	UINT8 heads[2];
};


class svi318_state : public driver_device
{
public:
	svi318_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_cassette(*this, "cassette"),
		m_dac(*this, "dac"),
		m_ppi(*this, "ppi8255"),
		m_ram(*this, RAM_TAG),
		m_centronics(*this, "centronics"),
		m_cent_data_out(*this, "cent_data_out"),
		m_ins8250_0(*this, "ins8250_0"),
		m_ins8250_1(*this, "ins8250_1"),
		m_cart(*this, "cartslot"),
		m_fd1793(*this, "wd179x"),
		m_crtc(*this, "crtc"),
		m_line(*this, "LINE"),
		m_joysticks(*this, "JOYSTICKS"),
		m_buttons(*this, "BUTTONS"),
		m_palette(*this, "palette")  { }

	SVI_318 m_svi;
	int m_centronics_busy;
	SVI318_FDC_STRUCT m_fdc;
	DECLARE_WRITE8_MEMBER(ppi_w);
	DECLARE_READ8_MEMBER(psg_port_a_r);
	DECLARE_WRITE8_MEMBER(psg_port_b_w);
	DECLARE_WRITE8_MEMBER(fdc_drive_motor_w);
	DECLARE_WRITE8_MEMBER(fdc_density_side_w);
	DECLARE_READ8_MEMBER(fdc_irqdrq_r);
	DECLARE_WRITE8_MEMBER(svi806_ram_enable_w);
	DECLARE_WRITE8_MEMBER(writemem1);
	DECLARE_WRITE8_MEMBER(writemem2);
	DECLARE_WRITE8_MEMBER(writemem3);
	DECLARE_WRITE8_MEMBER(writemem4);
	DECLARE_READ8_MEMBER(io_ext_r);
	DECLARE_WRITE8_MEMBER(io_ext_w);
	DECLARE_DRIVER_INIT(svi318);
	DECLARE_MACHINE_START(svi318_pal);
	DECLARE_MACHINE_RESET(svi318);
	DECLARE_MACHINE_RESET(svi328_806);
	DECLARE_VIDEO_START(svi328_806);
	DECLARE_MACHINE_START(svi318_ntsc);
	DECLARE_WRITE_LINE_MEMBER(vdp_interrupt);
	DECLARE_WRITE_LINE_MEMBER(ins8250_interrupt);
	DECLARE_READ8_MEMBER(ppi_port_a_r);
	DECLARE_READ8_MEMBER(ppi_port_b_r);
	DECLARE_WRITE8_MEMBER(ppi_port_c_w);
	DECLARE_WRITE_LINE_MEMBER(fdc_intrq_w);
	DECLARE_WRITE_LINE_MEMBER(fdc_drq_w);
	bool cart_verify(UINT8 *ROM);
	DECLARE_DEVICE_IMAGE_LOAD_MEMBER(svi318_cart);
	DECLARE_WRITE_LINE_MEMBER(write_centronics_busy);

	MC6845_UPDATE_ROW(crtc_update_row);
	memory_region *m_cart_rom;
	
	required_device<z80_device> m_maincpu;
protected:
	required_device<cassette_image_device> m_cassette;
	required_device<dac_device> m_dac;
	required_device<i8255_device> m_ppi;
	required_device<ram_device> m_ram;
	required_device<centronics_device> m_centronics;
	required_device<output_latch_device> m_cent_data_out;
	required_device<ins8250_device> m_ins8250_0;
	required_device<ins8250_device> m_ins8250_1;
	required_device<generic_slot_device> m_cart;
	required_device<fd1793_device> m_fd1793;
	optional_device<mc6845_device> m_crtc;
	required_ioport_array<11> m_line;
	required_ioport m_joysticks;
	required_ioport m_buttons;
public:
	optional_device<palette_device> m_palette;
protected:
	void svi318_set_banks();
	void svi318_80col_init();
	void svi318_vdp_interrupt(int i);
};

#endif /* SVI318_H_ */
