/*****************************************************************************
 *
 * includes/sms.h
 *
 ****************************************************************************/

#ifndef SMS_H_
#define SMS_H_

#define LOG_REG
#define LOG_PAGING
#define LOG_COLOR

#define NVRAM_SIZE             (0x08000)
#define CPU_ADDRESSABLE_SIZE   (0x10000)

#define MAX_CARTRIDGES        16

#define CONTROL1_TAG   "ctrl1"
#define CONTROL2_TAG   "ctrl2"

#include "bus/sms_ctrl/smsctrl.h"
#include "bus/sms_exp/smsexp.h"
#include "bus/sega8/sega8_slot.h"


class sms_state : public driver_device
{
public:
	sms_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_vdp(*this, "sms_vdp"),
		m_ym(*this, "ym2413"),
		m_main_scr(*this, "screen"),
		m_region_maincpu(*this, "maincpu"),
		m_port_ctrl1(*this, CONTROL1_TAG),
		m_port_ctrl2(*this, CONTROL2_TAG),
		m_port_pause(*this, "PAUSE"),
		m_port_reset(*this, "RESET"),
		m_port_start(*this, "START"),
		m_port_scope(*this, "SEGASCOPE"),
		m_port_scope_binocular(*this, "SSCOPE_BINOCULAR"),
		m_port_persist(*this, "PERSISTENCE"),
		m_mainram(NULL),
		m_is_gamegear(0),
		m_is_gg_region_japan(0),
		m_is_smsj(0),
		m_is_mark_iii(0),
		m_is_sdisp(0),
		m_has_bios_0400(0),
		m_has_bios_2000(0),
		m_has_bios_full(0),
		m_has_fm(0),
		m_has_jpn_sms_cart_slot(0),
		m_store_cart_selection_data(0) { }

	// devices
	required_device<cpu_device> m_maincpu;
	required_device<sega315_5124_device> m_vdp;
	optional_device<ym2413_device> m_ym;
	required_device<screen_device> m_main_scr;
	required_memory_region m_region_maincpu;
	optional_device<sms_control_port_device> m_port_ctrl1;
	optional_device<sms_control_port_device> m_port_ctrl2;
	optional_ioport m_port_pause;
	optional_ioport m_port_reset;
	optional_ioport m_port_start;
	optional_ioport m_port_scope;
	optional_ioport m_port_scope_binocular;
	optional_ioport m_port_persist;

	device_t *m_left_lcd;
	device_t *m_right_lcd;
	address_space *m_space;

	UINT8 m_bios_page_count;
	UINT8 m_fm_detect;
	UINT8 m_io_ctrl_reg;
	int m_paused;
	UINT8 m_mem_ctrl_reg;
	UINT8 m_mem_device_enabled;
	UINT8 *m_mainram;
	UINT8 *m_BIOS;
	UINT8 m_mapper[4];
	UINT8 m_port_dc_reg;
	UINT8 m_port_dd_reg;
	UINT8 m_gg_sio[5];

	// [0] for 0x400-0x3fff, [1] for 0x4000-0x7fff, [2] for 0x8000-0xffff, [3] for 0x0000-0x0400
	UINT8 m_bios_page[4];

	// for gamegear LCD persistence hack
	bitmap_rgb32 m_prev_bitmap;

	// for 3D glass binocular hack
	bitmap_rgb32 m_prevleft_bitmap;
	bitmap_rgb32 m_prevright_bitmap;

	// model identifiers
	UINT8 m_is_gamegear;
	UINT8 m_is_gg_region_japan;
	UINT8 m_is_smsj;
	UINT8 m_is_mark_iii;
	UINT8 m_is_sdisp;
	UINT8 m_has_bios_0400;
	UINT8 m_has_bios_2000;
	UINT8 m_has_bios_full;
	UINT8 m_has_fm;
	UINT8 m_has_jpn_sms_cart_slot;

	// Data needed for Light Phaser
	UINT8 m_ctrl1_th_state;
	UINT8 m_ctrl2_th_state;
	UINT8 m_ctrl1_th_latch;
	UINT8 m_ctrl2_th_latch;
	int m_lphaser_x_offs;   /* Needed to 'calibrate' lphaser; set at cart loading */

	// Data needed for SegaScope (3D glasses)
	UINT8 m_sscope_state;
	UINT8 m_frame_sscope_state;

	sega8_cart_slot_device *m_cartslot;
	sega8_card_slot_device *m_cardslot;
	sms_expansion_slot_device *m_expslot;

	// these are only used by the Store Display unit, but we keep them here temporarily to avoid the need of separate start/reset
	UINT8 m_store_control;
	UINT8 m_store_cart_selection_data;
	sega8_cart_slot_device *m_slots[16];
	sega8_card_slot_device *m_cards[16];
	void store_post_load();
	void store_select_cart(UINT8 data);

