/***************************************************************************

    namcops2.c

    Namco System 246 / System 256 games (Sony PS2 based)

    PS2 baseboard includes:
    * R5900 "Emotion Engine" - MIPS III with 128-bit integer regs & SIMD
    * R3000 IOP - Stock R3000 with cache, not like the PSXCPU
    * VU0 - can operate either as in-line R5900 coprocessor or run independently
    * VU1 - runs independently only, has special DMA path to GS
    * GS - Graphics Synthesizer.  Nothing fancy, draws flat, Gouraud, and/or
      textured tris with no, bilinear, or trilinear filtering very quickly.
    * SPU2 - Sound Processing Unit.  Almost literally 2 PS1 SPUs stuck together,
      with 2 MB of wave RAM, a 48 kHz sample rate (vs. 44.1 on PS1), and 2
      stereo DMADACs.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.


List of Namco System 246 games:
* Battle Gear 3 (Taito, 2002)
* Battle Gear 3 Tuned (Taito, 2003)
* Bloody Roar 3 (Namco / 8ing / Raizing, 2000)
* Capcom Fighting Jam (Capcom, 2004)
* Cobra: The Arcade (Namco, 2004)
* Dragon Chronicles (Namco, 2002)
* Fate/Unlimited Codes (Capcom / Type-Moon / Cavia / 8ing, 2008)
* Getchu Play! Tottoko Hamutaro (Banpresto, 2007)
* Mobile Suit Gundam SEED: Federation Vs. Z.A.F.T. (Capcom / Banpresto, 2005)
* Mobile Suit Gundam SEED Destiny: Federation Vs. Z.A.F.T. II (Banpresto, 2006)
* Mobile Suit Gundam Z: AEUG Vs. Titans (Capcom / Banpresto, 2003)
* Mobile Suit Gundam Z: AEUG Vs. Titans DX (Capcom / Banpresto, 2004)
* Minnade Kitaeru Zennou Training (Namco, 2006)
* Netchuu Pro Yakyuu 2002 (Namco, 2002)
* Pride GP 2003 (Capcom, 2003)
* Quiz and Variety Sukusuku Inufuku 2 (Namco / AMI / Hamster, 2007)
* Quiz Mobile Suit Gundam: Tou. Senshi (Banpresto, 2006)
* Raizin Ping Pong (Taito, 2001)
* Ridge Racer V: Arcade Battle (Namco, 2000)
* Sengoku Basara X Cross (Capcom / ARC System Works, 2008)
* Smash Court Pro Tournament (Namco, 2001)
* Soul Calibur II (Namco, 2002)
* Soul Calibur II Ver.D (Namco, 2003)
* Soul Calibur III Arcade Edition (Namco, 2006)
* Taiko No Tatsujin 7 (Namco, 2005)
* Taiko No Tatsujin 8 (Namco, 2006)
* Technic Beat (Arika, 2002)
* Tekken 4 (Namco, 2001)
* Time Crisis 3 (Namco, 2002)
* Vampire Night (Namco / Sega / WOW Entertainment, 2000)
* Wangan Midnight (Namco, 2001)
* Wangan Midnight R (Namco, 2002)
* Zoids Infinity (Taito, 2004)

List of Namco System 256 Games
* Chou Dragon Ball Z (Banpresto, 2005)
* Druaga Online - The Story of Aon (Namco, 2005)
* Kinnikuman Muscle Grand Prix (Banpresto, 2006)
* Kinnikuman Muscle Grand Prix 2 (Banpresto, 2007)
* Kinnikuman Muscle Grand Prix 2 Tokumori (Banpresto, 2008)
* Mobile Suit Gundam SEED Destiny: Federation Vs. Z.A.F.T. II (Banpresto, 2006)
* Mobile Suit Gundam: Gundam Vs. Gundam (Banpresto, 2008)
* Mobile Suit Gundam: Gundam Vs. Gundam Next (Banpresto, 2009)
* Quiz and Variety Sukusuku Inufuku 2 (Namco / AMI / Hamster, 2007)
* Sengoku Basara X Cross (Capcom / ARC System Works, 2008)
* Taiko No Tatsujin 9 (Namco, 2006)
* Taiko No Tatsujin 10 (Namco, 2007)
* Taiko No Tatsujin 11 (Namco, 2008)
* Taiko No Tatsujin 12 (Namco, 2008)
* Taiko No Tatsujin 12 Don! (Namco, 2009)
* Taiko No Tatsujin 13 (Namco, 2009)
* Taiko No Tatsujin 14 (Namco, 2010)
* Tekken 5 (Namco, 2004)
* Tekken 5.1 (Namco, 2005)
* Tekken 5 Dark Resurrection (Namco, 2005)
* The Battle of YuYu Hakusho (Banpresto, 2006)
* THE iDOLM@STER (Namco, 2005)
* Zoids Infinity EX (Taito, 2005)
* Zoids Infinity EX Plus (Taito, 2006)

List of System Super 256 Games
* Time Crisis 4 (Namco, 2006)

***************************************************************************/


