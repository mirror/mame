/* Sega Hikaru / 'Samurai' */

/*

Sega Hikaru Hardware Overview (last updated 4th July 2008 at 9:06pm)
-----------------------------

Note! This document will be updated from time to time when more dumps are available

This document covers all the known Sega Hikaru games, including....
!Air Trix                    (C) Sega, 2001
*Brave Fire Fighters         (C) Sega, 1999
*Cyber Troopers Virtual On 4 (C) Sega, 2001
*Nascar Racing               (C) Sega, 2000
!Planet Harriers             (C) Sega, 2001
!Star Wars Racer Arcade      (C) Sega, 2000

! - denotes secured but not fully dumped yet
* - denotes not dumped yet. If you can help with the remaining undumped games,
    please contact http://www.mameworld.net/gurudumps/

The Sega Hikaru system comprises the following PCBs.....
Mainboard      - 2 known versions exists. They're mostly the same. It contains many thin BGAs,
                 lots of RAM, 2x SH4 CPUs and 1x 16MBit boot EPROM
ROM PCB        - 2 known versions exist. They contain EPROMs, up to 16x TSOP48 flashROMs or SOP44
                 maskROMs, a small amount of SRAM and some security ICs that are also used on
                 NAOMI ROM carts (315-5881 - with a unique 317-xxxx number per game, a couple
                 of PLCC FPGAs and a X76F100 secured EEPROM). Plugs into mainboard into CONN3 and CONN4
Network PCB    - Contains a 68000, some SRAM, a few Sega ICs and a CPLD. Plugs into the top of the ROM board
Protection PCB - Contains one RAM and one QFP100 IC. Plugs into the mainboard into CONN5
Filter PCB     - Contains external connectors for power, controls, network, USB etc. Plugs into mainboard
                 into CONN1 and CONN2
I/O PCB        - Uses a standard Sega JVS I/O board. Contains USB connectors type A & B, a
                 couple of long IDC cable connectors for controls and a Toshiba TMP90PH44 MCU


Main Board
----------
Version 1 - 837-13402 171-7639E SEGA 1998
Version 2 - 837-14097 171-7639F SEGA 1998
|------------------------------------------------------------------------------------|
|       MC33470                DSW(4)                           CONN4                |
|                                    93C46                                           |
|                          SH-4                             62256 62256    93C46     |-|
|        SH-4                                  PAL1                                  | |
|                        5264165FTTA60         (GAL16V8)                    3771     | |
|                        5264165FTTA60                         315-6146              | |
|    5264165FTTA60       5264165FTTA60                                   14.7456MHz  | |
|    5264165FTTA60       5264165FTTA60                                   32MHz       | |CONN1
|    5264165FTTA60                                                                   | |
|    5264165FTTA60                                             3771 3771             | |
|                         315-6415                                                   | |
|      315-6415                                                               PC910  | |
|               33.3333MHz                                        CY37128            | |
|               CY2308SC                             CY2292SL     (315-6202)         |-|
|HY57V161610                                                      62256      ADM485  |
| PAL2                      CONN3                                 62256              |
| (GAL16V8)                                                                          |
|                                                                                    |
|HY57V161610                       315-6087  HY57V161610                             |-|
|              315-6083A                     HY57V161610              EPR-23400(A)   | |
|HY57V161610              D432232                      24.576MHz                     | |
|(PAL on V2 MB)                              HY57V161610   ADV7120         BATTERY   | |CONN2
|LED1                                        HY57V161610   ADV7120                   | |
|LED2                                                                                | |
|LED3                                                                                |-|
|LED4               315-6197                        315-6084     CONN5               |
|SW1                                315-6085                           K4S641632     |
|SW2                                                                                 |
|SW3    315-6086                                                      315-6232       |
|                                                   315-6084                         |
|                   315-6197        CY2308SC                    33.8688MHz           |
|(LED 1-4 moved here)          41.6666MHz                       25MHz                |
|(on V2 MB)                                                                          |
|------------------------------------------------------------------------------------|


ROM PCB
-------

Type 1 - 837-14140   171-8144B
|------------------------------------------------------------------------------------|
|                                                                                    |
|   IC45   IC41   IC37   IC50   IC46   IC42   IC38                                   |
|                                                                                    |
|                                                                        LATTICE     |
|   IC49   IC57   IC53   IC66   IC62   IC58   IC54                       M4A3-32/32  |
|                                                                        (315-6323)  |
|                                                                  X76F100           |
|   IC61                                                                             |
|                                                                         CY7C1399   |
|                                                                         CY7C1399   |
|   IC65                                                                             |
|           IC29  IC30  IC31  IC32  IC33  IC34  IC35  IC36                           |
|                                                                       28MHz        |
|                                                                            LATTICE |
|                                                                 315-5881   PLSI2032|
|                                                                          (315-6050)|
|------------------------------------------------------------------------------------|
ROM usage -
           Game       Sega Part No.     ROM Type
           ------------------------------------------------
           Air Trix -
                      MPR-23573.IC37    128M TSOP48 MASKROM
                      MPR-23577.IC38    "
                      MPR-23574.IC41    "
                      MPR-23578.IC42    "
                      MPR-23575.IC45    "
                      MPR-23579.IC46    "
                      MPR-23576.IC49    "
                      MPR-23580.IC50    "
                      IC53, IC54, IC57, \
                      IC58, IC61, IC62,  Not Used
                      IC65, IC66        /

                      EPR-23601A.IC29   27C322 EPROM
                      EPR-23602A.IC30   "
                      other EPROM sockets not used

                      315-5881 stamped 317-0294-COM


           Game       Sega Part No.     ROM Type
           ------------------------------------------------
           Planet Harriers -
                      MPR-23549.IC37    128M TSOP48 MASKROM
                      MPR-23553.IC38    "
                      MPR-23550.IC41    "
                      MPR-23554.IC42    "
                      MPR-23551.IC45    "
                      MPR-23555.IC46    "
                      MPR-23552.IC49    "
                      MPR-23556.IC50    "
                      MPR-23557.IC53    "
                      MPR-23558.IC57    "
                      MPR-23559.IC61    "
                      MPR-23560.IC65    "
                      MPR-23564.IC66    "
                      MPR-23563.IC62    "
                      MPR-23562.IC58    "
                      MPR-23561.IC54    "

                      EPR-23565A.IC29   27C322 EPROM
                      EPR-23566A.IC30   "
                      EPR-23567A.IC31   "
                      EPR-23568A.IC32   "
                      EPR-23569A.IC33   "
                      EPR-23570A.IC34   "
                      EPR-23571A.IC35   "
                      EPR-23572A.IC36   "
                      all EPROM sockets populated

                      315-5881 stamped 317-0297-COM


Type 2 - 837-13403   171-7640B
|------------------------------------------------------------------------------------|
|               LATTICE        CY7C199               X76F100                         |
|  315-5881     PLSI2032                                                             |
|                       28MHz  CY7C199               MACH111                         |
|                                                    (315-6203A)         IC30   IC32 |
|                                                                                    |
| IC37  IC39  IC41  IC43  IC45  IC47  IC49  IC51                                     |
|                                                                                    |
|                                                                                    |
| IC53  IC55  IC57  IC59  IC61  IC63  IC65  IC67  <-- these on other side of PCB     |
|                                                                                    |
|                                                                                    |
| IC38  IC40  IC42  IC44  IC46  IC48  IC50  IC52                                     |
|                                                                                    |
|                                                IC29  IC31  IC33  IC35  IC34  IC36  |
| IC54  IC56  IC58  IC60  IC62  IC64  IC66  IC68 <-- these on other side of PCB      |
|                                                                                    |
|------------------------------------------------------------------------------------|
ROM usage -
           Game       Sega Part No.     ROM Type
           ------------------------------------------------
           Star Wars Racer Arcade
                      MPR-23086.IC37    64M SOP44 MASKROM
                      MPR-23087.IC38    "
                      MPR-23088.IC39    "
                      MPR-23089.IC40    "
                      MPR-23090.IC41    "
                      MPR-23091.IC42    "
                      MPR-23092.IC43    "
                      MPR-23093.IC44    "
                      MPR-23094.IC45    "
                      MPR-23095.IC46    "
                      MPR-23096.IC47    "
                      MPR-23097.IC48    "
                      MPR-23098.IC49    "
                      MPR-23099.IC50    "
                      MPR-23100.IC51    "
                      MPR-23101.IC52    "
                      MPR-23102.IC53    "
                      MPR-23103.IC54    "
                      MPR-23104.IC55    "
                      MPR-23105.IC56    "
                      MPR-23106.IC57    "
                      MPR-23107.IC58    "
                      MPR-23108.IC59    "
                      MPR-23109.IC60    "
                      MPR-23110.IC61    "
                      MPR-23111.IC62    "
                      MPR-23112.IC63    "
                      MPR-23113.IC64    "
                      MPR-23114.IC65    "
                      MPR-23115.IC66    "
                      MPR-23116.IC67    "
                      MPR-23117.IC68    "

                      EPR-23174.IC29    27C322 EPROM
                      EPR-23175.IC30  "
                      EPR-23176.IC31  "
                      EPR-23177.IC32  "
                      other EPROM sockets not used

                      315-5881 stamped 317-0277-COM


Protection PCB
--------------

837-13629  171-7911B  SEGA 1998
|------------------|
|    K4S641632     |
|                  |
|                  |
|                  |
|     315-6232     |
|                  |
|                  |
|------------------|


Network PCB
-----------

837-13404  171-7641B
|-----------------------------------|
| 40MHz      LATTICE     315-5917   |
|            PLSI2032               |
|            (315-5958A)    315-5917|
|   PAL                             |
|                          62256    |
|             315-5804     62256    |
|                          62256    |
|                          62256    |
|  68000          PAL               |
|  62256                            |
|  62256          PAL               |
|-----------------------------------|

*/

