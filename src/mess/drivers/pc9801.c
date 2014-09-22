// license:MAME
// copyright-holders:Angelo Salese
/***************************************************************************************************

    PC-9801 (c) 1981 NEC

    driver by Angelo Salese

    TODO:
    - proper 8251 uart hook-up on keyboard
    - SASI /SCSI support;
    - Write a PC80S31K device (also used on PC-8801 and PC-88VA, it's the FDC + Z80 sub-system);
    - Finish DIP-Switches support
    - text scrolling
    - GRCG+
    - EGC
    - rewrite using slot devices
    - some later SWs put "Invalid command byte 05" (Absolutely Mahjong on Epson logo)
    - Basic games are mostly untested, but I think that upd7220 fails on those (Adventureland, Xevious)
    - investigate on POR bit
    - PC-9801RS+ should support uPD4990 RTC

    TODO (PC-9801RS):
    - extra features;
    - keyboard shift doesn't seem to disable properly;
    - clean-up duplicate code;

    TODO (PC-9821):
    - fix CPU for some clones;
    - "cache error"
    - undumped IDE ROM, kludged to work
    - slave PIC never enables floppy IRQ (PC=0xffd08)
    - Compatibility is untested;

    TODO: (PC-486MU)
    - Tries to read port C of i8255_sys (-> 0x35) at boot without setting up the control
      port. This causes a jump to invalid program area;
    - Dies on ARTIC check;
    - Presumably one ROM is undumped?

    floppy issues TODO (certain fail)
    - Unsupported disk types: *.nfd, *.fdd, *.nhd
    - 46 Okunen Monogatari - The Shinkaron
    - AD&D Champions of Krynn
    - AI Shougi (asserts upon loading, 3'5 image?)
    - Aki no Tsukasa no Fushigi no Kabe (works in PC-9801RS only)
    - Aoki Ookami no Shiroki Mejika - Gengis Khan
    - Arcshu
    - Arcus 2
    - Art Jigsaw
    - Atlantia (disk swap?)
    - Azusa 108 Jimusho
    - Bacta 2
    - BattleTech (disk swap?)
    - Bay City Elegy (disk swap?)
    - Beast (keeps reading command sense)
    - Beast 2
    - Bells Avenue (disk swap?)
    - Bible Master 2 (at new game loading)
    - Birdy World

    - Bokosuka Wars
    - Jangou 2: floppy fails to load after the title screen;

    List of per-game TODO:
    - 38 Man Kilo no Kokuu: doesn't seem possible to skip the intro?
    - 4D Boxing: inputs are unresponsive
    - A Ressha de Ikou 2: missing text (PC-9801RS only);
    - Absolutely Mahjong: Transitions are too fast.
    - Agumix Selects!: needs GDC = 5 MHz, interlace doesn't work properly;
    - Alice no Yakata: doesn't set bitmap interlace properly, can't do disk swaps via the File Manager;
    - Anniversary - Memories of Summer: thinks that a button is pressed;
    - Another Genesis: fails loading;
    - Apple Club 1: how to pass an hand?
    - Arctic: keyboard doesn't work?
    - Arcus 3: moans with a JP message "not enough memory (needs 640kb to start)";
    - Armored Flagship Atragon: needs HDD install
    - Arquephos: needs extra sound board(s)?
    - Asoko no Koufuku: black screen with BGM, waits at 0x225f6;
    - Aura Battler Dumbine: upd7220: unimplemented FIGD, has layer clearance bugs on gameplay;
    - Band-Kun: (how to run this without installing?)
    - Battle Chess: wants some dip-switches to be on in DSW4, too slow during IA thinking?
    - Bishoujo Audition: Moans with a "(program) ended. remove the floppy disk and turn off the poewr."
    - Bishoujo Hunter ZX: Doesn't color cycle at intro (seems stuck?), doesn't clear text selection at new game screen;
    - Bishoujo Shanshinkan: has white rectangles all over the place;
    - Bishoujo Tsuushin: hangs with a beep while writing some intro text;

    - Bomber Quest: beeps when speech is supposed to be played (0->1 to i/o port 0xfe8e?)
    - Deflektor: no sound, moans about a DIP-sw during loading, very slow on non-pc9801rs machines;
    - Dragon Buster: slight issue with window masking;
    - Far Side Moon: doesn't detect sound board (tied to 0x00ec ports)
    - Jan Borg Suzume: gets stuck at a pic8259 read;
    - Karateka: no sound?
    - Lovely Horror: Doesn't show kanji, tries to read it thru the 0xa9 port;
    - Madou Monogatari 1/2/3: doesn't display bitmap gfxs during gameplay;
    - Quarth: title screen is broken after first attract play;
    - Princess Maker 2: mouse is buggy;
    - Princess Maker 2: screen transitions are very ugly (btanb?)
    - Puyo Puyo: beeps out when it's supposed to play samples, Not supposed to use ADPCM, is it a PIT issue?
    - Puzznic: trips illegal irq 0x41 (?), prints an error on screen. (PC-9801RS only, writes an 1 to 0x69d2f for whatever reason, almost surely a btanb)
    - Runner's High: wrong double height on the title screen;
    - Sokoban Perfect: hangs at title screen, after loading the menu;
    - Sorcerian, Twilight Zone 3: Fails initial booting, issue with 2dd irq?
    - The Incredible Machine: hangs at main menu (YM mis-fires irq?)
    - Uchiyama Aki no Chou Bangai: keyboard irq is fussy (sometimes it doesn't register a key press);
    - Uno: uses EGC
    - Viper V16 Demo: moans with a JP message;

    per-game TODO (PC-9821):
    - Battle Skin Panic: gfx bugs at the Gainax logo, it crashes after it;
    - Policenauts: EMS error at boot;

    Notes:
    - Animahjong V3 makes advantage of the possibility of installing 2 sound boards, where SFX and BGMs are played on separate chips.
    - Apple Club 1/2 needs data disks to load properly;
    - Beast Lord: needs a titan.fnt, in MS-DOS

========================================================================================

    This series features a huge number of models released between 1982 and 1997. They
    were not IBM PC-compatible, but they had similar hardware (and software: in the
    1990s, they run MS Windows as OS)

    Models:

                      |  CPU                          |   RAM    |            Drives                                     | CBus| Release |
    PC-9801           |  8086 @ 5                     |  128 KB  | -                                                     |  6  | 1982/10 |
    PC-9801F1         |  8086-2 @ 5/8                 |  128 KB  | 5"2DDx1                                               |  4  | 1983/10 |
    PC-9801F2         |  8086-2 @ 5/8                 |  128 KB  | 5"2DDx2                                               |  4  | 1983/10 |
    PC-9801E          |  8086-2 @ 5/8                 |  128 KB  | -                                                     |  6  | 1983/11 |
    PC-9801F3         |  8086-2 @ 5/8                 |  256 KB  | 5"2DDx1, 10M SASI HDD                                 |  2  | 1984/10 |
    PC-9801M2         |  8086-2 @ 5/8                 |  256 KB  | 5"2HDx2                                               |  4  | 1984/11 |
    PC-9801M3         |  8086-2 @ 5/8                 |  256 KB  | 5"2HDx1, 20M SASI HDD                                 |  3  | 1985/02 |
    PC-9801U2         |  V30 @ 8                      |  128 KB  | 3.5"2HDx2                                             |  2  | 1985/05 |
    PC-98XA1          |  80286 @ 8                    |  512 KB  | -                                                     |  6  | 1985/05 |
    PC-98XA2          |  80286 @ 8                    |  512 KB  | 5"2DD/2HDx2                                           |  6  | 1985/05 |
    PC-98XA3          |  80286 @ 8                    |  512 KB  | 5"2DD/2HDx1, 20M SASI HDD                             |  6  | 1985/05 |
    PC-9801VF2        |  V30 @ 8                      |  384 KB  | 5"2DDx2                                               |  4  | 1985/07 |
    PC-9801VM0        |  V30 @ 8/10                   |  384 KB  | -                                                     |  4  | 1985/07 |
    PC-9801VM2        |  V30 @ 8/10                   |  384 KB  | 5"2DD/2HDx2                                           |  4  | 1985/07 |
    PC-9801VM4        |  V30 @ 8/10                   |  384 KB  | 5"2DD/2HDx2, 20M SASI HDD                             |  4  | 1985/10 |
    PC-98XA11         |  80286 @ 8                    |  512 KB  | -                                                     |  6  | 1986/05 |
    PC-98XA21         |  80286 @ 8                    |  512 KB  | 5"2DD/2HDx2                                           |  6  | 1986/05 |
    PC-98XA31         |  80286 @ 8                    |  512 KB  | 5"2DD/2HDx1, 20M SASI HDD                             |  6  | 1986/05 |
    PC-9801UV2        |  V30 @ 8/10                   |  384 KB  | 3.5"2DD/2HDx2                                         |  2  | 1986/05 |
    PC-98LT1          |  V50 @ 8                      |  384 KB  | 3.5"2DD/2HDx1                                         |  0  | 1986/11 |
    PC-98LT2          |  V50 @ 8                      |  384 KB  | 3.5"2DD/2HDx1                                         |  0  | 1986/11 |
    PC-9801VM21       |  V30 @ 8/10                   |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1986/11 |
    PC-9801VX0        |  80286 @ 8 & V30 @ 8/10       |  640 KB  | -                                                     |  4  | 1986/11 |
    PC-9801VX2        |  80286 @ 8 & V30 @ 8/10       |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1986/11 |
    PC-9801VX4        |  80286 @ 8 & V30 @ 8/10       |  640 KB  | 5"2DD/2HDx2, 20M SASI HDD                             |  4  | 1986/11 |
    PC-9801VX4/WN     |  80286 @ 8 & V30 @ 8/10       |  640 KB  | 5"2DD/2HDx2, 20M SASI HDD                             |  4  | 1986/11 |
    PC-98XL1          |  80286 @ 8 & V30 @ 8/10       | 1152 KB  | -                                                     |  4  | 1986/12 |
    PC-98XL2          |  80286 @ 8 & V30 @ 8/10       | 1152 KB  | 5"2DD/2HDx2                                           |  4  | 1986/12 |
    PC-98XL4          |  80286 @ 8 & V30 @ 8/10       | 1152 KB  | 5"2DD/2HDx1, 20M SASI HDD                             |  4  | 1986/12 |
    PC-9801VX01       |  80286-10 @ 8/10 & V30 @ 8/10 |  640 KB  | -                                                     |  4  | 1987/06 |
    PC-9801VX21       |  80286-10 @ 8/10 & V30 @ 8/10 |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1987/06 |
    PC-9801VX41       |  80286-10 @ 8/10 & V30 @ 8/10 |  640 KB  | 5"2DD/2HDx2, 20M SASI HDD                             |  4  | 1987/06 |
    PC-9801UV21       |  V30 @ 8/10                   |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1987/06 |
    PC-98XL^2         |  i386DX-16 @ 16 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1987/10 |
    PC-98LT11         |  V50 @ 8                      |  640 KB  | 3.5"2DD/2HDx1                                         |  0  | 1987/10 |
    PC-98LT21         |  V50 @ 8                      |  640 KB  | 3.5"2DD/2HDx1                                         |  0  | 1987/10 |
    PC-9801UX21       |  80286-10 @ 10 & V30 @ 8      |  640 KB  | 3.5"2DD/2HDx2                                         |  3  | 1987/10 |
    PC-9801UX41       |  80286-10 @ 10 & V30 @ 8      |  640 KB  | 3.5"2DD/2HDx2, 20M SASI HDD                           |  3  | 1987/10 |
    PC-9801LV21       |  V30 @ 8/10                   |  640 KB  | 3.5"2DD/2HDx2                                         |  0  | 1988/03 |
    PC-9801CV21       |  V30 @ 8/10                   |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1988/03 |
    PC-9801UV11       |  V30 @ 8/10                   |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1988/03 |
    PC-9801RA2        |  i386DX-16 @ 16 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1988/07 |
    PC-9801RA5        |  i386DX-16 @ 16 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1988/07 |
    PC-9801RX2        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1988/07 |
    PC-9801RX4        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2, 20M SASI HDD                             |  4  | 1988/07 |
    PC-98LT22         |  V50 @ 8                      |  640 KB  | 3.5"2DD/2HDx1                                         |  0  | 1988/11 |
    PC-98LS2          |  i386SX-16 @ 16 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2                                           |  0  | 1988/11 |
    PC-98LS5          |  i386SX-16 @ 16 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2, 40M SASI HDD                             |  0  | 1988/11 |
    PC-9801VM11       |  V30 @ 8/10                   |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1988/11 |
    PC-9801LV22       |  V30 @ 8/10                   |  640 KB  | 3.5"2DD/2HDx2                                         |  0  | 1989/01 |
    PC-98RL2          |  i386DX-20 @ 16/20 & V30 @ 8  |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1989/02 |
    PC-98RL5          |  i386DX-20 @ 16/20 & V30 @ 8  |  1.6 MB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1989/02 |
    PC-9801EX2        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2                                         |  3  | 1989/04 |
    PC-9801EX4        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 20M SASI HDD                           |  3  | 1989/04 |
    PC-9801ES2        |  i386SX-16 @ 16 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1989/04 |
    PC-9801ES5        |  i386SX-16 @ 16 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  3  | 1989/04 |
    PC-9801LX2        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2                                         |  0  | 1989/04 |
    PC-9801LX4        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 20M SASI HDD                           |  0  | 1989/04 |
    PC-9801LX5        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  0  | 1989/06 |
    PC-98DO           |  V30 @ 8/10                   |  640 KB  | 5"2DD/2HDx2                                           |  1  | 1989/06 |
    PC-9801LX5C       |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  0  | 1989/06 |
    PC-9801RX21       |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1989/10 |
    PC-9801RX51       |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1989/10 |
    PC-9801RA21       |  i386DX-20 @ 16/20 & V30 @ 8  |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1989/11 |
    PC-9801RA51       |  i386DX-20 @ 16/20 & V30 @ 8  |  1.6 MB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1989/11 |
    PC-9801RS21       |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1989/11 |
    PC-9801RS51       |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1989/11 |
    PC-9801N          |  V30 @ 10                     |  640 KB  | 3.5"2DD/2HDx1                                         |  0  | 1989/11 |
    PC-9801TW2        |  i386SX-20 @ 20 & V30 @ 8     |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1990/02 |
    PC-9801TW5        |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1990/02 |
    PC-9801TS5        |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1990/06 |
    PC-9801NS         |  i386SX-12 @ 12               |  1.6 MB  | 3.5"2DD/2HDx1                                         |  0  | 1990/06 |
    PC-9801TF5        |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1990/07 |
    PC-9801NS-20      |  i386SX-12 @ 12               |  1.6 MB  | 3.5"2DD/2HDx1, 20M SASI HDD                           |  0  | 1990/09 |
    PC-98RL21         |  i386DX-20 @ 20 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1990/09 |
    PC-98RL51         |  i386DX-20 @ 20 & V30 @ 8     |  1.6 MB  | 5"2DD/2HDx1, 40M SASI HDD                             |  4  | 1990/09 |
    PC-98DO+          |  V33A @ 8/16                  |  640 KB  | 5"2DD/2HDx2                                           |  1  | 1990/10 |
    PC-9801DX2        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1990/11 |
    PC-9801DX/U2      |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2                                         |  4  | 1990/11 |
    PC-9801DX5        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1990/11 |
    PC-9801DX/U5      |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  4  | 1990/11 |
    PC-9801NV         |  V30HL @ 8/16                 |  1.6 MB  | 3.5"2DD/2HDx1                                         |  0  | 1990/11 |
    PC-9801DS2        |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 5"2DD/2HDx2                                           |  4  | 1991/01 |
    PC-9801DS/U2      |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 3.5"2DD/2HDx2                                         |  4  | 1991/01 |
    PC-9801DS5        |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1991/01 |
    PC-9801DS/U5      |  i386SX-16 @ 16 & V30 @ 8     |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  4  | 1991/01 |
    PC-9801DA2        |  i386DX-20 @ 16/20 & V30 @ 8  |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1991/01 |
    PC-9801DA/U2      |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2                                         |  4  | 1991/01 |
    PC-9801DA5        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2, 40M SASI HDD                             |  4  | 1991/01 |
    PC-9801DA/U5      |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  4  | 1991/01 |
    PC-9801DA7        |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 5"2DD/2HDx2, 100M SCSI HDD                            |  4  | 1991/02 |
    PC-9801DA/U7      |  80286-12 @ 10/12 & V30 @ 8   |  640 KB  | 3.5"2DD/2HDx2, 100M SCSI HDD                          |  4  | 1991/02 |
    PC-9801UF         |  V30 @ 8/16                   |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1991/02 |
    PC-9801UR         |  V30 @ 8/16                   |  640 KB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  2  | 1991/02 |
    PC-9801UR/20      |  V30 @ 8/16                   |  640 KB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 20M SASI HDD          |  2  | 1991/02 |
    PC-9801NS/E       |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1991/06 |
    PC-9801NS/E20     |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 20M SASI HDD          |  0  | 1991/06 |
    PC-9801NS/E40     |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1991/06 |
    PC-9801TW7        |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 100M SCSI HDD                          |  2  | 1991/07 |
    PC-9801TF51       |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1991/07 |
    PC-9801TF71       |  i386SX-20 @ 20 & V30 @ 8     |  1.6 MB  | 3.5"2DD/2HDx2, 100M SCSI HDD                          |  2  | 1991/07 |
    PC-9801NC         |  i386SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1991/10 |
    PC-9801NC40       |  i386SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1991/10 |
    PC-9801CS2        |  i386SX-16 @ 16               |  640 KB  | 3.5"2DD/2HDx2                                         |  2  | 1991/10 |
    PC-9801CS5        |  i386SX-16 @ 16               |  640 KB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1991/10 |
    PC-9801CS5/W      |  i386SX-16 @ 16               |  3.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1991/11 |
    PC-98GS1          |  i386SX-20 @ 20 & V30 @ 8     |  2.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  3  | 1991/11 |
    PC-98GS2          |  i386SX-20 @ 20 & V30 @ 8     |  2.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD, 1xCD-ROM                 |  3  | 1991/11 |
    PC-9801FA2        |  i486SX-16 @ 16               |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1992/01 |
    PC-9801FA/U2      |  i486SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2                                         |  4  | 1992/01 |
    PC-9801FA5        |  i486SX-16 @ 16               |  1.6 MB  | 5"2DD/2HDx2, 40M SCSI HDD                             |  4  | 1992/01 |
    PC-9801FA/U5      |  i486SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2, 40M SCSI HDD                           |  4  | 1992/01 |
    PC-9801FA7        |  i486SX-16 @ 16               |  1.6 MB  | 5"2DD/2HDx2, 100M SCSI HDD                            |  4  | 1992/01 |
    PC-9801FA/U7      |  i486SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2, 100M SCSI HDD                          |  4  | 1992/01 |
    PC-9801FS2        |  i386SX-20 @ 16/20            |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1992/05 |
    PC-9801FS/U2      |  i386SX-20 @ 16/20            |  1.6 MB  | 3.5"2DD/2HDx2                                         |  4  | 1992/05 |
    PC-9801FS5        |  i386SX-20 @ 16/20            |  1.6 MB  | 5"2DD/2HDx2, 40M SCSI HDD                             |  4  | 1992/05 |
    PC-9801FS/U5      |  i386SX-20 @ 16/20            |  1.6 MB  | 3.5"2DD/2HDx2, 40M SCSI HDD                           |  4  | 1992/05 |
    PC-9801FS7        |  i386SX-20 @ 16/20            |  1.6 MB  | 5"2DD/2HDx2, 100M SCSI HDD                            |  4  | 1992/01 |
    PC-9801FS/U7      |  i386SX-20 @ 16/20            |  1.6 MB  | 3.5"2DD/2HDx2, 100M SCSI HDD                          |  4  | 1992/01 |
    PC-9801NS/T       |  i386SL(98) @ 20              |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1992/01 |
    PC-9801NS/T40     |  i386SL(98) @ 20              |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1992/01 |
    PC-9801NS/T80     |  i386SL(98) @ 20              |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 80M SASI HDD          |  0  | 1992/01 |
    PC-9801NL         |  V30H @ 8/16                  |  640 KB  | 1.25 MB RAM Disk                                      |  0  | 1992/01 |
    PC-9801FX2        |  i386SX-12 @ 10/12            |  1.6 MB  | 5"2DD/2HDx2                                           |  4  | 1992/05 |
    PC-9801FX/U2      |  i386SX-12 @ 10/12            |  1.6 MB  | 3.5"2DD/2HDx2                                         |  4  | 1992/05 |
    PC-9801FX5        |  i386SX-12 @ 10/12            |  1.6 MB  | 5"2DD/2HDx2, 40M SCSI HDD                             |  4  | 1992/05 |
    PC-9801FX/U5      |  i386SX-12 @ 10/12            |  1.6 MB  | 3.5"2DD/2HDx2, 40M SCSI HDD                           |  4  | 1992/05 |
    PC-9801US         |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2                                         |  2  | 1992/07 |
    PC-9801US40       |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2, 40M SASI HDD                           |  2  | 1992/07 |
    PC-9801US80       |  i386SX-16 @ 16               |  1.6 MB  | 3.5"2DD/2HDx2, 80M SASI HDD                           |  2  | 1992/07 |
    PC-9801NS/L       |  i386SX-20 @ 10/20            |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1992/07 |
    PC-9801NS/L40     |  i386SX-20 @ 10/20            |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1992/07 |
    PC-9801NA         |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1992/11 |
    PC-9801NA40       |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1992/11 |
    PC-9801NA120      |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 120M SASI HDD         |  0  | 1992/11 |
    PC-9801NA/C       |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1992/11 |
    PC-9801NA40/C     |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 40M SASI HDD          |  0  | 1992/11 |
    PC-9801NA120/C    |  i486SX-20 @ 20               |  2.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk, 120M SASI HDD         |  0  | 1992/11 |
    PC-9801NS/R       |  i486SX(J) @ 20               |  1.6 MB  | 3.5"2DD/2HDx1 (3mode), 1.25MB RAM Disk                |  0  | 1993/01 |
    PC-9801NS/R40     |  i486SX(J) @ 20               |  1.6 MB  | 3.5"2DD/2HDx1 (3mode), 1.25MB RAM Disk, 40M SASI HDD  |  0  | 1993/01 |
    PC-9801NS/R120    |  i486SX(J) @ 20               |  1.6 MB  | 3.5"2DD/2HDx1 (3mode), 1.25MB RAM Disk, 120M SASI HDD |  0  | 1993/01 |
    PC-9801BA/U2      |  i486DX2-40 @ 40              |  1.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1993/01 |
    PC-9801BA/U6      |  i486DX2-40 @ 40              |  3.6 MB  | 3.5"2DD/2HDx1, 40M SASI HDD                           |  3  | 1993/01 |
    PC-9801BA/M2      |  i486DX2-40 @ 40              |  1.6 MB  | 5"2DD/2HDx2                                           |  3  | 1993/01 |
    PC-9801BX/U2      |  i486SX-20 @ 20               |  1.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1993/01 |
    PC-9801BX/U6      |  i486SX-20 @ 20               |  3.6 MB  | 3.5"2DD/2HDx1, 40M SASI HDD                           |  3  | 1993/01 |
    PC-9801BX/M2      |  i486SX-20 @ 20               |  1.6 MB  | 5"2DD/2HDx2                                           |  3  | 1993/01 |
    PC-9801NX/C       |  i486SX(J) @ 20               |  1.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1993/07 |
    PC-9801NX/C120    |  i486SX(J) @ 20               |  3.6 MB  | 3.5"2DD/2HDx1, 1.25MB RAM Disk                        |  0  | 1993/07 |
    PC-9801P40/D      |  i486SX(J) @ 20               |  5.6 MB  | 40MB IDE HDD                                          |  0  | 1993/07 |
    PC-9801P80/W      |  i486SX(J) @ 20               |  7.6 MB  | 80MB IDE HDD                                          |  0  | 1993/07 |
    PC-9801P80/P      |  i486SX(J) @ 20               |  7.6 MB  | 80MB IDE HDD                                          |  0  | 1993/07 |
    PC-9801BA2/U2     |  i486DX2-66 @ 66              |  3.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1993/11 |
    PC-9801BA2/U7     |  i486DX2-66 @ 66              |  3.6 MB  | 3.5"2DD/2HDx1, 120MB IDE HDD                          |  3  | 1993/11 |
    PC-9801BA2/M2     |  i486DX2-66 @ 66              |  3.6 MB  | 5"2DD/2HDx2                                           |  3  | 1993/11 |
    PC-9801BS2/U2     |  i486SX-33 @ 33               |  3.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1993/11 |
    PC-9801BS2/U7     |  i486SX-33 @ 33               |  3.6 MB  | 3.5"2DD/2HDx1, 120MB IDE HDD                          |  3  | 1993/11 |
    PC-9801BS2/M2     |  i486SX-33 @ 33               |  3.6 MB  | 5"2DD/2HDx2                                           |  3  | 1993/11 |
    PC-9801BX2/U2     |  i486SX-25 @ 25               |  1.8 MB  | 3.5"2DD/2HDx2                                         |  3  | 1993/11 |
    PC-9801BX2/U7     |  i486SX-25 @ 25               |  3.6 MB  | 3.5"2DD/2HDx1, 120MB IDE HDD                          |  3  | 1993/11 |
    PC-9801BX2/M2     |  i486SX-25 @ 25               |  1.8 MB  | 5"2DD/2HDx2                                           |  3  | 1993/11 |
    PC-9801BA3/U2     |  i486DX-66 @ 66               |  3.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1995/01 |
    PC-9801BA3/U2/W   |  i486DX-66 @ 66               |  7.6 MB  | 3.5"2DD/2HDx2, 210MB IDE HDD                          |  3  | 1995/01 |
    PC-9801BX3/U2     |  i486SX-33 @ 33               |  1.6 MB  | 3.5"2DD/2HDx2                                         |  3  | 1995/01 |
    PC-9801BX3/U2/W   |  i486SX-33 @ 33               |  5.6 MB  | 3.5"2DD/2HDx2, 210MB IDE HDD                          |  3  | 1995/01 |
    PC-9801BX4/U2     |  AMD/i 486DX2-66 @ 66         |  2 MB    | 3.5"2DD/2HDx2                                         |  3  | 1995/07 |
    PC-9801BX4/U2/C   |  AMD/i 486DX2-66 @ 66         |  2 MB    | 3.5"2DD/2HDx2, 2xCD-ROM                               |  3  | 1995/07 |
    PC-9801BX4/U2-P   |  Pentium ODP @ 66             |  2 MB    | 3.5"2DD/2HDx2                                         |  3  | 1995/09 |
    PC-9801BX4/U2/C-P |  Pentium ODP @ 66             |  2 MB    | 3.5"2DD/2HDx2, 2xCD-ROM                               |  3  | 1995/09 |

    For more info (e.g. optional hardware), see http://www.geocities.jp/retro_zzz/machines/nec/9801/mdl98cpu.html


    PC-9821 Series

    PC-9821 (1992) - aka 98MULTi, desktop computer, 386 based
    PC-9821A series (1993->1994) - aka 98MATE A, desktop computers, 486 based
    PC-9821B series (1993) - aka 98MATE B, desktop computers, 486 based
    PC-9821C series (1993->1996) - aka 98MULTi CanBe, desktop & tower computers, various CPU
    PC-9821Es (1994) - aka 98FINE, desktop computer with integrated LCD, successor of the PC-98T
    PC-9821X series (1994->1995) - aka 98MATE X, desktop computers, Pentium based
    PC-9821V series (1995) - aka 98MATE Valuestar, desktop computers, Pentium based
    PC-9821S series (1995->2996) - aka 98Pro, tower computers, PentiumPro based
    PC-9821R series (1996->2000) - aka 98MATE R, desktop & tower & server computers, various CPU
    PC-9821C200 (1997) - aka CEREB, desktop computer, Pentium MMX based
    PC-9821 Ne/Ns/Np/Nm (1993->1995) - aka 98NOTE, laptops, 486 based
    PC-9821 Na/Nb/Nw (1995->1997) - aka 98NOTE Lavie, laptops, Pentium based
    PC-9821 Lt/Ld (1995) - aka 98NOTE Light, laptops, 486 based
    PC-9821 La/Ls (1995->1997) - aka 98NOTE Aile, laptops, Pentium based

====

Documentation notes (for unemulated stuff, courtesy of T. Kodaka and T. Kono):

IDE:
(r/w)
0x430: IDE drive switch
0x432: IDE drive switch
0x435: <unknown>

                                                    (ISA correlated i/o)
----------------------------------------------------------
0x0640      |WORD|R/W|Data Register                |01F0h
0x0642      |BYTE| R |Error Register               |01F1h
0x0642      |BYTE| W |Write Precomp Register       |01F1h
0x0644      |BYTE|R/W|Sector Count                 |01F2h
0x0646      |BYTE|R/W|Sector Number                |01F3h
0x0648      |BYTE|R/W|Cylinder Low                 |01F4h
0x064A      |BYTE|R/W|Cylinder High                |01F5h
0x064C      |BYTE|R/W|SDH Register                 |01F6h
0x064E      |BYTE| R |Status Register              |01F7h
0x064E      |BYTE| W |Command Register             |01F7h
0x074C      |BYTE| R |Alternate Status Register    |03F6h
0x074C      |BYTE| W |Digital Output Register      |03F6h
0x074E      |BYTE| R |Digital Input Register       |03F7h

Video F/F (i/o 0x68):
KAC mode (video ff = 5) is basically how the kanji ROM could be accessed, 1=thru the CG window ports, 0=thru the kanji
window RAM at 0xa4***.
My guess is that the system locks up or doesn't have any data if the wrong port is being accessed.

Ext Video F/F (i/o 0x6a):
0000 011x enables EGC
0000 111x enables PC-98GS
0010 000x enables multicolor (a.k.a. 256 colors mode)
0010 001x enables 65'536 colors
0010 010x 64k color palette related (?)
0010 011x full screen reverse (?)
0010 100x text and gfxs synthesis (?)
0010 101x 256 color palette registers fast write (?)
0010 110x 256 color overscan (?)
0100 000x (0) CRT (1) Plasma/LCD
0100 001x text and gfxs right shifted one dot (undocumented behaviour)
0100 010x hi-res mode in PC-9821
0110 000x EEGC mode
0110 001x VRAM config (0) plain (1) packed
0110 011x AGDC mode
0110 100x 480 lines
0110 110x VRAM bitmap orientation (0) MSB left-to-right LSB (1) LSB left-to-right MSB
1000 001x CHR GDC clock (0) 2,5 MHz (1) 5 MHz
1000 010x BMP GDC clock
1000 111x related to GFX accelerator cards (like Vision864)
1100 010x chart GDC operating mode (?)
(everything else is undocumented / unknown)

Keyboard TX commands:
0xfa ACK
0xfc NACK
0x95
---- --xx extension key settings (00 normal 11 Win and App Keys enabled)
0x96 identification codes
0x9c
-xx- ---- key delay (11 = 1000 ms, 10 = 500 ms, 01 = 500 ms, 00 = 250 ms)
---x xxxx repeat rate (slow 11111 -> 00001 fast)
0x9d keyboard LED settings
0x9f keyboard ID

****************************************************************************************************/