#include "emu.h"
#include "cpu/mips/mips3.h"
#include "cpu/mips/r3000.h"


class namcops2_state : public driver_device
{
public:
	namcops2_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_maincpu(*this, "maincpu")
	{ }

	UINT32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

protected:

	// devices
	required_device<cpu_device> m_maincpu;

	// driver_device overrides
	virtual void video_start();
};


void namcops2_state::video_start()
{
}

UINT32 namcops2_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	return 0;
}

static ADDRESS_MAP_START(ps2_map, AS_PROGRAM, 32, namcops2_state)
	AM_RANGE(0x00000000, 0x01ffffff) AM_RAM // 32 MB RAM in consumer PS2s, do these have more?
	AM_RANGE(0x1fc00000, 0x1fdfffff) AM_ROM AM_REGION("bios", 0)
ADDRESS_MAP_END

static INPUT_PORTS_START( system246 )
INPUT_PORTS_END

static MACHINE_CONFIG_START( system246, namcops2_state )
	MCFG_CPU_ADD("maincpu", R5000LE, 294000000) // actually R5900 @ 294 MHz
	MCFG_MIPS3_ICACHE_SIZE(16384)
	MCFG_MIPS3_DCACHE_SIZE(16384)
	MCFG_CPU_PROGRAM_MAP(ps2_map)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_UPDATE_DRIVER(namcops2_state, screen_update)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 639, 0, 479)

	MCFG_PALETTE_ADD("palette", 65536)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( system256, system246 )
MACHINE_CONFIG_END

#define SYSTEM246_BIOS  \
	ROM_LOAD( "r27v1602f.7d", 0x000000, 0x200000, CRC(2b2e41a2) SHA1(f0a74bbcaf801f3fd0b7002ebd0118564aae3528) )

#define SYSTEM256_BIOS  \
	ROM_LOAD( "r27v1602f.8g", 0x000000, 0x200000, CRC(b2a8eeb6) SHA1(bc4fb4e1e53adbd92385f1726bd69663ff870f1e) )

ROM_START( sys246 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	DISK_REGION("dvd")
ROM_END

ROM_START( sys256 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM256_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	DISK_REGION("dvd")
ROM_END

ROM_START( dragchrn )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "dc001vera.ic002", 0x000000, 0x800000, CRC(923351f0) SHA1(b34c46836af8fa7ab164156a70120da38fa1c31f) )
	ROM_LOAD( "dc001vera_spr.ic002", 0x800000, 0x040000, CRC(1f42dca9) SHA1(10f75649653b4cfa53c25f6c08308e404ed7b0f2) )

	DISK_REGION("dvd")
ROM_END

ROM_START( fghtjam )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "jam1vera.ic002", 0x000000, 0x800000, CRC(61cf3746) SHA1(165195a773bac717b5701647bca4073d86906f4e) )
	ROM_LOAD( "jam1vera_spr.ic002", 0x800000, 0x040000, CRC(5ff79918) SHA1(60146cddc3474cd4c5b51d13cf116dce1664a759) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "jam1-dvd0", 0, SHA1(c3a5814c2391a0727b7ebf5f52f08ac8b429733f) )
ROM_END