#include "driver.h"
#include "cpu/sh4/sh4.h"

#define CPU_CLOCK (200000000)
                                 /* MD2 MD1 MD0 MD6 MD4 MD3 MD5 MD7 MD8 */
//static const struct sh4_config sh4cpu_config = {  1,  0,  1,  0,  0,  0,  1,  1,  0, CPU_CLOCK };


VIDEO_START(hikaru)
{

}

VIDEO_UPDATE(hikaru)
{
	return 0;
}

static INPUT_PORTS_START( hikaru )
	PORT_START("IN0")
INPUT_PORTS_END

static ADDRESS_MAP_START( hikaru_map, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x00000000, 0x001fffff) AM_ROM
	AM_RANGE(0x0C000000, 0x0C00ffff) AM_RAM
ADDRESS_MAP_END


static MACHINE_DRIVER_START( hikaru )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", SH4, CPU_CLOCK)
//  MDRV_CPU_CONFIG(sh4cpu_config)
	MDRV_CPU_PROGRAM_MAP(hikaru_map,0)
//  MDRV_CPU_IO_MAP(hikaru_port,0)
//  MDRV_CPU_VBLANK_INT("main", hikaru,vblank)

//  MDRV_MACHINE_START( hikaru )
//  MDRV_MACHINE_RESET( hikaru )