	/* Cartridge slot info */
	DECLARE_WRITE8_MEMBER(sms_fm_detect_w);
	DECLARE_READ8_MEMBER(sms_fm_detect_r);
	DECLARE_WRITE8_MEMBER(sms_io_control_w);
	DECLARE_READ8_MEMBER(sms_count_r);
	DECLARE_READ8_MEMBER(sms_input_port_dc_r);
	DECLARE_READ8_MEMBER(sms_input_port_dd_r);
	DECLARE_WRITE8_MEMBER(sms_ym2413_register_port_w);
	DECLARE_WRITE8_MEMBER(sms_ym2413_data_port_w);
	DECLARE_READ8_MEMBER(gg_input_port_2_r);
	DECLARE_READ8_MEMBER(sms_sscope_r);
	DECLARE_WRITE8_MEMBER(sms_sscope_w);
	DECLARE_READ8_MEMBER(sms_mapper_r);

	DECLARE_READ8_MEMBER(read_0000);
	DECLARE_READ8_MEMBER(read_4000);
	DECLARE_READ8_MEMBER(read_8000);
	DECLARE_READ8_MEMBER(read_ram);
	DECLARE_WRITE8_MEMBER(write_ram);
	DECLARE_WRITE8_MEMBER(write_cart);

	DECLARE_WRITE8_MEMBER(sms_mapper_w);
	DECLARE_WRITE8_MEMBER(sms_mem_control_w);
	DECLARE_WRITE8_MEMBER(gg_sio_w);
	DECLARE_READ8_MEMBER(gg_sio_r);
	DECLARE_DRIVER_INIT(sg1000m3);
	DECLARE_DRIVER_INIT(gamegear);
	DECLARE_DRIVER_INIT(gamegeaj);
	DECLARE_DRIVER_INIT(sms1krfm);
	DECLARE_DRIVER_INIT(sms1kr);
	DECLARE_DRIVER_INIT(smskr);
	DECLARE_DRIVER_INIT(smsj);
	DECLARE_DRIVER_INIT(sms1);
	DECLARE_MACHINE_START(sms);
	DECLARE_MACHINE_RESET(sms);
	DECLARE_VIDEO_START(gamegear);
	DECLARE_VIDEO_START(sms1);
	DECLARE_VIDEO_RESET(sms1);
	UINT32 screen_update_gamegear(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_sms(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_sms1(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void screen_vblank_sms1(screen_device &screen, bool state);
	DECLARE_WRITE_LINE_MEMBER(sms_int_callback);
	DECLARE_WRITE_LINE_MEMBER(sms_pause_callback);
	DECLARE_READ32_MEMBER(sms_pixel_color);
	DECLARE_WRITE_LINE_MEMBER(sms_ctrl1_th_input);
	DECLARE_WRITE_LINE_MEMBER(sms_ctrl2_th_input);

protected:
	UINT8 read_bus(address_space &space, unsigned int bank, UINT16 base_addr, UINT16 offset);
	void setup_bios();
	void setup_media_slots();
	void setup_enabled_slots();
	void lphaser_hcount_latch();
	void sms_get_inputs(address_space &space);
};

class smssdisp_state : public sms_state
{
public:
	smssdisp_state(const machine_config &mconfig, device_type type, const char *tag)
	: sms_state(mconfig, type, tag),
	m_control_cpu(*this, "control")
	{ }

	required_device<cpu_device> m_control_cpu;

	DECLARE_READ8_MEMBER(sms_store_cart_select_r);
	DECLARE_WRITE8_MEMBER(sms_store_cart_select_w);
	DECLARE_WRITE8_MEMBER(sms_store_control_w);
	DECLARE_DRIVER_INIT(smssdisp);

	DECLARE_READ8_MEMBER(store_cart_peek);

	DECLARE_WRITE_LINE_MEMBER(sms_store_int_callback);
};


/*----------- defined in machine/sms.c -----------*/

#define IO_EXPANSION    (0x80)  /* Expansion slot enable (1= disabled, 0= enabled) */
#define IO_CARTRIDGE    (0x40)  /* Cartridge slot enable (1= disabled, 0= enabled) */
#define IO_CARD         (0x20)  /* Card slot disabled (1= disabled, 0= enabled) */
#define IO_WORK_RAM     (0x10)  /* Work RAM disabled (1= disabled, 0= enabled) */
#define IO_BIOS_ROM     (0x08)  /* BIOS ROM disabled (1= disabled, 0= enabled) */
#define IO_CHIP         (0x04)  /* I/O chip disabled (1= disabled, 0= enabled) */

#endif /* SMS_H_ */