ROM_START( kinniku )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM256_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "kn1vera.ic002", 0x000000, 0x800000, CRC(17aac6c3) SHA1(dddf37e88385f01bba27496d03f053fdc33882e2) )
	ROM_LOAD( "kn1vera_spr.ic002", 0x800000, 0x040000, CRC(a601f981) SHA1(39485ab3c10f3d58a2c9651cca82a73617b2fe52) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "kn1-b", 0, SHA1(2f0f9ebe74cdafe3713890221532b4d1dc18c74f) )
ROM_END

ROM_START( kinniku2 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM256_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "kn2vera.ic002", 0x000000, 0x800000, CRC(fb2f71f7) SHA1(29a331cc171d395ad10b352b9b30a61a455a50fe) ) 
	ROM_LOAD( "kn2vera_spr.ic002", 0x800000, 0x040000, CRC(9c18fa50) SHA1(1f75052cf264c3f2e5b332a755d30544d6e5f45c) ) 

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "kn2vera", 0, SHA1(3e1b773cc584911b673d46f9296a5b1a2cef9a45) )
ROM_END

ROM_START( netchu02 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "npy1verb.ic002", 0x000000, 0x800000, CRC(43c0f334) SHA1(5a7f6d607ae012b8477ff32cdfd091b765264499) )
	ROM_LOAD( "npy1verb_spr.ic002", 0x800000, 0x040000, CRC(6a3374f0) SHA1(0c0845edc0ac0e9871e65caade8b4157614b81eb) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "npy1cd0b", 0, SHA1(514adcd2d4205873b3d144a05c033822344798e3) )
ROM_END

ROM_START( soulclb2 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "sc23vera.ic002", 0x000000, 0x800000, CRC(5c537182) SHA1(ff4213db24b1200b494e6c3bd3eb7b75789e4032) )
	ROM_LOAD( "sc23vera_spr.ic002", 0x800000, 0x040000, CRC(8f548cbc) SHA1(81b844dc5873bb397cd4cd5aca101d7486d60385) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "sc21-dvd0d", 0, SHA1(9a7b1ea836adc9d78481928a3067530e0f8d74a6) )
ROM_END

ROM_START( soulcl2a )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "sc22vera.ic002", 0x000000, 0x800000, CRC(2a1031b4) SHA1(81ad0b9273734758da917c62910906f06e774bd6) )
	ROM_LOAD( "sc22vera_spr.ic002", 0x800000, 0x040000, CRC(6dd152e4) SHA1(1eb23b2c65f12b39fecf34d6b21916165441ebe4) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "sc21-dvd0d", 0, SHA1(9a7b1ea836adc9d78481928a3067530e0f8d74a6) )
ROM_END

ROM_START( soulcl2b )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "sc21vera.ic002", 0x000000, 0x800000, CRC(7e92ceeb) SHA1(0c8d9337476c04f30ed86c7a77996f81733c1953) )
	ROM_LOAD( "sc21vera_spr.ic002", 0x800000, 0x040000, CRC(f5502fdf) SHA1(064196982d855bd41bafe97db5ff5694b933016a) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "sc21-dvd0d", 0, SHA1(9a7b1ea836adc9d78481928a3067530e0f8d74a6) )
ROM_END

ROM_START( soulcl2w )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "scxx.ic002", 0x000000, 0x800000, NO_DUMP )   // no idea which dongle(s) this goes with, the SC2 sets are a huge mess now
	ROM_LOAD( "scxx_spr.ic002", 0x800000, 0x040000, NO_DUMP )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "sc21-dvd0b", 0, SHA1(883170f759b4d53c4031e00ff29bcd1a4d3fea97) )
ROM_END

ROM_START( soulclb3 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "sc31001-na-a.ic002", 0x000000, 0x800000, CRC(ddbe9774) SHA1(6bb2d31cb669336345b5508bcca56936ea97c04a) )
	ROM_LOAD( "sc31001-na-a_spr.ic002", 0x800000, 0x040000, CRC(18c6f56d) SHA1(13bc6a3688985c0cd9900b063824a4af691a1b31) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "sc31001-na-dvd0-b", 0, SHA1(b46ee35083f8fcc091ce562951c55fbdbb929e4b) )
ROM_END