#include "emu.h"

#include "cpu/i86/i86.h"
#include "cpu/nec/nec.h"
#include "cpu/i86/i286.h"
#include "cpu/i386/i386.h"

#include "machine/i8255.h"
#include "machine/pit8253.h"
#include "machine/am9517a.h"
#include "machine/pic8259.h"
#include "machine/upd765.h"
#include "machine/upd1990a.h"
#include "machine/i8251.h"

#include "bus/scsi/s1410.h"
#include "bus/scsi/scsi.h"
#include "bus/scsi/scsihd.h"
#include "machine/buffer.h"
#include "machine/latch.h"

#include "sound/beep.h"
#include "sound/speaker.h"
#include "sound/2608intf.h"

#include "video/upd7220.h"

#include "machine/ram.h"

#include "formats/pc98_dsk.h"
#include "formats/pc98fdi_dsk.h"

#include "machine/pc9801_26.h"
#include "machine/pc9801_86.h"
#include "machine/pc9801_118.h"
#include "machine/pc9801_cbus.h"
#include "machine/pc9801_kbd.h"

#include "machine/idectrl.h"


#define UPD1990A_TAG "upd1990a"
#define UPD8251_TAG  "upd8251"
#define SASIBUS_TAG  "sasi"

class pc9801_state : public driver_device
{
public:
	pc9801_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_dmac(*this, "i8237"),
		m_pit8253(*this, "pit8253"),
		m_pic1(*this, "pic8259_master"),
		m_pic2(*this, "pic8259_slave"),
		m_fdc_2hd(*this, "upd765_2hd"),
		m_fdc_2dd(*this, "upd765_2dd"),
		m_rtc(*this, UPD1990A_TAG),
		m_keyb(*this, "keyb"),
		m_sio(*this, UPD8251_TAG),
		m_hgdc1(*this, "upd7220_chr"),
		m_hgdc2(*this, "upd7220_btm"),
		m_sasibus(*this, SASIBUS_TAG),
		m_sasi_data_out(*this, "sasi_data_out"),
		m_sasi_data_in(*this, "sasi_data_in"),
		m_sasi_ctrl_in(*this, "sasi_ctrl_in"),
		m_ide(*this, "ide"),
		m_video_ram_1(*this, "video_ram_1"),
		m_video_ram_2(*this, "video_ram_2"),
		m_beeper(*this, "beeper"),
		m_ram(*this, RAM_TAG),
		m_gfxdecode(*this, "gfxdecode"),
		m_palette(*this, "palette")
	{
	}

	required_device<cpu_device> m_maincpu;
	required_device<am9517a_device> m_dmac;
	required_device<pit8253_device> m_pit8253;
	required_device<pic8259_device> m_pic1;
	required_device<pic8259_device> m_pic2;
	required_device<upd765a_device> m_fdc_2hd;
	optional_device<upd765a_device> m_fdc_2dd;
	required_device<upd1990a_device> m_rtc;
	required_device<pc9801_kbd_device> m_keyb;
	required_device<i8251_device> m_sio;
	required_device<upd7220_device> m_hgdc1;
	required_device<upd7220_device> m_hgdc2;
	optional_device<SCSI_PORT_DEVICE> m_sasibus;
	optional_device<output_latch_device> m_sasi_data_out;
	optional_device<input_buffer_device> m_sasi_data_in;
	optional_device<input_buffer_device> m_sasi_ctrl_in;
	optional_device<ata_interface_device> m_ide;
	required_shared_ptr<UINT8> m_video_ram_1;
	required_shared_ptr<UINT8> m_video_ram_2;
	required_device<beep_device> m_beeper;
	optional_device<ram_device> m_ram;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<palette_device> m_palette;

	virtual void video_start();
	UINT32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	UINT8 *m_ipl_rom;
	UINT8 *m_sound_bios;
	UINT8 *m_work_ram;
	UINT8 *m_ext_work_ram;
	UINT8 *m_char_rom;
	UINT8 *m_kanji_rom;
	UINT8 *m_ide_rom;
	UINT8 m_ide_bank[2];

	UINT8 m_portb_tmp;
	UINT8 m_dma_offset[4];
	int m_dack;

	UINT8 m_vrtc_irq_mask;
	UINT8 m_video_ff[8],m_gfx_ff;
	UINT8 m_txt_scroll_reg[8];
	UINT8 m_pal_clut[4];

	UINT8 *m_tvram;

	UINT16 m_font_addr;
	UINT8 m_font_line;
	UINT16 m_font_lr;

	UINT8 m_keyb_press;

	UINT8 m_fdc_2dd_ctrl,m_fdc_2hd_ctrl;
	UINT8 m_nmi_ff;

	UINT8 m_vram_bank;
	UINT8 m_vram_disp;

	/* PC9801RS specific */
	UINT8 m_gate_a20; //A20 line
	UINT8 m_nmi_enable;
	UINT8 m_access_ctrl; // DMA related
	UINT8 m_rom_bank;
	UINT8 m_fdc_ctrl;
	UINT32 m_ram_size;
	UINT8 m_ex_video_ff[128];
	struct {
		UINT8 pal_entry;
		UINT8 r[16],g[16],b[16];
	}m_analog16;
	struct {
		UINT8 pal_entry;
		UINT8 r[0x100],g[0x100],b[0x100];
	}m_analog256;
	struct {
		UINT8 mode;
		UINT8 tile[4], tile_index;
	}m_grcg;

	/* PC9821 specific */
	UINT8 m_sdip[24], m_sdip_bank;
	UINT8 *m_ide_ram;
	UINT8 *m_unk_rom;
	UINT8 *m_ext_gvram;
	UINT8 m_pc9821_window_bank;
	UINT8 m_joy_sel;
	UINT8 m_ext2_ff;
	UINT8 m_sys_type;

	DECLARE_WRITE_LINE_MEMBER( keyboard_irq );
	DECLARE_WRITE_LINE_MEMBER( write_uart_clock );
	DECLARE_READ8_MEMBER(pc9801_xx_r);
	DECLARE_WRITE8_MEMBER(pc9801_xx_w);
	DECLARE_READ8_MEMBER(pc9801_00_r);
	DECLARE_WRITE8_MEMBER(pc9801_00_w);
	DECLARE_READ8_MEMBER(pc9801_20_r);
	DECLARE_WRITE8_MEMBER(pc9801_20_w);
	DECLARE_READ8_MEMBER(pc9801_30_r);
	DECLARE_WRITE8_MEMBER(pc9801_30_w);
	DECLARE_READ8_MEMBER(pc9801_40_r);
	DECLARE_WRITE8_MEMBER(pc9801_40_w);
	DECLARE_READ8_MEMBER(pc9801_50_r);
	DECLARE_WRITE8_MEMBER(pc9801_50_w);
	DECLARE_READ8_MEMBER(pc9801_60_r);
	DECLARE_WRITE8_MEMBER(pc9801_60_w);
	DECLARE_WRITE8_MEMBER(pc9801_vrtc_mask_w);
	DECLARE_WRITE8_MEMBER(pc9801_video_ff_w);
	DECLARE_READ8_MEMBER(pc9801_70_r);
	DECLARE_WRITE8_MEMBER(pc9801_70_w);
	DECLARE_READ8_MEMBER(pc9801rs_70_r);
	DECLARE_WRITE8_MEMBER(pc9801rs_70_w);
	DECLARE_READ8_MEMBER(pc9801_sasi_r);
	DECLARE_WRITE8_MEMBER(pc9801_sasi_w);
	DECLARE_READ8_MEMBER(pc9801_a0_r);
	DECLARE_WRITE8_MEMBER(pc9801_a0_w);
	DECLARE_READ8_MEMBER(pc9801_fdc_2hd_r);
	DECLARE_WRITE8_MEMBER(pc9801_fdc_2hd_w);
	DECLARE_READ8_MEMBER(pc9801_fdc_2dd_r);
	DECLARE_WRITE8_MEMBER(pc9801_fdc_2dd_w);
	DECLARE_READ8_MEMBER(pc9801_tvram_r);
	DECLARE_WRITE8_MEMBER(pc9801_tvram_w);
	DECLARE_READ8_MEMBER(pc9801_gvram_r);
	DECLARE_WRITE8_MEMBER(pc9801_gvram_w);
	DECLARE_READ8_MEMBER(pc9801_mouse_r);
	DECLARE_WRITE8_MEMBER(pc9801_mouse_w);
	DECLARE_WRITE8_MEMBER(pc9801rs_mouse_freq_w);
	inline UINT8 m_pc9801rs_grcg_r(UINT32 offset,int vbank);
	inline void m_pc9801rs_grcg_w(UINT32 offset,int vbank,UINT8 data);
	DECLARE_CUSTOM_INPUT_MEMBER(system_type_r);
	DECLARE_READ8_MEMBER(pc9801ux_gvram_r);
	DECLARE_WRITE8_MEMBER(pc9801ux_gvram_w);
	DECLARE_READ8_MEMBER(pc9801ux_gvram0_r);
	DECLARE_WRITE8_MEMBER(pc9801ux_gvram0_w);
	UINT32 pc9801_286_a20(bool state);

	DECLARE_WRITE8_MEMBER(sasi_data_w);
	DECLARE_WRITE_LINE_MEMBER(write_sasi_io);
	DECLARE_READ8_MEMBER(sasi_status_r);
	DECLARE_WRITE8_MEMBER(sasi_ctrl_w);

	UINT8 m_sasi_data;
	int m_sasi_data_enable;
	UINT8 m_sasi_ctrl;

	DECLARE_READ8_MEMBER(pc9801rs_wram_r);
	DECLARE_WRITE8_MEMBER(pc9801rs_wram_w);
	DECLARE_READ8_MEMBER(pc9801rs_ex_wram_r);
	DECLARE_WRITE8_MEMBER(pc9801rs_ex_wram_w);
	DECLARE_READ8_MEMBER(pc9801rs_ipl_r);
	DECLARE_READ8_MEMBER(pc9801rs_knjram_r);
	DECLARE_WRITE8_MEMBER(pc9801rs_knjram_w);
	DECLARE_WRITE8_MEMBER(pc9801rs_bank_w);
	DECLARE_READ8_MEMBER(pc9801rs_f0_r);
	DECLARE_WRITE8_MEMBER(pc9801rs_f0_w);
	DECLARE_READ8_MEMBER(pc9801rs_30_r);
	DECLARE_READ8_MEMBER(pc9801rs_memory_r);
	DECLARE_WRITE8_MEMBER(pc9801rs_memory_w);
	DECLARE_READ8_MEMBER(pc9801rs_soundrom_r);
	DECLARE_READ8_MEMBER(pc9801rs_scsirom_r);
	DECLARE_READ8_MEMBER(pc9810rs_fdc_ctrl_r);
	DECLARE_WRITE8_MEMBER(pc9810rs_fdc_ctrl_w);
	DECLARE_READ8_MEMBER(pc9801rs_2hd_r);
	DECLARE_WRITE8_MEMBER(pc9801rs_2hd_w);
//  DECLARE_READ8_MEMBER(pc9801rs_2dd_r);
//  DECLARE_WRITE8_MEMBER(pc9801rs_2dd_w);
	DECLARE_WRITE8_MEMBER(pc9801rs_video_ff_w);
	DECLARE_WRITE8_MEMBER(pc9801rs_a0_w);
	DECLARE_WRITE8_MEMBER(pc9821_video_ff_w);
	DECLARE_READ8_MEMBER(pc9821_a0_r);
	DECLARE_WRITE8_MEMBER(pc9821_a0_w);
	DECLARE_READ8_MEMBER(pc9801rs_pit_mirror_r);
	DECLARE_WRITE8_MEMBER(pc9801rs_pit_mirror_w);
	DECLARE_READ8_MEMBER(pc9801rs_access_ctrl_r);
	DECLARE_WRITE8_MEMBER(pc9801rs_access_ctrl_w);
	DECLARE_WRITE8_MEMBER(pc9801rs_nmi_w);
	DECLARE_READ8_MEMBER(pc9801rs_midi_r);
	DECLARE_READ8_MEMBER(pc9821_memory_r);
	DECLARE_WRITE8_MEMBER(pc9821_memory_w);
//  DECLARE_READ8_MEMBER(pc9801_ext_opna_r);
//  DECLARE_WRITE8_MEMBER(pc9801_ext_opna_w);
	DECLARE_READ8_MEMBER(pic_r);
	DECLARE_WRITE8_MEMBER(pic_w);

	DECLARE_READ8_MEMBER(pc9801rs_ide_io_0_r);
	DECLARE_READ16_MEMBER(pc9801rs_ide_io_1_r);
	DECLARE_READ16_MEMBER(pc9801rs_ide_io_2_r);
	DECLARE_WRITE8_MEMBER(pc9801rs_ide_io_0_w);
	DECLARE_WRITE16_MEMBER(pc9801rs_ide_io_1_w);
	DECLARE_WRITE16_MEMBER(pc9801rs_ide_io_2_w);

	DECLARE_READ8_MEMBER(sdip_0_r);
	DECLARE_READ8_MEMBER(sdip_1_r);
	DECLARE_READ8_MEMBER(sdip_2_r);
	DECLARE_READ8_MEMBER(sdip_3_r);
	DECLARE_READ8_MEMBER(sdip_4_r);
	DECLARE_READ8_MEMBER(sdip_5_r);
	DECLARE_READ8_MEMBER(sdip_6_r);
	DECLARE_READ8_MEMBER(sdip_7_r);
	DECLARE_READ8_MEMBER(sdip_8_r);
	DECLARE_READ8_MEMBER(sdip_9_r);
	DECLARE_READ8_MEMBER(sdip_a_r);
	DECLARE_READ8_MEMBER(sdip_b_r);

	DECLARE_WRITE8_MEMBER(sdip_0_w);
	DECLARE_WRITE8_MEMBER(sdip_1_w);
	DECLARE_WRITE8_MEMBER(sdip_2_w);
	DECLARE_WRITE8_MEMBER(sdip_3_w);
	DECLARE_WRITE8_MEMBER(sdip_4_w);
	DECLARE_WRITE8_MEMBER(sdip_5_w);
	DECLARE_WRITE8_MEMBER(sdip_6_w);
	DECLARE_WRITE8_MEMBER(sdip_7_w);
	DECLARE_WRITE8_MEMBER(sdip_8_w);
	DECLARE_WRITE8_MEMBER(sdip_9_w);
	DECLARE_WRITE8_MEMBER(sdip_a_w);
	DECLARE_WRITE8_MEMBER(sdip_b_w);

	DECLARE_READ8_MEMBER(pc9801rs_ide_r);
	DECLARE_READ8_MEMBER(pc9821_ideram_r);
	DECLARE_WRITE8_MEMBER(pc9821_ideram_w);
	DECLARE_READ8_MEMBER(pc9821_ext_gvram_r);
	DECLARE_WRITE8_MEMBER(pc9821_ext_gvram_w);
	DECLARE_READ8_MEMBER(pc9821_window_bank_r);
	DECLARE_WRITE8_MEMBER(pc9821_window_bank_w);
	DECLARE_READ32_MEMBER(pc9821_timestamp_r);
	DECLARE_READ8_MEMBER(pc9821_ext2_video_ff_r);
	DECLARE_WRITE8_MEMBER(pc9821_ext2_video_ff_w);

	DECLARE_FLOPPY_FORMATS( floppy_formats );
	UPD7220_DISPLAY_PIXELS_MEMBER( hgdc_display_pixels );
	UPD7220_DRAW_TEXT_LINE_MEMBER( hgdc_draw_text );

private:
	UINT8 m_sdip_read(UINT16 port, UINT8 sdip_offset);
	void m_sdip_write(UINT16 port, UINT8 sdip_offset,UINT8 data);
public:
	DECLARE_MACHINE_START(pc9801_common);
	DECLARE_MACHINE_START(pc9801f);
	DECLARE_MACHINE_START(pc9801rs);
	DECLARE_MACHINE_START(pc9801bx2);
	DECLARE_MACHINE_START(pc9821);

	DECLARE_MACHINE_RESET(pc9801_common);
	DECLARE_MACHINE_RESET(pc9801f);
	DECLARE_MACHINE_RESET(pc9801rs);
	DECLARE_MACHINE_RESET(pc9821);

	DECLARE_PALETTE_INIT(pc9801);
	INTERRUPT_GEN_MEMBER(pc9801_vrtc_irq);
//  DECLARE_INPUT_CHANGED_MEMBER(key_stroke);
//  DECLARE_INPUT_CHANGED_MEMBER(shift_stroke);
	DECLARE_READ8_MEMBER(get_slave_ack);
	DECLARE_WRITE_LINE_MEMBER(pc9801_dma_hrq_changed);
	DECLARE_WRITE_LINE_MEMBER(pc9801_tc_w);
	DECLARE_READ8_MEMBER(pc9801_dma_read_byte);
	DECLARE_WRITE8_MEMBER(pc9801_dma_write_byte);
	DECLARE_WRITE_LINE_MEMBER(pc9801_dack0_w);
	DECLARE_WRITE_LINE_MEMBER(pc9801_dack1_w);
	DECLARE_WRITE_LINE_MEMBER(pc9801_dack2_w);
	DECLARE_WRITE_LINE_MEMBER(pc9801_dack3_w);
	DECLARE_READ8_MEMBER(fdc_2hd_r);
	DECLARE_WRITE8_MEMBER(fdc_2hd_w);
	DECLARE_READ8_MEMBER(fdc_2dd_r);
	DECLARE_WRITE8_MEMBER(fdc_2dd_w);
	DECLARE_READ8_MEMBER(ppi_sys_portc_r);
	DECLARE_WRITE8_MEMBER(ppi_sys_portc_w);
	DECLARE_READ8_MEMBER(ppi_fdd_porta_r);
	DECLARE_READ8_MEMBER(ppi_fdd_portb_r);
	DECLARE_READ8_MEMBER(ppi_fdd_portc_r);
	DECLARE_WRITE8_MEMBER(ppi_fdd_portc_w);

	DECLARE_WRITE_LINE_MEMBER(fdc_2dd_irq);
	DECLARE_WRITE_LINE_MEMBER(pc9801rs_fdc_irq);
	DECLARE_WRITE_LINE_MEMBER(pc9801rs_fdc_drq);

	DECLARE_READ8_MEMBER(ppi_mouse_porta_r);
	DECLARE_WRITE8_MEMBER(ppi_mouse_porta_w);
	DECLARE_WRITE8_MEMBER(ppi_mouse_portb_w);
	DECLARE_WRITE8_MEMBER(ppi_mouse_portc_w);
	struct{
		UINT8 control;
		UINT8 lx;
		UINT8 ly;
		UINT8 freq_reg;
		UINT8 freq_index;
	}m_mouse;
	TIMER_DEVICE_CALLBACK_MEMBER( mouse_irq_cb );

	void pc9801_fdc_2hd_update_ready(floppy_image_device *, int);
	inline UINT32 m_calc_grcg_addr(int i,UINT32 offset);

	DECLARE_DRIVER_INIT(pc9801_kanji);
	inline void set_dma_channel(int channel, int state);
};



#define ATTRSEL_REG 0
#define WIDTH40_REG 2
#define FONTSEL_REG 3
#define INTERLACE_REG 4
#define MEMSW_REG   6
#define DISPLAY_REG 7

#define ANALOG_16_MODE 0
#define ANALOG_256_MODE 0x10

void pc9801_state::video_start()
{
	//pc9801_state *state = machine.driver_data<pc9801_state>();

	m_tvram = auto_alloc_array(machine(), UINT8, 0x4000);

	// find memory regions
	m_char_rom = memregion("chargen")->base();
	m_kanji_rom = memregion("kanji")->base();
}

UINT32 pc9801_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(m_palette->black_pen(), cliprect);

	/* graphics */
	m_hgdc2->screen_update(screen, bitmap, cliprect);
	m_hgdc1->screen_update(screen, bitmap, cliprect);
	return 0;
}

UPD7220_DISPLAY_PIXELS_MEMBER( pc9801_state::hgdc_display_pixels )
{
	const rgb_t *palette = m_palette->palette()->entry_list_raw();
	int xi;
	int res_x,res_y;
	UINT8 pen;
	UINT8 interlace_on;
	UINT8 colors16_mode;

	if(m_video_ff[DISPLAY_REG] == 0) //screen is off
		return;

//  popmessage("%02x %d",m_video_ff[INTERLACE_REG],machine().first_screen()->visible_area().max_y + 1);
//  interlace_on = ((machine().first_screen()->visible_area().max_y + 1) >= 400) ? 1 : 0;
	interlace_on = m_video_ff[INTERLACE_REG];
	colors16_mode = (m_ex_video_ff[ANALOG_16_MODE]) ? 16 : 8;

	if(m_ex_video_ff[ANALOG_256_MODE])
	{
		for(xi=0;xi<8;xi++)
		{
			res_x = x + xi;
			res_y = y;

			if(!machine().first_screen()->visible_area().contains(res_x, res_y*2+0))
				return;

			pen = m_ext_gvram[(address*8+xi)+(m_vram_disp*0x40000)];

			bitmap.pix32(res_y*2+0, res_x) = palette[pen + 0x20];
			if(machine().first_screen()->visible_area().contains(res_x, res_y*2+1))
				bitmap.pix32(res_y*2+1, res_x) = palette[pen + 0x20];
		}
	}
	else
	{
		for(xi=0;xi<8;xi++)
		{
			res_x = x + xi;
			res_y = y;

			pen = ((m_video_ram_2[(address & 0x7fff) + (0x08000) + (m_vram_disp*0x20000)] >> (7-xi)) & 1) ? 1 : 0;
			pen|= ((m_video_ram_2[(address & 0x7fff) + (0x10000) + (m_vram_disp*0x20000)] >> (7-xi)) & 1) ? 2 : 0;
			pen|= ((m_video_ram_2[(address & 0x7fff) + (0x18000) + (m_vram_disp*0x20000)] >> (7-xi)) & 1) ? 4 : 0;
			if(m_ex_video_ff[ANALOG_16_MODE])
				pen|= ((m_video_ram_2[(address & 0x7fff) + (0) + (m_vram_disp*0x20000)] >> (7-xi)) & 1) ? 8 : 0;

			if(interlace_on)
			{
				if(machine().first_screen()->visible_area().contains(res_x, res_y*2+0))
					bitmap.pix32(res_y*2+0, res_x) = palette[pen + colors16_mode];
				/* TODO: it looks like that PC-98xx can only display even lines ... */
				if(machine().first_screen()->visible_area().contains(res_x, res_y*2+1))
					bitmap.pix32(res_y*2+1, res_x) = palette[pen + colors16_mode];
			}
			else
				bitmap.pix32(res_y, res_x) = palette[pen + colors16_mode];
		}
	}
}

