// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    Sega System 32/Multi 32 hardware

***************************************************************************/

#include "machine/eepromser.h"
#include "sound/multipcm.h"


class segas32_state : public driver_device
{
public:
	segas32_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_z80_shared_ram(*this,"z80_shared_ram"),
		m_ga2_dpram(*this,"ga2_dpram"),
		m_system32_workram(*this,"workram"),
		m_system32_videoram(*this,"videoram", 0),
		m_system32_spriteram(*this,"spriteram", 0),
		m_system32_paletteram(*this,"paletteram", 0) ,
		m_maincpu(*this, "maincpu"),
		m_soundcpu(*this, "soundcpu"),
		m_multipcm(*this, "sega"),
		m_eeprom(*this, "eeprom"),
		m_gfxdecode(*this, "gfxdecode"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette") { }

	required_shared_ptr<UINT8> m_z80_shared_ram;
	optional_shared_ptr<UINT8> m_ga2_dpram;
	optional_shared_ptr<UINT16> m_system32_workram;
	required_shared_ptr<UINT16> m_system32_videoram;
	required_shared_ptr<UINT16> m_system32_spriteram;
	optional_shared_ptr_array<UINT16, 2> m_system32_paletteram;

	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_soundcpu;
	optional_device<multipcm_device> m_multipcm;
	required_device<eeprom_serial_93cxx_device> m_eeprom;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<screen_device> m_screen;
	required_device<palette_device> m_palette;

	typedef void (segas32_state::*sys32_output_callback)(int which, UINT16 data);

	struct layer_info
	{
		bitmap_ind16 *bitmap;
		UINT8 *transparent;
	};

	struct extents_list
	{
		UINT8                   scan_extent[256];
		UINT16                  extent[32][16];
	};


	struct cache_entry
	{
		struct cache_entry *    next;
		tilemap_t *             tmap;
		UINT8                   page;
		UINT8                   bank;
	};

	UINT8 m_v60_irq_control[0x10];
	timer_device *m_v60_irq_timer[2];
	UINT8 m_sound_irq_control[4];
	UINT8 m_sound_irq_input;
	UINT8 m_sound_dummy_value;
	UINT16 m_sound_bank;
	UINT8 m_misc_io_data[2][0x10];
	read16_delegate m_custom_io_r[2];
	write16_delegate m_custom_io_w[2];
	UINT8 m_analog_bank;
	UINT8 m_analog_value[4];
	UINT8 m_sonic_last[6];
	sys32_output_callback m_sw1_output;
	sys32_output_callback m_sw2_output;
	sys32_output_callback m_sw3_output;
	UINT16* m_dual_pcb_comms;
	UINT16 *m_system32_protram;
	UINT16 m_system32_displayenable[2];
	UINT16 m_system32_tilebank_external;
	UINT16 m_arescue_dsp_io[6];
	UINT8 m_is_multi32;
	struct cache_entry *m_cache_head;
	struct layer_info m_layer_data[11];
	UINT16 m_mixer_control[2][0x40];
	UINT16 *m_solid_0000;
	UINT16 *m_solid_ffff;
	UINT8 m_sprite_render_count;
	UINT8 m_sprite_control_latched[8];
	UINT8 m_sprite_control[8];
	UINT32 *m_spriteram_32bit;
	void (*m_system32_prot_vblank)(device_t *device);
	int m_print_count;
	DECLARE_WRITE16_MEMBER(ga2_dpram_w);
	DECLARE_READ16_MEMBER(ga2_dpram_r);
	DECLARE_READ16_MEMBER(ga2_sprite_protection_r);
	DECLARE_READ16_MEMBER(ga2_wakeup_protection_r);
	DECLARE_WRITE16_MEMBER(sonic_level_load_protection);
	DECLARE_READ16_MEMBER(brival_protection_r);
	DECLARE_WRITE16_MEMBER(brival_protection_w);
	DECLARE_WRITE16_MEMBER(darkedge_protection_w);
	DECLARE_READ16_MEMBER(darkedge_protection_r);
	DECLARE_WRITE16_MEMBER(dbzvrvs_protection_w);
	DECLARE_READ16_MEMBER(dbzvrvs_protection_r);
	DECLARE_READ16_MEMBER(arabfgt_protection_r);
	DECLARE_WRITE16_MEMBER(arabfgt_protection_w);
	DECLARE_READ16_MEMBER(arf_wakeup_protection_r);
	DECLARE_WRITE16_MEMBER(jleague_protection_w);
	DECLARE_READ16_MEMBER(arescue_dsp_r);
	DECLARE_WRITE16_MEMBER(arescue_dsp_w);
	DECLARE_READ16_MEMBER(system32_paletteram_r);
	DECLARE_WRITE16_MEMBER(system32_paletteram_w);
	DECLARE_READ32_MEMBER(multi32_paletteram_0_r);
	DECLARE_WRITE32_MEMBER(multi32_paletteram_0_w);
	DECLARE_READ32_MEMBER(multi32_paletteram_1_r);
	DECLARE_WRITE32_MEMBER(multi32_paletteram_1_w);
	DECLARE_READ16_MEMBER(system32_videoram_r);
	DECLARE_WRITE16_MEMBER(system32_videoram_w);
	DECLARE_READ32_MEMBER(multi32_videoram_r);
	DECLARE_WRITE32_MEMBER(multi32_videoram_w);
	DECLARE_READ16_MEMBER(system32_sprite_control_r);
	DECLARE_WRITE16_MEMBER(system32_sprite_control_w);
	DECLARE_READ32_MEMBER(multi32_sprite_control_r);
	DECLARE_WRITE32_MEMBER(multi32_sprite_control_w);
	DECLARE_READ16_MEMBER(system32_spriteram_r);
	DECLARE_WRITE16_MEMBER(system32_spriteram_w);
	DECLARE_READ32_MEMBER(multi32_spriteram_r);
	DECLARE_WRITE32_MEMBER(multi32_spriteram_w);
	DECLARE_READ16_MEMBER(system32_mixer_r);
	DECLARE_WRITE16_MEMBER(system32_mixer_w);
	DECLARE_WRITE32_MEMBER(multi32_mixer_0_w);
	DECLARE_WRITE32_MEMBER(multi32_mixer_1_w);
	DECLARE_READ16_MEMBER(interrupt_control_16_r);
	DECLARE_WRITE16_MEMBER(interrupt_control_16_w);
	DECLARE_READ32_MEMBER(interrupt_control_32_r);
	DECLARE_WRITE32_MEMBER(interrupt_control_32_w);
	DECLARE_READ16_MEMBER(io_chip_r);
	DECLARE_WRITE16_MEMBER(io_chip_w);
	DECLARE_READ32_MEMBER(io_chip_0_r);
	DECLARE_WRITE32_MEMBER(io_chip_0_w);
	DECLARE_READ32_MEMBER(io_chip_1_r);
	DECLARE_WRITE32_MEMBER(io_chip_1_w);
	DECLARE_READ16_MEMBER(io_expansion_r);
	DECLARE_WRITE16_MEMBER(io_expansion_w);
	DECLARE_READ32_MEMBER(io_expansion_0_r);
	DECLARE_WRITE32_MEMBER(io_expansion_0_w);
	DECLARE_READ32_MEMBER(io_expansion_1_r);
	DECLARE_WRITE32_MEMBER(io_expansion_1_w);
	DECLARE_READ16_MEMBER(analog_custom_io_r);
	DECLARE_WRITE16_MEMBER(analog_custom_io_w);
	DECLARE_READ16_MEMBER(extra_custom_io_r);
	DECLARE_WRITE16_MEMBER(orunners_custom_io_w);
	DECLARE_READ16_MEMBER(sonic_custom_io_r);
	DECLARE_WRITE16_MEMBER(sonic_custom_io_w);
	DECLARE_WRITE16_MEMBER(random_number_16_w);
	DECLARE_READ16_MEMBER(random_number_16_r);
	DECLARE_WRITE32_MEMBER(random_number_32_w);
	DECLARE_READ32_MEMBER(random_number_32_r);
	DECLARE_READ16_MEMBER(shared_ram_16_r);
	DECLARE_WRITE16_MEMBER(shared_ram_16_w);
	DECLARE_READ32_MEMBER(shared_ram_32_r);
	DECLARE_WRITE32_MEMBER(shared_ram_32_w);
	DECLARE_WRITE8_MEMBER(sound_int_control_lo_w);
	DECLARE_WRITE8_MEMBER(sound_int_control_hi_w);
	DECLARE_WRITE8_MEMBER(sound_bank_lo_w);
	DECLARE_WRITE8_MEMBER(sound_bank_hi_w);
	DECLARE_READ8_MEMBER(sound_dummy_r);
	DECLARE_WRITE8_MEMBER(sound_dummy_w);
	DECLARE_WRITE16_MEMBER(dual_pcb_comms_w);
	DECLARE_READ16_MEMBER(dual_pcb_comms_r);
	DECLARE_READ16_MEMBER(dual_pcb_masterslave);
	DECLARE_READ16_MEMBER(arescue_handshake_r);
	DECLARE_READ16_MEMBER(arescue_slavebusy_r);
	DECLARE_WRITE16_MEMBER(f1en_comms_echo_w);
	DECLARE_WRITE8_MEMBER(multipcm_bank_w);
	DECLARE_WRITE8_MEMBER(scross_bank_w);
	DECLARE_DRIVER_INIT(titlef);
	DECLARE_DRIVER_INIT(slipstrm);
	DECLARE_DRIVER_INIT(radm);
	DECLARE_DRIVER_INIT(holo);
	DECLARE_DRIVER_INIT(svf);
	DECLARE_DRIVER_INIT(jleague);
	DECLARE_DRIVER_INIT(arescue);
	DECLARE_DRIVER_INIT(jpark);
	DECLARE_DRIVER_INIT(ga2);
	DECLARE_DRIVER_INIT(scross);
	DECLARE_DRIVER_INIT(spidman);
	DECLARE_DRIVER_INIT(sonicp);
	DECLARE_DRIVER_INIT(f1en);
	DECLARE_DRIVER_INIT(dbzvrvs);
	DECLARE_DRIVER_INIT(brival);
	DECLARE_DRIVER_INIT(harddunk);
	DECLARE_DRIVER_INIT(arabfgt);
	DECLARE_DRIVER_INIT(sonic);
	DECLARE_DRIVER_INIT(alien3);
	DECLARE_DRIVER_INIT(darkedge);
	DECLARE_DRIVER_INIT(radr);
	DECLARE_DRIVER_INIT(f1lap);
	DECLARE_DRIVER_INIT(orunners);
	TILE_GET_INFO_MEMBER(get_tile_info);
	DECLARE_MACHINE_RESET(system32);
	DECLARE_VIDEO_START(system32);
	DECLARE_VIDEO_START(multi32);
	UINT32 screen_update_system32(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_multi32_left(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_multi32_right(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(start_of_vblank_int);
	TIMER_CALLBACK_MEMBER(end_of_vblank_int);
	TIMER_CALLBACK_MEMBER(update_sprites);
	TIMER_DEVICE_CALLBACK_MEMBER(signal_v60_irq_callback);
	void common_start(int multi32);
	void system32_set_vblank(int state);
	inline UINT16 xBBBBBGGGGGRRRRR_to_xBGRBBBBGGGGRRRR(UINT16 value);
	inline UINT16 xBGRBBBBGGGGRRRR_to_xBBBBBGGGGGRRRRR(UINT16 value);
	inline void update_color(int offset, UINT16 data);
	inline UINT16 common_paletteram_r(address_space &space, int which, offs_t offset);
	void common_paletteram_w(address_space &space, int which, offs_t offset, UINT16 data, UINT16 mem_mask);
	tilemap_t *find_cache_entry(int page, int bank);
	inline void get_tilemaps(int bgnum, tilemap_t **tilemaps);
	UINT8 update_tilemaps(screen_device &screen, const rectangle &cliprect);
	void sprite_erase_buffer();
	void sprite_swap_buffers();
	int draw_one_sprite(UINT16 *data, int xoffs, int yoffs, const rectangle &clipin, const rectangle &clipout);
	void sprite_render_list();
	inline UINT8 compute_color_offsets(int which, int layerbit, int layerflag);
	inline UINT16 compute_sprite_blend(UINT8 encoding);
	inline UINT16 *get_layer_scanline(int layer, int scanline);
	void mix_all_layers(int which, int xoffs, bitmap_rgb32 &bitmap, const rectangle &cliprect, UINT8 enablemask);
	void print_mixer_data(int which);
	UINT32 multi32_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect, int index);
	void decrypt_ga2_protrom();
	void update_irq_state();
	void signal_v60_irq(int which);
	void int_control_w(address_space &space, int offset, UINT8 data);
	UINT16 common_io_chip_r(address_space &space, int which, offs_t offset, UINT16 mem_mask);
	void common_io_chip_w(address_space &space, int which, offs_t offset, UINT16 data, UINT16 mem_mask);
	void update_sound_irq_state();
	void segas32_common_init(read16_delegate custom_r, write16_delegate custom_w);
	void radm_sw1_output( int which, UINT16 data );
	void radm_sw2_output( int which, UINT16 data );
	void radr_sw2_output( int which, UINT16 data );
	void alien3_sw1_output( int which, UINT16 data );
	void arescue_sw1_output( int which, UINT16 data );
	void f1lap_sw1_output( int which, UINT16 data );
	void jpark_sw1_output( int which, UINT16 data );
	void orunners_sw1_output( int which, UINT16 data );
	void orunners_sw2_output( int which, UINT16 data );
	void harddunk_sw1_output( int which, UINT16 data );
	void harddunk_sw2_output( int which, UINT16 data );
	void harddunk_sw3_output( int which, UINT16 data );
	void titlef_sw1_output( int which, UINT16 data );
	void titlef_sw2_output( int which, UINT16 data );
	void scross_sw1_output( int which, UINT16 data );
	void scross_sw2_output( int which, UINT16 data );
	int compute_clipping_extents(screen_device &screen, int enable, int clipout, int clipmask, const rectangle &cliprect, struct extents_list *list);
	void update_tilemap_zoom(screen_device &screen, struct layer_info *layer, const rectangle &cliprect, int bgnum);
	void update_tilemap_rowscroll(screen_device &screen, struct layer_info *layer, const rectangle &cliprect, int bgnum);
	void update_tilemap_text(screen_device &screen, struct layer_info *layer, const rectangle &cliprect);
	void update_bitmap(screen_device &screen, struct layer_info *layer, const rectangle &cliprect);
	void update_background(struct layer_info *layer, const rectangle &cliprect);
	DECLARE_WRITE_LINE_MEMBER(ym3438_irq_handler);
};

/*----------- defined in machine/segas32.c -----------*/
void darkedge_fd1149_vblank(device_t *device);
void f1lap_fd1149_vblank(device_t *device);
extern const UINT8 ga2_v25_opcode_table[];