ROM_START( soulclb3a )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "sc31002-na-a.ic002", 0x000000, 0x840000, CRC(2ebf91ff) SHA1(01e628344b2cde2edbda9ffea53af6a63e3bddf1) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "sc31001-na-dvd0-b", 0, SHA1(b46ee35083f8fcc091ce562951c55fbdbb929e4b) )
ROM_END

ROM_START( sukuinuf )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "in2vera.ic002", 0x000000, 0x800000, CRC(bba7a744) SHA1(c1c6857317d0d6648898e9b51d4c693b83e49f16) )
	ROM_LOAD( "in2vera_spr.ic002", 0x800000, 0x040000, CRC(c43fed95) SHA1(b6001dc8ff34198400a7bf3e41e5ab73823685b0) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "hm-in2", 0, SHA1(4e2d95798a2bcc6f93bc82c364379a3936d68986) )
ROM_END

ROM_START( taiko9 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM256_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "tk91001-na-a.ic002", 0x000000, 0x800000, CRC(db4efc9a) SHA1(a24f10c726f5bc7313559a515d5c4c34cd129c97) )
	ROM_LOAD( "tk91001-na-a_spr.ic002", 0x800000, 0x040000, CRC(99ece8c0) SHA1(871b1c76ccc0311da04b81c59240e65117cbc9f4) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "tk9100-1-na-dvd0-a", 0, SHA1(6bd40b2c19f30a81689601c3dd46b6dac6d0a2f1) )
ROM_END

ROM_START( taiko10 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM256_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "t101001-na-a.ic002", 0x000000, 0x800000, CRC(fa7f4c4d) SHA1(4f6b24243f2c2fdffadc7acaa3a6fb668e497606) )
	ROM_LOAD( "t101001-na-a_spr.ic002", 0x800000, 0x040000, CRC(0a2926c4) SHA1(fb3d23545b5f9a649c4a14b6424c606139723bd5) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "tk10100-1-na-dvd0-a", 0, SHA1(9aef4a6b64295a6684d56334904b4c92a20abe15) )
ROM_END

ROM_START( tekken4 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "tef3verc.ic002", 0x000000, 0x800000, CRC(8a41290c) SHA1(2c674e3203c7b5302430b1c1115fcf591a0dcbf2) )
	ROM_LOAD( "tef3verc_spr.ic002", 0x800000, 0x040000, CRC(af248bf7) SHA1(b99193fcdad683c0bbd684f37dfea5c5412b398e) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "tef1dvd0", 0, SHA1(f39aa37156245f622a6e19e8a0e081418e247b36) )
ROM_END

ROM_START( tekken4a )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "tef2vera.ic002", 0x000000, 0x800000, CRC(6dbbde96) SHA1(101711f36fe428f3fdb5de88cb03efccebc6e68d) )
	ROM_LOAD( "tef2vera_spr.ic002", 0x800000, 0x040000, CRC(a95fd114) SHA1(669229d47d49a511ab77a6f9b8c8541c00d478cf) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "tef1dvd0", 0, SHA1(f39aa37156245f622a6e19e8a0e081418e247b36) )
ROM_END

ROM_START( tekken4b )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "tef1vera.bin", 0x000000, 0x800000, CRC(154c615b) SHA1(3823daa6dd5e8d9699f8d832d7ca690559b84e96) )
	ROM_LOAD( "tef1vera.spr", 0x800000, 0x040000, CRC(64e12053) SHA1(04383cf928b4fd82290d7cccc7b23104fbf2c2f2) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "tef1dvd0", 0, SHA1(f39aa37156245f622a6e19e8a0e081418e247b36) )
ROM_END

ROM_START( tekken4c )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "tef1verc.ic002", 0x000000, 0x840000, CRC(92697a2b) SHA1(e9ec254d52187f5be0d9be58b25821c1e63bba8e) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "tef1dvd0", 0, SHA1(f39aa37156245f622a6e19e8a0e081418e247b36) )
ROM_END

ROM_START( tekken51 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM256_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "te51verb.ic002", 0x000000, 0x800000, CRC(b4031e38) SHA1(72ee2aea4032e9b03a735b1b6c7574233f0c7711) )
	ROM_LOAD( "te51verb_spr.ic002", 0x800000, 0x040000, CRC(683bad0d) SHA1(ef10accbdc82143c31d29e2b8b812a209b341b1b) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "te51-dvd0", 0, SHA1(2a0ac3723725572c1810b0ef4bcfa7aa114062f8) )