UPD7220_DRAW_TEXT_LINE_MEMBER( pc9801_state::hgdc_draw_text )
{
	const rgb_t *palette = m_palette->palette()->entry_list_raw();
	int xi,yi;
	int x;
	UINT8 char_size;
//  UINT8 interlace_on;
	UINT16 tile;
	UINT8 kanji_lr;
	UINT8 kanji_sel;
	UINT8 x_step;

	if(m_video_ff[DISPLAY_REG] == 0) //screen is off
		return;

//  interlace_on = m_video_ff[INTERLACE_REG];
	char_size = m_video_ff[FONTSEL_REG] ? 16 : 8;
	tile = 0;

	for(x=0;x<pitch;x+=x_step)
	{
		UINT8 tile_data,secret,reverse,u_line,v_line,blink;
		UINT8 color;
		UINT8 attr;
		int pen;
		UINT32 tile_addr;
		UINT8 knj_tile;
		UINT8 gfx_mode;

		tile_addr = addr+(x*(m_video_ff[WIDTH40_REG]+1));

		kanji_sel = 0;
		kanji_lr = 0;

		tile = m_video_ram_1[(tile_addr*2) & 0x1fff] & 0xff;
		knj_tile = m_video_ram_1[(tile_addr*2+1) & 0x1fff] & 0xff;
		if(knj_tile)
		{
			/* Note: bit 7 doesn't really count, if a kanji is enabled then the successive tile is always the second part of it.
			   Trusted with Alice no Yakata, Animahjong V3, Aki no Tsukasa no Fushigi no Kabe, Apros ...
			*/
			//kanji_lr = (knj_tile & 0x80) >> 7;
			//kanji_lr |= (tile & 0x80) >> 7; // Tokimeki Sports Gal 3
			tile &= 0x7f;
			tile <<= 8;
			tile |= (knj_tile & 0x7f);
			kanji_sel = 1;
			if((tile & 0x7c00) == 0x0800) // 8x16 charset selector
				x_step = 1;
			else
				x_step = 2;
//          kanji_lr = 0;
		}
		else
			x_step = 1;

		attr = (m_video_ram_1[(tile_addr*2 & 0x1fff) | 0x2000] & 0xff);

		secret = (attr & 1) ^ 1;
		blink = attr & 2;
		reverse = attr & 4;
		u_line = attr & 8;
		v_line = (m_video_ff[ATTRSEL_REG]) ? 0 : attr & 0x10;
		gfx_mode = (m_video_ff[ATTRSEL_REG]) ? attr & 0x10 : 0;
		color = (attr & 0xe0) >> 5;

		for(kanji_lr=0;kanji_lr<x_step;kanji_lr++)
		{
			for(yi=0;yi<lr;yi++)
			{
				for(xi=0;xi<8;xi++)
				{
					int res_x,res_y;

					res_x = ((x+kanji_lr)*8+xi) * (m_video_ff[WIDTH40_REG]+1);
					res_y = y*lr+yi - (m_txt_scroll_reg[3] & 0xf);

					if(!machine().first_screen()->visible_area().contains(res_x, res_y))
						continue;

					tile_data = 0;

					if(!secret)
					{
						/* TODO: priority */
						if(gfx_mode)
						{
							int gfx_bit;
							tile_data = 0;

							/*
							    gfx strip mode:

							    number refers to the bit number in the tile data.
							    This mode is identical to the one seen in PC-8801
							    00004444
							    11115555
							    22226666
							    33337777
							*/

							gfx_bit = (xi & 4);
							gfx_bit+= (yi & (2 << (char_size == 16 ? 0x01 : 0x00)))>>(1+(char_size == 16));
							gfx_bit+= (yi & (4 << (char_size == 16 ? 0x01 : 0x00)))>>(1+(char_size == 16));

							tile_data = ((tile >> gfx_bit) & 1) ? 0xff : 0x00;
						}
						else if(kanji_sel)
							tile_data = (m_kanji_rom[tile*0x20+yi*2+kanji_lr]);
						else
							tile_data = (m_char_rom[tile*char_size+m_video_ff[FONTSEL_REG]*0x800+yi]);
					}

					if(reverse) { tile_data^=0xff; }
					if(u_line && yi == 7) { tile_data = 0xff; }
					if(v_line)  { tile_data|=8; }

					/* TODO: proper blink rate for these two */
					if(cursor_on && cursor_addr == tile_addr && machine().first_screen()->frame_number() & 0x10)
						tile_data^=0xff;

					if(blink && machine().first_screen()->frame_number() & 0x10)
						tile_data^=0xff;

					if(yi >= char_size)
						pen = -1;
					else
						pen = (tile_data >> (7-xi) & 1) ? color : -1;

					if(pen != -1)
						bitmap.pix32(res_y, res_x) = palette[pen];

					if(m_video_ff[WIDTH40_REG])
					{
						if(!machine().first_screen()->visible_area().contains(res_x+1, res_y))
							continue;

						if(pen != -1)
							bitmap.pix32(res_y, res_x+1) = palette[pen];
					}
				}
			}
		}
	}
}


#if 0
READ8_MEMBER(pc9801_state::pc9801_xx_r)
{
	if((offset & 1) == 0)
	{
		printf("Read to undefined port [%02x]\n",offset+0xxx);
		return 0xff;
	}
	else // odd
	{
		printf("Read to undefined port [%02x]\n",offset+0xxx);
		return 0xff;
	}
}

WRITE8_MEMBER(pc9801_state::pc9801_xx_w)
{
	if((offset & 1) == 0)
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0xxx,data);
	}
	else // odd
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0xxx,data);
	}
}

#endif

READ8_MEMBER(pc9801_state::pc9801_00_r)
{
	if((offset & 1) == 0)
	{
		if(offset & 0x14)
			printf("Read to undefined port [%02x]\n",offset+0x00);
		else
			return ((offset & 8) ? m_pic2 : m_pic1)->read(space, (offset & 2) >> 1);
	}
	else // odd
	{
		return m_dmac->read(space, (offset & 0x1e) >> 1, 0xff);
	}

	return 0xff;
}

WRITE8_MEMBER(pc9801_state::pc9801_00_w)
{
	if((offset & 1) == 0)
	{
		if(offset & 0x14)
			printf("Write to undefined port [%02x] <- %02x\n",offset+0x00,data);
		else
			((offset & 8) ? m_pic2 : m_pic1)->write(space, (offset & 2) >> 1, data);
	}
	else // odd
	{
		m_dmac->write(space, (offset & 0x1e) >> 1, data, 0xff);
	}
}

READ8_MEMBER(pc9801_state::pc9801_20_r)
{
	if((offset & 1) == 0)
	{
		if(offset == 0)
			printf("Read to RTC port [%02x]\n",offset+0x20);
		else
			printf("Read to undefined port [%02x]\n",offset+0x20);

		return 0xff;
	}
	else // odd
	{
		printf("Read to undefined port [%02x]\n",offset+0x20);
		return 0xff;
	}
}

WRITE8_MEMBER(pc9801_state::pc9801_20_w)
{
	if((offset & 1) == 0)
	{
		if(offset == 0)
		{
			m_rtc->c0_w((data & 0x01) >> 0);
			m_rtc->c1_w((data & 0x02) >> 1);
			m_rtc->c2_w((data & 0x04) >> 2);
			m_rtc->stb_w((data & 0x08) >> 3);
			m_rtc->clk_w((data & 0x10) >> 4);
			m_rtc->data_in_w(((data & 0x20) >> 5));
			if(data & 0xc0)
				printf("RTC write to undefined bits %02x\n",data & 0xc0);
		}
		else
			printf("Write to undefined port [%02x] <- %02x\n",offset+0x20,data);
	}
	else // odd
	{
//      printf("Write to DMA bank register %d %02x\n",((offset >> 1)+1) & 3,data);
		m_dma_offset[((offset >> 1)+1) & 3] = data & 0x0f;
	}
}

READ8_MEMBER(pc9801_state::pc9801_30_r)
{
	if((offset & 1) == 0)
	{
		if(offset & 4)
			printf("Read to undefined port [%02x]\n",offset+0x30);
		else
			printf("Read to RS-232c port [%02x]\n",offset+0x30);

		return 0xff;
	}
	else // odd
	{
		return machine().device<i8255_device>("ppi8255_sys")->read(space, (offset & 6) >> 1);
	}
}

WRITE8_MEMBER(pc9801_state::pc9801_30_w)
{
	if((offset & 1) == 0)
	{
		if(offset & 4)
			printf("Write to undefined port [%02x] %02x\n",offset+0x30,data);
		else
			printf("Write to RS-232c port [%02x] %02x\n",offset+0x30,data);
	}
	else // odd
	{
		machine().device<i8255_device>("ppi8255_sys")->write(space, (offset & 6) >> 1,data);
	}
}

READ8_MEMBER(pc9801_state::pc9801_40_r)
{
	if((offset & 1) == 0)
	{
		return machine().device<i8255_device>("ppi8255_prn")->read(space, (offset & 6) >> 1);
	}
	else // odd
	{
		if(offset & 4)
			printf("Read to undefined port [%02x]\n",offset+0x40);
		else
		{
			//printf("Read to 8251 kbd port [%02x] %08x\n",offset+0x40,m_maincpu->pc());
			if(offset == 1)
			{
				return m_keyb->rx_r(space,0);
			}

			return 1 | 4 | 2;
		}
	}

	return 0xff;
}

WRITE8_MEMBER(pc9801_state::pc9801_40_w)
{
	if((offset & 1) == 0)
	{
		machine().device<i8255_device>("ppi8255_prn")->write(space, (offset & 6) >> 1,data);
	}
	else // odd
	{
		if(offset & 4)
			printf("Write to undefined port [%02x] <- %02x\n",offset+0x40,data);
		else
		{
			if(offset == 1)
			{
				m_keyb->tx_w(space,0,data);
				return;
			}
			//printf("Write to 8251 kbd port [%02x] <- %02x\n",offset+0x40,data);
		}
	}
}

READ8_MEMBER(pc9801_state::pc9801_50_r)
{
	if((offset & 1) == 0)
	{
		if(offset & 4)
			printf("Read to undefined port [%02x]\n",offset+0x50);
		else
			printf("Read to NMI FF port [%02x]\n",offset+0x50);

		return 0xff;
	}
	else // odd
	{
		return machine().device<i8255_device>("ppi8255_fdd")->read(space, (offset & 6) >> 1);
	}
}

WRITE8_MEMBER(pc9801_state::pc9801_50_w)
{
	if((offset & 1) == 0)
	{
		if(offset & 4)
			printf("Write to undefined port [%02x] %02x\n",offset+0x50,data);
		else
			m_nmi_ff = (offset & 2) >> 1;

	}
	else // odd
	{
		machine().device<i8255_device>("ppi8255_fdd")->write(space, (offset & 6) >> 1,data);
	}
}

READ8_MEMBER(pc9801_state::pc9801_60_r)
{
	if((offset & 1) == 0)
	{
		return m_hgdc1->read(space, (offset & 2) >> 1); // upd7220 character port
	}
	else // odd
	{
		printf("Read to undefined port [%02x]\n",offset+0x60);
		return 0xff;
	}
}

WRITE8_MEMBER(pc9801_state::pc9801_60_w)
{
	if((offset & 1) == 0)
	{
		m_hgdc1->write(space, (offset & 2) >> 1,data); // upd7220 character port
	}
	else // odd
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0x60,data);
	}
}

WRITE8_MEMBER(pc9801_state::pc9801_vrtc_mask_w)
{
	if((offset & 1) == 0)
	{
		m_vrtc_irq_mask = 1;
	}
	else // odd
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0x64,data);
	}
}

WRITE8_MEMBER(pc9801_state::pc9801_video_ff_w)
{
	if((offset & 1) == 0)
	{
		/*
		TODO: this is my best bet so far. Register 4 is annoying, the pattern seems to be:
		Write to video FF register Graphic -> 00
		Write to video FF register 200 lines -> 0x
		Write to video FF register 200 lines -> 00

		where x is the current mode.
		*/
		switch((data & 0x0e) >> 1)
		{
			case 1:
				m_gfx_ff = 1;
				if(data & 1)
					printf("Graphic f/f actually enabled!\n");
					break;
			case 4:
				if(m_gfx_ff)
				{
					m_video_ff[(data & 0x0e) >> 1] = data &1;
					m_gfx_ff = 0;
				}
				break;
			default: m_video_ff[(data & 0x0e) >> 1] = data & 1; break;
		}

		if(0)
		{
			static const char *const video_ff_regnames[] =
			{
				"Attribute Select", // 0
				"Graphic",          // 1
				"Column",           // 2
				"Font Select",      // 3
				"200 lines",        // 4
				"KAC?",             // 5
				"Memory Switch",    // 6
				"Display ON"        // 7
			};

			printf("Write to video FF register %s -> %02x\n",video_ff_regnames[(data & 0x0e) >> 1],data & 1);
		}
	}
	else // odd
	{
		//printf("Write to undefined port [%02x] <- %02x\n",offset+0x68,data);
	}
}


READ8_MEMBER(pc9801_state::pc9801_70_r)
{
	if((offset & 1) == 0)
	{
		//printf("Read to display register [%02x]\n",offset+0x70);
		/* TODO: ok? */
		return m_txt_scroll_reg[offset >> 1];
	}
	else // odd
	{
		if(offset & 0x08)
			printf("Read to undefined port [%02x]\n",offset+0x70);
		else
			return m_pit8253->read(space, (offset & 6) >> 1);
	}

	return 0xff;
}

WRITE8_MEMBER(pc9801_state::pc9801_70_w)
{
	if((offset & 1) == 0)
	{
//      printf("Write to display register [%02x] %02x\n",offset+0x70,data);
		m_txt_scroll_reg[offset >> 1] = data;

		//popmessage("%02x %02x %02x %02x",m_txt_scroll_reg[0],m_txt_scroll_reg[1],m_txt_scroll_reg[2],m_txt_scroll_reg[3]);
	}
	else // odd
	{
		if(offset < 0x08)
			m_pit8253->write(space, (offset & 6) >> 1, data);
		//else
		//  printf("Write to undefined port [%02x] <- %02x\n",offset+0x70,data);
	}
}

READ8_MEMBER(pc9801_state::pc9801_sasi_r)
{
	if((offset & 1) == 0)
	{
		//printf("Read to SASI port [%02x]\n",offset+0x80);
		return 0x20;
	}
	else // odd
	{
		printf("Read to undefined port [%02x]\n",offset+0x80);
		return 0xff;
	}
}

WRITE8_MEMBER(pc9801_state::pc9801_sasi_w)
{
	if((offset & 1) == 0)
	{
		//printf("Write to SASI port [%02x] <- %02x\n",offset+0x80,data);
	}
	else // odd
	{
		//printf("Write to undefined port [%02x] <- %02x\n",offset+0xxx,data);
	}
}


READ8_MEMBER(pc9801_state::pc9801_a0_r)
{
	if((offset & 1) == 0)
	{
		switch(offset & 0xe)
		{
			case 0x00:
			case 0x02:
				return m_hgdc2->read(space, (offset & 2) >> 1);
			/* TODO: double check these two */
			case 0x04:
				return m_vram_disp & 1;
			case 0x06:
				return m_vram_bank & 1;
			/* bitmap palette clut read */
			case 0x08:
			case 0x0a:
			case 0x0c:
			case 0x0e:
				return m_pal_clut[(offset & 0x6) >> 1];
		}

		return 0xff; //code unreachable
	}
	else // odd
	{
		switch((offset & 0xe) + 1)
		{
			case 0x09://cg window font read
			{
				UINT32 pcg_offset;

				pcg_offset = (m_font_addr & 0x7f7f) << 5;
				pcg_offset|= m_font_line;
				pcg_offset|= m_font_lr;

				return m_kanji_rom[pcg_offset];
			}
		}

		printf("Read to undefined port [%02x]\n",offset+0xa0);
		return 0xff;
	}
}

WRITE8_MEMBER(pc9801_state::pc9801_a0_w)
{
	if((offset & 1) == 0)
	{
		switch(offset & 0xe)
		{
			case 0x00:
			case 0x02:
				m_hgdc2->write(space, (offset & 2) >> 1,data);
				return;
			case 0x04:
				m_vram_disp = data & 1;
				return;
			case 0x06:
				m_vram_bank = data & 1;
				return;
			/* bitmap palette clut write */
			case 0x08:
			case 0x0a:
			case 0x0c:
			case 0x0e:
			{
				UINT8 pal_entry;

				m_pal_clut[(offset & 0x6) >> 1] = data;

				/* can't be more twisted I presume ... :-/ */
				pal_entry = (((offset & 4) >> 1) | ((offset & 2) << 1)) >> 1;
				pal_entry ^= 3;

				m_palette->set_pen_color((pal_entry)|4|8, pal1bit((data & 0x2) >> 1), pal1bit((data & 4) >> 2), pal1bit((data & 1) >> 0));
				m_palette->set_pen_color((pal_entry)|8, pal1bit((data & 0x20) >> 5), pal1bit((data & 0x40) >> 6), pal1bit((data & 0x10) >> 4));
				return;
			}
			default:
				printf("Write to undefined port [%02x] <- %02x\n",offset+0xa0,data);
				return;
		}
	}
	else // odd
	{
		switch((offset & 0xe) + 1)
		{
			case 0x01:
				m_font_addr = (data & 0xff) | (m_font_addr & 0x7f00);
				return;
			case 0x03:
				m_font_addr = ((data & 0x7f) << 8) | (m_font_addr & 0xff);
				return;
			case 0x05:
				//printf("%02x\n",data);
				m_font_line = ((data & 0x0f) << 1);
				m_font_lr = ((data & 0x20) >> 5) ^ 1;
				return;
			case 0x09: //cg window font write
			{
				UINT32 pcg_offset;

				pcg_offset = m_font_addr << 5;
				pcg_offset|= m_font_line;
				pcg_offset|= m_font_lr;
				//printf("%04x %02x %02x %08x\n",m_font_addr,m_font_line,m_font_lr,pcg_offset);
				if((m_font_addr & 0xff00) == 0x5600 || (m_font_addr & 0xff00) == 0x5700)
				{
					m_kanji_rom[pcg_offset] = data;
					m_gfxdecode->gfx(2)->mark_dirty(pcg_offset >> 5);
				}
				return;
			}
		}

		//printf("Write to undefined port [%02x) <- %02x\n",offset+0xa0,data);
	}
}

DECLARE_WRITE_LINE_MEMBER(pc9801_state::write_uart_clock)
{
	m_sio->write_txc(state);
	m_sio->write_rxc(state);
}

READ8_MEMBER(pc9801_state::pc9801_fdc_2hd_r)
{
	if((offset & 1) == 0)
	{
		switch(offset & 6)
		{
			case 0: return machine().device<upd765a_device>("upd765_2hd")->msr_r(space, 0, 0xff);
			case 2: return machine().device<upd765a_device>("upd765_2hd")->fifo_r(space, 0, 0xff);
			case 4: return 0x5f; //unknown port meaning
		}
	}
	else
	{
		switch((offset & 6) + 1)
		{
			case 1: return m_sio->data_r(space, 0);
			case 3: return m_sio->status_r(space, 0);
		}
		printf("Read to undefined port [%02x]\n",offset+0x90);
		return 0xff;
	}

	return 0xff;
}

void pc9801_state::pc9801_fdc_2hd_update_ready(floppy_image_device *, int)
{
	bool ready = m_fdc_2hd_ctrl & 0x40;
	floppy_image_device *floppy;
	floppy = machine().device<floppy_connector>("upd765_2hd:0")->get_device();
	if(floppy && ready)
		ready = floppy->ready_r();
	floppy = machine().device<floppy_connector>("upd765_2hd:1")->get_device();
	if(floppy && ready)
		ready = floppy->ready_r();

	m_fdc_2hd->ready_w(ready);
}

WRITE8_MEMBER(pc9801_state::pc9801_fdc_2hd_w)
{
	if((offset & 1) == 0)
	{
		switch(offset & 6)
		{
			case 0: printf("Write to undefined port [%02x] <- %02x\n",offset+0x90,data); return;
			case 2: machine().device<upd765a_device>("upd765_2hd")->fifo_w(space, 0, data, 0xff); return;
			case 4:
				printf("%02x ctrl\n",data);
				if(((m_fdc_2hd_ctrl & 0x80) == 0) && (data & 0x80))
					machine().device<upd765a_device>("upd765_2hd")->reset();

				m_fdc_2hd_ctrl = data;
				pc9801_fdc_2hd_update_ready(NULL, 0);

				machine().device<floppy_connector>("upd765_2hd:0")->get_device()->mon_w(data & 0x40 ? ASSERT_LINE : CLEAR_LINE);
				machine().device<floppy_connector>("upd765_2hd:1")->get_device()->mon_w(data & 0x40 ? ASSERT_LINE : CLEAR_LINE);
				break;
		}
	}
	else
	{
		switch((offset & 6) + 1)
		{
			case 1: m_sio->data_w(space, 0, data); return;
			case 3: m_sio->control_w(space, 0, data); return;
		}
		printf("Write to undefined port [%02x] <- %02x\n",offset+0x90,data);
	}
}


READ8_MEMBER(pc9801_state::pc9801_fdc_2dd_r)
{
	if((offset & 1) == 0)
	{
		switch(offset & 6)
		{
			case 0: return machine().device<upd765a_device>("upd765_2dd")->msr_r(space, 0, 0xff);
			case 2: return machine().device<upd765a_device>("upd765_2dd")->fifo_r(space, 0, 0xff);
			case 4: return 0x40; //unknown port meaning, might be 0x70
		}
	}
	else
	{
		printf("Read to undefined port [%02x]\n",offset+0xc8);
		return 0xff;
	}

	return 0xff;
}

WRITE8_MEMBER(pc9801_state::pc9801_fdc_2dd_w)
{
	if((offset & 1) == 0)
	{
		switch(offset & 6)
		{
			case 0: printf("Write to undefined port [%02x] <- %02x\n",offset+0xc8,data); return;
			case 2: machine().device<upd765a_device>("upd765_2dd")->fifo_w(space, 0, data, 0xff); return;
			case 4:
				printf("%02x ctrl\n",data);
				if(((m_fdc_2dd_ctrl & 0x80) == 0) && (data & 0x80))
					machine().device<upd765a_device>("upd765_2dd")->reset();

				m_fdc_2dd_ctrl = data;
				machine().device<floppy_connector>("upd765_2dd:0")->get_device()->mon_w(data & 0x08 ? ASSERT_LINE : CLEAR_LINE);
				machine().device<floppy_connector>("upd765_2dd:1")->get_device()->mon_w(data & 0x08 ? ASSERT_LINE : CLEAR_LINE);
				break;
		}
	}
	else
	{
		printf("Write to undefined port [%02x] <- %02x\n",offset+0xc8,data);
	}
}


/* TODO: banking? */
READ8_MEMBER(pc9801_state::pc9801_tvram_r)
{
	UINT8 res;

	if((offset & 0x2000) && offset & 1)
		return 0xff;

	//res = upd7220_vram_r(machine().device("upd7220_chr"),offset);
	res = m_tvram[offset];

	return res;
}

WRITE8_MEMBER(pc9801_state::pc9801_tvram_w)
{
	if(offset < (0x3fe2) || m_video_ff[MEMSW_REG])
		m_tvram[offset] = data;

	m_video_ram_1[offset] = data; //TODO: check me
}

/* +0x8000 is trusted (bank 0 is actually used by 16 colors mode) */
READ8_MEMBER(pc9801_state::pc9801_gvram_r)
{
	return m_video_ram_2[offset+0x08000+m_vram_bank*0x20000];
}

WRITE8_MEMBER(pc9801_state::pc9801_gvram_w)
{
	m_video_ram_2[offset+0x08000+m_vram_bank*0x20000] = data;
}

inline UINT32 pc9801_state::m_calc_grcg_addr(int i,UINT32 offset)
{
	return (offset) + (((i+1)*0x8000) & 0x1ffff) + (m_vram_bank*0x20000);
}

inline UINT8 pc9801_state::m_pc9801rs_grcg_r(UINT32 offset,int vbank)
{
	UINT8 res;

	if((m_grcg.mode & 0x80) == 0 || (m_grcg.mode & 0x40))
		res = m_video_ram_2[offset+vbank*0x8000+m_vram_bank*0x20000];
	else
	{
		int i;

		res = 0;
		for(i=0;i<4;i++)
		{
			if((m_grcg.mode & (1 << i)) == 0)
				res |= (m_video_ram_2[m_calc_grcg_addr(i,offset)] ^ m_grcg.tile[i]);
		}

		res ^= 0xff;
	}

	return res;
}

inline void pc9801_state::m_pc9801rs_grcg_w(UINT32 offset,int vbank,UINT8 data)
{
	if((m_grcg.mode & 0x80) == 0)
		m_video_ram_2[offset+vbank*0x8000+m_vram_bank*0x20000] = data;
	else
	{
		int i;

		if(m_grcg.mode & 0x40) // RMW
		{
			for(i=0;i<4;i++)
			{
				if((m_grcg.mode & (1 << i)) == 0)
				{
					m_video_ram_2[m_calc_grcg_addr(i,offset)] &= ~data;
					m_video_ram_2[m_calc_grcg_addr(i,offset)] |= m_grcg.tile[i] & data;
				}
			}
		}
		else // TDW
		{
			for(i=0;i<4;i++)
			{
				if((m_grcg.mode & (1 << i)) == 0)
				{
					m_video_ram_2[m_calc_grcg_addr(i,offset)] = m_grcg.tile[i];
				}
			}
		}
	}
}


