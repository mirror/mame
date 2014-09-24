// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*************************************************************************

    The Game Room Lethal Justice hardware

**************************************************************************/

class lethalj_state : public driver_device
{
public:
	enum
	{
		TIMER_GEN_EXT1_INT
	};

	lethalj_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_maincpu(*this, "maincpu"),
		m_screen(*this, "screen") { }

	required_device<cpu_device> m_maincpu;
	required_device<screen_device> m_screen;
	UINT16 m_blitter_data[8];
	UINT16 *m_screenram;
	UINT8 m_vispage;
	UINT16 *m_blitter_base;
	int m_blitter_rows;
	UINT16 m_gunx;
	UINT16 m_guny;
	UINT8 m_blank_palette;
	DECLARE_WRITE16_MEMBER(ripribit_control_w);
	DECLARE_WRITE16_MEMBER(cfarm_control_w);
	DECLARE_WRITE16_MEMBER(cclownz_control_w);
	DECLARE_READ16_MEMBER(lethalj_gun_r);
	DECLARE_WRITE16_MEMBER(lethalj_blitter_w);
	void do_blit();
	DECLARE_CUSTOM_INPUT_MEMBER(cclownz_paddle);
	DECLARE_DRIVER_INIT(cfarm);
	DECLARE_DRIVER_INIT(ripribit);
	DECLARE_DRIVER_INIT(cclownz);
	virtual void video_start();
	inline void get_crosshair_xy(int player, int *x, int *y);
	TMS340X0_SCANLINE_IND16_CB_MEMBER(scanline_update);

protected:
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);
};

/*----------- defined in video/lethalj.c -----------*/
void lethalj_scanline_update(screen_device &screen, bitmap_ind16 &bitmap, int scanline, const tms34010_display_params *params);