ROM_END

ROM_START( zgundm )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "zga1vera.ic002", 0x000000, 0x800000, CRC(b9e0fcdc) SHA1(ed7329351e951b5a2aed893e55311018547b852b) )
	ROM_LOAD( "zga1vera_spr.ic002", 0x800000, 0x040000, CRC(8e4c715b) SHA1(a2218051f54d5ce4cdd21ef021b9acf7a384b766) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "zga1dvd0", 0, SHA1(7930e5a65f6079851438669dfb1f0e5f9e11329a) )
ROM_END

ROM_START( timecrs3 )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "tst1vera.ic002", 0x000000, 0x800000, CRC(2c7ede91) SHA1(b3d3547f5aac402da2fe76ef51dca3841a982a5e) )
	ROM_LOAD( "tst1vera_spr.ic002", 0x800000, 0x040000, CRC(ee9c8132) SHA1(fb00e102389e2163d2c7efcfefd4f680f0b4d4e8) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "tst1dvd0", 0, SHA1(f8a447d9a4224282516bea590f5217c751bdc4ae) )
ROM_END

ROM_START( timecrs3e )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "tst2vera.ic002", 0x000000, 0x800000, CRC(5e0d136f) SHA1(a0a7ca028518cb7399c96fc03b2a0815d5b805a7) )
	ROM_LOAD( "tst2vera_spr.ic002", 0x800000, 0x040000, CRC(6dcee22f) SHA1(1b395250cf99b5228d02c06efd639f7d39adc27d) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "tst1dvd0", 0, SHA1(f8a447d9a4224282516bea590f5217c751bdc4ae) )
ROM_END

ROM_START( zgundmdx )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "zdx1vera.ic002", 0x000000, 0x800000, CRC(ffcb6f3b) SHA1(57cae327a0af3f6a77291d6cda948d1349a43c00) )
	ROM_LOAD( "zdx1vera_spr.ic002", 0x800000, 0x040000, CRC(16446b28) SHA1(65bdcf216917beec7a36ff640e16aa5cf413c5e4) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "zdx1dvd0", 0, SHA1(fa21626f771106e2441c4515a0e5dff478187ccd) )
ROM_END

ROM_START( gundzaft )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "sed1vera.ic002", 0x000000, 0x800000, CRC(db52309d) SHA1(3e325dfa68dadcc2f9abd9d338e47ffa511e73f8) )
	ROM_LOAD( "sed1vera_spr.ic002", 0x800000, 0x040000, CRC(12641e0e) SHA1(64b7655f95a2e5e41b5a89998f2b858dab05ae75) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "sed1dvd0", 0, SHA1(0e6db61d94f66a4ddd7d4a3013983a838d256c5d) )
ROM_END

ROM_START( rrvac )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "rrv3vera.ic002", 0x000000, 0x800000, CRC(dd20c4a2) SHA1(07bddaac958ac62d9fc29671fc83bd1e3b27f4b8) )
	ROM_LOAD( "rrv3vera_spr.ic002", 0x800000, 0x040000, CRC(712e0e9a) SHA1(d396aaf918036ff7f909a84daefe8f651fdf9b05) )

	ROM_REGION(0x4010, "jvsio", 0)  // Namco "FCA" JVS I/O board PIC16F84 code (see namcos23.c for FCA details)
	ROM_LOAD( "fcap11.ic2",   0x000000, 0x004010, CRC(1b2592ce) SHA1(a1a487361053af564f6ec67e545413e370a3b38c) )

	ROM_REGION(0x80000, "steering", 0)  // Steering I/O board MB90242A code (see namcos23.c for steering board details)
	ROM_LOAD( "rrv3_str-0a.ic16", 0x000000, 0x080000, CRC(df8b6cac) SHA1(d45e150678218084925673e1d77edefc04135035) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "rrv1-a", 0, SHA1(77bb70407511cbb12ab999410e797dcaf0779229) )
ROM_END

