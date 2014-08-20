/*************************************************************************

    Chequered Flag

*************************************************************************/
#include "sound/k007232.h"
#include "video/k051960.h"
#include "video/k051316.h"
#include "video/k051733.h"
#include "video/konami_helper.h"

class chqflag_state : public driver_device
{
public:
	chqflag_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_audiocpu(*this, "audiocpu"),
		m_k007232_1(*this, "k007232_1"),
		m_k007232_2(*this, "k007232_2"),
		m_k051960(*this, "k051960"),
		m_k051316_1(*this, "k051316_1"),
		m_k051316_2(*this, "k051316_2"),
		m_palette(*this, "palette") { }

	/* memory pointers */
	UINT8 *    m_ram;
	dynamic_array<UINT8> m_paletteram;

	/* video-related */
	int        m_zoom_colorbase[2];
	int        m_sprite_colorbase;

	/* misc */
	int        m_k051316_readroms;
	int        m_last_vreg;
	int        m_analog_ctrl;
	int        m_accel;
	int        m_wheel;
	int		   m_bank;

	/* devices */
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_audiocpu;
	required_device<k007232_device> m_k007232_1;
	required_device<k007232_device> m_k007232_2;
	required_device<k051960_device> m_k051960;
	required_device<k051316_device> m_k051316_1;
	required_device<k051316_device> m_k051316_2;
	required_device<palette_device> m_palette;

	DECLARE_WRITE8_MEMBER(chqflag_bankswitch_w);
	DECLARE_WRITE8_MEMBER(chqflag_vreg_w);
	DECLARE_WRITE8_MEMBER(select_analog_ctrl_w);
	DECLARE_READ8_MEMBER(analog_read_r);
	DECLARE_WRITE8_MEMBER(chqflag_sh_irqtrigger_w);
	DECLARE_WRITE8_MEMBER(k007232_bankswitch_w);
	DECLARE_WRITE8_MEMBER(k007232_extvolume_w);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	UINT32 screen_update_chqflag(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_DEVICE_CALLBACK_MEMBER(chqflag_scanline);
	DECLARE_WRITE8_MEMBER(volume_callback0);
	DECLARE_WRITE8_MEMBER(volume_callback1);
	K051316_CB_MEMBER(zoom_callback_1);
	K051316_CB_MEMBER(zoom_callback_2);
	K051960_CB_MEMBER(sprite_callback);
	void bankswitch_restore();
};
