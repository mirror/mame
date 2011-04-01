/*************************************************************************

    Circus Charlie

*************************************************************************/

class circusc_state : public driver_device
{
public:
	circusc_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	/* memory pointers */
	UINT8 *        m_videoram;
	UINT8 *        m_colorram;
	UINT8 *        m_spriteram;
	UINT8 *        m_spriteram_2;
	UINT8 *        m_spritebank;
	UINT8 *        m_scroll;
	size_t         m_spriteram_size;

	/* video-related */
	tilemap_t        *m_bg_tilemap;

	/* sound-related */
	UINT8          m_sn_latch;

	/* devices */
	cpu_device *m_audiocpu;
	device_t *m_sn1;
	device_t *m_sn2;
	device_t *m_dac;
	device_t *m_discrete;
};


/*----------- defined in video/circusc.c -----------*/

WRITE8_HANDLER( circusc_videoram_w );
WRITE8_HANDLER( circusc_colorram_w );

VIDEO_START( circusc );
WRITE8_HANDLER( circusc_flipscreen_w );
PALETTE_INIT( circusc );
SCREEN_UPDATE( circusc );