ROM_START( scptour )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "scp1vera.ic002", 0x000000, 0x800000, CRC(4743a999) SHA1(97ae15d75dd9b80411d101b97dd215e31de56390) )
	ROM_LOAD( "scp1vera_spr.ic002", 0x800000, 0x040000, CRC(b7094978) SHA1(1e4903cd5f594c13dad2fd74666ba35c62550044) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "scp1cd0", 0, SHA1(19fa70ba22787704c40f0a8f27bc841218bbc99b) )
ROM_END

ROM_START( superdbz )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM256_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "db1verb.ic002", 0x000000, 0x800000, CRC(ae9aa06d) SHA1(dabb6d797f706bb3523ce4ca77e9ffb1652e845a) )
	ROM_LOAD( "db1verb_spr.ic002", 0x800000, 0x040000, CRC(baae64a1) SHA1(f82c5b1e98255976518f7b78f764e7a7bb3c9017) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "db1", 0, SHA1(5f4031e2beda9c1cd4a5a9a07740fa50946b73f2) )
ROM_END

ROM_START( wanganmd )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "wmn1vera.ic002", 0x000000, 0x800000, CRC(436b08a7) SHA1(b574c25ba2d4a8b497654a7cf6491103130f1317) )
	ROM_LOAD( "wmn1vera_spr.ic002", 0x800000, 0x040000, CRC(97253f9e) SHA1(652807972c62a96ba329b400e17dabd313134392) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "wmn1", 0, SHA1(4254e987e71d0d4038a87f11dc1a304396b3dffc) )
ROM_END

ROM_START( bldyr3b )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "br3-dongle.bin", 0x000000, 0x840000, CRC(abed2289) SHA1(e5220dbfd790b582ff6f828a636190e55d9cbc93) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "bldyr3b", 0, SHA1(1de9b14107a7a37ed31bccba17c1d062f0af5065) )
ROM_END

ROM_START( fateulc )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "fud1vera.ic002", 0x000000, 0x800000, CRC(892ffdd1) SHA1(4a444ed49e5c89dd0af4f026626a164b9eec61d1) )
	ROM_LOAD( "fud1vera_spr.ic002", 0x800000, 0x040000, CRC(0fca4e99) SHA1(0bd74de26f10089ee848f03093229abfa8c84663) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "fud-hdd0-a", 0, SHA1(1189863ae0e339e11708b9660521f86b3b97bc9e) )
ROM_END

ROM_START( fateulcb )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "fates-dongle.bin", 0x000000, 0x840000, CRC(b0f15996) SHA1(8161c61f18700ddaeecd89bf3a7fb685431355e7) )

	DISK_REGION("dvd")  // actually HDD for this game
	DISK_IMAGE_READONLY( "fateulcb", 0, SHA1(073e67a5219ad53292716093b8c35deb20761c04) )
ROM_END

ROM_START( gdvsgd )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM256_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "gdvsgd.ic002", 0x000000, 0x800000, NO_DUMP )
	ROM_LOAD( "gdvsgd_spr.ic002", 0x800000, 0x040000, NO_DUMP )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "gdvsgd", 0, SHA1(3cf9ade5495982fcb8e106e7be4067429530f864) )
ROM_END

ROM_START( gdvsgdnx )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM256_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "gnx1001-na-a.ic002", 0x000000, 0x800000, CRC(1d6d2f54) SHA1(17f6e7278e61b5b81605175d3f2df7b747ca7246) ) 
	ROM_LOAD( "gnx1001-na-a_spr.ic002", 0x800000, 0x040000, CRC(a999ba5c) SHA1(009a56f7be50b57bf435fb8c8b41cf14086b1d1a) ) 

	DISK_REGION("dvd")	// actually HDD for this game
	DISK_IMAGE_READONLY( "gnx100-1na-a", 0, SHA1(a2344f533895793a2e13198c7de0c759f0dbf817) )
ROM_END

ROM_START( yuyuhaku )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM256_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "dongle.bin",   0x000000, 0x840000, CRC(36492878) SHA1(afd14aee033cf360c07d281112566d0463d17a1f) )

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "yuyuhaku", 0, SHA1(ffdf1333d2c235e5fcec3780480f110afd20a7df) )
ROM_END