//  MDRV_NVRAM_HANDLER(hikaru_eeproms)

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500) /* not accurate */)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(hikaru)
	MDRV_VIDEO_UPDATE(hikaru)

//  MDRV_SPEAKER_STANDARD_STEREO("left", "right")
//  MDRV_SOUND_ADD("aica", AICA, 0)
//  MDRV_SOUND_CONFIG(aica_config)
//  MDRV_SOUND_ROUTE(0, "left", 2.0)
//  MDRV_SOUND_ROUTE(0, "right", 2.0)
MACHINE_DRIVER_END


#define ROM_LOAD16_WORD_SWAP_BIOS(bios,name,offset,length,hash) \
		ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_BIOS(bios+1)) /* Note '+1' */


#define HIKARU_BIOS \
	ROM_SYSTEM_BIOS( 0, "bios0", "epr23400a" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 0, "epr23400a.ic94",   0x000000, 0x200000, CRC(2aa906a7) SHA1(098c9909b123ed6c338ac874f2ee90e3b2da4c02) ) \
	ROM_SYSTEM_BIOS( 1, "bios1", "epr23400" ) \
	ROM_LOAD16_WORD_SWAP_BIOS( 1, "epr-23400.ic94",   0x000000, 0x200000, CRC(3d557104) SHA1(d39879f5a1acbd54ad8ee4fbd412f870c9ff4aa5) ) \