READ8_MEMBER(pc9801_state::pc9801_mouse_r)
{
	if((offset & 1) == 0)
		return 0xff;
	else
	{
		return machine().device<i8255_device>("ppi8255_mouse")->read(space, (offset & 6) >> 1);
	}
}

WRITE8_MEMBER(pc9801_state::pc9801_mouse_w)
{
	if((offset & 1) == 0)
	{
		//return 0xff;
	}
	else
	{
		machine().device<i8255_device>("ppi8255_mouse")->write(space, (offset & 6) >> 1,data);
	}
}

WRITE8_MEMBER( pc9801_state::sasi_data_w )
{
	m_sasi_data = data;

	if (m_sasi_data_enable)
	{
		m_sasi_data_out->write(m_sasi_data);
	}
}

WRITE_LINE_MEMBER( pc9801_state::write_sasi_io )
{
	m_sasi_ctrl_in->write_bit2(state);

	m_sasi_data_enable = !state;

	if (m_sasi_data_enable)
	{
		m_sasi_data_out->write(m_sasi_data);
	}
	else
	{
		m_sasi_data_out->write(0);
	}
}

#include "debugger.h"

READ8_MEMBER( pc9801_state::sasi_status_r )
{
	UINT8 res = 0;

	if(m_sasi_ctrl & 0x40) // read status
	{
	/*
	    x--- -.-- REQ
	    -x-- ---- ACK
	    --x- ---- BSY
	    ---x ---- MSG
	    ---- x--- CD
	    ---- -x-- IO
	    ---- ---x INT?
	*/
		res |= m_sasi_ctrl_in->read();
	}
	else // read drive info
	{
/*
        xx-- ---- unknown but tested
        --xx x--- SASI-1 media type
        ---- -xxx SASI-2 media type
*/
		res |= 7 << 3; // read mediatype SASI-1
		res |= 7;   // read mediatype SASI-2
	}

	return res;
}

WRITE8_MEMBER( pc9801_state::sasi_ctrl_w )
{
	/*
	    x--- ---- channel enable
	    -x-- ---- read switch
	    --x- ---- sel
	    ---- x--- reset line
	    ---- --x- dma enable
	    ---- ---x irq enable
	*/

	m_sasibus->write_sel(BIT(data, 5));

	if(m_sasi_ctrl & 8 && ((data & 8) == 0)) // 1 -> 0 transition
	{
		m_sasibus->write_rst(1);
//      m_timer_rst->adjust(attotime::from_nsec(100));
	}
	else
		m_sasibus->write_rst(0); // TODO

	m_sasi_ctrl = data;

//  m_sasibus->write_sel(BIT(data, 0));
}

static ADDRESS_MAP_START( pc9801_map, AS_PROGRAM, 16, pc9801_state )
	AM_RANGE(0x00000, 0x9ffff) AM_RAM //work RAM
	AM_RANGE(0xa0000, 0xa3fff) AM_READWRITE8(pc9801_tvram_r,pc9801_tvram_w,0xffff) //TVRAM
	AM_RANGE(0xa8000, 0xbffff) AM_READWRITE8(pc9801_gvram_r,pc9801_gvram_w,0xffff) //bitmap VRAM
	AM_RANGE(0xcc000, 0xcdfff) AM_ROM AM_REGION("sound_bios",0) //sound BIOS
	AM_RANGE(0xd6000, 0xd6fff) AM_ROM AM_REGION("fdc_bios_2dd",0) //floppy BIOS 2dd
	AM_RANGE(0xd7000, 0xd7fff) AM_ROM AM_REGION("fdc_bios_2hd",0) //floppy BIOS 2hd
	AM_RANGE(0xe8000, 0xfffff) AM_ROM AM_REGION("ipl",0)
ADDRESS_MAP_END

/* first device is even offsets, second one is odd offsets */
static ADDRESS_MAP_START( pc9801_io, AS_IO, 16, pc9801_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x001f) AM_READWRITE8(pc9801_00_r,pc9801_00_w,0xffff) // i8259 PIC (bit 3 ON slave / master) / i8237 DMA
	AM_RANGE(0x0020, 0x0027) AM_READWRITE8(pc9801_20_r,pc9801_20_w,0xffff) // RTC / DMA registers (LS244)
	AM_RANGE(0x0030, 0x0037) AM_READWRITE8(pc9801_30_r,pc9801_30_w,0xffff) //i8251 RS232c / i8255 system port
	AM_RANGE(0x0040, 0x0047) AM_READWRITE8(pc9801_40_r,pc9801_40_w,0xffff) //i8255 printer port / i8251 keyboard
	AM_RANGE(0x0050, 0x0057) AM_READWRITE8(pc9801_50_r,pc9801_50_w,0xffff) // NMI FF / i8255 floppy port (2d?)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(pc9801_60_r,pc9801_60_w,0xffff) //upd7220 character ports / <undefined>
	AM_RANGE(0x0064, 0x0065) AM_WRITE8(pc9801_vrtc_mask_w,0xffff)
	AM_RANGE(0x0068, 0x0069) AM_WRITE8(pc9801_video_ff_w,0xffff) //mode FF / <undefined>
//  AM_RANGE(0x006c, 0x006f) border color / <undefined>
	AM_RANGE(0x0070, 0x007b) AM_READWRITE8(pc9801_70_r,pc9801_70_w,0xffff) //display registers / i8253 pit
//  AM_RANGE(0x0080, 0x0083) AM_READWRITE8(pc9801_sasi_r,pc9801_sasi_w,0xffff) //HDD SASI interface / <undefined>
	AM_RANGE(0x0080, 0x0081) AM_DEVREAD8("sasi_data_in", input_buffer_device, read, 0x00ff) AM_WRITE8(sasi_data_w, 0x00ff)
	AM_RANGE(0x0082, 0x0083) AM_READWRITE8(sasi_status_r, sasi_ctrl_w,0x00ff)
	AM_RANGE(0x0090, 0x0097) AM_READWRITE8(pc9801_fdc_2hd_r,pc9801_fdc_2hd_w,0xffff) //upd765a 2hd / cmt
	AM_RANGE(0x00a0, 0x00af) AM_READWRITE8(pc9801_a0_r,pc9801_a0_w,0xffff) //upd7220 bitmap ports / display registers
	AM_RANGE(0x00c8, 0x00cd) AM_READWRITE8(pc9801_fdc_2dd_r,pc9801_fdc_2dd_w,0xffff) //upd765a 2dd / <undefined>
//  AM_RANGE(0x0188, 0x018b) AM_READWRITE8(pc9801_opn_r,pc9801_opn_w,0xffff) //ym2203 opn / <undefined>
	AM_RANGE(0x7fd8, 0x7fdf) AM_READWRITE8(pc9801_mouse_r,pc9801_mouse_w,0xffff) // <undefined> / mouse ppi8255 ports
ADDRESS_MAP_END

/*************************************
 *
 * PC-9801RS specific handlers (IA-32)
 *
 ************************************/

READ8_MEMBER(pc9801_state::pc9801rs_ide_r) { return m_ide_rom[offset]; }

READ8_MEMBER(pc9801_state::pc9801rs_wram_r) { return m_work_ram[offset]; }
WRITE8_MEMBER(pc9801_state::pc9801rs_wram_w) { m_work_ram[offset] = data; }

READ8_MEMBER(pc9801_state::pc9801rs_ex_wram_r) { return m_ext_work_ram[offset]; }
WRITE8_MEMBER(pc9801_state::pc9801rs_ex_wram_w) { m_ext_work_ram[offset] = data; }

READ8_MEMBER(pc9801_state::pc9801rs_ipl_r) { return m_ipl_rom[(offset & 0x1ffff)+(m_rom_bank*0x20000)]; }

/* TODO: it's possible that the offset calculation is actually linear. */
/* TODO: having this non-linear makes the system to boot in BASIC for PC-9821. Perhaps it stores settings? How to change these? */
READ8_MEMBER(pc9801_state::pc9801rs_knjram_r)
{
	UINT32 pcg_offset;

	pcg_offset = m_font_addr << 5;
	pcg_offset|= offset & 0x1e;
	pcg_offset|= m_font_lr;

	/* TODO: investigate on this difference */
	if((m_font_addr & 0xff00) == 0x5600 || (m_font_addr & 0xff00) == 0x5700)
		return m_kanji_rom[pcg_offset];

	pcg_offset = m_font_addr << 5;
	pcg_offset|= offset & 0x1f;
//  pcg_offset|= m_font_lr;

	return m_kanji_rom[pcg_offset];
}

WRITE8_MEMBER(pc9801_state::pc9801rs_knjram_w)
{
	UINT32 pcg_offset;

	pcg_offset = m_font_addr << 5;
	pcg_offset|= offset & 0x1e;
	pcg_offset|= m_font_lr;

	if((m_font_addr & 0xff00) == 0x5600 || (m_font_addr & 0xff00) == 0x5700)
	{
		m_kanji_rom[pcg_offset] = data;
		m_gfxdecode->gfx(2)->mark_dirty(pcg_offset >> 5);
	}
}

/* FF-based */
WRITE8_MEMBER(pc9801_state::pc9801rs_bank_w)
{
	if(offset == 1)
	{
		if((data & 0xf0) == 0x00 || (data & 0xf0) == 0x10)
		{
			if((data & 0xed) == 0x00)
			{
				m_rom_bank = (data & 2) >> 1;
				return;
			}
		}

		printf("Unknown EMS ROM setting %02x\n",data);
	}
	if(offset == 3)
	{
		if((data & 0xf0) == 0x20)
			m_vram_bank = (data & 2) >> 1;
		else
		{
			printf("Unknown EMS RAM setting %02x\n",data);
		}
	}
}

READ8_MEMBER(pc9801_state::pc9801rs_f0_r)
{
	if(offset == 0x02)
		return (m_gate_a20 ^ 1) | 0xfe;
	else if(offset == 0x06)
		return (m_gate_a20 ^ 1) | (m_nmi_enable << 1);

	return 0x00;
}

WRITE8_MEMBER(pc9801_state::pc9801rs_f0_w)
{
	if(offset == 0x00)
	{
		UINT8 por;
		/* reset POR bit, TODO: is there any other way? */
		por = machine().device<i8255_device>("ppi8255_sys")->read(space, 2) & ~0x20;
		machine().device<i8255_device>("ppi8255_sys")->write(space, 2,por);
		m_maincpu->set_input_line(INPUT_LINE_A20, 0);
		m_maincpu->set_input_line(INPUT_LINE_RESET, PULSE_LINE);
		m_gate_a20 = 0;
	}

	if(offset == 0x02)
		m_gate_a20 = 1;

	if(offset == 0x06)
	{
		if(data == 0x02)
			m_gate_a20 = 1;
		else if(data == 0x03)
			m_gate_a20 = 0;
	}
	m_maincpu->set_input_line(INPUT_LINE_A20, m_gate_a20);
}

READ8_MEMBER(pc9801_state::pc9801rs_30_r)
{
	return pc9801_30_r(space,offset);
}

READ8_MEMBER(pc9801_state::pc9801rs_70_r)
{
	if(offset == 0xc)
	{
		printf("GRCG mode R\n");
		return 0xff;
	}
	else if(offset == 0x0e)
	{
		printf("GRCG tile R\n");
		return 0xff;
	}

	return  pc9801_70_r(space,offset);;
}

WRITE8_MEMBER(pc9801_state::pc9801rs_70_w)
{
	if(offset == 0xc)
	{
//      printf("%02x GRCG MODE\n",data);
		m_grcg.mode = data;
		m_grcg.tile_index = 0;
		return;
	}
	else if(offset == 0x0e)
	{
//      printf("%02x GRCG TILE %02x\n",data,m_grcg.tile_index);
		m_grcg.tile[m_grcg.tile_index] = data;
		m_grcg.tile_index ++;
		m_grcg.tile_index &= 3;
		return;
	}

	pc9801_70_w(space,offset,data);
}

READ8_MEMBER(pc9801_state::pc9801rs_soundrom_r)
{
	return m_sound_bios[offset];
}

READ8_MEMBER(pc9801_state::pc9801rs_memory_r)
{
	if(m_gate_a20 == 0)
		offset &= 0xfffff;

	if     (                        offset <= 0x0009ffff)                   { return pc9801rs_wram_r(space,offset);               }
	else if(offset >= 0x000a0000 && offset <= 0x000a3fff)                   { return pc9801_tvram_r(space,offset-0xa0000);        }
	else if(offset >= 0x000a4000 && offset <= 0x000a4fff)                   { return pc9801rs_knjram_r(space,offset & 0xfff);     }
	else if(offset >= 0x000a8000 && offset <= 0x000affff)                   { return m_pc9801rs_grcg_r(offset & 0x7fff,1);        }
	else if(offset >= 0x000b0000 && offset <= 0x000b7fff)                   { return m_pc9801rs_grcg_r(offset & 0x7fff,2);        }
	else if(offset >= 0x000b8000 && offset <= 0x000bffff)                   { return m_pc9801rs_grcg_r(offset & 0x7fff,3);        }
	else if(offset >= 0x000cc000 && offset <= 0x000cffff)                   { return pc9801rs_soundrom_r(space,offset & 0x3fff);}
	else if(offset >= 0x000d8000 && offset <= 0x000d9fff)                   { return pc9801rs_ide_r(space,offset & 0x1fff);         }
	else if(offset >= 0x000e0000 && offset <= 0x000e7fff)                   { return m_pc9801rs_grcg_r(offset & 0x7fff,0);        }
	else if(offset >= 0x000e0000 && offset <= 0x000fffff)                   { return pc9801rs_ipl_r(space,offset & 0x1ffff);      }
	else if(offset >= 0x00100000 && offset <= 0x00100000+m_ram_size-1)      { return pc9801rs_ex_wram_r(space,offset-0x00100000); }
	else if(offset >= 0xfffe0000 && offset <= 0xffffffff)                   { return pc9801rs_ipl_r(space,offset & 0x1ffff);      }

//  printf("%08x\n",offset);
	return 0x00;
}


WRITE8_MEMBER(pc9801_state::pc9801rs_memory_w)
{
	if(m_gate_a20 == 0)
		offset &= 0xfffff;

	if     (                        offset <= 0x0009ffff)                   { pc9801rs_wram_w(space,offset,data);                  }
	else if(offset >= 0x000a0000 && offset <= 0x000a3fff)                   { pc9801_tvram_w(space,offset-0xa0000,data);           }
	else if(offset >= 0x000a4000 && offset <= 0x000a4fff)                   { pc9801rs_knjram_w(space,offset & 0xfff,data);        }
	else if(offset >= 0x000a8000 && offset <= 0x000affff)                   { m_pc9801rs_grcg_w(offset & 0x7fff,1,data);        }
	else if(offset >= 0x000b0000 && offset <= 0x000b7fff)                   { m_pc9801rs_grcg_w(offset & 0x7fff,2,data);        }
	else if(offset >= 0x000b8000 && offset <= 0x000bffff)                   { m_pc9801rs_grcg_w(offset & 0x7fff,3,data);        }
	else if(offset >= 0x000e0000 && offset <= 0x000e7fff)                   { m_pc9801rs_grcg_w(offset & 0x7fff,0,data);        }
	else if(offset >= 0x00100000 && offset <= 0x00100000+m_ram_size-1)      { pc9801rs_ex_wram_w(space,offset-0x00100000,data);    }
	//else
	//  printf("%08x %08x\n",offset,data);
}

READ8_MEMBER(pc9801_state::pc9810rs_fdc_ctrl_r)
{
	return (m_fdc_ctrl & 3) | 0xf0 | 8 | 4;
}

WRITE8_MEMBER(pc9801_state::pc9810rs_fdc_ctrl_w)
{
	/*
	---- x--- ready line?
	---- --x- select type (1) 2hd (0) 2dd
	---- ---x select irq
	*/

	machine().device<floppy_connector>("upd765_2hd:0")->get_device()->set_rpm(data & 0x02 ? 360 : 300);
	machine().device<floppy_connector>("upd765_2hd:1")->get_device()->set_rpm(data & 0x02 ? 360 : 300);

	machine().device<upd765a_device>("upd765_2hd")->set_rate(data & 0x02 ? 500000 : 250000);

	m_fdc_ctrl = data;
	//if(data & 0xfc)
	//  printf("FDC ctrl called with %02x\n",data);
}

READ8_MEMBER(pc9801_state::pc9801rs_2hd_r)
{
	if((offset & 1) == 0)
	{
		switch(offset & 6)
		{
			case 0: return machine().device<upd765a_device>("upd765_2hd")->msr_r(space, 0, 0xff);
			case 2: return machine().device<upd765a_device>("upd765_2hd")->fifo_r(space, 0, 0xff);
			case 4: return 0x44; //2hd flag
		}
	}

	printf("Read to undefined port [%02x]\n",offset+0x90);

	return 0xff;
}

WRITE8_MEMBER(pc9801_state::pc9801rs_2hd_w)
{
	if((offset & 1) == 0)
	{
		switch(offset & 6)
		{
			case 2: machine().device<upd765a_device>("upd765_2hd")->fifo_w(space, 0, data, 0xff); return;
			case 4:
				if(data & 0x80)
					machine().device<upd765a_device>("upd765_2hd")->reset();

				pc9801_fdc_2hd_update_ready(NULL, 0);

				machine().device<floppy_connector>("upd765_2hd:0")->get_device()->mon_w(data & 0x40 ? ASSERT_LINE : CLEAR_LINE);
				machine().device<floppy_connector>("upd765_2hd:1")->get_device()->mon_w(data & 0x40 ? ASSERT_LINE : CLEAR_LINE);

//              machine().device<floppy_connector>("upd765_2hd:0")->get_device()->mon_w(data & 0x08 ? ASSERT_LINE : CLEAR_LINE);
//              machine().device<floppy_connector>("upd765_2hd:1")->get_device()->mon_w(data & 0x08 ? ASSERT_LINE : CLEAR_LINE);
				return;
		}
	}

	printf("Write to undefined port [%02x] %02x\n",offset+0x90,data);
}

#if 0
READ8_MEMBER(pc9801_state::pc9801rs_2dd_r)
{
//  if(m_fdc_ctrl & 1)
//      return 0xff;

	if((offset & 1) == 0)
	{
		switch(offset & 6)
		{
			case 0: return machine().device<upd765a_device>("upd765_2hd")->msr_r(space, 0, 0xff);
			case 2: return machine().device<upd765a_device>("upd765_2hd")->fifo_r(space, 0, 0xff);
			case 4: return 0x44; //2dd flag
		}
	}

	printf("Read to undefined port [%02x]\n",offset+0x90);

	return 0xff;
}

WRITE8_MEMBER(pc9801_state::pc9801rs_2dd_w)
{
//  if(m_fdc_ctrl & 1)
//      return;

	if((offset & 1) == 0)
	{
		switch(offset & 6)
		{
			case 2: machine().device<upd765a_device>("upd765_2hd")->fifo_w(space, 0, data, 0xff); return;
			case 4: printf("%02x 2DD FDC ctrl\n",data); return;
		}
	}

	printf("Write to undefined port [%02x] %02x\n",offset+0x90,data);
}
#endif

WRITE8_MEMBER(pc9801_state::pc9801rs_video_ff_w)
{
	if(offset == 2)
	{
		if((data & 0xf0) == 0) /* disable any PC-9821 specific HW regs */
			m_ex_video_ff[(data & 0xfe) >> 1] = data & 1;

		if(0)
		{
			static const char *const ex_video_ff_regnames[] =
			{
				"16 colors mode",   // 0
				"<unknown>",        // 1
				"EGC related",      // 2
				"<unknown>"         // 3
			};

			printf("Write to extended video FF register %s -> %02x\n",ex_video_ff_regnames[(data & 0x06) >> 1],data & 1);
		}
		//else
		//  printf("Write to extended video FF register %02x\n",data);

		return;
	}

	pc9801_video_ff_w(space,offset,data);
}

WRITE8_MEMBER(pc9801_state::pc9801rs_a0_w)
{
	if((offset & 1) == 0 && offset & 8 && m_ex_video_ff[ANALOG_16_MODE])
	{
		switch(offset)
		{
			case 0x08: m_analog16.pal_entry = data & 0xf; break;
			case 0x0a: m_analog16.g[m_analog16.pal_entry] = data & 0xf; break;
			case 0x0c: m_analog16.r[m_analog16.pal_entry] = data & 0xf; break;
			case 0x0e: m_analog16.b[m_analog16.pal_entry] = data & 0xf; break;
		}

		m_palette->set_pen_color((m_analog16.pal_entry)+0x10,
												pal4bit(m_analog16.r[m_analog16.pal_entry]),
												pal4bit(m_analog16.g[m_analog16.pal_entry]),
												pal4bit(m_analog16.b[m_analog16.pal_entry]));
		return;
	}

	pc9801_a0_w(space,offset,data);
}

READ8_MEMBER( pc9801_state::pc9801rs_access_ctrl_r )
{
	if(offset == 1)
		return m_access_ctrl;

	return 0xff;
}

WRITE8_MEMBER( pc9801_state::pc9801rs_access_ctrl_w )
{
	if(offset == 1)
		m_access_ctrl = data;
}

WRITE8_MEMBER( pc9801_state::pc9801rs_mouse_freq_w )
{
	/* TODO: bit 3 used */
	if(offset == 3)
	{
		m_mouse.freq_reg = data & 3;
		m_mouse.freq_index = 0;
	}
}


WRITE8_MEMBER( pc9801_state::pc9801rs_nmi_w )
{
	if(offset == 0)
		m_nmi_enable = 0;

	if(offset == 2)
		m_nmi_enable = 1;
}

READ8_MEMBER( pc9801_state::pc9801rs_midi_r )
{
	/* unconnect, needed by Amaranth KH to boot */
	return 0xff;
}

READ8_MEMBER(pc9801_state::pc9801rs_pit_mirror_r)
{
	if((offset & 1) == 0)
	{
		printf("Read to undefined port [%04x]\n",offset+0x3fd8);
		return 0xff;
	}
	else // odd
	{
		if(offset & 0x08)
			printf("Read to undefined port [%02x]\n",offset+0x3fd8);
		else
			return m_pit8253->read(space, (offset & 6) >> 1);
	}

	return 0xff;
}

WRITE8_MEMBER(pc9801_state::pc9801rs_pit_mirror_w)
{
	if((offset & 1) == 0)
	{
		printf("Write to undefined port [%04x] <- %02x\n",offset+0x3fd8,data);
	}
	else // odd
	{
		if(offset < 0x08)
			m_pit8253->write(space, (offset & 6) >> 1, data);
		else
			printf("Write to undefined port [%04x] <- %02x\n",offset+0x3fd8,data);
	}
}

READ8_MEMBER(pc9801_state::pc9801rs_ide_io_0_r)
{
	printf("IDE r %02x\n",offset);
	return m_ide_bank[offset];
}

WRITE8_MEMBER(pc9801_state::pc9801rs_ide_io_0_w)
{
	/*
	[0x430]
	[Read/write]
	    bit 7-0: unknown
	    00 h = IDE Bank # 1
	    01 h = IDE Bank # 2

	[0x432]
	    bit 7-0: Bank select
	    80 h = readout for dummy (only [WRITE])
	    00 h = IDE Bank # 1 choice
	    01 h = IDE Bank # 2 selection
	*/

	printf("IDE w %02x %02x\n",offset,data);

	if ((data & 0x80) == 0x00)
		m_ide_bank[offset] = data & 0x7f;
}

/* TODO: is mapping correct? */
READ16_MEMBER(pc9801_state::pc9801rs_ide_io_1_r)
{
	return m_ide->read_cs0(space, offset, mem_mask);
}

WRITE16_MEMBER(pc9801_state::pc9801rs_ide_io_1_w)
{
	m_ide->write_cs0(space, offset, data, mem_mask);
}

READ16_MEMBER(pc9801_state::pc9801rs_ide_io_2_r)
{
	return m_ide->read_cs1(space, offset + 6, mem_mask);
}

WRITE16_MEMBER(pc9801_state::pc9801rs_ide_io_2_w)
{
	m_ide->write_cs1(space, offset + 6, data, mem_mask);
}

static ADDRESS_MAP_START( pc9801rs_map, AS_PROGRAM, 32, pc9801_state )
	AM_RANGE(0x00000000, 0xffffffff) AM_READWRITE8(pc9801rs_memory_r,pc9801rs_memory_w,0xffffffff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc9801rs_io, AS_IO, 32, pc9801_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x001f) AM_READWRITE8(pc9801_00_r,        pc9801_00_w,        0xffffffff) // i8259 PIC (bit 3 ON slave / master) / i8237 DMA
	AM_RANGE(0x0020, 0x0027) AM_READWRITE8(pc9801_20_r,        pc9801_20_w,        0xffffffff) // RTC / DMA registers (LS244)
	AM_RANGE(0x0030, 0x0037) AM_READWRITE8(pc9801rs_30_r,      pc9801_30_w,        0xffffffff) //i8251 RS232c / i8255 system port
	AM_RANGE(0x0040, 0x0047) AM_READWRITE8(pc9801_40_r,        pc9801_40_w,        0xffffffff) //i8255 printer port / i8251 keyboard
	AM_RANGE(0x0050, 0x0053) AM_WRITE8(pc9801rs_nmi_w, 0xffffffff)
	AM_RANGE(0x005c, 0x005f) AM_READ(pc9821_timestamp_r) AM_WRITENOP // artic
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(pc9801_60_r,        pc9801_60_w,        0xffffffff) //upd7220 character ports / <undefined>
	AM_RANGE(0x0064, 0x0067) AM_WRITE8(pc9801_vrtc_mask_w, 0xffffffff)
	AM_RANGE(0x0068, 0x006b) AM_WRITE8(pc9801rs_video_ff_w,0xffffffff) //mode FF / <undefined>
	AM_RANGE(0x0070, 0x007f) AM_READWRITE8(pc9801rs_70_r,      pc9801rs_70_w,      0xffffffff) //display registers "GRCG" / i8253 pit
	AM_RANGE(0x0080, 0x0083) AM_READWRITE8(pc9801_sasi_r,      pc9801_sasi_w,      0xffffffff) //HDD SASI interface / <undefined>
	AM_RANGE(0x0090, 0x0097) AM_READWRITE8(pc9801rs_2hd_r,     pc9801rs_2hd_w,     0xffffffff)
	AM_RANGE(0x00a0, 0x00af) AM_READWRITE8(pc9801_a0_r,        pc9801rs_a0_w,      0xffffffff) //upd7220 bitmap ports / display registers
	AM_RANGE(0x00bc, 0x00bf) AM_READWRITE8(pc9810rs_fdc_ctrl_r,pc9810rs_fdc_ctrl_w,0xffffffff)
	AM_RANGE(0x00c8, 0x00cf) AM_READWRITE8(pc9801rs_2hd_r,     pc9801rs_2hd_w,     0xffffffff)