ROM_START( sbxc )
	ROM_REGION(0x200000, "bios", 0)
	SYSTEM246_BIOS

	ROM_REGION(0x840000, "key", ROMREGION_ERASE00)
	ROM_LOAD( "bax1vera.ic002", 0x000000, 0x800000, CRC(18a6f424) SHA1(027a8d371fb6782c906434b86db9779057eaa954) ) 
	ROM_LOAD( "bax1vera_spr.ic002", 0x800000, 0x040000, CRC(abfb749b) SHA1(b45f8c79dd0cc0359f27c33f55626d6cad82127c) ) 

	DISK_REGION("dvd")
	DISK_IMAGE_READONLY( "bax1_dvd0", 0, SHA1(56d58e66eeaa57ff07668000491360853b064936) )
ROM_END

GAME(2001, sys246,          0, system246, system246, driver_device, 0, ROT0, "Namco", "System 246 BIOS", GAME_IS_SKELETON|GAME_IS_BIOS_ROOT)
GAME(2001, bldyr3b,    sys246, system246, system246, driver_device, 0, ROT0, "bootleg", "Bloody Roar 3 (bootleg)", GAME_IS_SKELETON)
GAME(2001, rrvac,      sys246, system246, system246, driver_device, 0, ROT0, "Namco", "Ridge Racer V Arcade Battle (RRV3 Ver. A)", GAME_IS_SKELETON)
GAME(2001, wanganmd,   sys246, system246, system246, driver_device, 0, ROT0, "Namco", "Wangan Midnight (WMN1 Ver. A)", GAME_IS_SKELETON)
GAME(2002, dragchrn,   sys246, system246, system246, driver_device, 0, ROT0, "Namco", "Dragon Chronicles (DC001 Ver. A)", GAME_IS_SKELETON)
GAME(2002, netchu02,   sys246, system246, system246, driver_device, 0, ROT0, "Namco", "Netchuu Pro Yakyuu 2002 (NPY1 Ver. A)", GAME_IS_SKELETON)
GAME(2002, scptour,    sys246, system246, system246, driver_device, 0, ROT0, "Namco", "Smash Court Pro Tournament (SCP1)", GAME_IS_SKELETON)
GAME(2002, soulclb2,   sys246, system246, system246, driver_device, 0, ROT0, "Namco", "Soul Calibur II (SC23 Ver. A)", GAME_IS_SKELETON)
GAME(2002, soulcl2a, soulclb2, system246, system246, driver_device, 0, ROT0, "Namco", "Soul Calibur II (SC22 Ver. A)", GAME_IS_SKELETON)
GAME(2002, soulcl2b, soulclb2, system246, system246, driver_device, 0, ROT0, "Namco", "Soul Calibur II (SC21 Ver. A)", GAME_IS_SKELETON)
GAME(2002, soulcl2w, soulclb2, system246, system246, driver_device, 0, ROT0, "Namco", "Soul Calibur II (SC2? world version)", GAME_IS_SKELETON)
GAME(2002, tekken4,    sys246, system246, system246, driver_device, 0, ROT0, "Namco", "Tekken 4 (TEF3 Ver. C)", GAME_IS_SKELETON)
GAME(2002, tekken4a,  tekken4, system246, system246, driver_device, 0, ROT0, "Namco", "Tekken 4 (TEF2 Ver. A)", GAME_IS_SKELETON)
GAME(2002, tekken4b,  tekken4, system246, system246, driver_device, 0, ROT0, "Namco", "Tekken 4 (TEF1 Ver. A)", GAME_IS_SKELETON)
GAME(2002, tekken4c,  tekken4, system246, system246, driver_device, 0, ROT0, "Namco", "Tekken 4 (TEF1 Ver. C)", GAME_IS_SKELETON)
GAME(2003, timecrs3,   sys246, system246, system246, driver_device, 0, ROT0, "Namco", "Time Crisis 3 (TST1)", GAME_IS_SKELETON)
GAME(2003, timecrs3e,timecrs3, system246, system246, driver_device, 0, ROT0, "Namco", "Time Crisis 3 (TST2 Ver. A)", GAME_IS_SKELETON)
GAME(2003, zgundm,     sys246, system246, system246, driver_device, 0, ROT0, "Capcom / Banpresto", "Mobile Suit Z-Gundam: A.E.U.G. vs Titans (ZGA1 Ver. A)", GAME_IS_SKELETON)
GAME(2004, fghtjam,    sys246, system246, system246, driver_device, 0, ROT0, "Capcom / Namco", "Capcom Fighting Jam (JAM1 Ver. A)", GAME_IS_SKELETON)
GAME(2004, sukuinuf,   sys246, system246, system246, driver_device, 0, ROT0, "Namco", "Quiz and Variety Suku Suku Inufuku 2 (IN2 Ver. A)", GAME_IS_SKELETON)
GAME(2004, zgundmdx,   sys246, system246, system246, driver_device, 0, ROT0, "Capcom / Banpresto", "Mobile Suit Z-Gundam: A.E.U.G. vs Titans DX (ZDX1 Ver. A)", GAME_IS_SKELETON)
GAME(2005, gundzaft,   sys246, system246, system246, driver_device, 0, ROT0, "Capcom / Banpresto", "Gundam Seed: Federation vs. Z.A.F.T. (SED1 Ver. A)", GAME_IS_SKELETON)
GAME(2005, soulclb3,   sys246, system246, system246, driver_device, 0, ROT0, "Namco", "Soul Calibur III (SC31001-NA-A)", GAME_IS_SKELETON)
GAME(2005, soulclb3a,soulclb3, system246, system246, driver_device, 0, ROT0, "Namco", "Soul Calibur III (SC31002-NA-A)", GAME_IS_SKELETON)
GAME(2008, fateulc,    sys246, system246, system246, driver_device, 0, ROT0, "Capcom / Namco", "Fate: Unlimited Codes (FUD1 ver. A)", GAME_IS_SKELETON)
GAME(2008, fateulcb,  fateulc, system246, system246, driver_device, 0, ROT0, "bootleg", "Fate: Unlimited Codes (bootleg)", GAME_IS_SKELETON)
GAME(2008, sbxc,       sys246, system246, system246, driver_device, 0, ROT0, "Capcom / Arc System Works", "Sengoku Basara X Cross", GAME_IS_SKELETON)

