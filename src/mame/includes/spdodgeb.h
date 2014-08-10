/*************************************************************************

    Super Dodge Ball hardware

*************************************************************************/

#include "sound/msm5205.h"

class spdodgeb_state : public driver_device
{
public:
	spdodgeb_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_videoram(*this, "videoram"),
		m_spriteram(*this, "spriteram"),
		m_maincpu(*this,"maincpu"),
		m_audiocpu(*this, "audiocpu"),
		m_msm1(*this, "msm1"),
		m_msm2(*this, "msm2"),
		m_gfxdecode(*this, "gfxdecode"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette") { }

	int m_toggle;
	int m_adpcm_pos[2];
	int m_adpcm_end[2];
	int m_adpcm_idle[2];
	int m_adpcm_data[2];
	int m_mcu63701_command;
	int m_inputs[4];
	UINT8 m_tapc[4];
	UINT8 m_last_port[2];
	UINT8 m_last_dash[2];
#if 0
	int m_running[2];
	int m_jumped[2];
	int m_prev[2][2];
	int m_countup[2][2];
	int m_countdown[2][2];
	int m_prev[2];
#endif
	required_shared_ptr<UINT8> m_videoram;
	int m_tile_palbank;
	int m_sprite_palbank;
	tilemap_t *m_bg_tilemap;
	int m_lastscroll;
	required_shared_ptr<UINT8> m_spriteram;

	required_device<cpu_device> m_maincpu;
	DECLARE_WRITE8_MEMBER(sound_command_w);
	DECLARE_WRITE8_MEMBER(spd_adpcm_w);
	DECLARE_READ8_MEMBER(mcu63701_r);
	DECLARE_WRITE8_MEMBER(mcu63701_w);
	DECLARE_WRITE8_MEMBER(spdodgeb_scrollx_lo_w);
	DECLARE_WRITE8_MEMBER(spdodgeb_ctrl_w);
	DECLARE_WRITE8_MEMBER(spdodgeb_videoram_w);
	TILEMAP_MAPPER_MEMBER(background_scan);
	TILE_GET_INFO_MEMBER(get_bg_tile_info);
	virtual void machine_reset();
	virtual void video_start();
	DECLARE_PALETTE_INIT(spdodgeb);
	UINT32 screen_update_spdodgeb(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_DEVICE_CALLBACK_MEMBER(spdodgeb_interrupt);
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect );
	void mcu63705_update_inputs();
	DECLARE_CUSTOM_INPUT_MEMBER(mcu63705_busy_r);
	DECLARE_WRITE_LINE_MEMBER(irqhandler);
	void spd_adpcm_int(msm5205_device *device, int chip);
	DECLARE_WRITE_LINE_MEMBER(spd_adpcm_int_1);
	DECLARE_WRITE_LINE_MEMBER(spd_adpcm_int_2);
	required_device<cpu_device> m_audiocpu;
	required_device<msm5205_device> m_msm1;
	required_device<msm5205_device> m_msm2;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<screen_device> m_screen;
	required_device<palette_device> m_palette;
};