//  AM_RANGE(0x00ec, 0x00ef) PC-9801-86 sound board
	AM_RANGE(0x00f0, 0x00ff) AM_READWRITE8(pc9801rs_f0_r,      pc9801rs_f0_w,      0xffffffff)
//  AM_RANGE(0x0188, 0x018f) AM_READWRITE8(pc9801_opn_r,       pc9801_opn_w,       0xffffffff) //ym2203 opn / <undefined>
	AM_RANGE(0x0430, 0x0433) AM_READWRITE8(pc9801rs_ide_io_0_r,  pc9801rs_ide_io_0_w,0x00ff00ff)

	AM_RANGE(0x0438, 0x043b) AM_READWRITE8(pc9801rs_access_ctrl_r,pc9801rs_access_ctrl_w,0xffffffff)
	AM_RANGE(0x043c, 0x043f) AM_WRITE8(pc9801rs_bank_w,    0xffffffff) //ROM/RAM bank

	AM_RANGE(0x0640, 0x064f) AM_READWRITE16(pc9801rs_ide_io_1_r,  pc9801rs_ide_io_1_w,0xffffffff)
	AM_RANGE(0x074c, 0x074f) AM_READWRITE16(pc9801rs_ide_io_2_r,  pc9801rs_ide_io_2_w,0xffffffff)

	AM_RANGE(0x3fd8, 0x3fdf) AM_READWRITE8(pc9801rs_pit_mirror_r,        pc9801rs_pit_mirror_w,        0xffffffff) // <undefined> / pit mirror ports
	AM_RANGE(0x7fd8, 0x7fdf) AM_READWRITE8(pc9801_mouse_r,     pc9801_mouse_w,     0xffffffff) // <undefined> / mouse ppi8255 ports
//  AM_RANGE(0xa460, 0xa463) AM_READWRITE8(pc9801_ext_opna_r,  pc9801_ext_opna_w,  0xffffffff)
	AM_RANGE(0xbfd8, 0xbfdf) AM_WRITE8(pc9801rs_mouse_freq_w, 0xffffffff)
	AM_RANGE(0xe0d0, 0xe0d3) AM_READ8(pc9801rs_midi_r, 0xffffffff)
ADDRESS_MAP_END

READ8_MEMBER(pc9801_state::pic_r)
{
	return ((offset >= 4) ? m_pic2 : m_pic1)->read(space, offset & 3);
}

WRITE8_MEMBER(pc9801_state::pic_w)
{
	((offset >= 4) ? m_pic2 : m_pic1)->write(space, offset & 3, data);
}

READ8_MEMBER(pc9801_state::pc9801ux_gvram_r)
{
	return m_pc9801rs_grcg_r(offset & 0x7fff,(offset>>15)+1);
}

WRITE8_MEMBER(pc9801_state::pc9801ux_gvram_w)
{
	m_pc9801rs_grcg_w(offset & 0x7fff,(offset>>15)+1,data);
}

READ8_MEMBER(pc9801_state::pc9801ux_gvram0_r)
{
	return m_pc9801rs_grcg_r(offset & 0x7fff,0);
}

WRITE8_MEMBER(pc9801_state::pc9801ux_gvram0_w)
{
	m_pc9801rs_grcg_w(offset & 0x7fff,0,data);
}

