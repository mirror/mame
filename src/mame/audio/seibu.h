/***************************************************************************

    Seibu Sound System v1.02, games using this include:

    Cross Shooter    1987   * "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812)
    Cabal            1988   * "Michel/Seibu    sound 11/04/88" (YM2151 substituted for YM3812, unknown ADPCM)
    Dead Angle       1988   * "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (2xYM2203 substituted for YM3812, unknown ADPCM)
    Dynamite Duke    1989   * "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
    Toki             1989   * "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
    Raiden           1990   * "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
    Blood Brothers   1990     "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
    D-Con            1992     "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."

    Related sound programs (not implemented yet):

    Zero Team                 "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
    Legionaire                "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812)
    Raiden 2                  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812, plus extra MSM6205)
    Raiden DX                 "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812, plus extra MSM6205)
    Cup Soccer                "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812, plus extra MSM6205)
    SD Gundam Psycho Salamander "Copyright by King Bee Sol 1991"
    * = encrypted

***************************************************************************/

#include "cpu/z80/z80.h"
#include "sound/3812intf.h"
#include "sound/2151intf.h"
#include "sound/2203intf.h"
#include "sound/okim6295.h"

ADDRESS_MAP_EXTERN(seibu_sound_map, 8);
ADDRESS_MAP_EXTERN(seibu2_sound_map, 8);
ADDRESS_MAP_EXTERN(seibu2_airraid_sound_map, 8);
ADDRESS_MAP_EXTERN(seibu2_raiden2_sound_map, 8);
ADDRESS_MAP_EXTERN(seibu_newzeroteam_sound_map, 8);
ADDRESS_MAP_EXTERN(seibu3_sound_map, 8);
ADDRESS_MAP_EXTERN(seibu3_adpcm_sound_map, 8);

class seibu_sound_device : public device_t
{
public:
	seibu_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~seibu_sound_device() {}

	DECLARE_READ16_MEMBER( main_word_r );
	DECLARE_READ8_MEMBER( main_v30_r );
	DECLARE_WRITE16_MEMBER( main_word_w );
	DECLARE_WRITE8_MEMBER( main_v30_w );

	DECLARE_WRITE16_MEMBER( main_mustb_w );

	DECLARE_WRITE8_MEMBER( irq_clear_w );
	DECLARE_WRITE8_MEMBER( rst10_ack_w );
	DECLARE_WRITE8_MEMBER( rst18_ack_w );
	DECLARE_WRITE8_MEMBER( bank_w );
	DECLARE_WRITE8_MEMBER( coin_w );
	void ym3812_irqhandler(int linestate);
	WRITE_LINE_MEMBER(ym2151_irqhandler);
	void ym2203_irqhandler(int linestate);
	DECLARE_READ8_MEMBER( soundlatch_r );
	DECLARE_READ8_MEMBER( main_data_pending_r );
	DECLARE_WRITE8_MEMBER( main_data_w );
	DECLARE_WRITE8_MEMBER( pending_w );
	void decrypt(const char *cpu,int length);
	void update_irq_lines(int param);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	private:
	// internal state
	device_t *m_sound_cpu;
	UINT8 m_main2sub[2];
	UINT8 m_sub2main[2];
	int m_main2sub_pending;
	int m_sub2main_pending;
	UINT8 m_rst10_irq;
	UINT8 m_rst18_irq;

	enum
	{
		VECTOR_INIT,
		RST10_ASSERT,
		RST10_CLEAR,
		RST18_ASSERT,
		RST18_CLEAR
	};
};

extern const device_type SEIBU_SOUND;


// Seibu ADPCM device