// bios 0 is SAMURAI boot rom 0.96 / 2000/8/10
// bios 1 is SAMURAI boot rom 0.92 / 1999/7/2


ROM_START( hikaru )
	ROM_REGION( 0x200000, "main", 0)
	HIKARU_BIOS

	ROM_REGION( 0x400000, "user1", ROMREGION_ERASE)
ROM_END


ROM_START( airtrix )
	ROM_REGION( 0x200000, "main", 0)
	HIKARU_BIOS

	ROM_REGION( 0x800000, "user1", 0)
	ROM_LOAD32_WORD("epr23601a.ic29", 0x0000000, 0x0400000, CRC(cd3ccc05) SHA1(49de32d3588511f37486aff900773453739d706d) )
	ROM_LOAD32_WORD("epr23602a.ic30", 0x0000002, 0x0400000, CRC(24f1bca9) SHA1(719dc4e003c1d13fcbb39604c156c89042c47dfd) )
	/* ic31 unpopulated */
	/* ic32 unpopulated */
	/* ic33 unpopulated */
	/* ic34 unpopulated */
	/* ic35 unpopulated */
	/* ic36 unpopulated */

	/* ROM board using 128M TSOP48 MASKROMs */
	ROM_REGION( 0x10000000, "user2", 0)
	ROM_LOAD("mpr-23573.ic37" , 0x0000000, 0x1000000, CRC(000b59d7) SHA1(7a4b90b5821ea8cce77c70628fdb9097e18596e2) ) // NOT verified
	ROM_LOAD("mpr-23577.ic38" , 0x1000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23574.ic41" , 0x2000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23578.ic42" , 0x3000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23575.ic45" , 0x4000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23579.ic46" , 0x5000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23576.ic49" , 0x6000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23580.ic50" , 0x7000000, 0x1000000, NO_DUMP )
	/* ic53 unpopulated */
	/* ic54 unpopulated */
	/* ic57 unpopulated */
	/* ic58 unpopulated */
	/* ic61 unpopulated */
	/* ic62 unpopulated */
	/* ic65 unpopulated */
	/* ic66 unpopulated */
ROM_END


ROM_START( pharrier )
	ROM_REGION( 0x200000, "main", 0)
	HIKARU_BIOS

	ROM_REGION( 0x2000000, "user1", 0)
	ROM_LOAD32_WORD("epr23565a.ic29", 0x0000000, 0x0400000, CRC(ca9af8a7) SHA1(e7d6badc03ec5833ee89e49dd389ee19b45da29c) )
	ROM_LOAD32_WORD("epr23566a.ic30", 0x0000002, 0x0400000, CRC(aad0057c) SHA1(c18c0f1797432c74dc21bcd806cb5760916e4936) )
	ROM_LOAD32_WORD("epr23567.ic31",  0x0800000, 0x0400000, CRC(f0e3dcdc) SHA1(422978a13e39f439da54e43a65dcad1a5b1f2f27) )
	ROM_LOAD32_WORD("epr23568.ic32",  0x0800002, 0x0400000, CRC(6eee734c) SHA1(0941761b1690ad4eeac0bf682459992c6f38a930) )
	ROM_LOAD32_WORD("epr23569.ic33",  0x1000000, 0x0400000, CRC(867c7064) SHA1(5cf0d88a1c739ba69b33f1ba3a0e5544331f63f3) )
	ROM_LOAD32_WORD("epr23570.ic34",  0x1000002, 0x0400000, CRC(556ff58b) SHA1(7eb527aee823d037d1045d850427efa42d5da787) )
	ROM_LOAD32_WORD("epr23571.ic35",  0x1800000, 0x0400000, CRC(5a75fa92) SHA1(b5e0c8c995ecc954b74d5eb36f3ae2a732a5986b) )
	ROM_LOAD32_WORD("epr23572.ic36",  0x1800002, 0x0400000, CRC(46054067) SHA1(449800bdc2c40c76aed9bc5e7e8831d8f03ef286) )

	/* ROM board using 128M TSOP48 MASKROMs */
	ROM_REGION( 0x10000000, "user2", 0)
	ROM_LOAD("mpr-23549.ic37" , 0x0000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23553.ic38" , 0x1000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23550.ic41" , 0x2000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23554.ic42" , 0x3000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23551.ic45" , 0x4000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23555.ic46" , 0x5000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23552.ic49" , 0x6000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23556.ic50" , 0x7000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23557.ic53" , 0x8000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23561.ic54" , 0x9000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23558.ic57" , 0xa000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23562.ic58" , 0xb000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23559.ic61" , 0xc000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23563.ic62" , 0xd000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23560.ic65" , 0xe000000, 0x1000000, NO_DUMP )
	ROM_LOAD("mpr-23564.ic66" , 0xf000000, 0x1000000, NO_DUMP )