static ADDRESS_MAP_START( pc9801ux_map, AS_PROGRAM, 16, pc9801_state )
	AM_RANGE(0x000000, 0x09ffff) AM_RAMBANK("wram")
	AM_RANGE(0x0a0000, 0x0a3fff) AM_READWRITE8(pc9801_tvram_r, pc9801_tvram_w, 0xffff)
	AM_RANGE(0x0a4000, 0x0a4fff) AM_READWRITE8(pc9801rs_knjram_r, pc9801rs_knjram_w, 0xffff)
	AM_RANGE(0x0a8000, 0x0bffff) AM_READWRITE8(pc9801ux_gvram_r, pc9801ux_gvram_w, 0xffff)
	AM_RANGE(0x0e0000, 0x0e7fff) AM_READWRITE8(pc9801ux_gvram0_r,pc9801ux_gvram0_w, 0xffff)
	AM_RANGE(0x0e0000, 0x0fffff) AM_READ8(pc9801rs_ipl_r, 0xffff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc9801ux_io, AS_IO, 16, pc9801_state )
	ADDRESS_MAP_UNMAP_HIGH

	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("i8237", am9517a_device, read, write, 0xff00)
	AM_RANGE(0x0000, 0x000f) AM_READWRITE8(pic_r, pic_w, 0x00ff) // i8259 PIC (bit 3 ON slave / master) / i8237 DMA
	AM_RANGE(0x0020, 0x0027) AM_READWRITE8(pc9801_20_r,        pc9801_20_w,        0xffff) // RTC / DMA registers (LS244)
	AM_RANGE(0x0030, 0x0037) AM_DEVREADWRITE8("ppi8255_sys", i8255_device, read, write, 0xff00)
	AM_RANGE(0x0030, 0x0037) AM_READWRITE8(pc9801rs_30_r,      pc9801_30_w,        0xffff) //i8251 RS232c / i8255 system port
	AM_RANGE(0x0040, 0x0047) AM_DEVREADWRITE8("ppi8255_prn", i8255_device, read, write, 0x00ff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8("keyb", pc9801_kbd_device, rx_r, tx_w, 0xff00) //i8255 printer port / i8251 keyboard
	AM_RANGE(0x005c, 0x005f) AM_WRITENOP // time-stamp?
	AM_RANGE(0x0060, 0x0063) AM_DEVREADWRITE8("upd7220_chr", upd7220_device, read, write, 0x00ff) //upd7220 character ports / <undefined>
	AM_RANGE(0x0064, 0x0067) AM_WRITE8(pc9801_vrtc_mask_w, 0xffff)
	AM_RANGE(0x0068, 0x006b) AM_WRITE8(pc9801rs_video_ff_w,0xffff) //mode FF / <undefined>
	AM_RANGE(0x0070, 0x007f) AM_DEVREADWRITE8("pit8253", pit8253_device, read, write, 0xff00)
	AM_RANGE(0x0070, 0x007f) AM_READWRITE8(pc9801rs_70_r,      pc9801rs_70_w,      0xffff) //display registers "GRCG" / i8253 pit
	AM_RANGE(0x0090, 0x0097) AM_READWRITE8(pc9801rs_2hd_r,     pc9801rs_2hd_w,     0xffff)
	AM_RANGE(0x00a0, 0x00af) AM_READWRITE8(pc9801_a0_r,        pc9801rs_a0_w,      0xffff) //upd7220 bitmap ports / display registers
	AM_RANGE(0x00bc, 0x00bf) AM_READWRITE8(pc9810rs_fdc_ctrl_r,pc9810rs_fdc_ctrl_w,0xffff)
	AM_RANGE(0x00c8, 0x00cf) AM_READWRITE8(pc9801rs_2hd_r,     pc9801rs_2hd_w,     0xffff)
	AM_RANGE(0x00f0, 0x00ff) AM_READWRITE8(pc9801rs_f0_r,      pc9801rs_f0_w,      0xffff)
//  AM_RANGE(0x0188, 0x018f) AM_READWRITE8(pc9801_opn_r,       pc9801_opn_w,       0xffff) //ym2203 opn / <undefined>
	AM_RANGE(0x0438, 0x043b) AM_READWRITE8(pc9801rs_access_ctrl_r,pc9801rs_access_ctrl_w,0xffff)
	AM_RANGE(0x043c, 0x043f) AM_WRITE8(pc9801rs_bank_w,    0xffff) //ROM/RAM bank
	AM_RANGE(0x3fd8, 0x3fdf) AM_DEVREADWRITE8("pit8253", pit8253_device, read, write, 0xff00)
	AM_RANGE(0x7fd8, 0x7fdf) AM_DEVREADWRITE8("ppi8255_mouse", i8255_device, read, write, 0xff00)
//  AM_RANGE(0xa460, 0xa463) AM_READWRITE8(pc9801_ext_opna_r,  pc9801_ext_opna_w,  0xffff)

ADDRESS_MAP_END

static ADDRESS_MAP_START( pc9801bx2_io, AS_IO, 32, pc9801_state )
	AM_IMPORT_FROM(pc9801rs_io)

	AM_RANGE(0x841c, 0x841f) AM_READWRITE8(sdip_0_r,sdip_0_w,0xffffffff)
	AM_RANGE(0x851c, 0x851f) AM_READWRITE8(sdip_1_r,sdip_1_w,0xffffffff)
	AM_RANGE(0x861c, 0x861f) AM_READWRITE8(sdip_2_r,sdip_2_w,0xffffffff)
	AM_RANGE(0x871c, 0x871f) AM_READWRITE8(sdip_3_r,sdip_3_w,0xffffffff)
	AM_RANGE(0x881c, 0x881f) AM_READWRITE8(sdip_4_r,sdip_4_w,0xffffffff)
	AM_RANGE(0x891c, 0x891f) AM_READWRITE8(sdip_5_r,sdip_5_w,0xffffffff)
	AM_RANGE(0x8a1c, 0x8a1f) AM_READWRITE8(sdip_6_r,sdip_6_w,0xffffffff)
	AM_RANGE(0x8b1c, 0x8b1f) AM_READWRITE8(sdip_7_r,sdip_7_w,0xffffffff)
	AM_RANGE(0x8c1c, 0x8c1f) AM_READWRITE8(sdip_8_r,sdip_8_w,0xffffffff)
	AM_RANGE(0x8d1c, 0x8d1f) AM_READWRITE8(sdip_9_r,sdip_9_w,0xffffffff)
	AM_RANGE(0x8e1c, 0x8e1f) AM_READWRITE8(sdip_a_r,sdip_a_w,0xffffffff)
	AM_RANGE(0x8f1c, 0x8f1f) AM_READWRITE8(sdip_b_r,sdip_b_w,0xffffffff)
ADDRESS_MAP_END

/*************************************
 *
 * PC-9821 specific handlers
 *
 ************************************/

/* Note: not hooking this up causes "MEMORY ERROR" at POST */
READ8_MEMBER(pc9801_state::pc9821_ideram_r) { return m_ide_ram[offset]; }
WRITE8_MEMBER(pc9801_state::pc9821_ideram_w) { m_ide_ram[offset] = data; }

READ8_MEMBER(pc9801_state::pc9821_ext_gvram_r) { return m_ext_gvram[offset]; }
WRITE8_MEMBER(pc9801_state::pc9821_ext_gvram_w) { m_ext_gvram[offset] = data; }


READ8_MEMBER(pc9801_state::pc9821_memory_r)
{
	if(m_gate_a20 == 0)
		offset &= 0xfffff;

	if(offset >= 0x00080000 && offset <= 0x0009ffff)
		offset = (offset & 0x1ffff) | (m_pc9821_window_bank & 0xfe) * 0x10000;

	/* TODO: window bank at 0xa0000 - 0xbffff */

	if     (                        offset <= 0x0009ffff)                   { return pc9801rs_wram_r(space,offset);               }
//  else if(offset >= 0x00080000 && offset <= 0x0009ffff)                   { return pc9821_winram_r(space,offset & 0x1ffff);     }
	else if(offset >= 0x000a0000 && offset <= 0x000a3fff)                   { return pc9801_tvram_r(space,offset-0xa0000);        }
	else if(offset >= 0x000a4000 && offset <= 0x000a4fff)                   { return pc9801rs_knjram_r(space,offset & 0xfff);     }
	else if(offset >= 0x000a8000 && offset <= 0x000affff)                   { return m_pc9801rs_grcg_r(offset & 0x7fff,1);        }
	else if(offset >= 0x000b0000 && offset <= 0x000b7fff)                   { return m_pc9801rs_grcg_r(offset & 0x7fff,2);        }
	else if(offset >= 0x000b8000 && offset <= 0x000bffff)                   { return m_pc9801rs_grcg_r(offset & 0x7fff,3);        }
	else if(offset >= 0x000cc000 && offset <= 0x000cffff)                   { return pc9801rs_soundrom_r(space,offset & 0x3fff);}
	else if(offset >= 0x000d8000 && offset <= 0x000d9fff)                   { return pc9801rs_ide_r(space,offset & 0x1fff);         }
	else if(offset >= 0x000da000 && offset <= 0x000dbfff)                   { return pc9821_ideram_r(space,offset & 0x1fff);      }
	else if(offset >= 0x000e0000 && offset <= 0x000e7fff)                   { return m_pc9801rs_grcg_r(offset & 0x7fff,0);        }
	else if(offset >= 0x000e0000 && offset <= 0x000fffff)                   { return pc9801rs_ipl_r(space,offset & 0x1ffff);      }
	else if(offset >= 0x00100000 && offset <= 0x00100000+m_ram_size-1)      { return pc9801rs_ex_wram_r(space,offset-0x00100000); }
	else if(offset >= 0x00f00000 && offset <= 0x00f9ffff)                   { return pc9821_ext_gvram_r(space,offset-0x00f00000); }
	else if(offset >= 0xfffe0000 && offset <= 0xffffffff)                   { return pc9801rs_ipl_r(space,offset & 0x1ffff);      }

	//printf("%08x\n",offset);
	return 0x00;
}


WRITE8_MEMBER(pc9801_state::pc9821_memory_w)
{
	if(m_gate_a20 == 0)
		offset &= 0xfffff;

	if(offset >= 0x00080000 && offset <= 0x0009ffff)
		offset = (offset & 0x1ffff) | (m_pc9821_window_bank & 0xfe) * 0x10000;

	/* TODO: window bank at 0xa0000 - 0xbffff */

	if     (                        offset <= 0x0009ffff)                   { pc9801rs_wram_w(space,offset,data);                  }
//  else if(offset >= 0x00080000 && offset <= 0x0009ffff)                   { pc9821_winram_w(space,offset & 0x1ffff,data);        }
	else if(offset >= 0x000a0000 && offset <= 0x000a3fff)                   { pc9801_tvram_w(space,offset-0xa0000,data);           }
	else if(offset >= 0x000a4000 && offset <= 0x000a4fff)                   { pc9801rs_knjram_w(space,offset & 0xfff,data);        }
	else if(offset >= 0x000a8000 && offset <= 0x000affff)                   { m_pc9801rs_grcg_w(offset & 0x7fff,1,data);        }
	else if(offset >= 0x000b0000 && offset <= 0x000b7fff)                   { m_pc9801rs_grcg_w(offset & 0x7fff,2,data);        }
	else if(offset >= 0x000b8000 && offset <= 0x000bffff)                   { m_pc9801rs_grcg_w(offset & 0x7fff,3,data);        }
	else if(offset >= 0x000cc000 && offset <= 0x000cffff)                   { /* TODO: shadow ROM */                               }
	else if(offset >= 0x000d8000 && offset <= 0x000d9fff)                   { /* TODO: shadow ROM */                               }
	else if(offset >= 0x000da000 && offset <= 0x000dbfff)                   { pc9821_ideram_w(space,offset & 0x1fff,data);         }
	else if(offset >= 0x000e0000 && offset <= 0x000e7fff)                   { m_pc9801rs_grcg_w(offset & 0x7fff,0,data);        }
	else if(offset >= 0x000e8000 && offset <= 0x000fffff)                   { /* TODO: shadow ROM */                               }
	else if(offset >= 0x00100000 && offset <= 0x00100000+m_ram_size-1)      { pc9801rs_ex_wram_w(space,offset-0x00100000,data);    }
	else if(offset >= 0x00f00000 && offset <= 0x00f9ffff)                   { pc9821_ext_gvram_w(space,offset-0x00f00000,data);    }
	//else
	//  printf("%08x %08x\n",offset,data);

}

WRITE8_MEMBER(pc9801_state::pc9821_video_ff_w)
{
	if(offset == 2)
	{
		m_ex_video_ff[(data & 0xfe) >> 1] = data & 1;

		//if((data & 0xfe) == 0x20)
		//  printf("%02x\n",data & 1);
	}

	/* Intentional fall-through */
	pc9801rs_video_ff_w(space,offset,data);
}

READ8_MEMBER(pc9801_state::pc9821_a0_r)
{
	if((offset & 1) == 0 && offset & 8)
	{
		if(m_ex_video_ff[ANALOG_256_MODE])
		{
			printf("256 color mode [%02x] R\n",offset);
			return 0;
		}
		else if(m_ex_video_ff[ANALOG_16_MODE]) //16 color mode, readback possible there
		{
			UINT8 res = 0;

			switch(offset)
			{
				case 0x08: res = m_analog16.pal_entry & 0xf; break;
				case 0x0a: res = m_analog16.g[m_analog16.pal_entry] & 0xf; break;
				case 0x0c: res = m_analog16.r[m_analog16.pal_entry] & 0xf; break;
				case 0x0e: res = m_analog16.b[m_analog16.pal_entry] & 0xf; break;
			}

			return res;
		}
	}

	return pc9801_a0_r(space,offset);
}

WRITE8_MEMBER(pc9801_state::pc9821_a0_w)
{
	if((offset & 1) == 0 && offset & 8 && m_ex_video_ff[ANALOG_256_MODE])
	{
		switch(offset)
		{
			case 0x08: m_analog256.pal_entry = data & 0xff; break;
			case 0x0a: m_analog256.g[m_analog256.pal_entry] = data & 0xff; break;
			case 0x0c: m_analog256.r[m_analog256.pal_entry] = data & 0xff; break;
			case 0x0e: m_analog256.b[m_analog256.pal_entry] = data & 0xff; break;
		}

		m_palette->set_pen_color((m_analog256.pal_entry)+0x20,
												m_analog256.r[m_analog256.pal_entry],
												m_analog256.g[m_analog256.pal_entry],
												m_analog256.b[m_analog256.pal_entry]);
		return;
	}

	pc9801rs_a0_w(space,offset,data);
}

READ8_MEMBER(pc9801_state::pc9821_window_bank_r)
{
	if(offset == 1)
		return m_pc9821_window_bank & 0xfe;

	return 0xff;
}

WRITE8_MEMBER(pc9801_state::pc9821_window_bank_w)
{
	if(offset == 1)
		m_pc9821_window_bank = data & 0xfe;
	else
		printf("PC-9821 $f0000 window bank %02x\n",data);
}

UINT8 pc9801_state::m_sdip_read(UINT16 port, UINT8 sdip_offset)
{
	if(port == 2)
		return m_sdip[sdip_offset];

	printf("Warning: read from unknown SDIP area %02x %04x\n",port,0x841c + port + (sdip_offset % 12)*0x100);
	return 0xff;
}

void pc9801_state::m_sdip_write(UINT16 port, UINT8 sdip_offset,UINT8 data)
{
	if(port == 2)
	{
		m_sdip[sdip_offset] = data;
		return;
	}

	printf("Warning: write from unknown SDIP area %02x %04x %02x\n",port,0x841c + port + (sdip_offset % 12)*0x100,data);
}

READ8_MEMBER(pc9801_state::sdip_0_r) { return m_sdip_read(offset, 0+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_1_r) { return m_sdip_read(offset, 1+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_2_r) { return m_sdip_read(offset, 2+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_3_r) { return m_sdip_read(offset, 3+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_4_r) { return m_sdip_read(offset, 4+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_5_r) { return m_sdip_read(offset, 5+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_6_r) { return m_sdip_read(offset, 6+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_7_r) { return m_sdip_read(offset, 7+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_8_r) { return m_sdip_read(offset, 8+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_9_r) { return m_sdip_read(offset, 9+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_a_r) { return m_sdip_read(offset, 10+m_sdip_bank*12); }
READ8_MEMBER(pc9801_state::sdip_b_r) { return m_sdip_read(offset, 11+m_sdip_bank*12); }

WRITE8_MEMBER(pc9801_state::sdip_0_w) { m_sdip_write(offset,0+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_1_w) { m_sdip_write(offset,1+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_2_w) { m_sdip_write(offset,2+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_3_w) { m_sdip_write(offset,3+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_4_w) { m_sdip_write(offset,4+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_5_w) { m_sdip_write(offset,5+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_6_w) { m_sdip_write(offset,6+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_7_w) { m_sdip_write(offset,7+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_8_w) { m_sdip_write(offset,8+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_9_w) { m_sdip_write(offset,9+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_a_w) { m_sdip_write(offset,10+m_sdip_bank*12,data); }
WRITE8_MEMBER(pc9801_state::sdip_b_w)
{
	if(offset == 3)
		m_sdip_bank = (data & 0x40) >> 6;

	if(offset == 2)
		m_sdip_write(offset,11+m_sdip_bank*12,data);

	if((offset & 2) == 0)
		printf("SDIP area B write %02x %02x\n",offset,data);
}

READ32_MEMBER(pc9801_state::pc9821_timestamp_r)
{
	return m_maincpu->total_cycles();
}

/* basically a read-back of various registers */
READ8_MEMBER(pc9801_state::pc9821_ext2_video_ff_r)
{
	UINT8 res;

	if(offset)
		return 0xff;

	res = 0;

	switch(m_ext2_ff)
	{
		case 3: res = m_video_ff[DISPLAY_REG]; break; // display reg
		default:
			printf("PC-9821: read ext2 f/f with value %02x\n",m_ext2_ff);
	}

	return res;
}

WRITE8_MEMBER(pc9801_state::pc9821_ext2_video_ff_w)
{
	if(offset == 0)
		m_ext2_ff = data;
}

static ADDRESS_MAP_START( pc9821_map, AS_PROGRAM, 32, pc9801_state )
	AM_RANGE(0x00000000, 0xffffffff) AM_READWRITE8(pc9821_memory_r,pc9821_memory_w,0xffffffff)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc9821_io, AS_IO, 32, pc9801_state )
//  ADDRESS_MAP_UNMAP_HIGH // TODO: a read to somewhere makes this to fail at POST
	AM_RANGE(0x0000, 0x001f) AM_READWRITE8(pc9801_00_r,        pc9801_00_w,        0xffffffff) // i8259 PIC (bit 3 ON slave / master) / i8237 DMA
	AM_RANGE(0x0020, 0x0027) AM_READWRITE8(pc9801_20_r,        pc9801_20_w,        0xffffffff) // RTC / DMA registers (LS244)
	AM_RANGE(0x0030, 0x0037) AM_READWRITE8(pc9801rs_30_r,      pc9801_30_w,        0xffffffff) //i8251 RS232c / i8255 system port
	AM_RANGE(0x0040, 0x0047) AM_READWRITE8(pc9801_40_r,        pc9801_40_w,        0xffffffff) //i8255 printer port / i8251 keyboard
	AM_RANGE(0x0050, 0x0053) AM_WRITE8(pc9801rs_nmi_w, 0xffffffff)
	AM_RANGE(0x005c, 0x005f) AM_READ(pc9821_timestamp_r) AM_WRITENOP // artic
	AM_RANGE(0x0060, 0x0063) AM_READWRITE8(pc9801_60_r,        pc9801_60_w,        0xffffffff) //upd7220 character ports / <undefined>
	AM_RANGE(0x0064, 0x0067) AM_WRITE8(pc9801_vrtc_mask_w, 0xffffffff)
	AM_RANGE(0x0068, 0x006b) AM_WRITE8(pc9821_video_ff_w,  0xffffffff) //mode FF / <undefined>
	AM_RANGE(0x0070, 0x007f) AM_READWRITE8(pc9801rs_70_r,      pc9801rs_70_w,    0xffffffff) //display registers "GRCG" / i8253 pit
	AM_RANGE(0x0080, 0x0083) AM_READWRITE8(pc9801_sasi_r,      pc9801_sasi_w,      0xffffffff) //HDD SASI interface / <undefined>
	AM_RANGE(0x0090, 0x0097) AM_READWRITE8(pc9801rs_2hd_r,     pc9801rs_2hd_w,     0xffffffff)
	AM_RANGE(0x00a0, 0x00af) AM_READWRITE8(pc9821_a0_r,        pc9821_a0_w,        0xffffffff) //upd7220 bitmap ports / display registers
//  AM_RANGE(0x00b0, 0x00b3) PC9861k (serial port?)
//  AM_RANGE(0x00b9, 0x00b9) PC9861k
//  AM_RANGE(0x00bb, 0x00bb) PC9861k
	AM_RANGE(0x00bc, 0x00bf) AM_READWRITE8(pc9810rs_fdc_ctrl_r,pc9810rs_fdc_ctrl_w,0xffffffff)
	AM_RANGE(0x00c8, 0x00cf) AM_READWRITE8(pc9801rs_2hd_r,     pc9801rs_2hd_w,     0xffffffff)
//  AM_RANGE(0x00d8, 0x00df) AMD98 (sound?) board
	AM_RANGE(0x00f0, 0x00ff) AM_READWRITE8(pc9801rs_f0_r,      pc9801rs_f0_w,      0xffffffff)
//  AM_RANGE(0x0188, 0x018f) AM_READWRITE8(pc9801_opn_r,       pc9801_opn_w,       0xffffffff) //ym2203 opn / <undefined>
//  AM_RANGE(0x018c, 0x018f) YM2203 OPN extended ports / <undefined>
	AM_RANGE(0x0430, 0x0433) AM_READWRITE8(pc9801rs_ide_io_0_r,  pc9801rs_ide_io_0_w,0x00ff00ff) // IDE bank register
	AM_RANGE(0x0438, 0x043b) AM_READWRITE8(pc9801rs_access_ctrl_r,pc9801rs_access_ctrl_w,0xffffffff)
//  AM_RANGE(0x043d, 0x043d) ROM/RAM bank (NEC)
	AM_RANGE(0x043c, 0x043f) AM_WRITE8(pc9801rs_bank_w,    0xffffffff) //ROM/RAM bank (EPSON)
	AM_RANGE(0x0460, 0x0463) AM_READWRITE8(pc9821_window_bank_r,pc9821_window_bank_w, 0xffffffff)
//  AM_RANGE(0x04a0, 0x04af) EGC
//  AM_RANGE(0x04be, 0x04be) FDC "RPM" register
	AM_RANGE(0x0640, 0x064f) AM_READWRITE16(pc9801rs_ide_io_1_r,  pc9801rs_ide_io_1_w,0xffffffff) // IDE registers / <undefined>
	AM_RANGE(0x074c, 0x074f) AM_READWRITE16(pc9801rs_ide_io_2_r,  pc9801rs_ide_io_2_w,0xffffffff) // IDE status (r) - IDE control registers (w) / <undefined>
//  AM_RANGE(0x08e0, 0x08ea) <undefined> / EMM SIO registers
	AM_RANGE(0x09a0, 0x09a3) AM_READWRITE8(pc9821_ext2_video_ff_r, pc9821_ext2_video_ff_w, 0xffffffff) // GDC extended register r/w
//  AM_RANGE(0x09a8, 0x09a8) GDC 31KHz register r/w
//  AM_RANGE(0x0c07, 0x0c07) EPSON register w
//  AM_RANGE(0x0c03, 0x0c03) EPSON register 0 r
//  AM_RANGE(0x0c13, 0x0c14) EPSON register 1 r
//  AM_RANGE(0x0c24, 0x0c24) cs4231 PCM board register control
//  AM_RANGE(0x0c2b, 0x0c2b) cs4231 PCM board low byte control
//  AM_RANGE(0x0c2d, 0x0c2d) cs4231 PCM board hi byte control
//  AM_RANGE(0x0cc0, 0x0cc7) SCSI interface / <undefined>
//  AM_RANGE(0x0cfc, 0x0cff) PCI bus
	AM_RANGE(0x3fd8, 0x3fdf) AM_READWRITE8(pc9801rs_pit_mirror_r,        pc9801rs_pit_mirror_w,        0xffffffff) // <undefined> / pit mirror ports
	AM_RANGE(0x7fd8, 0x7fdf) AM_READWRITE8(pc9801_mouse_r,      pc9801_mouse_w,      0xffffffff) // <undefined> / mouse ppi8255 ports
	AM_RANGE(0x841c, 0x841f) AM_READWRITE8(sdip_0_r,sdip_0_w,0xffffffff)
	AM_RANGE(0x851c, 0x851f) AM_READWRITE8(sdip_1_r,sdip_1_w,0xffffffff)
	AM_RANGE(0x861c, 0x861f) AM_READWRITE8(sdip_2_r,sdip_2_w,0xffffffff)
	AM_RANGE(0x871c, 0x871f) AM_READWRITE8(sdip_3_r,sdip_3_w,0xffffffff)
	AM_RANGE(0x881c, 0x881f) AM_READWRITE8(sdip_4_r,sdip_4_w,0xffffffff)
	AM_RANGE(0x891c, 0x891f) AM_READWRITE8(sdip_5_r,sdip_5_w,0xffffffff)
	AM_RANGE(0x8a1c, 0x8a1f) AM_READWRITE8(sdip_6_r,sdip_6_w,0xffffffff)
	AM_RANGE(0x8b1c, 0x8b1f) AM_READWRITE8(sdip_7_r,sdip_7_w,0xffffffff)
	AM_RANGE(0x8c1c, 0x8c1f) AM_READWRITE8(sdip_8_r,sdip_8_w,0xffffffff)
	AM_RANGE(0x8d1c, 0x8d1f) AM_READWRITE8(sdip_9_r,sdip_9_w,0xffffffff)
	AM_RANGE(0x8e1c, 0x8e1f) AM_READWRITE8(sdip_a_r,sdip_a_w,0xffffffff)
	AM_RANGE(0x8f1c, 0x8f1f) AM_READWRITE8(sdip_b_r,sdip_b_w,0xffffffff)
//  AM_RANGE(0xa460, 0xa463) AM_READWRITE8(pc9801_ext_opna_r,  pc9801_ext_opna_w,  0xffffffff)
//  AM_RANGE(0xa460, 0xa46f) cs4231 PCM extended port / <undefined>
//  AM_RANGE(0xbfdb, 0xbfdb) mouse timing port
//  AM_RANGE(0xc0d0, 0xc0d3) MIDI port, option 0 / <undefined>
//  AM_RANGE(0xc4d0, 0xc4d3) MIDI port, option 1 / <undefined>
//  AM_RANGE(0xc8d0, 0xc8d3) MIDI port, option 2 / <undefined>
//  AM_RANGE(0xccd0, 0xccd3) MIDI port, option 3 / <undefined>
//  AM_RANGE(0xd0d0, 0xd0d3) MIDI port, option 4 / <undefined>
//  AM_RANGE(0xd4d0, 0xd4d3) MIDI port, option 5 / <undefined>
//  AM_RANGE(0xd8d0, 0xd8d3) MIDI port, option 6 / <undefined>
//  AM_RANGE(0xdcd0, 0xdcd3) MIDI port, option 7 / <undefined>
	AM_RANGE(0xe0d0, 0xe0d3) AM_READ8(pc9801rs_midi_r, 0xffffffff) // MIDI port, option 8 / <undefined>
//  AM_RANGE(0xe4d0, 0xe4d3) MIDI port, option 9 / <undefined>
//  AM_RANGE(0xe8d0, 0xe8d3) MIDI port, option A / <undefined>
//  AM_RANGE(0xecd0, 0xecd3) MIDI port, option B / <undefined>
//  AM_RANGE(0xf0d0, 0xf0d3) MIDI port, option C / <undefined>
//  AM_RANGE(0xf4d0, 0xf4d3) MIDI port, option D / <undefined>
//  AM_RANGE(0xf8d0, 0xf8d3) MIDI port, option E / <undefined>
//  AM_RANGE(0xfcd0, 0xfcd3) MIDI port, option F / <undefined>
ADDRESS_MAP_END

static ADDRESS_MAP_START( upd7220_1_map, AS_0, 8, pc9801_state )
	AM_RANGE(0x00000, 0x3ffff) AM_RAM AM_SHARE("video_ram_1")
ADDRESS_MAP_END

static ADDRESS_MAP_START( upd7220_2_map, AS_0, 8, pc9801_state )
	AM_RANGE(0x00000, 0x3ffff) AM_RAM AM_SHARE("video_ram_2")
ADDRESS_MAP_END


CUSTOM_INPUT_MEMBER(pc9801_state::system_type_r)
{
//  System Type (0x00 stock PC-9801, 0xc0 PC-9801U / PC-98LT, PC-98HA, 0x80 others)
	return m_sys_type;
}

static INPUT_PORTS_START( pc9801 )
	PORT_START("DSW1")
	PORT_BIT(0x0001, IP_ACTIVE_HIGH,IPT_SPECIAL) PORT_READ_LINE_DEVICE_MEMBER("upd1990a", upd1990a_device, data_out_r)
	PORT_DIPNAME( 0x0002, 0x0000, "DSW1" ) // error beep if OFF
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Display Type" ) PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING(      0x0000, "Normal Display" )
	PORT_DIPSETTING(      0x0008, "Hi-Res Display" )
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START("DSW5")
	PORT_DIPNAME( 0x01, 0x00, "DSW5" ) // goes into basic with this off, PC-9801VF / PC-9801U setting
	PORT_DIPSETTING(      0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) ) // V30 / V33
	PORT_DIPSETTING(      0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) ) // printer busy
	PORT_DIPSETTING(      0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) // 8 / 4096
	PORT_DIPSETTING(      0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) ) // LCD display
	PORT_DIPSETTING(      0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) ) //system clock = 5 MHz (0) / 8 MHz (1)
	PORT_DIPSETTING(      0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x00, DEF_STR( On ) )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM_MEMBER(DEVICE_SELF, pc9801_state, system_type_r, NULL)

	PORT_START("DSW2")
	PORT_DIPNAME( 0x01, 0x01, "System Specification" ) PORT_DIPLOCATION("SW1:1") //jumps to daa00 if off, presumably some card booting
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Terminal Mode" ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Text width" ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING(    0x04, "40 chars/line" )
	PORT_DIPSETTING(    0x00, "80 chars/line" )
	PORT_DIPNAME( 0x08, 0x00, "Text height" ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING(    0x08, "20 lines/screen" )
	PORT_DIPSETTING(    0x00, "25 lines/screen" )
	PORT_DIPNAME( 0x10, 0x00, "Memory Switch Init" ) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) ) //Fix memory switch condition
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) ) //Initialize Memory Switch with the system default
	PORT_DIPUNUSED_DIPLOC( 0x20, 0x20, "SW1:6" )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW1:7" )
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW1:8" )

	PORT_START("DSW3")
	PORT_DIPNAME( 0x01, 0x01, "DSW3" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("DSW4")
	PORT_DIPNAME( 0x01, 0x01, "DSW4" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("MOUSE_X")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X ) PORT_RESET PORT_SENSITIVITY(30) PORT_KEYDELTA(30)

	PORT_START("MOUSE_Y")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y ) PORT_RESET PORT_SENSITIVITY(30) PORT_KEYDELTA(30)

	PORT_START("MOUSE_B")
	PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_CODE(MOUSECODE_BUTTON3) PORT_NAME("Mouse Middle Button")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CODE(MOUSECODE_BUTTON2) PORT_NAME("Mouse Right Button")
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CODE(MOUSECODE_BUTTON1) PORT_NAME("Mouse Left Button")

	PORT_START("ROM_LOAD")
	PORT_CONFNAME( 0x01, 0x01, "Load floppy 2hd BIOS" )
	PORT_CONFSETTING(    0x00, DEF_STR( Yes ) )
	PORT_CONFSETTING(    0x01, DEF_STR( No ) )
	PORT_CONFNAME( 0x02, 0x02, "Load floppy 2dd BIOS" )
	PORT_CONFSETTING(    0x00, DEF_STR( Yes ) )
	PORT_CONFSETTING(    0x02, DEF_STR( No ) )
INPUT_PORTS_END

static INPUT_PORTS_START( pc9801rs )
	PORT_INCLUDE( pc9801 )

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x80, 0x80, "GDC clock" ) PORT_DIPLOCATION("SW1:8") // DSW 2-8
	PORT_DIPSETTING(    0x80, "2.5 MHz" )
	PORT_DIPSETTING(    0x00, "5 MHz" )

	PORT_MODIFY("DSW4")
	PORT_DIPNAME( 0x04, 0x00, "CPU Type" ) PORT_DIPLOCATION("SW4:8") // DSW 3-8
	PORT_DIPSETTING(    0x04, "V30" )
	PORT_DIPSETTING(    0x00, "I386" )

	PORT_MODIFY("DSW5")
	PORT_DIPNAME( 0x08, 0x00, "Graphic Function" ) // DSW 1-8
	PORT_DIPSETTING(      0x08, "Basic (8 Colors)" )
	PORT_DIPSETTING(      0x00, "Expanded (16/4096 Colors)"  )

	PORT_MODIFY("ROM_LOAD")
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )

//  PORT_START("SOUND_CONFIG")
//  PORT_CONFNAME( 0x01, 0x00, "Sound Type" )
//  PORT_CONFSETTING(    0x00, "YM2203 (OPN)" )
//  PORT_CONFSETTING(    0x01, "YM2608 (OPNA)" )
INPUT_PORTS_END

static INPUT_PORTS_START( pc9821 )
	PORT_INCLUDE( pc9801rs )

	PORT_MODIFY("DSW2")
	PORT_DIPNAME( 0x01, 0x00, "S-Dip SW Init" ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_MODIFY("DSW3")
	PORT_DIPNAME( 0x40, 0x40, "Conventional RAM size" ) PORT_DIPLOCATION("SW3:7")
	PORT_DIPSETTING(    0x40, "640 KB" )
	PORT_DIPSETTING(    0x00, "512 KB" )
INPUT_PORTS_END

static const gfx_layout charset_8x8 =
{
	8,8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout charset_8x16 =
{
	8,16,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16
};

static const gfx_layout charset_16x16 =
{
	16,16,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ STEP16(0,1) },
	{ STEP16(0,16) },
	16*16
};

static GFXDECODE_START( pc9801 )
	GFXDECODE_ENTRY( "chargen", 0x00000, charset_8x8,     0x000, 0x01 )
	GFXDECODE_ENTRY( "chargen", 0x00800, charset_8x16,    0x000, 0x01 )
	GFXDECODE_ENTRY( "kanji",   0x00000, charset_16x16,   0x000, 0x01 )
	GFXDECODE_ENTRY( "raw_kanji",   0x00000, charset_16x16,   0x000, 0x01 )
	GFXDECODE_ENTRY( "new_chargen",0, charset_16x16,   0x000, 0x01 )
GFXDECODE_END

/****************************************
*
* I8259 PIC interface
*
****************************************/

/*
irq assignment (PC-9801F):

8259 master:
ir0 PIT
ir1 keyboard
ir2 vblank
ir3
ir4 rs-232c
ir5
ir6
ir7 slave irq

8259 slave:
ir0 printer
ir1 IDE?
ir2 2dd floppy irq
ir3 2hd floppy irq
ir4 opn
ir5 mouse
ir6
ir7
*/


READ8_MEMBER(pc9801_state::get_slave_ack)
{
	if (offset==7) { // IRQ = 7
		return m_pic2->acknowledge();
	}
	return 0x00;
}

/****************************************
*
* I8253 PIT interface
*
****************************************/

/* basically, PC-98xx series has two xtals.
   My guess is that both are on the PCB, and they clocks the various system components.
   PC-9801RS needs X1 for the pit, otherwise Uchiyama Aki no Chou Bangai has sound pitch bugs
   PC-9821 definitely needs X2, otherwise there's a timer error at POST. Unless it needs a different clock anyway ...
   */
#define MAIN_CLOCK_X1 XTAL_1_9968MHz
#define MAIN_CLOCK_X2 XTAL_2_4576MHz

/****************************************
*
* I8237 DMA interface
*
****************************************/

WRITE_LINE_MEMBER(pc9801_state::pc9801_dma_hrq_changed)
{
	m_maincpu->set_input_line(INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	m_dmac->hack_w(state);

//  printf("%02x HLDA\n",state);
}

WRITE_LINE_MEMBER(pc9801_state::pc9801_tc_w )
{
	/* floppy terminal count */
	m_fdc_2hd->tc_w(state);
	// TODO: 2dd?

//  printf("TC %02x\n",state);
}

READ8_MEMBER(pc9801_state::pc9801_dma_read_byte)
{
	address_space &program = m_maincpu->space(AS_PROGRAM);
	offs_t addr = (m_dma_offset[m_dack] << 16) | offset;

//  printf("%08x\n",addr);

	return program.read_byte(addr);
}


WRITE8_MEMBER(pc9801_state::pc9801_dma_write_byte)
{
	address_space &program = m_maincpu->space(AS_PROGRAM);
	offs_t addr = (m_dma_offset[m_dack] << 16) | offset;

//  printf("%08x %02x\n",addr,data);

	program.write_byte(addr, data);
}

void pc9801_state::set_dma_channel(int channel, int state)
{
	if (!state) m_dack = channel;
}

WRITE_LINE_MEMBER(pc9801_state::pc9801_dack0_w){ /*printf("%02x 0\n",state);*/ set_dma_channel(0, state); }
WRITE_LINE_MEMBER(pc9801_state::pc9801_dack1_w){ /*printf("%02x 1\n",state);*/ set_dma_channel(1, state); }
WRITE_LINE_MEMBER(pc9801_state::pc9801_dack2_w){ /*printf("%02x 2\n",state);*/ set_dma_channel(2, state); }
WRITE_LINE_MEMBER(pc9801_state::pc9801_dack3_w){ /*printf("%02x 3\n",state);*/ set_dma_channel(3, state); }

READ8_MEMBER(pc9801_state::fdc_2hd_r)
{
	return m_fdc_2hd->dma_r();
}

WRITE8_MEMBER(pc9801_state::fdc_2hd_w)
{
	m_fdc_2hd->dma_w(data);
}

READ8_MEMBER(pc9801_state::fdc_2dd_r)
{
	return m_fdc_2dd->dma_r();
}

WRITE8_MEMBER(pc9801_state::fdc_2dd_w)
{
	m_fdc_2dd->dma_w(data);
}

/*
ch1 cs-4231a
ch2 FDC
ch3 SCSI
*/

/****************************************
*
* PPI interfaces
*
****************************************/

WRITE8_MEMBER(pc9801_state::ppi_sys_portc_w)
{
	m_beeper->set_state(!(data & 0x08));
}

READ8_MEMBER(pc9801_state::ppi_sys_portc_r)
{
	return 0xa0; // 0x80 cpu triple fault reset flag?
}

READ8_MEMBER(pc9801_state::ppi_fdd_porta_r)
{
	return 0xff;
}

READ8_MEMBER(pc9801_state::ppi_fdd_portb_r)
{
	return 0xff; //upd765_status_r(machine().device("upd765_2dd"),space, 0);
}

READ8_MEMBER(pc9801_state::ppi_fdd_portc_r)
{
	return 0xff; //upd765_data_r(machine().device("upd765_2dd"),space, 0);
}

WRITE8_MEMBER(pc9801_state::ppi_fdd_portc_w)
{
	//upd765_data_w(machine().device("upd765_2dd"),space, 0,data);
}

READ8_MEMBER(pc9801_state::ppi_mouse_porta_r)
{
	UINT8 res;
	UINT8 isporthi;
	const char *const mousenames[] = { "MOUSE_X", "MOUSE_Y" };

	res = ioport("MOUSE_B")->read() & 0xf0;
	isporthi = ((m_mouse.control & 0x20) >> 5)*4;

	if((m_mouse.control & 0x80) == 0)
		res |= ioport(mousenames[(m_mouse.control & 0x40) >> 6])->read() >> (isporthi) & 0xf;
	else
	{
		if(m_mouse.control & 0x40)
			res |= (m_mouse.ly >> isporthi) & 0xf;
		else
			res |= (m_mouse.lx >> isporthi) & 0xf;
	}

//  printf("A\n");
	return res;
}

WRITE8_MEMBER(pc9801_state::ppi_mouse_porta_w)
{
//  printf("A %02x\n",data);
}

WRITE8_MEMBER(pc9801_state::ppi_mouse_portb_w)
{
//  printf("B %02x\n",data);
}

WRITE8_MEMBER(pc9801_state::ppi_mouse_portc_w)
{
	if((m_mouse.control & 0x80) == 0 && data & 0x80)
	{
		m_mouse.lx = ioport("MOUSE_X")->read();
		m_mouse.ly = ioport("MOUSE_Y")->read();
	}

	m_mouse.control = data;
}

/****************************************
*
* UPD765 interface
*
****************************************/

static SLOT_INTERFACE_START( pc9801_floppies )
	SLOT_INTERFACE( "525hd", FLOPPY_525_HD )
	SLOT_INTERFACE( "35hd", FLOPPY_35_HD )
SLOT_INTERFACE_END

static SLOT_INTERFACE_START( pc9801_cbus )
//  PC-9801-14
	SLOT_INTERFACE( "pc9801_26", PC9801_26 )
	SLOT_INTERFACE( "pc9801_86", PC9801_86 )
//  PC-9801-86
//  PC-9801-26 + PC-9801-86 (?)
//  PC-9801-86 + Chibi-Oto
	SLOT_INTERFACE( "pc9801_118", PC9801_118 )
//  Speak Board
//  Spark Board
//  AMD-98 (AmuseMent boarD)
SLOT_INTERFACE_END

//  Jast Sound, could be put independently

WRITE_LINE_MEMBER( pc9801_state::fdc_2dd_irq )
{
	printf("IRQ 2DD %d\n",state);

	if(m_fdc_2dd_ctrl & 8)
	{
		m_pic2->ir2_w(state);
	}
}

WRITE_LINE_MEMBER( pc9801_state::pc9801rs_fdc_irq )
{
	/* 0xffaf8 */

	//printf("%02x %d\n",m_fdc_ctrl,state);

	if(m_fdc_ctrl & 1)
		m_pic2->ir3_w(state);
	else
		m_pic2->ir2_w(state);
}

WRITE_LINE_MEMBER( pc9801_state::pc9801rs_fdc_drq )
{
//  printf("DRQ %d\n",state);

	if(m_fdc_ctrl & 1)
		m_dmac->dreq2_w(state ^ 1);
	else
		printf("DRQ %02x %d\n",m_fdc_ctrl,state);
}

UINT32 pc9801_state::pc9801_286_a20(bool state)
{
	return (state ? 0xffffff : 0x0fffff);
}

/****************************************
*
* Init emulation status
*
****************************************/

//
PALETTE_INIT_MEMBER(pc9801_state,pc9801)
{
	int i;

	for(i=0;i<8;i++)
		palette.set_pen_color(i, pal1bit(i >> 1), pal1bit(i >> 2), pal1bit(i >> 0));
	for(i=8;i<palette.entries();i++)
		palette.set_pen_color(i, pal1bit(0), pal1bit(0), pal1bit(0));
}

MACHINE_START_MEMBER(pc9801_state,pc9801_common)
{
	m_rtc->cs_w(1);
	m_rtc->oe_w(1);

	m_ipl_rom = memregion("ipl")->base();
	m_sound_bios = memregion("sound_bios")->base();

	save_item(NAME(m_sasi_data));
	save_item(NAME(m_sasi_data_enable));
	save_item(NAME(m_sasi_ctrl));
}

MACHINE_START_MEMBER(pc9801_state,pc9801f)
{
	MACHINE_START_CALL_MEMBER(pc9801_common);

	upd765a_device *fdc;
	fdc = machine().device<upd765a_device>(":upd765_2hd");
	if (fdc)
	{
		floppy_image_device *floppy;
		floppy = machine().device<floppy_connector>("upd765_2hd:0")->get_device();
		if(floppy)
			floppy->setup_ready_cb(floppy_image_device::ready_cb(FUNC(pc9801_state::pc9801_fdc_2hd_update_ready), this));

		floppy = machine().device<floppy_connector>("upd765_2hd:1")->get_device();
		if(floppy)
			floppy->setup_ready_cb(floppy_image_device::ready_cb(FUNC(pc9801_state::pc9801_fdc_2hd_update_ready), this));
	}

	m_fdc_2hd->set_rate(500000);
	m_fdc_2dd->set_rate(250000);
	m_sys_type = 0x00 >> 6;
}

MACHINE_START_MEMBER(pc9801_state,pc9801rs)
{
	MACHINE_START_CALL_MEMBER(pc9801_common);

	m_work_ram = m_ram->pointer();
	m_ext_work_ram = m_ram->pointer() + 0xa0000;

	m_ram_size = m_ram->size() - 0xa0000;

	// TODO: rs and 9821 also
	if(!strncmp(machine().system().name, "pc9801ux", 8))
	{
		address_space& space = m_maincpu->space(AS_PROGRAM);
		membank("wram")->set_base(m_ram->pointer());
		space.install_read_bank(0x100000,  0x100000 + m_ram_size - 1, "ext_wram");
		space.install_write_bank(0x100000,  0x100000 + m_ram_size - 1, "ext_wram");
		membank("ext_wram")->set_base(m_ram->pointer() + 0xa0000);
	}

	m_ide_rom = memregion("ide")->base();
	m_sys_type = 0x80 >> 6;
}

MACHINE_START_MEMBER(pc9801_state,pc9801bx2)
{
	MACHINE_START_CALL_MEMBER(pc9801rs);

	save_pointer(NAME(m_sdip), 24);
}


MACHINE_START_MEMBER(pc9801_state,pc9821)
{
	MACHINE_START_CALL_MEMBER(pc9801rs);

	m_ide_ram = auto_alloc_array(machine(), UINT8, 0x2000);
	m_ext_gvram = auto_alloc_array(machine(), UINT8, 0xa0000);

	save_pointer(NAME(m_sdip), 24);
	save_pointer(NAME(m_ide_ram), 0x2000);
	save_pointer(NAME(m_ext_gvram), 0xa0000);
}

MACHINE_RESET_MEMBER(pc9801_state,pc9801_common)
{
	memset(m_tvram, 0, sizeof(UINT8) * 0x4000);
	/* this looks like to be some kind of backup ram, system will boot with green colors otherwise */
	{
		int i;
		static const UINT8 default_memsw_data[0x10] =
		{
			0xe1, 0x48, 0xe1, 0x05, 0xe1, 0x04, 0xe1, 0x00, 0xe1, 0x01, 0xe1, 0x00, 0xe1, 0x00, 0xe1, 0x6e
//          0xe1, 0xff, 0xe1, 0xff, 0xe1, 0xff, 0xe1, 0xff, 0xe1, 0xff, 0xe1, 0xff, 0xe1, 0xff, 0xe1, 0xff
		};

		for(i=0;i<0x10;i++)
			m_tvram[(0x3fe0)+i*2] = default_memsw_data[i];
	}

	m_beeper->set_frequency(2400);
	m_beeper->set_state(0);

	m_nmi_ff = 0;
	m_mouse.control = 0xff;
	m_mouse.freq_reg = 0;
	m_mouse.freq_index = 0;
}

MACHINE_RESET_MEMBER(pc9801_state,pc9801f)
{
	MACHINE_RESET_CALL_MEMBER(pc9801_common);

	/* 2dd interface ready line is ON by default */
	floppy_image_device *floppy;
	floppy = machine().device<floppy_connector>(":upd765_2hd:0")->get_device();
	if (floppy)
		floppy->mon_w(CLEAR_LINE);
	floppy = machine().device<floppy_connector>(":upd765_2hd:1")->get_device();
	if (floppy)
		floppy->mon_w(CLEAR_LINE);

	{
		UINT8 op_mode;
		UINT8 *ROM;
		UINT8 *PRG = memregion("fdc_data")->base();
		int i;

		ROM = memregion("fdc_bios_2dd")->base();
		op_mode = (ioport("ROM_LOAD")->read() & 2) >> 1;

		for(i=0;i<0x1000;i++)
			ROM[i] = PRG[i+op_mode*0x8000];

		ROM = memregion("fdc_bios_2hd")->base();
		op_mode = ioport("ROM_LOAD")->read() & 1;

		for(i=0;i<0x1000;i++)
			ROM[i] = PRG[i+op_mode*0x8000+0x10000];
	}
}

MACHINE_RESET_MEMBER(pc9801_state,pc9801rs)
{
	MACHINE_RESET_CALL_MEMBER(pc9801_common);

	m_gate_a20 = 0;
	m_rom_bank = 0;
	m_fdc_ctrl = 3;
	m_access_ctrl = 0;
	m_keyb_press = 0xff; // temp kludge, for PC-9821 booting
//  m_has_opna = ioport("SOUND_CONFIG")->read() & 1;
	memset(m_work_ram, 0, sizeof(UINT8) * 0xa0000);
	m_maincpu->set_input_line(INPUT_LINE_A20, m_gate_a20);
}

MACHINE_RESET_MEMBER(pc9801_state,pc9821)
{
	MACHINE_RESET_CALL_MEMBER(pc9801rs);

	m_pc9821_window_bank = 0x08;
}

INTERRUPT_GEN_MEMBER(pc9801_state::pc9801_vrtc_irq)
{
	if(m_vrtc_irq_mask)
	{
		m_pic1->ir2_w(0);
		m_pic1->ir2_w(1);
		m_vrtc_irq_mask = 0; // TODO: this irq auto-masks?
	}
//  else
//      pic8259_ir2_w(machine().device("pic8259_master"), 0);
}


FLOPPY_FORMATS_MEMBER( pc9801_state::floppy_formats )
	FLOPPY_PC98_FORMAT,
	FLOPPY_PC98FDI_FORMAT
FLOPPY_FORMATS_END

TIMER_DEVICE_CALLBACK_MEMBER( pc9801_state::mouse_irq_cb )
{
	if((m_mouse.control & 0x10) == 0)
	{
		m_mouse.freq_index ++;

//      printf("%02x\n",m_mouse.freq_index);
		if(m_mouse.freq_index > m_mouse.freq_reg)
		{
//          printf("irq %02x\n",m_mouse.freq_reg);
			m_mouse.freq_index = 0;
			m_pic2->ir5_w(0);
			m_pic2->ir5_w(1);
		}
	}
}

WRITE_LINE_MEMBER( pc9801_state::keyboard_irq )
{
	m_pic1->ir1_w(state);
}

static MACHINE_CONFIG_FRAGMENT( pc9801_keyboard )
	MCFG_DEVICE_ADD("keyb", PC9801_KBD, 53)
	MCFG_PC9801_KBD_IRQ_CALLBACK(WRITELINE(pc9801_state, keyboard_irq))
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( pc9801_mouse )
	MCFG_DEVICE_ADD("ppi8255_mouse", I8255, 0)
	MCFG_I8255_IN_PORTA_CB(READ8(pc9801_state, ppi_mouse_porta_r))
	MCFG_I8255_OUT_PORTA_CB(WRITE8(pc9801_state, ppi_mouse_porta_w))
	MCFG_I8255_IN_PORTB_CB(IOPORT("DSW3"))
	MCFG_I8255_OUT_PORTB_CB(WRITE8(pc9801_state, ppi_mouse_portb_w))
	MCFG_I8255_IN_PORTC_CB(IOPORT("DSW4"))
	MCFG_I8255_OUT_PORTC_CB(WRITE8(pc9801_state, ppi_mouse_portc_w))

	MCFG_TIMER_DRIVER_ADD_PERIODIC("mouse_timer", pc9801_state, mouse_irq_cb, attotime::from_hz(120))
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( pc9801_cbus )
	MCFG_PC9801CBUS_SLOT_ADD("cbus0", pc9801_cbus, "pc9801_26")
	MCFG_PC9801CBUS_SLOT_ADD("cbus1", pc9801_cbus, NULL)
//  TODO: six max slots
MACHINE_CONFIG_END

static MACHINE_CONFIG_FRAGMENT( pc9801_sasi )
	MCFG_DEVICE_ADD(SASIBUS_TAG, SCSI_PORT, 0)
	MCFG_SCSI_DATA_INPUT_BUFFER("sasi_data_in")
	MCFG_SCSI_IO_HANDLER(WRITELINE(pc9801_state, write_sasi_io)) // bit2
	MCFG_SCSI_CD_HANDLER(DEVWRITELINE("sasi_ctrl_in", input_buffer_device, write_bit3))
	MCFG_SCSI_MSG_HANDLER(DEVWRITELINE("sasi_ctrl_in", input_buffer_device, write_bit4))
	MCFG_SCSI_BSY_HANDLER(DEVWRITELINE("sasi_ctrl_in", input_buffer_device, write_bit5))
	MCFG_SCSI_ACK_HANDLER(DEVWRITELINE("sasi_ctrl_in", input_buffer_device, write_bit6))
	MCFG_SCSI_REQ_HANDLER(DEVWRITELINE("sasi_ctrl_in", input_buffer_device, write_bit7))

	MCFG_SCSIDEV_ADD(SASIBUS_TAG ":" SCSI_PORT_DEVICE1, "harddisk", S1410, SCSI_ID_0) // TODO: correct one, perhaps ttl

	MCFG_SCSI_OUTPUT_LATCH_ADD("sasi_data_out", SASIBUS_TAG)
	MCFG_DEVICE_ADD("sasi_data_in", INPUT_BUFFER, 0)
	MCFG_DEVICE_ADD("sasi_ctrl_in", INPUT_BUFFER, 0)
MACHINE_CONFIG_END


static MACHINE_CONFIG_FRAGMENT( pc9801_ide )
	MCFG_ATA_INTERFACE_ADD("ide", ata_devices, "hdd", NULL, false)
	MCFG_ATA_INTERFACE_IRQ_HANDLER(DEVWRITELINE("pic8259_slave", pic8259_device, ir1_w))
MACHINE_CONFIG_END


static MACHINE_CONFIG_START( pc9801, pc9801_state )
	MCFG_CPU_ADD("maincpu", I8086, 5000000) //unknown clock
	MCFG_CPU_PROGRAM_MAP(pc9801_map)
	MCFG_CPU_IO_MAP(pc9801_io)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", pc9801_state, pc9801_vrtc_irq)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DEVICE("pic8259_master", pic8259_device, inta_cb)

	MCFG_MACHINE_START_OVERRIDE(pc9801_state,pc9801f)
	MCFG_MACHINE_RESET_OVERRIDE(pc9801_state,pc9801f)

	MCFG_DEVICE_ADD("pit8253", PIT8253, 0)
	MCFG_PIT8253_CLK0(MAIN_CLOCK_X1) /* heartbeat IRQ */
	MCFG_PIT8253_OUT0_HANDLER(DEVWRITELINE("pic8259_master", pic8259_device, ir0_w))
	MCFG_PIT8253_CLK1(MAIN_CLOCK_X1) /* Memory Refresh */
	MCFG_PIT8253_CLK2(MAIN_CLOCK_X1) /* RS-232c */
	MCFG_PIT8253_OUT2_HANDLER(WRITELINE(pc9801_state, write_uart_clock))

	MCFG_DEVICE_ADD("i8237", AM9517A, 5000000) // unknown clock, TODO: check channels 0 - 1
	MCFG_I8237_OUT_HREQ_CB(WRITELINE(pc9801_state, pc9801_dma_hrq_changed))
	MCFG_I8237_OUT_EOP_CB(WRITELINE(pc9801_state, pc9801_tc_w))
	MCFG_I8237_IN_MEMR_CB(READ8(pc9801_state, pc9801_dma_read_byte))
	MCFG_I8237_OUT_MEMW_CB(WRITE8(pc9801_state, pc9801_dma_write_byte))
	MCFG_I8237_IN_IOR_2_CB(READ8(pc9801_state, fdc_2hd_r))
	MCFG_I8237_IN_IOR_3_CB(READ8(pc9801_state, fdc_2dd_r))
	MCFG_I8237_OUT_IOW_2_CB(WRITE8(pc9801_state, fdc_2hd_w))
	MCFG_I8237_OUT_IOW_3_CB(WRITE8(pc9801_state, fdc_2dd_w))
	MCFG_I8237_OUT_DACK_0_CB(WRITELINE(pc9801_state, pc9801_dack0_w))
	MCFG_I8237_OUT_DACK_1_CB(WRITELINE(pc9801_state, pc9801_dack1_w))
	MCFG_I8237_OUT_DACK_2_CB(WRITELINE(pc9801_state, pc9801_dack2_w))
	MCFG_I8237_OUT_DACK_3_CB(WRITELINE(pc9801_state, pc9801_dack3_w))
	MCFG_PIC8259_ADD( "pic8259_master", INPUTLINE("maincpu", 0), VCC, READ8(pc9801_state,get_slave_ack) )
	MCFG_PIC8259_ADD( "pic8259_slave", DEVWRITELINE("pic8259_master", pic8259_device, ir7_w), GND, NULL ) // TODO: Check ir7_w

	MCFG_DEVICE_ADD("ppi8255_sys", I8255, 0)
	MCFG_I8255_IN_PORTA_CB(IOPORT("DSW2"))
	MCFG_I8255_IN_PORTB_CB(IOPORT("DSW1"))
	MCFG_I8255_IN_PORTC_CB(READ8(pc9801_state, ppi_sys_portc_r))
	MCFG_I8255_OUT_PORTC_CB(WRITE8(pc9801_state, ppi_sys_portc_w))

	MCFG_DEVICE_ADD("ppi8255_prn", I8255, 0)
	/* TODO: check this one */
	MCFG_I8255_IN_PORTB_CB(IOPORT("DSW5"))

	MCFG_DEVICE_ADD("ppi8255_fdd", I8255, 0)
	MCFG_I8255_IN_PORTA_CB(READ8(pc9801_state, ppi_fdd_porta_r))
	MCFG_I8255_IN_PORTB_CB(READ8(pc9801_state, ppi_fdd_portb_r))
	MCFG_I8255_IN_PORTC_CB(READ8(pc9801_state, ppi_fdd_portc_r))
	MCFG_I8255_OUT_PORTC_CB(WRITE8(pc9801_state, ppi_fdd_portc_w))

	MCFG_FRAGMENT_ADD(pc9801_keyboard)
	MCFG_FRAGMENT_ADD(pc9801_mouse)
	MCFG_FRAGMENT_ADD(pc9801_cbus)
	MCFG_FRAGMENT_ADD(pc9801_sasi)
	MCFG_UPD1990A_ADD(UPD1990A_TAG, XTAL_32_768kHz, NULL, NULL)

	MCFG_DEVICE_ADD(UPD8251_TAG, I8251, 0)

	MCFG_UPD765A_ADD("upd765_2hd", false, true)
	MCFG_UPD765_INTRQ_CALLBACK(DEVWRITELINE("pic8259_slave", pic8259_device, ir3_w))
	MCFG_UPD765_DRQ_CALLBACK(DEVWRITELINE("i8237", am9517a_device, dreq2_w)) MCFG_DEVCB_INVERT
	MCFG_UPD765A_ADD("upd765_2dd", false, true)
	MCFG_UPD765_INTRQ_CALLBACK(WRITELINE(pc9801_state, fdc_2dd_irq))
	MCFG_UPD765_DRQ_CALLBACK(DEVWRITELINE("i8237", am9517a_device, dreq3_w)) MCFG_DEVCB_INVERT
	MCFG_FLOPPY_DRIVE_ADD("upd765_2hd:0", pc9801_floppies, "525hd", pc9801_state::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("upd765_2hd:1", pc9801_floppies, "525hd", pc9801_state::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("upd765_2dd:0", pc9801_floppies, "525hd", pc9801_state::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("upd765_2dd:1", pc9801_floppies, "525hd", pc9801_state::floppy_formats)

	MCFG_SOFTWARE_LIST_ADD("disk_list","pc98")


	#if 0
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")
	MCFG_RAM_EXTRA_OPTIONS("256K,384K,512K,640K")
	#endif

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_UPDATE_DRIVER(pc9801_state, screen_update)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)

	MCFG_DEVICE_ADD("upd7220_chr", UPD7220, 5000000/2)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, upd7220_1_map)
	MCFG_UPD7220_DRAW_TEXT_CALLBACK_OWNER(pc9801_state, hgdc_draw_text)
	MCFG_UPD7220_VSYNC_CALLBACK(DEVWRITELINE("upd7220_btm", upd7220_device, ext_sync_w))

	MCFG_DEVICE_ADD("upd7220_btm", UPD7220, 5000000/2)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, upd7220_2_map)
	MCFG_UPD7220_DISPLAY_PIXELS_CALLBACK_OWNER(pc9801_state, hgdc_display_pixels)

	MCFG_PALETTE_ADD("palette", 16)
	MCFG_PALETTE_INIT_OWNER(pc9801_state,pc9801)
	MCFG_GFXDECODE_ADD("gfxdecode", "palette", pc9801)

	MCFG_SPEAKER_STANDARD_MONO("mono")

	MCFG_SOUND_ADD("beeper", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.15)
MACHINE_CONFIG_END

#if 0
static MACHINE_CONFIG_DERIVED( pc9801vm, pc9801 )
	MCFG_CPU_REPLACE("maincpu",V30,10000000)
	MCFG_CPU_PROGRAM_MAP(pc9801_map)
	MCFG_CPU_IO_MAP(pc9801_io)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", pc9801_state, pc9801_vrtc_irq)
MACHINE_CONFIG_END
#endif

static MACHINE_CONFIG_START( pc9801rs, pc9801_state )
	MCFG_CPU_ADD("maincpu", I386, MAIN_CLOCK_X1*8) // unknown clock.
	MCFG_CPU_PROGRAM_MAP(pc9801rs_map)
	MCFG_CPU_IO_MAP(pc9801rs_io)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", pc9801_state, pc9801_vrtc_irq)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DEVICE("pic8259_master", pic8259_device, inta_cb)

	MCFG_MACHINE_START_OVERRIDE(pc9801_state,pc9801rs)
	MCFG_MACHINE_RESET_OVERRIDE(pc9801_state,pc9801rs)

	MCFG_DEVICE_ADD("pit8253", PIT8253, 0)
	MCFG_PIT8253_CLK0(MAIN_CLOCK_X1) /* heartbeat IRQ */
	MCFG_PIT8253_OUT0_HANDLER(DEVWRITELINE("pic8259_master", pic8259_device, ir0_w))
	MCFG_PIT8253_CLK1(MAIN_CLOCK_X1) /* Memory Refresh */
	MCFG_PIT8253_CLK2(MAIN_CLOCK_X1) /* RS-232c */
	MCFG_PIT8253_OUT2_HANDLER(WRITELINE(pc9801_state, write_uart_clock))

	MCFG_DEVICE_ADD("i8237", AM9517A, MAIN_CLOCK_X1*8) // unknown clock
	MCFG_I8237_OUT_HREQ_CB(WRITELINE(pc9801_state, pc9801_dma_hrq_changed))
	MCFG_I8237_OUT_EOP_CB(WRITELINE(pc9801_state, pc9801_tc_w))
	MCFG_I8237_IN_MEMR_CB(READ8(pc9801_state, pc9801_dma_read_byte))
	MCFG_I8237_OUT_MEMW_CB(WRITE8(pc9801_state, pc9801_dma_write_byte))
	MCFG_I8237_IN_IOR_2_CB(READ8(pc9801_state, fdc_2hd_r))
	MCFG_I8237_OUT_IOW_2_CB(WRITE8(pc9801_state, fdc_2hd_w))
	MCFG_I8237_OUT_DACK_0_CB(WRITELINE(pc9801_state, pc9801_dack0_w))
	MCFG_I8237_OUT_DACK_1_CB(WRITELINE(pc9801_state, pc9801_dack1_w))
	MCFG_I8237_OUT_DACK_2_CB(WRITELINE(pc9801_state, pc9801_dack2_w))
	MCFG_I8237_OUT_DACK_3_CB(WRITELINE(pc9801_state, pc9801_dack3_w))
	MCFG_PIC8259_ADD( "pic8259_master", INPUTLINE("maincpu", 0), VCC, READ8(pc9801_state,get_slave_ack) )
	MCFG_PIC8259_ADD( "pic8259_slave", DEVWRITELINE("pic8259_master", pic8259_device, ir7_w), GND, NULL ) // TODO: Check ir7_w

	MCFG_DEVICE_ADD("ppi8255_sys", I8255, 0)
	MCFG_I8255_IN_PORTA_CB(IOPORT("DSW2"))
	MCFG_I8255_IN_PORTB_CB(IOPORT("DSW1"))
	MCFG_I8255_IN_PORTC_CB(READ8(pc9801_state, ppi_sys_portc_r))
	MCFG_I8255_OUT_PORTC_CB(WRITE8(pc9801_state, ppi_sys_portc_w))

	MCFG_DEVICE_ADD("ppi8255_prn", I8255, 0)
	/* TODO: check this one */
	MCFG_I8255_IN_PORTB_CB(IOPORT("DSW5"))

	MCFG_DEVICE_ADD("ppi8255_fdd", I8255, 0)
	MCFG_I8255_IN_PORTA_CB(READ8(pc9801_state, ppi_fdd_porta_r))
	MCFG_I8255_IN_PORTB_CB(READ8(pc9801_state, ppi_fdd_portb_r))
	MCFG_I8255_IN_PORTC_CB(READ8(pc9801_state, ppi_fdd_portc_r))
	MCFG_I8255_OUT_PORTC_CB(WRITE8(pc9801_state, ppi_fdd_portc_w))

	MCFG_FRAGMENT_ADD(pc9801_keyboard)
	MCFG_FRAGMENT_ADD(pc9801_mouse)
	MCFG_FRAGMENT_ADD(pc9801_ide)
	MCFG_UPD1990A_ADD("upd1990a", XTAL_32_768kHz, NULL, NULL)
	MCFG_DEVICE_ADD(UPD8251_TAG, I8251, 0)

	MCFG_UPD765A_ADD("upd765_2hd", false, true)
	MCFG_UPD765_INTRQ_CALLBACK(WRITELINE(pc9801_state, pc9801rs_fdc_irq))
	MCFG_UPD765_DRQ_CALLBACK(WRITELINE(pc9801_state, pc9801rs_fdc_drq))
	//"upd765_2dd"
	MCFG_FLOPPY_DRIVE_ADD("upd765_2hd:0", pc9801_floppies, "525hd", pc9801_state::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("upd765_2hd:1", pc9801_floppies, "525hd", pc9801_state::floppy_formats)

	MCFG_SOFTWARE_LIST_ADD("disk_list","pc98")

	MCFG_FRAGMENT_ADD(pc9801_cbus)

	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1664K")
	MCFG_RAM_EXTRA_OPTIONS("640K,3712K,7808K")

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_UPDATE_DRIVER(pc9801_state, screen_update)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)

	MCFG_DEVICE_ADD("upd7220_chr", UPD7220, 5000000/2)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, upd7220_1_map)
	MCFG_UPD7220_DRAW_TEXT_CALLBACK_OWNER(pc9801_state, hgdc_draw_text)
	MCFG_UPD7220_VSYNC_CALLBACK(DEVWRITELINE("upd7220_btm", upd7220_device, ext_sync_w))

	MCFG_DEVICE_ADD("upd7220_btm", UPD7220, 5000000/2)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, upd7220_2_map)
	MCFG_UPD7220_DISPLAY_PIXELS_CALLBACK_OWNER(pc9801_state, hgdc_display_pixels)

	MCFG_PALETTE_ADD("palette", 16+16)
	MCFG_PALETTE_INIT_OWNER(pc9801_state,pc9801)
	MCFG_GFXDECODE_ADD("gfxdecode", "palette", pc9801)

	MCFG_SPEAKER_STANDARD_MONO("mono")

//  MCFG_SOUND_ADD("opna", YM2608, MAIN_CLOCK_X1*4) // unknown clock / divider
//  MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_SOUND_ADD("beeper", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.15)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( pc9801ux, pc9801rs )
	MCFG_CPU_REPLACE("maincpu",I80286,10000000)
	MCFG_CPU_PROGRAM_MAP(pc9801ux_map)
	MCFG_CPU_IO_MAP(pc9801ux_io)
	MCFG_80286_A20(pc9801_state, pc9801_286_a20)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", pc9801_state, pc9801_vrtc_irq)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DEVICE("pic8259_master", pic8259_device, inta_cb)
//  MCFG_DEVICE_MODIFY("i8237", AM9157A, 10000000) // unknown clock
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( pc9801bx2, pc9801rs )
	MCFG_CPU_REPLACE("maincpu",I486,25000000)
	MCFG_CPU_PROGRAM_MAP(pc9801rs_map)
	MCFG_CPU_IO_MAP(pc9801bx2_io)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", pc9801_state, pc9801_vrtc_irq)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DEVICE("pic8259_master", pic8259_device, inta_cb)

	MCFG_MACHINE_START_OVERRIDE(pc9801_state,pc9801bx2)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( pc9821, pc9801_state )
	MCFG_CPU_ADD("maincpu", I486, 16000000) // unknown clock
	MCFG_CPU_PROGRAM_MAP(pc9821_map)
	MCFG_CPU_IO_MAP(pc9821_io)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", pc9801_state, pc9801_vrtc_irq)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DEVICE("pic8259_master", pic8259_device, inta_cb)

	MCFG_MACHINE_START_OVERRIDE(pc9801_state,pc9821)
	MCFG_MACHINE_RESET_OVERRIDE(pc9801_state,pc9821)

	MCFG_DEVICE_ADD("pit8253", PIT8253, 0)
	MCFG_PIT8253_CLK0(MAIN_CLOCK_X2) /* heartbeat IRQ */
	MCFG_PIT8253_OUT0_HANDLER(DEVWRITELINE("pic8259_master", pic8259_device, ir0_w))
	MCFG_PIT8253_CLK1(MAIN_CLOCK_X2) /* Memory Refresh */
	MCFG_PIT8253_CLK2(MAIN_CLOCK_X2) /* RS-232c */
	MCFG_PIT8253_OUT2_HANDLER(WRITELINE(pc9801_state, write_uart_clock))

	MCFG_DEVICE_ADD("i8237", AM9517A, 16000000) // unknown clock
	MCFG_I8237_OUT_HREQ_CB(WRITELINE(pc9801_state, pc9801_dma_hrq_changed))
	MCFG_I8237_OUT_EOP_CB(WRITELINE(pc9801_state, pc9801_tc_w))
	MCFG_I8237_IN_MEMR_CB(READ8(pc9801_state, pc9801_dma_read_byte))
	MCFG_I8237_OUT_MEMW_CB(WRITE8(pc9801_state, pc9801_dma_write_byte))
	MCFG_I8237_IN_IOR_2_CB(READ8(pc9801_state, fdc_2hd_r))
	MCFG_I8237_OUT_IOW_2_CB(WRITE8(pc9801_state, fdc_2hd_w))
	MCFG_I8237_OUT_DACK_0_CB(WRITELINE(pc9801_state, pc9801_dack0_w))
	MCFG_I8237_OUT_DACK_1_CB(WRITELINE(pc9801_state, pc9801_dack1_w))
	MCFG_I8237_OUT_DACK_2_CB(WRITELINE(pc9801_state, pc9801_dack2_w))
	MCFG_I8237_OUT_DACK_3_CB(WRITELINE(pc9801_state, pc9801_dack3_w))
	MCFG_PIC8259_ADD( "pic8259_master", INPUTLINE("maincpu", 0), VCC, READ8(pc9801_state,get_slave_ack) )
	MCFG_PIC8259_ADD( "pic8259_slave", DEVWRITELINE("pic8259_master", pic8259_device, ir7_w), GND, NULL ) // TODO: Check ir7_w

	MCFG_DEVICE_ADD("ppi8255_sys", I8255, 0)
	MCFG_I8255_IN_PORTA_CB(IOPORT("DSW2"))
	MCFG_I8255_IN_PORTB_CB(IOPORT("DSW1"))
	MCFG_I8255_IN_PORTC_CB(READ8(pc9801_state, ppi_sys_portc_r))
	MCFG_I8255_OUT_PORTC_CB(WRITE8(pc9801_state, ppi_sys_portc_w))

	MCFG_DEVICE_ADD("ppi8255_prn", I8255, 0)
	/* TODO: check this one */
	MCFG_I8255_IN_PORTB_CB(IOPORT("DSW5"))

	MCFG_DEVICE_ADD("ppi8255_fdd", I8255, 0)
	MCFG_I8255_IN_PORTA_CB(READ8(pc9801_state, ppi_fdd_porta_r))
	MCFG_I8255_IN_PORTB_CB(READ8(pc9801_state, ppi_fdd_portb_r))
	MCFG_I8255_IN_PORTC_CB(READ8(pc9801_state, ppi_fdd_portc_r))
	MCFG_I8255_OUT_PORTC_CB(WRITE8(pc9801_state, ppi_fdd_portc_w))

	MCFG_FRAGMENT_ADD(pc9801_keyboard)
	MCFG_FRAGMENT_ADD(pc9801_mouse)
	MCFG_FRAGMENT_ADD(pc9801_ide)
	MCFG_UPD1990A_ADD("upd1990a", XTAL_32_768kHz, NULL, NULL)
	MCFG_DEVICE_ADD(UPD8251_TAG, I8251, 0)

	MCFG_UPD765A_ADD("upd765_2hd", false, true)
	MCFG_UPD765_INTRQ_CALLBACK(DEVWRITELINE("pic8259_slave", pic8259_device, ir3_w))
	MCFG_UPD765_DRQ_CALLBACK(DEVWRITELINE("i8237", am9517a_device, dreq2_w)) MCFG_DEVCB_INVERT
	//"upd765_2dd"
	MCFG_FLOPPY_DRIVE_ADD("upd765_2hd:0", pc9801_floppies, "525hd", pc9801_state::floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD("upd765_2hd:1", pc9801_floppies, "525hd", pc9801_state::floppy_formats)

	MCFG_SOFTWARE_LIST_ADD("disk_list","pc98")

	MCFG_FRAGMENT_ADD(pc9801_cbus)

	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("1664K")
	MCFG_RAM_EXTRA_OPTIONS("3712K,7808K")

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_UPDATE_DRIVER(pc9801_state, screen_update)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 200-1)

	MCFG_DEVICE_ADD("upd7220_chr", UPD7220, 5000000/2)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, upd7220_1_map)
	MCFG_UPD7220_DRAW_TEXT_CALLBACK_OWNER(pc9801_state, hgdc_draw_text)
	MCFG_UPD7220_VSYNC_CALLBACK(DEVWRITELINE("upd7220_btm", upd7220_device, ext_sync_w))

	MCFG_DEVICE_ADD("upd7220_btm", UPD7220, 5000000/2)
	MCFG_DEVICE_ADDRESS_MAP(AS_0, upd7220_2_map)
	MCFG_UPD7220_DISPLAY_PIXELS_CALLBACK_OWNER(pc9801_state, hgdc_display_pixels)

	MCFG_PALETTE_ADD("palette", 16+16+256)
	MCFG_PALETTE_INIT_OWNER(pc9801_state,pc9801)
	MCFG_GFXDECODE_ADD("gfxdecode", "palette", pc9801)

	MCFG_SPEAKER_STANDARD_MONO("mono")

//  MCFG_SOUND_ADD("opna", YM2608, MAIN_CLOCK_X1*4) // unknown clock / divider
//  MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MCFG_SOUND_ADD("beeper", BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.15)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( pc9821v20, pc9821 )
	MCFG_CPU_REPLACE("maincpu",PENTIUM,32000000) /* TODO: clock */
	MCFG_CPU_PROGRAM_MAP(pc9821_map)
	MCFG_CPU_IO_MAP(pc9821_io)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", pc9801_state, pc9801_vrtc_irq)
	MCFG_CPU_IRQ_ACKNOWLEDGE_DEVICE("pic8259_master", pic8259_device, inta_cb)
MACHINE_CONFIG_END

/* took from "raw" memory dump, uncomment ROM_FILL if you want to play with it */
#define LOAD_IDE_ROM \
	ROM_REGION( 0x2000, "ide", ROMREGION_ERASEFF ) \
	ROM_LOAD( "d8000.rom", 0x0000, 0x2000, BAD_DUMP CRC(5dda57cc) SHA1(d0dead41c5b763008a4d777aedddce651eb6dcbb) ) \
	ROM_IGNORE( 0x2000 ) \
	ROM_IGNORE( 0x2000 ) \
	ROM_IGNORE( 0x2000 ) \
	ROM_FILL( 0x0000, 0x2000, 0xcb )

// all of these are half size :/
#define LOAD_KANJI_ROMS \
	ROM_REGION( 0x80000, "raw_kanji", ROMREGION_ERASEFF ) \
	ROM_LOAD16_BYTE( "24256c-x01.bin", 0x00000, 0x4000, BAD_DUMP CRC(28ec1375) SHA1(9d8e98e703ce0f483df17c79f7e841c5c5cd1692) ) \
	ROM_CONTINUE(                      0x20000, 0x4000  ) \
	ROM_LOAD16_BYTE( "24256c-x02.bin", 0x00001, 0x4000, BAD_DUMP CRC(90985158) SHA1(78fb106131a3f4eb054e87e00fe4f41193416d65) ) \
	ROM_CONTINUE(                      0x20001, 0x4000  ) \
	ROM_LOAD16_BYTE( "24256c-x03.bin", 0x40000, 0x4000, BAD_DUMP CRC(d4893543) SHA1(eb8c1bee0f694e1e0c145a24152222d4e444e86f) ) \
	ROM_CONTINUE(                      0x60000, 0x4000  ) \
	ROM_LOAD16_BYTE( "24256c-x04.bin", 0x40001, 0x4000, BAD_DUMP CRC(5dec0fc2) SHA1(41000da14d0805ed0801b31eb60623552e50e41c) ) \
	ROM_CONTINUE(                      0x60001, 0x4000  ) \
	ROM_REGION( 0x100000, "kanji", ROMREGION_ERASEFF ) \
	ROM_REGION( 0x80000, "new_chargen", ROMREGION_ERASEFF )
/*
F - 8086 5
*/

ROM_START( pc9801f )
	ROM_REGION( 0x18000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "urm01-02.bin", 0x00000, 0x4000, CRC(cde04615) SHA1(8f6fb587c0522af7a8131b45d13f8ae8fc60e8cd) )
	ROM_LOAD16_BYTE( "urm02-02.bin", 0x00001, 0x4000, CRC(9e39b8d1) SHA1(df1f3467050a41537cb9d071e4034f0506f07eda) )
	ROM_LOAD16_BYTE( "urm03-02.bin", 0x08000, 0x4000, CRC(95e79064) SHA1(c27d96949fad82aeb26e316200c15a4891e1063f) )
	ROM_LOAD16_BYTE( "urm04-02.bin", 0x08001, 0x4000, CRC(e4855a53) SHA1(223f66482c77409706cfc64c214cec7237c364e9) )
	ROM_LOAD16_BYTE( "urm05-02.bin", 0x10000, 0x4000, CRC(ffefec65) SHA1(106e0d920e857e59da12225a489ca2756ca405c1) )
	ROM_LOAD16_BYTE( "urm06-02.bin", 0x10001, 0x4000, CRC(1147760b) SHA1(4e0299091dfd53ac7988d40c5a6775a10389faac) )

	ROM_REGION( 0x4000, "sound_bios", ROMREGION_ERASEFF ) /* FM board*/
	ROM_LOAD( "sound.rom", 0x0000, 0x4000, CRC(80eabfde) SHA1(e09c54152c8093e1724842c711aed6417169db23) )

	ROM_REGION( 0x1000, "fdc_bios_2dd", ROMREGION_ERASEFF )

	ROM_REGION( 0x1000, "fdc_bios_2hd", ROMREGION_ERASEFF )

	ROM_REGION( 0x20000, "fdc_data", ROMREGION_ERASEFF ) // 2dd fdc bios, presumably bad size (should be 0x800 for each rom)
	ROM_LOAD16_BYTE( "urf01-01.bin", 0x00000, 0x4000, BAD_DUMP CRC(2f5ae147) SHA1(69eb264d520a8fc826310b4fce3c8323867520ee) )
	ROM_LOAD16_BYTE( "urf02-01.bin", 0x00001, 0x4000, BAD_DUMP CRC(62a86928) SHA1(4160a6db096dbeff18e50cbee98f5d5c1a29e2d1) )
	ROM_LOAD( "2hdif.rom", 0x10000, 0x1000, BAD_DUMP CRC(9652011b) SHA1(b607707d74b5a7d3ba211825de31a8f32aec8146) ) // needs dumping from a board

	ROM_REGION( 0x800, "kbd_mcu", ROMREGION_ERASEFF)
	ROM_LOAD( "mcu.bin", 0x0000, 0x0800, NO_DUMP ) //connected through a i8251 UART, needs decapping

	/* note: ROM names of following two might be swapped */
	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "d23128c-17.bin", 0x00000, 0x00800, BAD_DUMP CRC(eea57180) SHA1(4aa037c684b72ad4521212928137d3369174eb1e) ) //original is a bad dump, this is taken from i386 model
	ROM_LOAD("hn613128pac8.bin",0x00800, 0x01000, BAD_DUMP CRC(b5a15b5c) SHA1(e5f071edb72a5e9a8b8b1c23cf94a74d24cb648e) ) //bad dump, 8x16 charset? (it's on the kanji board)

	LOAD_KANJI_ROMS
ROM_END

/*
UX - 80286 10 + V30 8
*/

ROM_START( pc9801ux )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf_ux.rom",  0x18000, 0x08000, CRC(c7942563) SHA1(61bb210d64c7264be939b11df1e9cd14ffeee3c9) )
	ROM_LOAD( "bios_ux.rom", 0x28000, 0x18000, BAD_DUMP CRC(97375ca2) SHA1(bfe458f671d90692104d0640730972ca8dc0a100) )
	// floppy recalibration happens too fast so skip this test
	ROM_FILL(0x3f60a, 1, 0xff)
	// correct checksum
	ROM_FILL(0x3fffe, 1, 0x9a)

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound_ux.rom", 0x0000, 0x4000, CRC(80eabfde) SHA1(e09c54152c8093e1724842c711aed6417169db23) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_ux.rom",     0x000000, 0x046800, BAD_DUMP CRC(19a76eeb) SHA1(96a006e8515157a624599c2b53a581ae0dd560fd) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
RX - 80286 12 (no V30?)

IPL is for a 386 model, is same as RS below
*/

ROM_START( pc9801rx )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf_rs.rom",  0x18000, 0x08000, BAD_DUMP CRC(c1815325) SHA1(a2fb11c000ed7c976520622cfb7940ed6ddc904e) )
	ROM_LOAD( "bios_rx.rom", 0x28000, 0x018000, BAD_DUMP CRC(0a682b93) SHA1(76a7360502fa0296ea93b4c537174610a834d367) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound_rx.rom",    0x000000, 0x004000, CRC(fe9f57f2) SHA1(d5dbc4fea3b8367024d363f5351baecd6adcd8ef) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_rx.rom",     0x000000, 0x046800, CRC(456d9fc7) SHA1(78ba9960f135372825ab7244b5e4e73a810002ff) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
RS - 386SX 16

(note: might be a different model!)
*/

ROM_START( pc9801rs )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf_rs.rom",  0x18000, 0x08000, CRC(c1815325) SHA1(a2fb11c000ed7c976520622cfb7940ed6ddc904e) )
	ROM_LOAD( "bios_rs.rom", 0x28000, 0x18000, BAD_DUMP CRC(315d2703) SHA1(4f208d1dbb68373080d23bff5636bb6b71eb7565) )

	/* following is an emulator memory dump, should be checked and nuked */
	ROM_REGION( 0x100000, "memory", 0 )
	ROM_LOAD( "00000.rom", 0x00000, 0x8000, CRC(6e299128) SHA1(d0e7d016c005cdce53ea5ecac01c6f883b752b80) )
	ROM_LOAD( "c0000.rom", 0xc0000, 0x8000, CRC(1b43eabd) SHA1(ca711c69165e1fa5be72993b9a7870ef6d485249) )  // 0xff everywhere
	ROM_LOAD( "c8000.rom", 0xc8000, 0x8000, CRC(f2a262b0) SHA1(fe97d2068d18bbb7425d9774e2e56982df2aa1fb) )
	ROM_LOAD( "d0000.rom", 0xd0000, 0x8000, CRC(1b43eabd) SHA1(ca711c69165e1fa5be72993b9a7870ef6d485249) )  // 0xff everywhere
	ROM_LOAD( "e8000.rom", 0xe8000, 0x8000, CRC(4e32081e) SHA1(e23571273b7cad01aa116cb7414c5115a1093f85) )  // contains n-88 basic (86) v2.0
	ROM_LOAD( "f0000.rom", 0xf0000, 0x8000, CRC(4da85a6c) SHA1(18dccfaf6329387c0c64cc4c91b32c25cde8bd5a) )
	ROM_LOAD( "f8000.rom", 0xf8000, 0x8000, CRC(2b1e45b1) SHA1(1fec35f17d96b2e2359e3c71670575ad9ff5007e) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound.rom", 0x0000, 0x4000, CRC(80eabfde) SHA1(e09c54152c8093e1724842c711aed6417169db23) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_rs.rom", 0x00000, 0x46800, BAD_DUMP CRC(da370e7a) SHA1(584d0c7fde8c7eac1f76dc5e242102261a878c5e) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
BX2/U2 - 486SX - (should be 33, but "dumper" note says it's 25 MHz)

Yet another franken-dump done with a lame program, shrug

*/

ROM_START( pc9801bx2 )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "pc98bank0.bin",  0x00000, 0x08000, BAD_DUMP CRC(bfd100cc) SHA1(cf8e6a5679cca7761481abef0ba4b35ead39efdb) )
	ROM_LOAD( "pc98bank1.bin",  0x00000, 0x08000, BAD_DUMP CRC(d0562af8) SHA1(2c4fd27eb598f4b8a00f3e86941ba27007d58e47) )
	ROM_LOAD( "pc98bank2.bin",  0x00000, 0x08000, BAD_DUMP CRC(12818a14) SHA1(9c31e8ac85d78fa779d6bbc2095557065294ec09) )
	ROM_LOAD( "pc98bank3.bin",  0x00000, 0x08000, BAD_DUMP CRC(d0bda44e) SHA1(c1022a3b2be4d2a1e43914df9e4605254e5f99d5) )
	ROM_LOAD( "pc98bank4.bin",  0x18000, 0x08000, BAD_DUMP CRC(be8092f4) SHA1(12c8a166b8c6ebbef85568b67e1f098562883365) )
	ROM_LOAD( "pc98bank5.bin",  0x00000, 0x08000, BAD_DUMP CRC(4e32081e) SHA1(e23571273b7cad01aa116cb7414c5115a1093f85) )
	ROM_LOAD( "pc98bank6.bin",  0x00000, 0x08000, BAD_DUMP CRC(f878c160) SHA1(cad47f09075ffe4f7b51bb937c9f716c709d4596) )
	ROM_LOAD( "pc98bank7.bin",  0x00000, 0x08000, BAD_DUMP CRC(1bd6537b) SHA1(ff9ee1c976a12b87851635ce8991ac4ad607675b) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound.rom", 0x0000, 0x4000, CRC(80eabfde) SHA1(e09c54152c8093e1724842c711aed6417169db23) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_rs.rom", 0x00000, 0x46800, BAD_DUMP CRC(da370e7a) SHA1(584d0c7fde8c7eac1f76dc5e242102261a878c5e) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
VM - V30 8/10

TODO: this ISN'T a real VM model!
*/

ROM_START( pc9801vm )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf_rs.rom",  0x18000, 0x08000, CRC(c1815325) SHA1(a2fb11c000ed7c976520622cfb7940ed6ddc904e) )
	ROM_LOAD( "bios_vm.rom", 0x28000, 0x018000, CRC(2e2d7cee) SHA1(159549f845dc70bf61955f9469d2281a0131b47f) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound_vm.rom",    0x000000, 0x004000, CRC(fe9f57f2) SHA1(d5dbc4fea3b8367024d363f5351baecd6adcd8ef) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_vm.rom",     0x000000, 0x046800, BAD_DUMP CRC(456d9fc7) SHA1(78ba9960f135372825ab7244b5e4e73a810002ff) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
98MATE A - 80486SX 25

(note: might be a different model!)
*/

ROM_START( pc9821 )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf.rom",  0x18000, 0x08000, CRC(dd4c7bb8) SHA1(cf3aa193df2722899066246bccbed03f2e79a74a) )
	ROM_LOAD( "bios.rom", 0x28000, 0x18000, BAD_DUMP CRC(34a19a59) SHA1(2e92346727b0355bc1ec9a7ded1b444a4917f2b9) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound.rom", 0x0000, 0x4000, CRC(a21ef796) SHA1(34137c287c39c44300b04ee97c1e6459bb826b60) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font.rom", 0x00000, 0x46800, BAD_DUMP CRC(a61c0649) SHA1(554b87377d176830d21bd03964dc71f8e98676b1) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
As - 80486DX 33
*/

ROM_START( pc9821as )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf.rom",     0x18000, 0x08000, BAD_DUMP CRC(dd4c7bb8) SHA1(cf3aa193df2722899066246bccbed03f2e79a74a) )
	ROM_LOAD( "bios_as.rom", 0x28000, 0x018000, BAD_DUMP CRC(0a682b93) SHA1(76a7360502fa0296ea93b4c537174610a834d367) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound_as.rom",    0x000000, 0x004000, CRC(fe9f57f2) SHA1(d5dbc4fea3b8367024d363f5351baecd6adcd8ef) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_as.rom",     0x000000, 0x046800, BAD_DUMP CRC(456d9fc7) SHA1(78ba9960f135372825ab7244b5e4e73a810002ff) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
PC-9821AP2/U8W
*/

ROM_START( pc9821ap2 )
	ROM_REGION( 0x90000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "phd0102.rom",     0x08000, 0x80000, CRC(3036774c) SHA1(59856a348f156adf5eca06326f967aca54ff871c) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound.rom", 0x0000, 0x4000, CRC(a21ef796) SHA1(34137c287c39c44300b04ee97c1e6459bb826b60) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font.rom", 0x00000, 0x46800, BAD_DUMP CRC(a61c0649) SHA1(554b87377d176830d21bd03964dc71f8e98676b1) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END


/*
98NOTE - i486SX 33
*/

ROM_START( pc9821ne )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf.rom",     0x18000, 0x08000, CRC(dd4c7bb8) SHA1(cf3aa193df2722899066246bccbed03f2e79a74a) )
	ROM_LOAD( "bios_ne.rom", 0x28000, 0x18000, BAD_DUMP CRC(2ae070c4) SHA1(d7963942042bfd84ed5fc9b7ba8f1c327c094172) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound_ne.rom", 0x0000, 0x4000, CRC(a21ef796) SHA1(34137c287c39c44300b04ee97c1e6459bb826b60) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_ne.rom", 0x00000, 0x46800, BAD_DUMP CRC(fb213757) SHA1(61525826d62fb6e99377b23812faefa291d78c2e) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
Epson PC-486MU - 486 based, unknown clock
*/

ROM_START( pc486mu )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "bios_486mu.rom", 0x08000, 0x18000, BAD_DUMP CRC(57b5d701) SHA1(15029800842e93e07615b0fd91fb9f2bfe3e3c24))
	ROM_RELOAD(                 0x28000, 0x18000 ) // missing rom?

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound_486mu.rom", 0x0000, 0x4000, CRC(6cdfa793) SHA1(4b8250f9b9db66548b79f961d61010558d6d6e1c))

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_486mu.rom", 0x0000, 0x46800, CRC(456d9fc7) SHA1(78ba9960f135372825ab7244b5e4e73a810002ff))

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
98MULTi Ce2 - 80486SX 25
*/

ROM_START( pc9821ce2 )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf_ce2.rom",  0x18000, 0x08000, CRC(273e9e88) SHA1(9bca7d5116788776ed0f297bccb4dfc485379b41) )
	ROM_LOAD( "bios_ce2.rom", 0x28000, 0x018000, BAD_DUMP CRC(76affd90) SHA1(910fae6763c0cd59b3957b6cde479c72e21f33c1) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound_ce2.rom",    0x000000, 0x004000, CRC(a21ef796) SHA1(34137c287c39c44300b04ee97c1e6459bb826b60) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_ce2.rom",     0x000000, 0x046800, CRC(d1c2702a) SHA1(e7781e9d35b6511d12631641d029ad2ba3f7daef) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
98MATE X - 486/Pentium based
*/

ROM_START( pc9821xs )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf.rom",  0x18000, 0x08000, BAD_DUMP CRC(dd4c7bb8) SHA1(cf3aa193df2722899066246bccbed03f2e79a74a) )
	ROM_LOAD( "bios_xs.rom",     0x28000, 0x018000, BAD_DUMP CRC(0a682b93) SHA1(76a7360502fa0296ea93b4c537174610a834d367) )

	ROM_REGION( 0x10000, "soundcpu", 0 )
	ROM_LOAD( "sound_xs.rom",    0x000000, 0x004000, CRC(80eabfde) SHA1(e09c54152c8093e1724842c711aed6417169db23) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_xs.rom",     0x000000, 0x046800, BAD_DUMP CRC(c9a77d8f) SHA1(deb8563712eb2a634a157289838b95098ba0c7f2) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END


/*
98MATE VALUESTAR - Pentium based
*/

ROM_START( pc9821v13 )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf.rom",      0x18000, 0x08000, CRC(dd4c7bb8) SHA1(cf3aa193df2722899066246bccbed03f2e79a74a) )
	ROM_LOAD( "bios_v13.rom", 0x28000, 0x18000, BAD_DUMP CRC(0a682b93) SHA1(76a7360502fa0296ea93b4c537174610a834d367) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound_v13.rom", 0x0000, 0x4000, CRC(a21ef796) SHA1(34137c287c39c44300b04ee97c1e6459bb826b60) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_a.rom", 0x00000, 0x46800, BAD_DUMP CRC(c9a77d8f) SHA1(deb8563712eb2a634a157289838b95098ba0c7f2) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END

/*
98MATE VALUESTAR - Pentium based
*/

ROM_START( pc9821v20 )
	ROM_REGION( 0x60000, "ipl", ROMREGION_ERASEFF )
	ROM_LOAD( "itf_v20.rom",  0x18000, 0x08000, CRC(10e52302) SHA1(f95b8648e3f5a23e507a9fbda8ab2e317d8e5151) )
	ROM_LOAD( "bios_v20.rom", 0x28000, 0x018000, BAD_DUMP CRC(d5d1f13b) SHA1(bf44b5f4e138e036f1b848d6616fbd41b5549764) )

	ROM_REGION( 0x10000, "sound_bios", 0 )
	ROM_LOAD( "sound_v20.rom",    0x000000, 0x004000, CRC(80eabfde) SHA1(e09c54152c8093e1724842c711aed6417169db23) )

	ROM_REGION( 0x80000, "chargen", 0 )
	ROM_LOAD( "font_v20.rom",     0x000000, 0x046800, BAD_DUMP CRC(6244c4c0) SHA1(9513cac321e89b4edb067b30e9ecb1adae7e7be7) )

	LOAD_KANJI_ROMS
	LOAD_IDE_ROM
ROM_END



DRIVER_INIT_MEMBER(pc9801_state,pc9801_kanji)
{
	#define copy_kanji_strip(_dst,_src,_fill_type) \
	for(i=_dst,k=_src;i<_dst+0x20;i++,k++) \
	{ \
		for(j=0;j<0x20;j++) \
			kanji[j+(i << 5)] = _fill_type ? new_chargen[j+(k << 5)] : 0; \
	}
	UINT32 i,j,k;
	UINT32 pcg_tile;
	UINT8 *kanji = memregion("kanji")->base();
	UINT8 *raw_kanji = memregion("raw_kanji")->base();
	UINT8 *new_chargen = memregion("new_chargen")->base();
	UINT8 *chargen = memregion("chargen")->base();

	/* Convert the ROM bitswap here from the original structure */
	/* TODO: kanji bitswap should be completely wrong, will check it out once that a dump is remade. */
	for(i=0;i<0x80000/0x20;i++)
	{
		for(j=0;j<0x20;j++)
		{
			pcg_tile = BITSWAP16(i,15,14,13,12,11,7,6,5,10,9,8,4,3,2,1,0) << 5;
			kanji[j+(i << 5)] = raw_kanji[j+pcg_tile];
		}
	}

	/* convert charset into even/odd structure */
	for(i=0;i<0x80000/0x20;i++)
	{
		for(j=0;j<0x10;j++)
		{
			new_chargen[j*2+(i << 5)] = chargen[j+(i<<5)];
			new_chargen[j*2+(i << 5)+1] = chargen[j+(i<<5)+0x10];
		}
	}

	/* now copy the data from the fake roms into our kanji struct */
	copy_kanji_strip(0x0800,   -1,0); copy_kanji_strip(0x0820,   -1,0); copy_kanji_strip(0x0840,   -1,0); copy_kanji_strip(0x0860,   -1,0);
	copy_kanji_strip(0x0900,   -1,0); copy_kanji_strip(0x0920,0x3c0,1); copy_kanji_strip(0x0940,0x3e0,1); copy_kanji_strip(0x0960,0x400,1);
	copy_kanji_strip(0x0a00,   -1,0); copy_kanji_strip(0x0a20,0x420,1); copy_kanji_strip(0x0a40,0x440,1); copy_kanji_strip(0x0a60,0x460,1);
	copy_kanji_strip(0x0b00,   -1,0); copy_kanji_strip(0x0b20,0x480,1); copy_kanji_strip(0x0b40,0x4a0,1); copy_kanji_strip(0x0b60,0x4c0,1);
	copy_kanji_strip(0x0c00,   -1,0); copy_kanji_strip(0x0c20,0x4e0,1); copy_kanji_strip(0x0c40,0x500,1); copy_kanji_strip(0x0c60,0x520,1);
	copy_kanji_strip(0x0d00,   -1,0); copy_kanji_strip(0x0d20,0x540,1); copy_kanji_strip(0x0d40,0x560,1); copy_kanji_strip(0x0d60,0x580,1);
	copy_kanji_strip(0x0e00,   -1,0); copy_kanji_strip(0x0e20,   -1,0); copy_kanji_strip(0x0e40,   -1,0); copy_kanji_strip(0x0e60,   -1,0);
	copy_kanji_strip(0x0f00,   -1,0); copy_kanji_strip(0x0f20,   -1,0); copy_kanji_strip(0x0f40,   -1,0); copy_kanji_strip(0x0f60,   -1,0);
	{
		int src_1,dst_1;

		for(src_1=0x1000,dst_1=0x660;src_1<0x8000;src_1+=0x100,dst_1+=0x60)
		{
			copy_kanji_strip(src_1,             -1,0);
			copy_kanji_strip(src_1+0x20,dst_1+0x00,1);
			copy_kanji_strip(src_1+0x40,dst_1+0x20,1);
			copy_kanji_strip(src_1+0x60,dst_1+0x40,1);
		}
	}
	#undef copy_kanji_strip
}

/* Genuine dumps */
COMP( 1983, pc9801f,   0,       0,     pc9801,   pc9801,   pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9801F",  GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)

/* TODO: ANYTHING below there needs REDUMPING! */
COMP( 1989, pc9801rs,  0,       0,     pc9801rs, pc9801rs, pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9801RS", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND) //TODO: not sure about the exact model
COMP( 1985, pc9801vm,  pc9801rs,0,     pc9801rs, pc9801rs, pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9801VM", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMP( 1987, pc9801ux,  pc9801rs,0,     pc9801ux, pc9801rs, pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9801UX", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMP( 1988, pc9801rx,  pc9801rs,0,     pc9801ux, pc9801rs, pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9801RX", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMP( 1993, pc9801bx2, pc9801rs,0,     pc9801bx2,pc9801rs, pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9801BX2/U2", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMP( 1994, pc9821,    0,       0,     pc9821,   pc9821,   pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9821 (98MATE)",  GAME_NOT_WORKING | GAME_IMPERFECT_SOUND) //TODO: not sure about the exact model
COMP( 1993, pc9821as,  pc9821,  0,     pc9821,   pc9821,   pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9821 (98MATE A)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMP( 1993, pc9821ap2, pc9821,  0,     pc9821,   pc9821,   pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9821AP2/U8W (98MATE A)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMP( 1994, pc9821xs,  pc9821,  0,     pc9821,   pc9821,   pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9821 (98MATE Xs)", GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMP( 1994, pc9821ce2, pc9821,  0,     pc9821,   pc9821,   pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9821 (98MULTi Ce2)",  GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMP( 1994, pc9821ne,  pc9821,  0,     pc9821,   pc9821,   pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9821 (98NOTE)",  GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMP( 1994, pc486mu,   pc9821,  0,     pc9821,   pc9821,   pc9801_state, pc9801_kanji, "Epson",                       "PC-486MU",  GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1998, pc9821v13, pc9821,  0,     pc9821,   pc9821,   pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9821 (98MATE VALUESTAR 13)",  GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
COMP( 1998, pc9821v20, pc9821,  0,     pc9821v20,pc9821,   pc9801_state, pc9801_kanji, "Nippon Electronic Company",   "PC-9821 (98MATE VALUESTAR 20)",  GAME_NOT_WORKING | GAME_IMPERFECT_SOUND)