GAME(2004, sys256,          0, system256, system246, driver_device, 0, ROT0, "Namco", "System 256 BIOS", GAME_IS_SKELETON|GAME_IS_BIOS_ROOT)
GAME(2005, tekken51,   sys256, system256, system246, driver_device, 0, ROT0, "Namco", "Tekken 5.1 (TE51 Ver. B)", GAME_IS_SKELETON)
GAME(2005, superdbz,   sys256, system256, system246, driver_device, 0, ROT0, "Banpresto / Spike", "Super Dragon Ball Z (DB1 Ver. B)", GAME_IS_SKELETON)
GAME(2006, kinniku,    sys256, system256, system246, driver_device, 0, ROT0, "Namco", "Kinnikuman Muscle Grand Prix (KN1 Ver. A)", GAME_IS_SKELETON)
GAME(2006, taiko9,     sys256, system256, system246, driver_device, 0, ROT0, "Namco", "Taiko No Tatsujin 9 (TK91001-NA-A)", GAME_IS_SKELETON)
GAME(2006, yuyuhaku,   sys256, system256, system246, driver_device, 0, ROT0, "Banpresto", "The Battle of Yu Yu Hakusho: Shitou! Ankoku Bujutsukai!", GAME_IS_SKELETON)
GAME(2007, kinniku2,   sys256, system256, system246, driver_device, 0, ROT0, "Namco", "Kinnikuman Muscle Grand Prix 2 (KN2 Ver. A)", GAME_IS_SKELETON)
GAME(2007, taiko10,    sys256, system256, system246, driver_device, 0, ROT0, "Namco", "Taiko No Tatsujin 10 (T101001-NA-A)", GAME_IS_SKELETON)
GAME(2008, gdvsgd,     sys256, system256, system246, driver_device, 0, ROT0, "Capcom / Bandai", "Gundam vs. Gundam", GAME_IS_SKELETON)
GAME(2009, gdvsgdnx,   sys256, system256, system246, driver_device, 0, ROT0, "Capcom / Bandai", "Gundam vs. Gundam Next", GAME_IS_SKELETON)