ROM_END

ROM_START( podrace )
	ROM_REGION( 0x200000, "main", 0)
	HIKARU_BIOS

	ROM_REGION( 0x2000000, "user1", 0)
	ROM_LOAD32_WORD("epr-23174.ic29", 0x0000000, 0x0400000, CRC(eae62b46) SHA1(f1458072b002d64bbb7c43c582e3191e8031e19a) )
	ROM_LOAD32_WORD("epr-23175.ic30", 0x0000002, 0x0400000, CRC(b92da060) SHA1(dd9ecbd0977aef7629441ff45f4ad807b2408603) )
	ROM_LOAD32_WORD("epr-23176.ic31", 0x0800000, 0x0400000, CRC(2f2824a7) SHA1(a375719e3cababab5b33d00d8696c7cd62c5af30) )
	ROM_LOAD32_WORD("epr-23177.ic32", 0x0800002, 0x0400000, CRC(7a5e3f0f) SHA1(e8ca00cfaaa9be4f9d269e4d8f6bcbbd7de8f6d6) )
	/* ic33 unpopulated */
	/* ic34 unpopulated */
	/* ic35 unpopulated */
	/* ic36 unpopulated */

	/* ROM board using 64M SOP44 MASKROM */
	ROM_REGION( 0x10000000, "user2", 0)
	ROM_LOAD("mpr-23086.ic37" , 0x0000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23087.ic38" , 0x0800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23088.ic39" , 0x1000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23089.ic40" , 0x1800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23090.ic41" , 0x2000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23091.ic42" , 0x2800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23092.ic43" , 0x3000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23093.ic44" , 0x3800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23094.ic45" , 0x4000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23095.ic46" , 0x4800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23096.ic47" , 0x5000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23097.ic48" , 0x5800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23098.ic49" , 0x6000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23099.ic50" , 0x6800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23100.ic51" , 0x7000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23101.ic52" , 0x7800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23102.ic53" , 0x8000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23103.ic54" , 0x8800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23104.ic55" , 0x9000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23105.ic56" , 0x9800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23106.ic57" , 0xa000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23107.ic58" , 0xa800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23108.ic59" , 0xb000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23109.ic60" , 0xb800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23110.ic61" , 0xc000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23111.ic62" , 0xc800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23112.ic63" , 0xd000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23113.ic64" , 0xd800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23114.ic65" , 0xe000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23115.ic66" , 0xe800000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23116.ic67" , 0xf000000, 0x0800000, NO_DUMP )
	ROM_LOAD("mpr-23117.ic68" , 0xf800000, 0x0800000, NO_DUMP )
ROM_END



GAME( 2000, hikaru,   0,        hikaru,   hikaru,   0, ROT0, "Sega",            "Hikaru Bios", GAME_NO_SOUND|GAME_NOT_WORKING|GAME_IS_BIOS_ROOT )
GAME( 2000, airtrix,  hikaru,   hikaru,   hikaru,   0, ROT0, "Sega",            "Air Trix", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, pharrier, hikaru,   hikaru,   hikaru,   0, ROT0, "Sega",            "Planet Harriers", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2001, podrace,  hikaru,   hikaru,   hikaru,   0, ROT0, "Sega",            "Star Wars Pod Racer", GAME_NO_SOUND|GAME_NOT_WORKING )