class seibu_adpcm_device : public device_t,
									public device_sound_interface
{
public:
	seibu_adpcm_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	~seibu_adpcm_device() {}

	static void set_adpcm_rom_tag(device_t &device, const char *tag) { downcast<seibu_adpcm_device &>(device).m_rom_tag = tag; }

	void decrypt(const char *region);
	DECLARE_WRITE8_MEMBER( adr_w );
	DECLARE_WRITE8_MEMBER( ctl_w );

protected:
	// device-level overrides
	virtual void device_start();

	// sound stream update overrides
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples);

	private:
	// internal state
	oki_adpcm_state m_adpcm;
	sound_stream *m_stream;
	UINT32 m_current;
	UINT32 m_end;
	UINT8 m_nibble;
	UINT8 m_playing;
	const char *m_rom_tag;
	UINT8 *m_base;
};

extern const device_type SEIBU_ADPCM;

/**************************************************************************/

#define MCFG_SEIBU_ADPCM_ROMREGION(_tag) \
	seibu_adpcm_device::set_adpcm_rom_tag(*device, _tag);

#define SEIBU_COIN_INPUTS                                           \
	PORT_START("COIN")                                              \
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(4)     \
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(4)     \
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNUSED )

#define SEIBU_COIN_INPUTS_INVERT                                    \
	PORT_START("COIN")                                              \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(4)      \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(4)      \
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )


#define SEIBU_SOUND_SYSTEM_CPU(freq)                                \
	MCFG_CPU_ADD("audiocpu", Z80, freq)                             \
	MCFG_CPU_PROGRAM_MAP(seibu_sound_map)                           \
	MCFG_DEVICE_ADD("seibu_sound", SEIBU_SOUND, 0)

#define SEIBU2_SOUND_SYSTEM_CPU(freq)                               \
	MCFG_CPU_ADD("audiocpu", Z80, freq)                             \
	MCFG_CPU_PROGRAM_MAP(seibu2_sound_map)                          \
	MCFG_DEVICE_ADD("seibu_sound", SEIBU_SOUND, 0)

#define SEIBU2_AIRRAID_SOUND_SYSTEM_CPU(freq)                       \
	MCFG_CPU_ADD("audiocpu", Z80, freq)                             \
	MCFG_CPU_PROGRAM_MAP(seibu2_airraid_sound_map)                  \
	MCFG_DEVICE_ADD("seibu_sound", SEIBU_SOUND, 0)

#define SEIBU2_RAIDEN2_SOUND_SYSTEM_CPU(freq)                       \
	MCFG_CPU_ADD("audiocpu",  Z80, freq)                            \
	MCFG_CPU_PROGRAM_MAP(seibu2_raiden2_sound_map)                  \
	MCFG_DEVICE_ADD("seibu_sound", SEIBU_SOUND, 0)

#define SEIBU_NEWZEROTEAM_SOUND_SYSTEM_CPU(freq)                    \
	MCFG_CPU_ADD("audiocpu", Z80, freq)                             \
	MCFG_CPU_PROGRAM_MAP(seibu_newzeroteam_sound_map)               \
	MCFG_DEVICE_ADD("seibu_sound", SEIBU_SOUND, 0)

#define SEIBU3_SOUND_SYSTEM_CPU(freq)                               \
	MCFG_CPU_ADD("audiocpu", Z80, freq)                             \
	MCFG_CPU_PROGRAM_MAP(seibu3_sound_map)                          \
	MCFG_DEVICE_ADD("seibu_sound", SEIBU_SOUND, 0)

#define SEIBU3A_SOUND_SYSTEM_CPU(freq)                              \
	MCFG_CPU_ADD("audiocpu", Z80, freq)                             \
	MCFG_CPU_PROGRAM_MAP(seibu3_adpcm_sound_map)                    \
	MCFG_DEVICE_ADD("seibu_sound", SEIBU_SOUND, 0)

#define SEIBU_SOUND_SYSTEM_YM3812_INTERFACE(freq1,freq2)            \
	MCFG_SPEAKER_STANDARD_MONO("mono")                              \
																	\
	MCFG_SOUND_ADD("ymsnd", YM3812, freq1)                              \
	MCFG_YM3812_IRQ_HANDLER(DEVWRITELINE("seibu_sound", seibu_sound_device, ym3812_irqhandler)) \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)                      \
																	\
	MCFG_OKIM6295_ADD("oki", freq2, OKIM6295_PIN7_LOW)              \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)

#define SEIBU_SOUND_SYSTEM_YM3812_RAIDEN_INTERFACE(freq1,freq2)     \
	MCFG_SPEAKER_STANDARD_MONO("mono")                              \
																	\
	MCFG_SOUND_ADD("ymsnd", YM3812, freq1)                              \
	MCFG_YM3812_IRQ_HANDLER(DEVWRITELINE("seibu_sound", seibu_sound_device, ym3812_irqhandler)) \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)                      \
																	\
	MCFG_OKIM6295_ADD("oki", freq2, OKIM6295_PIN7_HIGH)             \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

#define SEIBU_SOUND_SYSTEM_YM2151_INTERFACE(freq1,freq2)            \
	MCFG_SPEAKER_STANDARD_MONO("mono")                              \
																	\
	MCFG_YM2151_ADD("ymsnd", freq1)                             \
	MCFG_YM2151_IRQ_HANDLER(DEVWRITELINE("seibu_sound", seibu_sound_device, ym2151_irqhandler)) \
	MCFG_SOUND_ROUTE(0, "mono", 0.50)                               \
	MCFG_SOUND_ROUTE(1, "mono", 0.50)                               \
																	\
	MCFG_OKIM6295_ADD("oki", freq2, OKIM6295_PIN7_LOW)              \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)

#define SEIBU_AIRRAID_SOUND_SYSTEM_YM2151_INTERFACE(freq1)          \
	MCFG_SPEAKER_STANDARD_MONO("mono")                              \
																	\
	MCFG_YM2151_ADD("ymsnd", freq1)                             \
	MCFG_YM2151_IRQ_HANDLER(DEVWRITELINE("seibu_sound", seibu_sound_device, ym2151_irqhandler)) \
	MCFG_SOUND_ROUTE(0, "mono", 0.50)                               \
	MCFG_SOUND_ROUTE(1, "mono", 0.50)

#define SEIBU_SOUND_SYSTEM_YM2151_RAIDEN2_INTERFACE(freq1,freq2,regiona, regionb)       \
	MCFG_SPEAKER_STANDARD_MONO("mono")                              \
																	\
	MCFG_YM2151_ADD("ymsnd", freq1)                             \
	MCFG_YM2151_IRQ_HANDLER(DEVWRITELINE("seibu_sound", seibu_sound_device, ym2151_irqhandler)) \
	MCFG_SOUND_ROUTE(0, "mono", 0.50)                               \
	MCFG_SOUND_ROUTE(1, "mono", 0.50)                               \
																	\
	MCFG_OKIM6295_ADD("oki1", freq2, OKIM6295_PIN7_HIGH)            \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)                     \
																	\
	MCFG_OKIM6295_ADD("oki2", freq2, OKIM6295_PIN7_HIGH)            \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)

#define SEIBU_SOUND_SYSTEM_YM2203_INTERFACE(freq)                   \
	MCFG_SPEAKER_STANDARD_MONO("mono")                              \
																	\
	MCFG_SOUND_ADD("ym1", YM2203, freq)                             \
	MCFG_YM2203_IRQ_HANDLER(DEVWRITELINE("seibu_sound", seibu_sound_device, ym2203_irqhandler)) \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)                     \
																	\
	MCFG_SOUND_ADD("ym2", YM2203, freq)                             \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.15)

#define SEIBU_SOUND_SYSTEM_ADPCM_INTERFACE                          \
	MCFG_SOUND_ADD("adpcm1", SEIBU_ADPCM, 8000)                     \
	MCFG_SEIBU_ADPCM_ROMREGION("adpcm1")                      \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)                     \
																	\
	MCFG_SOUND_ADD("adpcm2", SEIBU_ADPCM, 8000)                     \
	MCFG_SEIBU_ADPCM_ROMREGION("adpcm2")                      \
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)

/**************************************************************************/
