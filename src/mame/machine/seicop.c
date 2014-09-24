
/* Seibu 'COP' (co-processor)  protection

  there appear to be 3 revisions of this protection (based on the external rom)

  COPX-D1 - Seibu Cup Soccer / Olympic Soccer '92
          - Legionnaire
  COPX-D2 - Heated Barrel
          - Godzilla
          - SD Gundam Sangokushi Rainbow Tairiku Senki
          - Denjin Makai
          - Raiden 2
          - Raiden DX
          - Zero Team
  COPX-D3 - Raiden 2/DX New (V33 PCB version)
          - New Zero Team

  COPX / COPX-D2 based games appear to function in a similar way to each other
  while the games using COPX-D3 appears to access the protection device very
  differently to the others.

  As this is only the external rom it isn't confirmed that the actual protection
  devices are identical even where the external rom matches.

  Protection features include BCD math protection, 'command sequences', DMA.
  memory clearing etc.

  it is not confirmed which custom Seibu chip contains the actual co-processor,
  nor if the co-processor is a real MCU with internal code, or a custom designed
  'blitter' like device.

  I suspect that for the earlier games it's part of the COP300 or COP1000 chips,
  for the COPX-D3 based games it's probably inside the system controller SEI333

  the external COP rom is probably used as a lookup for maths operations.

  there should probably only be a single cop2_r / cop2_w function, the chip
  looks to be configurable via the table uploaded to
  0x432 / 0x434 / 0x438 / 0x43a / 0x43c, with 'macro' commands triggered via
  writes to 0x500.

  this simulation is incomplete

  COP TODO list:
  - collision detection, hitbox parameter is a complete mystery;
  - collision detection, unknown usage of reads at 0x582-0x588;
  - The RNG relies on the master CPU clock cycles, but without accurate waitstate support is
    basically impossible to have a 1:1 matchup. Seibu Cup Soccer Selection for instance should show the
    first match as Germany vs. USA at twilight, right now is Spain vs. Brazil at sunset.
  - (MANY other things that needs to be listed here)

  Protection information

ALL games using COPX-D/D2 upload a series of command tables, using the following upload pattern.
This happens ONCE at startup.

Each table is 8 words long, and the table upload offsets aren't always in sequential order, as
you can see from this example (cupsocs) it uploads in the following order

00 - 07, 08 - 0f, 10 - 17, 18 - 1f, 28 - 2f, 60 - 67, 80 - 87, 88 - 8f
90 - 97, 98 - 9f, 20 - 27, 30 - 37, 38 - 3f, 40 - 47, 48 - 4f, 68 - 6f
c0 - c7, a0 - a7, a8 - af, b0 - b7, b8 - bf, c8 - cf, d0 - d7, d8 - df
e0 - e7, e8 - ef, 50 - 57, 58 - 5f, 78 - 7f, f0 - f7

table data is never overwritten, and in this case no data is uploaded
in the 70-77 or f8 - ff region.

It is assumed that the data written before each part of the table is associated
with that table.

000620:  write data 0205 at offset 003c <- 'trigger' associated with this table
000624:  write data 0006 at offset 0038 <- 'unknown1 (4-bit)'
000628:  write data ffeb at offset 003a <- 'unknown2'

000632:  write data 0000 at offset 0034 <- 'table offset'
000638:  write data 0188 at offset 0032 <- '12-bit data' for this offset
000632:  write data 0001 at offset 0034
000638:  write data 0282 at offset 0032
000632:  write data 0002 at offset 0034
000638:  write data 0082 at offset 0032
000632:  write data 0003 at offset 0034
000638:  write data 0b8e at offset 0032
000632:  write data 0004 at offset 0034
000638:  write data 098e at offset 0032
000632:  write data 0005 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0006 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0007 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 0905 at offset 003c
000624:  write data 0006 at offset 0038
000628:  write data fbfb at offset 003a

000632:  write data 0008 at offset 0034
000638:  write data 0194 at offset 0032
000632:  write data 0009 at offset 0034
000638:  write data 0288 at offset 0032
000632:  write data 000a at offset 0034
000638:  write data 0088 at offset 0032
000632:  write data 000b at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 000c at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 000d at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 000e at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 000f at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 138e at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data bf7f at offset 003a

000632:  write data 0010 at offset 0034
000638:  write data 0984 at offset 0032
000632:  write data 0011 at offset 0034
000638:  write data 0aa4 at offset 0032
000632:  write data 0012 at offset 0034
000638:  write data 0d82 at offset 0032
000632:  write data 0013 at offset 0034
000638:  write data 0aa2 at offset 0032
000632:  write data 0014 at offset 0034
000638:  write data 039b at offset 0032
000632:  write data 0015 at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 0016 at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 0017 at offset 0034
000638:  write data 0a9a at offset 0032
--------------------------------------------
000620:  write data 1905 at offset 003c
000624:  write data 0006 at offset 0038
000628:  write data fbfb at offset 003a

000632:  write data 0018 at offset 0034
000638:  write data 0994 at offset 0032
000632:  write data 0019 at offset 0034
000638:  write data 0a88 at offset 0032
000632:  write data 001a at offset 0034
000638:  write data 0088 at offset 0032
000632:  write data 001b at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 001c at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 001d at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 001e at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 001f at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 2a05 at offset 003c
000624:  write data 0006 at offset 0038
000628:  write data ebeb at offset 003a

000632:  write data 0028 at offset 0034
000638:  write data 09af at offset 0032
000632:  write data 0029 at offset 0034
000638:  write data 0a82 at offset 0032
000632:  write data 002a at offset 0034
000638:  write data 0082 at offset 0032
000632:  write data 002b at offset 0034
000638:  write data 0a8f at offset 0032
000632:  write data 002c at offset 0034
000638:  write data 018e at offset 0032
000632:  write data 002d at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 002e at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 002f at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 6200 at offset 003c
000624:  write data 0008 at offset 0038
000628:  write data f3e7 at offset 003a

000632:  write data 0060 at offset 0034
000638:  write data 03a0 at offset 0032
000632:  write data 0061 at offset 0034
000638:  write data 03a6 at offset 0032
000632:  write data 0062 at offset 0034
000638:  write data 0380 at offset 0032
000632:  write data 0063 at offset 0034
000638:  write data 0aa0 at offset 0032
000632:  write data 0064 at offset 0034
000638:  write data 02a6 at offset 0032
000632:  write data 0065 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0066 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0067 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 8100 at offset 003c
000624:  write data 0007 at offset 0038
000628:  write data fdfb at offset 003a

000632:  write data 0080 at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 0081 at offset 0034
000638:  write data 0b88 at offset 0032
000632:  write data 0082 at offset 0034
000638:  write data 0888 at offset 0032
000632:  write data 0083 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0084 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0085 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0086 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0087 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 8900 at offset 003c
000624:  write data 0007 at offset 0038
000628:  write data fdfb at offset 003a

000632:  write data 0088 at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 0089 at offset 0034
000638:  write data 0b8a at offset 0032
000632:  write data 008a at offset 0034
000638:  write data 088a at offset 0032
000632:  write data 008b at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 008c at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 008d at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 008e at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 008f at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 9180 at offset 003c
000624:  write data 0007 at offset 0038
000628:  write data f8f7 at offset 003a

000632:  write data 0090 at offset 0034
000638:  write data 0b80 at offset 0032
000632:  write data 0091 at offset 0034
000638:  write data 0b94 at offset 0032
000632:  write data 0092 at offset 0034
000638:  write data 0b94 at offset 0032
000632:  write data 0093 at offset 0034
000638:  write data 0894 at offset 0032
000632:  write data 0094 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0095 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0096 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0097 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 9980 at offset 003c
000624:  write data 0007 at offset 0038
000628:  write data f8f7 at offset 003a

000632:  write data 0098 at offset 0034
000638:  write data 0b80 at offset 0032
000632:  write data 0099 at offset 0034
000638:  write data 0b94 at offset 0032
000632:  write data 009a at offset 0034
000638:  write data 0b94 at offset 0032
000632:  write data 009b at offset 0034
000638:  write data 0896 at offset 0032
000632:  write data 009c at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 009d at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 009e at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 009f at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 2288 at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data f5df at offset 003a

000632:  write data 0020 at offset 0034
000638:  write data 0f8a at offset 0032
000632:  write data 0021 at offset 0034
000638:  write data 0b8a at offset 0032
000632:  write data 0022 at offset 0034
000638:  write data 0388 at offset 0032
000632:  write data 0023 at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 0024 at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 0025 at offset 0034
000638:  write data 0a9a at offset 0032
000632:  write data 0026 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0027 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 338e at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data bf7f at offset 003a

000632:  write data 0030 at offset 0034
000638:  write data 0984 at offset 0032
000632:  write data 0031 at offset 0034
000638:  write data 0aa4 at offset 0032
000632:  write data 0032 at offset 0034
000638:  write data 0d82 at offset 0032
000632:  write data 0033 at offset 0034
000638:  write data 0aa2 at offset 0032
000632:  write data 0034 at offset 0034
000638:  write data 039c at offset 0032
000632:  write data 0035 at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 0036 at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 0037 at offset 0034
000638:  write data 0a9a at offset 0032
--------------------------------------------
000620:  write data 3bb0 at offset 003c
000624:  write data 0004 at offset 0038
000628:  write data 007f at offset 003a

000632:  write data 0038 at offset 0034
000638:  write data 0f9c at offset 0032
000632:  write data 0039 at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 003a at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 003b at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 003c at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 003d at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 003e at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 003f at offset 0034
000638:  write data 099c at offset 0032
--------------------------------------------
000620:  write data 42c2 at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data fcdd at offset 003a

000632:  write data 0040 at offset 0034
000638:  write data 0f9a at offset 0032
000632:  write data 0041 at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 0042 at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 0043 at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 0044 at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 0045 at offset 0034
000638:  write data 029c at offset 0032
000632:  write data 0046 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0047 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 4aa0 at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data fcdd at offset 003a

000632:  write data 0048 at offset 0034
000638:  write data 0f9a at offset 0032
000632:  write data 0049 at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 004a at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 004b at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 004c at offset 0034
000638:  write data 0b9c at offset 0032
000632:  write data 004d at offset 0034
000638:  write data 099b at offset 0032
000632:  write data 004e at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 004f at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 6880 at offset 003c
000624:  write data 000a at offset 0038
000628:  write data fff3 at offset 003a

000632:  write data 0068 at offset 0034
000638:  write data 0b80 at offset 0032
000632:  write data 0069 at offset 0034
000638:  write data 0ba0 at offset 0032
000632:  write data 006a at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 006b at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 006c at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 006d at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 006e at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 006f at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data c480 at offset 003c
000624:  write data 000a at offset 0038
000628:  write data ff00 at offset 003a

000632:  write data 00c0 at offset 0034
000638:  write data 0080 at offset 0032
000632:  write data 00c1 at offset 0034
000638:  write data 0882 at offset 0032
000632:  write data 00c2 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00c3 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00c4 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00c5 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00c6 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00c7 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data a180 at offset 003c
000624:  write data 0000 at offset 0038
000628:  write data ffff at offset 003a

000632:  write data 00a0 at offset 0034
000638:  write data 0b80 at offset 0032
000632:  write data 00a1 at offset 0034
000638:  write data 0b82 at offset 0032
000632:  write data 00a2 at offset 0034
000638:  write data 0b84 at offset 0032
000632:  write data 00a3 at offset 0034
000638:  write data 0b86 at offset 0032
000632:  write data 00a4 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00a5 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00a6 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00a7 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data a980 at offset 003c
000624:  write data 000f at offset 0038
000628:  write data ffff at offset 003a

000632:  write data 00a8 at offset 0034
000638:  write data 0ba0 at offset 0032
000632:  write data 00a9 at offset 0034
000638:  write data 0ba2 at offset 0032
000632:  write data 00aa at offset 0034
000638:  write data 0ba4 at offset 0032
000632:  write data 00ab at offset 0034
000638:  write data 0ba6 at offset 0032
000632:  write data 00ac at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00ad at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00ae at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00af at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data b100 at offset 003c
000624:  write data 0009 at offset 0038
000628:  write data ffff at offset 003a

000632:  write data 00b0 at offset 0034
000638:  write data 0b40 at offset 0032
000632:  write data 00b1 at offset 0034
000638:  write data 0bc0 at offset 0032
000632:  write data 00b2 at offset 0034
000638:  write data 0bc2 at offset 0032
000632:  write data 00b3 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00b4 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00b5 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00b6 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00b7 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data b900 at offset 003c
000624:  write data 0006 at offset 0038
000628:  write data ffff at offset 003a

000632:  write data 00b8 at offset 0034
000638:  write data 0b60 at offset 0032
000632:  write data 00b9 at offset 0034
000638:  write data 0be0 at offset 0032
000632:  write data 00ba at offset 0034
000638:  write data 0be2 at offset 0032
000632:  write data 00bb at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00bc at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00bd at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00be at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00bf at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data cb8f at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data bf7f at offset 003a

000632:  write data 00c8 at offset 0034
000638:  write data 0984 at offset 0032
000632:  write data 00c9 at offset 0034
000638:  write data 0aa4 at offset 0032
000632:  write data 00ca at offset 0034
000638:  write data 0d82 at offset 0032
000632:  write data 00cb at offset 0034
000638:  write data 0aa2 at offset 0032
000632:  write data 00cc at offset 0034
000638:  write data 039b at offset 0032
000632:  write data 00cd at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 00ce at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 00cf at offset 0034
000638:  write data 0a9f at offset 0032
--------------------------------------------
000620:  write data d104 at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data fffb at offset 003a

000632:  write data 00d0 at offset 0034
000638:  write data 0ac2 at offset 0032
000632:  write data 00d1 at offset 0034
000638:  write data 09e0 at offset 0032
000632:  write data 00d2 at offset 0034
000638:  write data 00a2 at offset 0032
000632:  write data 00d3 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00d4 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00d5 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00d6 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00d7 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data dde5 at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data 7ff7 at offset 003a

000632:  write data 00d8 at offset 0034
000638:  write data 0f80 at offset 0032
000632:  write data 00d9 at offset 0034
000638:  write data 0aa2 at offset 0032
000632:  write data 00da at offset 0034
000638:  write data 0984 at offset 0032
000632:  write data 00db at offset 0034
000638:  write data 00c2 at offset 0032
000632:  write data 00dc at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00dd at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00de at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00df at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data e38e at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data b07f at offset 003a

000632:  write data 00e0 at offset 0034
000638:  write data 0984 at offset 0032
000632:  write data 00e1 at offset 0034
000638:  write data 0ac4 at offset 0032
000632:  write data 00e2 at offset 0034
000638:  write data 0d82 at offset 0032
000632:  write data 00e3 at offset 0034
000638:  write data 0ac2 at offset 0032
000632:  write data 00e4 at offset 0034
000638:  write data 039b at offset 0032
000632:  write data 00e5 at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 00e6 at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 00e7 at offset 0034
000638:  write data 0a9a at offset 0032
--------------------------------------------
000620:  write data eb8e at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data b07f at offset 003a

000632:  write data 00e8 at offset 0034
000638:  write data 0984 at offset 0032
000632:  write data 00e9 at offset 0034
000638:  write data 0ac4 at offset 0032
000632:  write data 00ea at offset 0034
000638:  write data 0d82 at offset 0032
000632:  write data 00eb at offset 0034
000638:  write data 0ac2 at offset 0032
000632:  write data 00ec at offset 0034
000638:  write data 039b at offset 0032
000632:  write data 00ed at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 00ee at offset 0034
000638:  write data 0b9a at offset 0032
000632:  write data 00ef at offset 0034
000638:  write data 0a9f at offset 0032
--------------------------------------------
000620:  write data 5105 at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data fefb at offset 003a

000632:  write data 0050 at offset 0034
000638:  write data 0a80 at offset 0032
000632:  write data 0051 at offset 0034
000638:  write data 0984 at offset 0032
000632:  write data 0052 at offset 0034
000638:  write data 0082 at offset 0032
000632:  write data 0053 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0054 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0055 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0056 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 0057 at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 5905 at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data fffb at offset 003a

000632:  write data 0058 at offset 0034
000638:  write data 09c8 at offset 0032
000632:  write data 0059 at offset 0034
000638:  write data 0a84 at offset 0032
000632:  write data 005a at offset 0034
000638:  write data 00a2 at offset 0032
000632:  write data 005b at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 005c at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 005d at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 005e at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 005f at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data 7905 at offset 003c
000624:  write data 0006 at offset 0038
000628:  write data fffb at offset 003a

000632:  write data 0078 at offset 0034
000638:  write data 01a2 at offset 0032
000632:  write data 0079 at offset 0034
000638:  write data 02c2 at offset 0032
000632:  write data 007a at offset 0034
000638:  write data 00a2 at offset 0032
000632:  write data 007b at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 007c at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 007d at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 007e at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 007f at offset 0034
000638:  write data 0000 at offset 0032
--------------------------------------------
000620:  write data f105 at offset 003c
000624:  write data 0005 at offset 0038
000628:  write data fefb at offset 003a

000632:  write data 00f0 at offset 0034
000638:  write data 0a88 at offset 0032
000632:  write data 00f1 at offset 0034
000638:  write data 0994 at offset 0032
000632:  write data 00f2 at offset 0034
000638:  write data 0088 at offset 0032
000632:  write data 00f3 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00f4 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00f5 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00f6 at offset 0034
000638:  write data 0000 at offset 0032
000632:  write data 00f7 at offset 0034
000638:  write data 0000 at offset 0032

These uploads appear to form the basis of one part of the protection; command lists.

The games upload these tables

cupsoc, cupsoca, cupsocs, cupsocs2, olysoc92
t    u1  u2     trg    tbl
00 | 6 | ffeb | 0205 | 188 282 082 b8e 98e
01 | 6 | fbfb | 0905 | 194 288 088
02 | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a a9a
03 | 6 | fbfb | 1905 | 994 a88 088
04 | 5 | f5df | 2288 | f8a b8a 388 b9a b9a a9a
05 | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
06 | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
07 | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
08 | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
09 | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
0a | 5 | fefb | 5105 | a80 984 082
0b | 5 | fffb | 5905 | 9c8 a84 0a2
0c | 8 | f3e7 | 6200 | 3a0 3a6 380 aa0 2a6
0d | a | fff3 | 6880 | b80 ba0
0e | 0 | 0000 | 0000 |
0f | 6 | fffb | 7905 | 1a2 2c2 0a2
10 | 7 | fdfb | 8100 | b9a b88 888
11 | 7 | fdfb | 8900 | b9a b8a 88a
12 | 7 | f8f7 | 9180 | b80 b94 b94 894
13 | 7 | f8f7 | 9980 | b80 b94 b94 896
14 | 0 | ffff | a180 | b80 b82 b84 b86
15 | f | ffff | a980 | ba0 ba2 ba4 ba6
16 | 9 | ffff | b100 | b40 bc0 bc2
17 | 6 | ffff | b900 | b60 be0 be2
18 | a | ff00 | c480 | 080 882
19 | 5 | bf7f | cb8f | 984 aa4 d82 aa2 39b b9a b9a a9f
1a | 5 | fffb | d104 | ac2 9e0 0a2
1b | 5 | 7ff7 | dde5 | f80 aa2 984 0c2
1c | 5 | b07f | e38e | 984 ac4 d82 ac2 39b b9a b9a a9a
1d | 5 | b07f | eb8e | 984 ac4 d82 ac2 39b b9a b9a a9f
1e | 5 | fefb | f105 | a88 994 088
1f | 0 | 0000 | 0000 |

heatbrl, heatbrl2, heatbrlo, heatbrlu
t    u1  u2     trg    tbl
00 | 6 | ffeb | 0205 | 188 282 082 b8e 98e
01 | 6 | fbfb | 0905 | 194 288 088
02 | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a b9a
03 | 6 | fbfb | 1905 | 994 a88 088
04 | 5 | f5df | 2288 | f8a b8a 388 b9c b9a a9a
05 | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
06 | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
07 | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
08 | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
09 | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
0a | 0 | 0000 | 0000 |
0b | 0 | 0000 | 0000 |
0c | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
0d | a | fff3 | 6880 | b80 ba0
0e | 0 | 0000 | 0000 |
0f | 0 | 0000 | 0000 |
10 | 7 | fdfb | 8100 | b9a b88 888
11 | 7 | fdfb | 8900 | b9a b8a 88a
12 | 7 | f8f7 | 9180 | b80 b94 b94 894
13 | 7 | f8f7 | 9980 | b80 b96 b96 896
14 | 0 | ffff | a100 | b80 b82 b84 b86
15 | f | ffff | a900 | ba0 ba2 ba4 ba6
16 | 9 | ffff | b080 | b40 bc0 bc2
17 | 6 | ffff | b880 | b60 be0 be2
18 | a | ff00 | c480 | 080 882
19 | 0 | 0000 | 0000 |
1a | 0 | 0000 | 0000 |
1b | 0 | 0000 | 0000 |
1c | 0 | 0000 | 0000 |
1d | 0 | 0000 | 0000 |
1e | 0 | 0000 | 0000 |
1f | 0 | 0000 | 0000 |

legionna, legionnau (commands are the same as heatbrl, triggers are different)
t    u1  u2     trg    tbl
00 | 6 | ffeb | 0205 | 188 282 082 b8e 98e
01 | 6 | fbfb | 0905 | 194 288 088
02 | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a b9a
03 | 6 | fbfb | 1905 | 994 a88 088
04 | 5 | f5df | 2288 | f8a b8a 388 b9c b9a a9a
05 | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
06 | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
07 | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
08 | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
09 | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
0a | 0 | 0000 | 0000 |
0b | 0 | 0000 | 0000 |
0c | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
0d | a | fff3 | 6880 | b80 ba0
0e | 0 | 0000 | 0000 |
0f | 0 | 0000 | 0000 |
10 | 7 | fdfb | 8100 | b9a b88 888
11 | 7 | fdfb | 8900 | b9a b8a 88a
12 | 7 | f8f7 | 9180 | b80 b94 b94 894
13 | 7 | f8f7 | 9980 | b80 b96 b96 896
14 | 0 | ffff | a180 | b80 b82 b84 b86
15 | f | ffff | a980 | ba0 ba2 ba4 ba6
16 | 9 | ffff | b100 | b40 bc0 bc2
17 | 6 | ffff | b900 | b60 be0 be2
18 | a | ff00 | c480 | 080 882
19 | 0 | 0000 | 0000 |
1a | 0 | 0000 | 0000 |
1b | 0 | 0000 | 0000 |
1c | 0 | 0000 | 0000 |
1d | 0 | 0000 | 0000 |
1e | 0 | 0000 | 0000 |
1f | 0 | 0000 | 0000 |

godzilla, denjinmk
(denjinmk doesn't actually make use of the table, it never writes to the execute trigger)
t    u1  u2     trg    tbl
00 | 6 | ffeb | 0205 | 188 282 082 b8e 98e
01 | 6 | fbfb | 0905 | 194 288 088
02 | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a a9a
03 | 6 | fbfb | 1905 | 994 a88 088
04 | 5 | f5df | 2288 | f8a b8a 388 b9a b9a a9a
05 | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
06 | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
07 | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
08 | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
09 | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
0a | 0 | 0000 | 0000 |
0b | 0 | 0000 | 0000 |
0c | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
0d | a | fff3 | 6880 | b80 ba0
0e | 0 | 0000 | 0000 |
0f | 0 | 0000 | 0000 |
10 | 7 | fdfb | 8100 | b9a b88 888
11 | 7 | fdfb | 8900 | b9a b8a 88a
12 | 7 | f8f7 | 9180 | b80 b94 b94 894
13 | 7 | f8f7 | 9980 | b80 b94 b94 896
14 | 0 | ffff | a180 | b80 b82 b84 b86
15 | f | ffff | a980 | ba0 ba2 ba4 ba6
16 | 9 | ffff | b100 | b40 bc0 bc2
17 | 6 | ffff | b900 | b60 be0 be2
18 | a | ff00 | c480 | 080 882
19 | 0 | 0000 | 0000 |
1a | 0 | 0000 | 0000 |
1b | 0 | 0000 | 0000 |
1c | 0 | 0000 | 0000 |
1d | 0 | 0000 | 0000 |
1e | 0 | 0000 | 0000 |
1f | 0 | 0000 | 0000 |

grainbow
t    u1  u2     trg    tbl
00 | 6 | ffeb | 0205 | 188 282 082 b8e 98e
01 | 6 | fbfb | 0905 | 194 288 088
02 | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a a9a
03 | 6 | fbfb | 1905 | 994 a88 088
04 | 5 | f5df | 2288 | f8a b8a 388 b9a b9a a9a
05 | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
06 | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
07 | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
08 | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
09 | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
0a | 5 | fefb | 5105 | a80 984 082
0b | 5 | fffb | 5905 | 9c8 a84 0a2
0c | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
0d | a | fff3 | 6980 | b80 ba0
0e | 0 | 0000 | 0000 |
0f | 6 | fffb | 7905 | 1a2 2c2 0a2
10 | 7 | fdfb | 8100 | b9a b88 888
11 | 7 | fdfb | 8900 | b9a b8a 88a
12 | 7 | f8f7 | 9180 | b80 b94 b94 894
13 | 7 | f8f7 | 9980 | b80 b94 b94 896
14 | 0 | 02ff | a180 | b80 b82 b84 b86
15 | f | 02ff | a980 | ba0 ba2 ba4 ba6
16 | 9 | ffff | b100 | b40 bc0 bc2
17 | 6 | ffff | b900 | b60 be0 be2
18 | a | ff00 | c480 | 080 882
19 | 5 | bf7f | cb8f | 984 aa4 d82 aa2 39b b9a b9a a9f
1a | 5 | fffb | d104 | ac2 9e0 0a2
1b | 5 | 7ff7 | dde5 | f80 aa2 984 0c2
1c | 5 | b07f | e38e | 984 ac4 d82 ac2 39b b9a b9a a9a
1d | 5 | b07f | eb8e | 984 ac4 d82 ac2 39b b9a b9a a9f
1e | 5 | fefb | f105 | a88 994 088
1f | 0 | 0000 | 0000 |

raiden2, raiden2a, raiden2b, raiden2c, raiden2d, raiden2e, raiden2f
t    u1  u2     trg    tbl
00 | 6 | ffeb | 0205 | 188 282 082 b8e 98e
01 | 6 | fbfb | 0905 | 194 288 088
02 | 5 | bf7f | 130e | 984 aa4 d82 aa2 39b b9a b9a a9a
03 | 6 | fbfb | 1905 | 994 a88 088
04 | 5 | f5df | 2208 | f8a b8a 388 b9a b9a a9a
05 | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
06 | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
07 | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
08 | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
09 | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
0a | 6 | fff7 | 5205 | 180 2e0 3a0 0a0 3a0
0b | 6 | fff7 | 5a05 | 180 2e0 3a0 0a0 3a0
0c | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
0d | 0 | 0000 | 0000 |
0e | 0 | 0000 | 0000 |
0f | 0 | 0000 | 0000 |
10 | 7 | fdfb | 8100 | b9a b88 888
11 | 7 | fdfb | 8900 | b9a b8a 88a
12 | 7 | fefb | 9100 | b80 b94 894
13 | 7 | fefb | 9900 | b80 b94 896
14 | 0 | 00ff | a100 | b80 b82 b84 b86
15 | f | 00ff | a900 | ba0 ba2 ba4 ba6
16 | 9 | ffff | b100 | b40 bc0 bc2
17 | 6 | ffff | b900 | b60 be0 be2
18 | 0 | 0000 | 0000 |
19 | 0 | 0000 | 0000 |
1a | 0 | 0000 | 0000 |
1b | 0 | 0000 | 0000 |
1c | 0 | 0000 | 0000 |
1d | 0 | 0000 | 0000 |
1e | 6 | fff7 | f205 | 182 2e0 3c0 0c0 3c0
1f | 0 | 0000 | 0000 |

raidndx, raidndxj, raidndxm, raidndxt
(the same as raiden2, but adds an extra command with trigger 7e05)
t    u1  u2     trg    tbl
00 | 6 | ffeb | 0205 | 188 282 082 b8e 98e
01 | 6 | fbfb | 0905 | 194 288 088
02 | 5 | bf7f | 130e | 984 aa4 d82 aa2 39b b9a b9a a9a
03 | 6 | fbfb | 1905 | 994 a88 088
04 | 5 | f5df | 2208 | f8a b8a 388 b9a b9a a9a
05 | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
06 | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
07 | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
08 | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
09 | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
0a | 6 | fff7 | 5205 | 180 2e0 3a0 0a0 3a0
0b | 6 | fff7 | 5a05 | 180 2e0 3a0 0a0 3a0
0c | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
0d | 0 | 0000 | 0000 |
0e | 0 | 0000 | 0000 |
0f | 6 | fffb | 7e05 | 180 282 080 180 282
10 | 7 | fdfb | 8100 | b9a b88 888
11 | 7 | fdfb | 8900 | b9a b8a 88a
12 | 7 | fefb | 9100 | b80 b94 894
13 | 7 | fefb | 9900 | b80 b94 896
14 | 0 | 00ff | a100 | b80 b82 b84 b86
15 | f | 00ff | a900 | ba0 ba2 ba4 ba6
16 | 9 | ffff | b100 | b40 bc0 bc2
17 | 6 | ffff | b900 | b60 be0 be2
18 | 0 | 0000 | 0000 |
19 | 0 | 0000 | 0000 |
1a | 0 | 0000 | 0000 |
1b | 0 | 0000 | 0000 |
1c | 0 | 0000 | 0000 |
1d | 0 | 0000 | 0000 |
1e | 6 | fff7 | f205 | 182 2e0 3c0 0c0 3c0
1f | 0 | 0000 | 0000 |


zeroteam, zeroteama, zeroteamb, zeroteamc, zeroteams, xsedae
t    u1  u2     trg    tbl
00 | 6 | ffeb | 0205 | 188 282 082 b8e 98e
01 | 6 | fbfb | 0905 | 194 288 088
02 | 5 | bf7f | 130e | 984 aa4 d82 aa2 39b b9a b9a a9a
03 | 6 | fbfb | 1905 | 994 a88 088
04 | 5 | f5df | 2208 | f8a b8a 388 b9a b9a a9a
05 | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
06 | 5 | bf7f | 330e | 984 aa4 d82 aa2 39c b9c b9c a9a
07 | 4 | 007f | 3b30 | f9c b9c b9c b9c b9c b9c b9c 99c
08 | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
09 | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
0a | 6 | fffb | 5105 | 180 2e0 0a0
0b | 6 | ffdb | 5a85 | 180 2e0 0a0 182 2e0 0c0 3c0
0c | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
0d | a | fff3 | 6980 | b80 ba0
0e | 8 | fdfd | 7100 | b80 a80 b80
0f | 0 | 0000 | 0000 |
10 | 7 | fdfb | 8100 | b9a b88 888
11 | 7 | fdfb | 8900 | b9a b8a 88a
12 | 7 | f8f7 | 9100 | b80 b94 b94 894
13 | 7 | f8f7 | 9900 | b80 b94 b94 896
14 | 0 | ffff | a100 | b80 b82 b84 b86
15 | f | ffff | a900 | ba0 ba2 ba4 ba6
16 | 9 | ffff | b100 | b40 bc0 bc2
17 | 6 | ffff | b900 | b60 be0 be2
18 | a | ff00 | 7c80 | 080 882
19 | 0 | 0000 | 0000 |
1a | 0 | 0000 | 0000 |
1b | 0 | 0000 | 0000 |
1c | 5 | 06fb | e105 | a88 994 088
1d | 5 | 05f7 | ede5 | f88 a84 986 08a
1e | 4 | 00ff | f790 | f80 b84 b84 b84 b84 b84 b84 b84
1f | 6 | 00ff | fc84 | 182 280

as you can see, there are a lot of command 'command lists' between the games.
(todo, comment ones which seem to have a known function)

executing these seems to cause various operations to occur, be it memory transfer, collision checking, or movement checking.

each 12-bit entry in the tables is probably some kind of 'opcode' which runs and processes data placed in registers / memory.

If we rearrange these tables a bit, so that we can see which are common between games we get the following, take note of
the ones with slight changes, these could be important to figuring out how this works!


Game       |u1 |u2    | trig | macrolist

Table 00 - Same on All games
(grainbow) | 6 | ffeb | 0205 | 188 282 082 b8e 98e
(cupsoc)   | 6 | ffeb | 0205 | 188 282 082 b8e 98e
(legionna) | 6 | ffeb | 0205 | 188 282 082 b8e 98e
(godzilla) | 6 | ffeb | 0205 | 188 282 082 b8e 98e
(heatbrl)  | 6 | ffeb | 0205 | 188 282 082 b8e 98e
(zeroteam) | 6 | ffeb | 0205 | 188 282 082 b8e 98e
(raiden2)  | 6 | ffeb | 0205 | 188 282 082 b8e 98e
(raidndx)  | 6 | ffeb | 0205 | 188 282 082 b8e 98e

Table 01 - Same on All games
(grainbow) | 6 | fbfb | 0905 | 194 288 088
(cupsoc)   | 6 | fbfb | 0905 | 194 288 088
(legionna) | 6 | fbfb | 0905 | 194 288 088
(godzilla) | 6 | fbfb | 0905 | 194 288 088
(heatbrl)  | 6 | fbfb | 0905 | 194 288 088
(zeroteam) | 6 | fbfb | 0905 | 194 288 088
(raiden2)  | 6 | fbfb | 0905 | 194 288 088
(raidndx)  | 6 | fbfb | 0905 | 194 288 088

Table 02 - grainbow and heatbrl have different last entry.  triggers differ on v30 hw
(grainbow) | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a b9a
(cupsoc)   | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a a9a
(legionna) | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a a9a
(godzilla) | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a a9a
(heatbrl)  | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a b9a
(zeroteam) | 5 | bf7f | 130e | 984 aa4 d82 aa2 39b b9a b9a a9a
(raiden2)  | 5 | bf7f | 130e | 984 aa4 d82 aa2 39b b9a b9a a9a
(raidndx)  | 5 | bf7f | 130e | 984 aa4 d82 aa2 39b b9a b9a a9a

Table 03 - Same on All games
(grainbow) | 6 | fbfb | 1905 | 994 a88 088
(cupsoc)   | 6 | fbfb | 1905 | 994 a88 088
(legionna) | 6 | fbfb | 1905 | 994 a88 088
(godzilla) | 6 | fbfb | 1905 | 994 a88 088
(heatbrl)  | 6 | fbfb | 1905 | 994 a88 088
(zeroteam) | 6 | fbfb | 1905 | 994 a88 088
(raiden2)  | 6 | fbfb | 1905 | 994 a88 088
(raidndx)  | 6 | fbfb | 1905 | 994 a88 088

Table 04 - grainbow and heatbrl have a b9c in the 4th slot, triggers differ on v30 hw
(grainbow) | 5 | f5df | 2288 | f8a b8a 388 b9c b9a a9a
(cupsoc)   | 5 | f5df | 2288 | f8a b8a 388 b9a b9a a9a
(legionna) | 5 | f5df | 2288 | f8a b8a 388 b9a b9a a9a
(godzilla) | 5 | f5df | 2288 | f8a b8a 388 b9a b9a a9a
(heatbrl)  | 5 | f5df | 2288 | f8a b8a 388 b9c b9a a9a
(zeroteam) | 5 | f5df | 2208 | f8a b8a 388 b9a b9a a9a
(raiden2)  | 5 | f5df | 2208 | f8a b8a 388 b9a b9a a9a
(raidndx)  | 5 | f5df | 2208 | f8a b8a 388 b9a b9a a9a

Table 05 - Same on All games
(grainbow) | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
(cupsoc)   | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
(legionna) | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
(godzilla) | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
(heatbrl)  | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
(zeroteam) | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
(raiden2)  | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e
(raidndx)  | 6 | ebeb | 2a05 | 9af a82 082 a8f 18e

Table 06 - different trigger on zeroteam (330e)
(grainbow) | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
(cupsoc)   | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
(legionna) | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
(godzilla) | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
(heatbrl)  | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
(zeroteam) | 5 | bf7f | 330e | 984 aa4 d82 aa2 39c b9c b9c a9a
(raiden2)  | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a
(raidndx)  | 5 | bf7f | 338e | 984 aa4 d82 aa2 39c b9c b9c a9a

Table 07 - different trigger on zeroteam (3b30)
(grainbow) | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
(cupsoc)   | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
(legionna) | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
(godzilla) | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
(heatbrl)  | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
(zeroteam) | 4 | 007f | 3b30 | f9c b9c b9c b9c b9c b9c b9c 99c
(raiden2)  | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
(raidndx)  | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c

Table 08 - Same on All games
(grainbow) | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
(cupsoc)   | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
(legionna) | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
(godzilla) | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
(heatbrl)  | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
(zeroteam) | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
(raiden2)  | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c
(raidndx)  | 5 | fcdd | 42c2 | f9a b9a b9c b9c b9c 29c

Table 09 - Same on All games
(grainbow) | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
(cupsoc)   | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
(legionna) | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
(godzilla) | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
(heatbrl)  | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
(zeroteam) | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
(raiden2)  | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b
(raidndx)  | 5 | fcdd | 4aa0 | f9a b9a b9c b9c b9c 99b

Table 0a - Game specific
(grainbow) | 5 | fefb | 5105 | a80 984 082
(cupsoc)   | 5 | fefb | 5105 | a80 984 082
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 6 | fffb | 5105 | 180 2e0 0a0
(raiden2)  | 6 | fff7 | 5205 | 180 2e0 3a0 0a0 3a0
(raidndx)  | 6 | fff7 | 5205 | 180 2e0 3a0 0a0 3a0

Table 0b - Game specific
(grainbow) | 5 | fffb | 5905 | 9c8 a84 0a2
(cupsoc)   | 5 | fffb | 5905 | 9c8 a84 0a2
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 6 | ffdb | 5a85 | 180 2e0 0a0 182 2e0 0c0 3c0
(raiden2)  | 6 | fff7 | 5a05 | 180 2e0 3a0 0a0 3a0
(raidndx)  | 6 | fff7 | 5a05 | 180 2e0 3a0 0a0 3a0

Table 0c - cupsoc has various modifications
notice how *80 is replaced by *a0 and *9a is replaced with *a6, maybe it has the same function, but on different registers?
(grainbow) | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
(cupsoc)   | 8 | f3e7 | 6200 | 3a0 3a6 380 aa0 2a6
(legionna) | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
(godzilla) | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
(heatbrl)  | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
(zeroteam) | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
(raiden2)  | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
(raidndx)  | 8 | f3e7 | 6200 | 380 39a 380 a80 29a

Table 0d - Zero team uses different trigger, doesn't exist on raiden2/dx
(grainbow) | a | fff3 | 6980 | b80 ba0
(cupsoc)   | a | fff3 | 6880 | b80 ba0
(legionna) | a | fff3 | 6880 | b80 ba0
(godzilla) | a | fff3 | 6880 | b80 ba0
(heatbrl)  | a | fff3 | 6880 | b80 ba0
(zeroteam) | a | fff3 | 6980 | b80 ba0
(raiden2)  | 0 | 0000 | 0000 |
(raidndx)  | 0 | 0000 | 0000 |

Table 0e - Zero Team only
(grainbow) | 0 | 0000 | 0000 |
(cupsoc)   | 0 | 0000 | 0000 |
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 8 | fdfd | 7100 | b80 a80 b80
(raiden2)  | 0 | 0000 | 0000 |
(raidndx)  | 0 | 0000 | 0000 |

Table 0f - Same on grainbow/cupsoc, different on raidndx (added compared to raiden2)
(grainbow) | 6 | fffb | 7905 | 1a2 2c2 0a2
(cupsoc)   | 6 | fffb | 7905 | 1a2 2c2 0a2
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 0 | 0000 | 0000 |
(raiden2)  | 0 | 0000 | 0000 |
(raidndx)  | 6 | fffb | 7e05 | 180 282 080 180 282

Table 10 - Same on all games
(grainbow) | 7 | fdfb | 8100 | b9a b88 888
(cupsoc)   | 7 | fdfb | 8100 | b9a b88 888
(legionna) | 7 | fdfb | 8100 | b9a b88 888
(godzilla) | 7 | fdfb | 8100 | b9a b88 888
(heatbrl)  | 7 | fdfb | 8100 | b9a b88 888
(zeroteam) | 7 | fdfb | 8100 | b9a b88 888
(raiden2)  | 7 | fdfb | 8100 | b9a b88 888
(raidndx)  | 7 | fdfb | 8100 | b9a b88 888

Table 11 - Same on all games
(grainbow) | 7 | fdfb | 8900 | b9a b8a 88a
(cupsoc)   | 7 | fdfb | 8900 | b9a b8a 88a
(legionna) | 7 | fdfb | 8900 | b9a b8a 88a
(godzilla) | 7 | fdfb | 8900 | b9a b8a 88a
(heatbrl)  | 7 | fdfb | 8900 | b9a b8a 88a
(zeroteam) | 7 | fdfb | 8900 | b9a b8a 88a
(raiden2)  | 7 | fdfb | 8900 | b9a b8a 88a
(raidndx)  | 7 | fdfb | 8900 | b9a b8a 88a

Table 12 - Raiden2/DX differ from others (list and trigger)
(grainbow) | 7 | f8f7 | 9180 | b80 b94 b94 894
(cupsoc)   | 7 | f8f7 | 9180 | b80 b94 b94 894
(legionna) | 7 | f8f7 | 9180 | b80 b94 b94 894
(godzilla) | 7 | f8f7 | 9180 | b80 b94 b94 894
(heatbrl)  | 7 | f8f7 | 9180 | b80 b94 b94 894
(zeroteam) | 7 | f8f7 | 9100 | b80 b94 b94 894
(raiden2)  | 7 | fefb | 9100 | b80 b94 894
(raidndx)  | 7 | fefb | 9100 | b80 b94 894

Table 13 - Raiden2/DX differ from others , slight changes on legionna and hearbrl too
           (*94 replaced with *96, to operate on a different register?)
(grainbow) | 7 | f8f7 | 9980 | b80 b94 b94 896
(cupsoc)   | 7 | f8f7 | 9980 | b80 b94 b94 896
(legionna) | 7 | f8f7 | 9980 | b80 b96 b96 896
(godzilla) | 7 | f8f7 | 9980 | b80 b94 b94 896
(heatbrl)  | 7 | f8f7 | 9980 | b80 b96 b96 896
(zeroteam) | 7 | f8f7 | 9900 | b80 b94 b94 896
(raiden2)  | 7 | fefb | 9900 | b80 b94 896
(raidndx)  | 7 | fefb | 9900 | b80 b94 896

Table 14 - Trigger differs on heatbrl + v30 games, unknown param differs on grainbow + v30 games
(grainbow) | 0 | 02ff | a180 | b80 b82 b84 b86
(cupsoc)   | 0 | ffff | a180 | b80 b82 b84 b86
(legionna) | 0 | ffff | a180 | b80 b82 b84 b86
(godzilla) | 0 | ffff | a180 | b80 b82 b84 b86
(heatbrl)  | 0 | ffff | a100 | b80 b82 b84 b86
(zeroteam) | 0 | ffff | a100 | b80 b82 b84 b86
(raiden2)  | 0 | 00ff | a100 | b80 b82 b84 b86
(raidndx)  | 0 | 00ff | a100 | b80 b82 b84 b86

Table 15 - Trigger differs on heatbrl + v30 games, unknown param differs on grainbow + v30 games
(grainbow) | f | 02ff | a980 | ba0 ba2 ba4 ba6
(cupsoc)   | f | ffff | a980 | ba0 ba2 ba4 ba6
(legionna) | f | ffff | a980 | ba0 ba2 ba4 ba6
(godzilla) | f | ffff | a980 | ba0 ba2 ba4 ba6
(heatbrl)  | f | ffff | a900 | ba0 ba2 ba4 ba6
(zeroteam) | f | ffff | a900 | ba0 ba2 ba4 ba6
(raiden2)  | f | 00ff | a900 | ba0 ba2 ba4 ba6
(raidndx)  | f | 00ff | a900 | ba0 ba2 ba4 ba6

Table 16 - Trigger differs on heatbrl
(grainbow) | 9 | ffff | b100 | b40 bc0 bc2
(cupsoc)   | 9 | ffff | b100 | b40 bc0 bc2
(legionna) | 9 | ffff | b100 | b40 bc0 bc2
(godzilla) | 9 | ffff | b100 | b40 bc0 bc2
(heatbrl)  | 9 | ffff | b080 | b40 bc0 bc2
(zeroteam) | 9 | ffff | b100 | b40 bc0 bc2
(raiden2)  | 9 | ffff | b100 | b40 bc0 bc2
(raidndx)  | 9 | ffff | b100 | b40 bc0 bc2

Table 17 - Trigger differs on heatbrl
(grainbow) | 6 | ffff | b900 | b60 be0 be2
(cupsoc)   | 6 | ffff | b900 | b60 be0 be2
(legionna) | 6 | ffff | b900 | b60 be0 be2
(godzilla) | 6 | ffff | b900 | b60 be0 be2
(heatbrl)  | 6 | ffff | b880 | b60 be0 be2
(zeroteam) | 6 | ffff | b900 | b60 be0 be2
(raiden2)  | 6 | ffff | b900 | b60 be0 be2
(raidndx)  | 6 | ffff | b900 | b60 be0 be2

Table 18 - Same for all 68k games, zero team has different trigger, not on Raiden2/DX
(grainbow) | a | ff00 | c480 | 080 882
(cupsoc)   | a | ff00 | c480 | 080 882
(legionna) | a | ff00 | c480 | 080 882
(godzilla) | a | ff00 | c480 | 080 882
(heatbrl)  | a | ff00 | c480 | 080 882
(zeroteam) | a | ff00 | 7c80 | 080 882
(raiden2)  | 0 | 0000 | 0000 |
(raidndx)  | 0 | 0000 | 0000 |

Table 19 - grainbow / cupsoc only
(grainbow) | 5 | bf7f | cb8f | 984 aa4 d82 aa2 39b b9a b9a a9f
(cupsoc)   | 5 | bf7f | cb8f | 984 aa4 d82 aa2 39b b9a b9a a9f
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 0 | 0000 | 0000 |
(raiden2)  | 0 | 0000 | 0000 |
(raidndx)  | 0 | 0000 | 0000 |

Table 1a - grainbow / cupsoc only
(grainbow) | 5 | fffb | d104 | ac2 9e0 0a2
(cupsoc)   | 5 | fffb | d104 | ac2 9e0 0a2
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 0 | 0000 | 0000 |
(raiden2)  | 0 | 0000 | 0000 |
(raidndx)  | 0 | 0000 | 0000 |

Table 1b - grainbow / cupsoc only
(grainbow) | 5 | 7ff7 | dde5 | f80 aa2 984 0c2
(cupsoc)   | 5 | 7ff7 | dde5 | f80 aa2 984 0c2
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 0 | 0000 | 0000 |
(raiden2)  | 0 | 0000 | 0000 |
(raidndx)  | 0 | 0000 | 0000 |

Table 1c - grainbow / cupsoc are the same, different on zero team
(grainbow) | 5 | b07f | e38e | 984 ac4 d82 ac2 39b b9a b9a a9a
(cupsoc)   | 5 | b07f | e38e | 984 ac4 d82 ac2 39b b9a b9a a9a
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 5 | 06fb | e105 | a88 994 088
(raiden2)  | 0 | 0000 | 0000 |
(raidndx)  | 0 | 0000 | 0000 |

Table 1d - grainbow / cupsoc are the same, different on zero team
(grainbow) | 5 | b07f | eb8e | 984 ac4 d82 ac2 39b b9a b9a a9f
(cupsoc)   | 5 | b07f | eb8e | 984 ac4 d82 ac2 39b b9a b9a a9f
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 5 | 05f7 | ede5 | f88 a84 986 08a
(raiden2)  | 0 | 0000 | 0000 |
(raidndx)  | 0 | 0000 | 0000 |

Table 1e - grainbow / cupsoc are the same, different on zero team, different on raiden2/dx
(grainbow) | 5 | fefb | f105 | a88 994 088
(cupsoc)   | 5 | fefb | f105 | a88 994 088
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 4 | 00ff | f790 | f80 b84 b84 b84 b84 b84 b84 b84
(raiden2)  | 6 | fff7 | f205 | 182 2e0 3c0 0c0 3c0
(raidndx)  | 6 | fff7 | f205 | 182 2e0 3c0 0c0 3c0

Table 1f - zeroteam specific
(grainbow) | 0 | 0000 | 0000 |
(cupsoc)   | 0 | 0000 | 0000 |
(legionna) | 0 | 0000 | 0000 |
(godzilla) | 0 | 0000 | 0000 |
(heatbrl)  | 0 | 0000 | 0000 |
(zeroteam) | 6 | 00ff | fc84 | 182 280
(raiden2)  | 0 | 0000 | 0000 |
(raidndx)  | 0 | 0000 | 0000 |



typically the games write data for use with the commands at MCUBASE+0xa0 - 0xaf  and MCUBASE+0xc0 - 0xcf before triggering
the operation by writing to MCUBASE+0x100 (or MCUBASE+0x102) with the trigger value.  I believe the commands can change
both COP registers and system memory.

(MCUBASE typically being 0x100400 in the 68k games, and 0x400 in the v30 games)

Seibu Cup Soccer sometimes attempts to use a trigger value which wasn't defined in the table, I don't know what should
happen in that case!

----

Protection Part 2: BCD Maths

some additional registers serve as a math box type device, converting numbers + other functions.  Godzilla seems to use
this for a protection check, other games (Denjin Makai, Raiden 2) use it for scoring:

----

Protection Part 3: Private Buffer DMA + RAM Clear
(todo, expand on this)

address ranges can be specified which allows DMA Fill / Clear operations to be performed, as well as transfering
tilemap+palette data to private buffers for rendering.  If you don't use these nothing gets updated on the real
hardware!.  These don't currently make much sense because the hardware specifies ranges which aren't mapped, or
contain nothing.  It's possible the original hardware has mirroring which this function relies on.

the DMA to private buffer operations are currently ignored due to
if ((cop_dma_trigger==0x14) || (cop_dma_trigger==0x15)) return;

----

Other Protections?

Denjin Makai seems to rely on a byteswapped mirror to write the palette.
Various other ports go through the COP area, and get mapped to inputs / sounds / video registers, this adds to
the confusion and makes it less clear what is / isn't protection related

=================================================================================================================
Seibu COP memory map

DMA mode partial documentation:
0x476
???? ???? ???? ???? SRC table value used for palette DMAs, val << 10

0x478
xxxx xxxx xxxx xxxx SRC address register, val << 6

0x47a
xxxx xxxx xxxx xxxx length register, val << 5

0x47c
xxxx xxxx xxxx xxxx DST address register, val << 6

0x47e
---- ---x ---x ---- DMA mode (00: DMA, work RAM to work RAM 01: DMA, work RAM to private buffers / 10 <unknown> / 11: fill work RAM)
---- ---- x--- ---- palette DMA mode (used for brightness effects)
---- ---- ---- x--- Transfer type (0: word 1:dword)
---- ---- ---- -xxx Channel #

- channels 0x4 and 0x5 are always used to transfer respectively the VRAM and the palette data to the private buffers.
  It isn't know at current stage if it's just a design choice or they are dedicated DMAs.

- Some games (Heated Barrel start-up menu, Olympic Soccer '92 OBJ test) sets up the layer clearance in the midst of the
  frame interval. It might indicate that it delays those DMAs inside the buffers at vblank time.

- Raiden 2 / Raiden DX sets 0x14 with DST = 0xfffe and size as a sprite limit behaviour. The former is probably used to change the
  order of the loaded tables (they are the only known cases where spriteram is smallest address-wise).

- Reading here is probably used for DMA status of the individual channels or just for read-back of the register, but nothing seems to rely
  on it so far so nothing is really known about it.

0x6fc
???? ???? ???? ???? triggers DMA loaded in registers, value looks meaningless

Miscellaneous registers:
0x470
???? ???? ???? ???? External pin register, used by some games for prg/gfx banking (per-game specific)


---

commands 0x8100/0x8900:

status always 0x8007 (doesn't seem to care)
raw | amp | scale | sin       | cos      |
------------------------------------------
y     0x00     x    0x00000000 0x00000000 (i.e. if amp is 0 then sin/cos are zero too)
0     0x40     0    0x00000000 0x00020000
0     0x40     1    0x00000000 0x00040000
0     0x40     2    0x00000000 0x00080000
0     0x40     3    0x00000000 0x00100000
0x40  0x40     0    0x00020000 0x00000000
0x40  0x40     1    0x00040000 0x00000000
0x40  0x40     2    0x00080000 0x00000000
0x40  0x40     3    0x00100000 0x00000000
0x80  0x40     0    0x00000000 0xfffc0000
0x80  0x40     1    0x00000000 0xfff80000
0x80  0x40     2    0x00000000 0xfff00000
0x80  0x40     3    0x00000000 0xffe00000
0xc0  0x40     0    0xfffc0000 0x00000000
0xc0  0x40     1    0xfff80000 0x00000000
0xc0  0x40     2    0xfff00000 0x00000000
0xc0  0x40     3    0xffe00000 0x00000000
0     0x80     0    0x00000000 0x00040000
0     0x80     1    0x00000000 0x00080000
0     0x80     2    0x00000000 0x00100000
0     0x80     3    0x00000000 0x00200000
0x40  0x80     0    0x00040000 0x00000000
0x40  0x80     1    0x00080000 0x00000000
0x40  0x80     2    0x00100000 0x00000000
0x40  0x80     3    0x00200000 0x00000000
0x80  0x80     0    0x00000000 0xfff80000
0x80  0x80     1    0x00000000 0xfff00000
0x80  0x80     2    0x00000000 0xffe00000
0x80  0x80     3    0x00000000 0xffc00000
0xc0  0x80     0    0xfff80000 0x00000000
0xc0  0x80     1    0xfff00000 0x00000000
0xc0  0x80     2    0xffe00000 0x00000000
0xc0  0x80     3    0xffc00000 0x00000000
0     0xc0     0    0x00000000 0x00060000
0     0xc0     1    0x00000000 0x000c0000
0     0xc0     2    0x00000000 0x00180000
0     0xc0     3    0x00000000 0x00300000
0x40  0xc0     0    0x00060000 0x00000000
0x40  0xc0     1    0x000c0000 0x00000000
0x40  0xc0     2    0x00180000 0x00000000
0x40  0xc0     3    0x00300000 0x00000000
0x80  0xc0     0    0x00000000 0xfff40000
0x80  0xc0     1    0x00000000 0xffe80000
0x80  0xc0     2    0x00000000 0xffd00000
0x80  0xc0     3    0x00000000 0xffa00000
0xc0  0xc0     0    0xfff40000 0x00000000
0xc0  0xc0     1    0xffe80000 0x00000000
0xc0  0xc0     2    0xffd00000 0x00000000
0xc0  0xc0     3    0xffa00000 0x00000000

commands 0x130e/0x138e: (dx dy 2 fixed point 0x80)
dx     dy      angle
---------------------
0x00   0x00    0x20
0x20   0x00    0x25 (0x26 in test)
0x40   0x00    0x2d (0x2b in test)
0x60   0x00    0x36
0x80   0x00    0x00 cop status 0x8007
0xa0   0x00    0x4a
0xc0   0x00    0x53
0xe0   0x00    0x5b (0x5a)
0x100  0x00    0x60
0x120  0x00    0x65
0x140  0x00    0x69 (0x68)
0x160  0x00    0x6b
0x180  0x00    0x6e (0x6d)
0x1a0  0x00    0x6f
0x1c0  0x00    0x71
0x1e0  0x00    0x72


command 0x3bb0
dx    dy   | dist
-----------|-----
0x00  0x00 | 0xb5
0x20  0x00 | 0xa0
0x40  0x00 | 0x8f
0x60  0x00 | 0x83
0x80  0x00 | 0x80
0xa0  0x00 | 0x83
0xc0  0x00 | 0x8f
0xe0  0x00 | 0xa0

command 0x42c2
dx    dy   | stat   dist angle scale  r34(r) r36     r38*  r3a
-----------|-----------------------------------------------------
0x00  0x00 | 0x0067 0x20 0x20  0x0000 0x0001 0x0020  0xb5 0x16a0
0x20  0x00 | 0x0027 0x20 0x26  0x0000 0x0001 0x0026  0xa0 0x1400
0x40  0x00 | 0x0067 0x20 0x2d  0x0000 0x0001 0x002d  0x8f 0x11e3
0x60  0x00 | 0x0067 0x20 0x36  0x0000 0x0001 0x0036  0x83 0x107e
0x80  0x00 | 0x0027 0x20 0x00  0x0000 0x0001 0x0000  0x80 0x1000
0xa0  0x00 | 0x0067 0x20 0x4a  0x0000 0x0001 0x004a  0x83 0x107e
0xc0  0x00 | 0x0067 0x20 0x53  0x0000 0x0001 0x0053  0x8f 0x11e3
0xe0  0x00 | 0x0027 0x20 0x5a  0x0000 0x0001 0x005a  0xa0 0x1400

0x00  0x00 | ****** 0x10 0x20  0x0000 0x0002 0x0020  0xb5 0x0b50
0x20  0x00 | ****** 0x10 0x26  0x0000 0x0002 0x0026  0xa0 0x0a00
0x40  0x00 | ****** 0x10 0x2d  0x0000 0x0002 0x002d  0x8f 0x08f1
0x60  0x00 | ****** 0x10 0x36  0x0000 0x0002 0x0036  0x83 0x083f
0x80  0x00 | ****** 0x10 0x00  0x0000 0x0002 0x0000  0x80 0x0800
0xa0  0x00 | ****** 0x10 0x4a  0x0000 0x0002 0x004a  0x83 0x083f
0xc0  0x00 | ****** 0x10 0x53  0x0000 0x0002 0x0053  0x8f 0x08f1
0xe0  0x00 | ****** 0x10 0x5a  0x0000 0x0002 0x005a  0xa0 0x0a00

0x00  0x00 | ****** 0x08 0x20  0x0000 0x0004 0x0020  0xb5 0x05a8
0x20  0x00 | ****** 0x08 0x26  0x0000 0x0004 0x0026  0xa0 0x0500

0x20  0x00 | ****** 0x02 0x26  0x0000 0x0010 0x0026  0xa0 0x0140

0xc0  0x00 | 0x0047 0x01 0x53  0x0000 0x0020 0x0053  0x8f 0x008f

0x60  0x00 | 0x0047 0x00 0x36  0x0000 0x0040 0x0036  0x83 0x0041

0x40  0x00 | 0x0047 0x00 0x2d  0x0000 0x0080 0x002d  0x8f 0x0023

0x40  0x00 | 0x8007 0x00 0x2d  0x0000 0x0400 0x008f  0x8f 0x0000

0x00  0x00 | 0x0067 0x10 0x2d  0x0000 0x0001 0x0020  0xb5 0x0b50

*same as 0x3bb0

command 0x6200
raw angle|angle compare|angle mod value| res |
---------|-------------|---------------|-----|
0x00      ****          0x00            0x00
0x00      0x00          0x20            0x00
0x00      0x20          0x20            0x20
0x00      0x40          0x20            0x20
0x00      0x60          0x20            0x20
0x00      0x80          0x20            0xe0
0x00      0xa0          0x20            0xe0
0x00      0xc0          0x20            0xe0
0x00      0xe0          0x20            0xe0
0x00      0x00          0x40            0x00
0x00      0x20          0x40            0x20
0x00      0x40          0x40            0x40
0x00      0x60          0x40            0x40
0x00      0x80          0x40            0xc0
0x00      0xa0          0x40            0xc0
0x00      0xc0          0x40            0xc0
0x00      0xe0          0x40            0xe0
0x00      0x00          0x60            0x00
0x00      0x20          0x60            0x20
0x00      0x40          0x60            0x60 *
0x00      0x60          0x60            0x60
0x00      0x80          0x60            0xa0
0x00      0xa0          0x60            0xa0
0x00      0xc0          0x60            0xc0
0x00      0xe0          0x60            0xe0
0x00      0x00          0x80            0x00
0x00      0x20          0x80            0x80 *
0x00      0x40          0x80            0x80 *
0x00      0x60          0x80            0x80 *
0x00      0x80          0x80            0x80 *
0x00      0xa0          0x80            0x80
0x00      0xc0          0x80            0x80
0x00      0xe0          0x80            0x80
0x00      0x00          0xa0            0x00
0x00      0x20          0xa0            0x20
0x00      0x40          0xa0            0xa0
0x00      0x60          0xa0            0xa0
0x00      0x80          0xa0            0x60
0x00      0xa0          0xa0            0x60
0x00      0xc0          0xa0            0x60
0x00      0xe0          0xa0            0xe0
0x00      0x00          0xc0            0x00
0x00      0x20          0xc0            0x20
0x00      0x40          0xc0            0xc0
0x00      0x60          0xc0            ****
0x00      0x80          0xc0            0x40
0x00      0xa0          0xc0            ****
0x00      0xc0          0xc0            0xc0
0x00      0xe0          0xc0            0xe0
0x00      0x00          0xe0            0x00
0x00      0x20          0xe0            0x20
0x00      0x40          0xe0            0xe0
0x00      0x60          0xe0            0xe0
0x00      0x80          0xe0            0x20
0x00      0xa0          0xe0            0x20
0x00      0xc0          0xe0            0xc0
0x00      0xe0          0xe0            0xe0
*/

#include "emu.h"
#include "includes/legionna.h"
#include "machine/seicop.h"


const device_type SEIBU_COP_LEGACY = &device_creator<seibu_cop_legacy_device>;

seibu_cop_legacy_device::seibu_cop_legacy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, SEIBU_COP_LEGACY, "Seibu COP Legacy", tag, owner, clock, "seibu_cop_legacy", __FILE__),
	m_cop_mcu_ram(NULL),
	m_cop_hit_val_x(0),
	m_cop_hit_val_y(0),
	m_cop_hit_val_z(0),
	m_legacycop_angle_compare(0),
	m_legacycop_angle_mod_val(0),
	m_r0(0),
	m_r1(0),
	m_cop_rom_addr_lo(0),
	m_cop_rom_addr_hi(0),
	m_cop_rom_addr_unk(0),
	m_cop_sprite_dma_src(0),
	m_cop_sprite_dma_abs_x(0),
	m_cop_sprite_dma_abs_y(0),
	m_raiden2cop(*this, ":raiden2cop")
{


}

#define seibu_cop_log logerror

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void seibu_cop_legacy_device::device_config_complete()
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void seibu_cop_legacy_device::device_start()
{
	m_cop_mcu_ram = reinterpret_cast<UINT16 *>(machine().root_device().memshare("cop_mcu_ram")->ptr());


	save_item(NAME(m_cop_hit_val_x));
	save_item(NAME(m_cop_hit_val_y));
	save_item(NAME(m_cop_hit_val_z));
	save_item(NAME(m_legacycop_angle_compare));
	save_item(NAME(m_legacycop_angle_mod_val));
	save_item(NAME(m_r0));
	save_item(NAME(m_r1));
	save_item(NAME(m_cop_rom_addr_lo));
	save_item(NAME(m_cop_rom_addr_hi));
	save_item(NAME(m_cop_rom_addr_unk));
	save_item(NAME(m_cop_sprite_dma_src));
	save_item(NAME(m_cop_sprite_dma_abs_x));
	save_item(NAME(m_cop_sprite_dma_abs_y));
}

//-------------------------------------------------
//  device_
//  device-specific startup
//-------------------------------------------------

void seibu_cop_legacy_device::device_reset()
{
}



/*
"The area of ASM snippets"

player-1 priorities list:
1086d8: show this sprite (bit 15)
1086dc: lives (BCD,bits 3,2,1,0)
1086de: energy bar (upper byte)
1086e0: walk animation (lower byte)
1086ec: "death" status (bit 15)
1086f4: sprite y axis
1086f0: sprite x axis

Sprite DMA TODO:
- various bits not yet understood in the sprite src tables and in the 0x400/0x402 sprite param;

spriteram DMA [1]
001DE4: 3086                     move.w  D6, (A0) ;$100400,color + other stuff
001DE6: 2440                     movea.l D0, A2
001DE8: 0269 0004 0002           andi.w  #$4, ($2,A1)
001DEE: 3152 000C                move.w  (A2), ($c,A0) ;DMA size
001DF2: 3145 0002                move.w  D5, ($2,A0)
001DF6: 0245 0040                andi.w  #$40, D5
001DFA: 2009                     move.l  A1, D0
001DFC: 3140 00C0                move.w  D0, ($c0,A0) ;RAM -> $1004c0 (work ram index?)
001E00: 4840                     swap    D0
001E02: 3140 00A0                move.w  D0, ($a0,A0) ;RAM -> $1004a0
001E06: 200A                     move.l  A2, D0
001E08: 3140 0014                move.w  D0, ($14,A0) ;$ROM lo -> $100414 src
001E0C: 4840                     swap    D0
001E0E: 3140 0012                move.w  D0, ($12,A0) ;$ROM hi -> $100412
001E12: 2679 0010 8116           movea.l $108116.l, A3 ;points to dst spriteram
001E18: 3839 0010 810A           move.w  $10810a.l, D4 ;spriteram index
001E1E: 260B                     move.l  A3, D3
001E20: 3143 00C8                move.w  D3, ($c8,A0) ;sets the dst spriteram
001E24: 4843                     swap    D3
001E26: 3143 00A8                move.w  D3, ($a8,A0)
001E2A: 45EA 0004                lea     ($4,A2), A2
//at this point we're ready for DMAing
001E2E: 317C A180 0100           move.w  #$a180, ($100,A0) ;<-get x/y from sprite
001E34: 317C 6980 0102           move.w  #$6980, ($102,A0) ;<-adjust sprite x/y
001E3A: 317C C480 0102           move.w  #$c480, ($102,A0) ;<-load sprite offset
001E40: 317C 0000 0010           move.w  #$0, ($10,A0)     ;<-do the job?
001E46: 302A 0002                move.w  ($2,A2), D0
001E4A: 816B 0006                or.w    D0, ($6,A3)
001E4E: 45EA 0006                lea     ($6,A2), A2
001E52: 302B 0008                move.w  ($8,A3), D0
001E56: B079 0010 8112           cmp.w   $108112.l, D0
001E5C: 6E00 0054                bgt     $1eb2
001E60: B079 0010 8110           cmp.w   $108110.l, D0
001E66: 6D00 004A                blt     $1eb2
001E6A: 026B 7FFF 000A           andi.w  #$7fff, ($a,A3)
001E70: 8B6B 0004                or.w    D5, ($4,A3)
001E74: 47EB 0008                lea     ($8,A3), A3
001E78: 260B                     move.l  A3, D3
001E7A: 3143 00C8                move.w  D3, ($c8,A0)
001E7E: 4843                     swap    D3
001E80: 3143 00A8                move.w  D3, ($a8,A0)
001E84: 5244                     addq.w  #1, D4
001E86: B879 0010 8114           cmp.w   $108114.l, D4
001E8C: 6500 000C                bcs     $1e9a
001E90: 0069 0002 0002           ori.w   #$2, ($2,A1)
001E96: 6000 000C                bra     $1ea4
001E9A: 3028 01B0                move.w  ($1b0,A0), D0 ;bit 1 = DMA job finished
001E9E: 0240 0002                andi.w  #$2, D0
001EA2: 6790                     beq     $1e34
001EA4: 33C4 0010 810A           move.w  D4, $10810a.l
001EAA: 23CB 0010 8116           move.l  A3, $108116.l
001EB0: 4E75                     rts

x/y check [2]
002030: E58D                     lsl.l   #2, D5
002032: 0685 0003 0000           addi.l  #$30000, D5
002038: 33C5 0010 04C4           move.w  D5, $1004c4.l
00203E: 4845                     swap    D5
002040: 33C5 0010 04A4           move.w  D5, $1004a4.l
002046: E58E                     lsl.l   #2, D6
002048: 0686 0003 0000           addi.l  #$30000, D6
00204E: 33C6 0010 04C6           move.w  D6, $1004c6.l
002054: 4846                     swap    D6
002056: 33C6 0010 04A6           move.w  D6, $1004a6.l
00205C: 33FC A180 0010 0500      move.w  #$a180, $100500.l
002064: 33FC B100 0010 0500      move.w  #$b100, $100500.l
00206C: 33FC A980 0010 0500      move.w  #$a980, $100500.l
002074: 33FC B900 0010 0500      move.w  #$b900, $100500.l
00207C: 4E75                     rts
[...]
//then reads at $580

sine cosine has a weird math problem, it needs that the amp is multiplied by two when the direction is TOTALLY left or TOTALLY up.
No known explanation to this so far ...

003306: move.w  #$8100, ($100,A0)
00330C: move.w  #$8900, ($100,A0)
003312: cmpi.w  #$80, ($36,A1) ;checks if angle is equal to 0x80 (left direction of objects)
003318: bne     $332a
00331C: move.l  ($14,A1), D0 ;divide by two if so
003320: asr.l   #1, D0
003322: move.l  D0, ($14,A1)
003326: bra     $333e
00332A: cmpi.w  #$c0, ($36,A1) ;checks if angle is equal to 0xc0 (up direction of objects)
003330: bne     $333e
003334: move.l  ($10,A1), D0 ;divide by two if so
003338: asr.l   #1, D0
00333A: move.l  D0, ($10,A1)
00333E: movem.l (A7)+, D0/A0-A1
003342: rts

*/

/********************************************************************************************

  COPX bootleg simulation
    - Seibu Cup Soccer (bootleg)

 *******************************************************************************************/


READ16_MEMBER( seibu_cop_legacy_device::copdxbl_0_r )
{
	UINT16 retvalue = m_cop_mcu_ram[offset];

	switch(offset)
	{
		default:
		{
			logerror("%06x: COPX unhandled read returning %04x from offset %04x\n", space.device().safe_pc(), retvalue, offset*2);
			return retvalue;
		}



		//case (0x47e/2):
		//case (0x5b0/2):
		//case (0x5b4/2):
		//  return m_cop_mcu_ram[offset];

		case (0x700/2): return space.machine().root_device().ioport("DSW1")->read();
		case (0x704/2): return space.machine().root_device().ioport("PLAYERS12")->read();
		case (0x708/2): return space.machine().root_device().ioport("PLAYERS34")->read();
		case (0x70c/2): return space.machine().root_device().ioport("SYSTEM")->read();
		case (0x71c/2): return space.machine().root_device().ioport("DSW2")->read();
	}
}

WRITE16_MEMBER( seibu_cop_legacy_device::copdxbl_0_w )
{
	legionna_state *state = space.machine().driver_data<legionna_state>();
	COMBINE_DATA(&m_cop_mcu_ram[offset]);

	switch(offset)
	{
		default:
		{
			logerror("%06x: COPX unhandled write data %04x at offset %04x\n", space.device().safe_pc(), data, offset*2);
			break;
		}

		
		case (0x500/2):
		case (0x502/2):
		case (0x504/2):
			switch(m_cop_mcu_ram[offset])
			{
				case 0x8100:
				{
					int raw_angle = (space.read_word(m_raiden2cop->cop_regs[0]+(0x34^2)) & 0xff);
					double angle = raw_angle * M_PI / 128;
					double amp = (65536 >> 5)*(space.read_word(m_raiden2cop->cop_regs[0]+(0x36^2)) & 0xff);
					int res;

				/* TODO: up direction, why? */
					if(raw_angle == 0xc0)
						amp*=2;

					res = int(amp*sin(angle)) << m_raiden2cop->cop_scale;

					space.write_dword(m_raiden2cop->cop_regs[0] + 0x10, res);

					break;
				}
				case 0x8900:
				{
					int raw_angle = (space.read_word(m_raiden2cop->cop_regs[0]+(0x34^2)) & 0xff);
					double angle = raw_angle * M_PI / 128;
					double amp = (65536 >> 5)*(space.read_word(m_raiden2cop->cop_regs[0]+(0x36^2)) & 0xff);
					int res;

					/* TODO: left direction, why? */
					if(raw_angle == 0x80)
						amp*=2;

					res = int(amp*cos(angle)) << m_raiden2cop->cop_scale;

					space.write_dword(m_raiden2cop->cop_regs[0] + 20, res);

					break;
				}
				case 0x0205:
				{
					UINT8 offs;

					offs = (offset & 3) * 4;
					int ppos = space.read_dword(m_raiden2cop->cop_regs[0] + 4 + offs);
					int npos = ppos + space.read_dword(m_raiden2cop->cop_regs[0] + 0x10 + offs);
					int delta = (npos >> 16) - (ppos >> 16);

					space.write_dword(m_raiden2cop->cop_regs[0] + 4 + offs, npos);
					space.write_word(m_raiden2cop->cop_regs[0] + 0x1c + offs, space.read_word(m_raiden2cop->cop_regs[0] + 0x1c + offs) + delta);
				
					break;
				}
				case 0x130e:
				case 0x138e:
				{
					int dy = space.read_dword(m_raiden2cop->cop_regs[1]+4) - space.read_dword(m_raiden2cop->cop_regs[0]+4);
					int dx = space.read_dword(m_raiden2cop->cop_regs[1]+8) - space.read_dword(m_raiden2cop->cop_regs[0]+8);

					m_raiden2cop->cop_status = 7;
					if(!dx) {
						m_raiden2cop->cop_status |= 0x8000;
						m_raiden2cop->cop_angle = 0;
					} else {
						m_raiden2cop->cop_angle = atan(double(dy)/double(dx)) * 128.0 / M_PI;
						if(dx<0)
							m_raiden2cop->cop_angle += 0x80;
					}

					m_r0 = dy;
					m_r1 = dx;

					if(m_cop_mcu_ram[offset] & 0x80)
						space.write_word(m_raiden2cop->cop_regs[0]+(0x34^2), m_raiden2cop->cop_angle);

					break;
				}
				case 0x3b30:
				case 0x3bb0:
				{
					int dy = m_r0;
					int dx = m_r1;

					dx >>= 16;
					dy >>= 16;
					m_raiden2cop->cop_dist = sqrt((double)(dx*dx+dy*dy));

					if(m_cop_mcu_ram[offset] & 0x80)
						space.write_word(m_raiden2cop->cop_regs[0]+(0x38), m_raiden2cop->cop_dist);

					break;
				}
				default:
					printf("%04x\n",m_cop_mcu_ram[offset]);
					break;
			}
			break;
		
		/*TODO: kludge on x-axis.*/
		case (0x660/2): { state->m_scrollram16[0] = m_cop_mcu_ram[offset] - 0x1f0; break; }
		case (0x662/2): { state->m_scrollram16[1] = m_cop_mcu_ram[offset]; break; }
		case (0x664/2): { state->m_scrollram16[2] = m_cop_mcu_ram[offset] - 0x1f0; break; }
		case (0x666/2): { state->m_scrollram16[3] = m_cop_mcu_ram[offset]; break; }
		case (0x668/2): { state->m_scrollram16[4] = m_cop_mcu_ram[offset] - 0x1f0; break; }
		case (0x66a/2): { state->m_scrollram16[5] = m_cop_mcu_ram[offset]; break; }
		case (0x66c/2): { state->m_scrollram16[6] = m_cop_mcu_ram[offset] - 0x1f0; break; }
		case (0x66e/2): { state->m_scrollram16[7] = m_cop_mcu_ram[offset]; break; }

		case (0x740/2):
		{
			state->soundlatch_byte_w(space, 0, data & 0xff);
			state->m_audiocpu->set_input_line(INPUT_LINE_NMI, PULSE_LINE );
			break;
		}
	}
}

/* Generic COP functions
  -- the game specific handlers fall through to these if there
     isn't a specific case for them.  these implement behavior
     which seems common to all the games
*/





/*
_
*/

/*
Godzilla 0x12c0 X = 0x21ed Y = 0x57da
Megaron  0x12d0 X = 0x1ef1 Y = 0x55db
King Ghidorah 0x12c8 X = 0x26eb Y = 0x55dc
Mecha Ghidorah 0x12dc X = 0x24ec Y = 0x55dc
Mecha Godzilla 0x12d4 X = 0x1cf1 Y = 0x52dc
Gigan 0x12cc X = 0x23e8 Y = 0x55db

(DC.W $1020, $F0C0, $0000, $0000)
X = collides at the same spot
Y = collides between 0xd0 and 0x20
0x588 bits 2 & 3 = 0
(DC.W $F0C0, $1020, $0000, $0000)
X = collides between 0xb0 and 0x50 (inclusive)
Y = collides between 0xd0 and 0x30 (not inclusive)
0x588 bits 2 & 3 = 0x580 bits 0 & 1
*/
void seibu_cop_legacy_device::cop_take_hit_box_params(UINT8 offs)
{
	INT16 start_x,start_y,height,width;

	{
		height = UINT8(m_cop_collision_info[offs].hitbox_y >> 8);
		start_y = INT8(m_cop_collision_info[offs].hitbox_y);
		width = UINT8(m_cop_collision_info[offs].hitbox_x >> 8);
		start_x = INT8(m_cop_collision_info[offs].hitbox_x);
	}

	m_cop_collision_info[offs].min_x = (m_cop_collision_info[offs].x >> 16) + start_x;
	m_cop_collision_info[offs].max_x = m_cop_collision_info[offs].min_x + width;
	m_cop_collision_info[offs].min_y = (m_cop_collision_info[offs].y >> 16) + start_y;
	m_cop_collision_info[offs].max_y = m_cop_collision_info[offs].min_y + height;
}


UINT8 seibu_cop_legacy_device::cop_calculate_collsion_detection()
{
	static UINT8 res;

	res = 3;

	/* outbound X check */
	if(m_cop_collision_info[0].max_x >= m_cop_collision_info[1].min_x && m_cop_collision_info[0].min_x <= m_cop_collision_info[1].max_x)
		res &= ~2;

	if(m_cop_collision_info[1].max_x >= m_cop_collision_info[0].min_x && m_cop_collision_info[1].min_x <= m_cop_collision_info[0].max_x)
		res &= ~2;

	/* outbound Y check */
	if(m_cop_collision_info[0].max_y >= m_cop_collision_info[1].min_y && m_cop_collision_info[0].min_y <= m_cop_collision_info[1].max_y)
		res &= ~1;

	if(m_cop_collision_info[1].max_y >= m_cop_collision_info[0].min_y && m_cop_collision_info[1].min_y <= m_cop_collision_info[0].max_y)
		res &= ~1;

	m_cop_hit_val_x = (m_cop_collision_info[0].x - m_cop_collision_info[1].x) >> 16;
	m_cop_hit_val_y = (m_cop_collision_info[0].y - m_cop_collision_info[1].y) >> 16;
	m_cop_hit_val_z = 1;
	m_raiden2cop->cop_hit_val_stat = res; // TODO: there's also bit 2 and 3 triggered in the tests, no known meaning

	//popmessage("%d %d %04x %04x %04x %04x",m_cop_hit_val_x,m_cop_hit_val_y,m_cop_collision_info[0].hitbox_x,m_cop_collision_info[0].hitbox_y,m_cop_collision_info[1].hitbox_x,m_cop_collision_info[1].hitbox_y);

	//if(res == 0)
	//popmessage("0:%08x %08x %08x 1:%08x %08x %08x\n",m_cop_collision_info[0].x,m_cop_collision_info[0].y,m_cop_collision_info[0].hitbox,m_cop_collision_info[1].x,m_cop_collision_info[1].y,m_cop_collision_info[1].hitbox);
//  popmessage("0:%08x %08x %08x %08x 1:%08x %08x %08x %08x\n",m_cop_collision_info[0].min_x,m_cop_collision_info[0].max_x,m_cop_collision_info[0].min_y, m_cop_collision_info[0].max_y,
//                                                   m_cop_collision_info[1].min_x,m_cop_collision_info[1].max_x,m_cop_collision_info[1].min_y, m_cop_collision_info[1].max_y);

	return res;
}

READ16_MEMBER( seibu_cop_legacy_device::generic_cop_r )
{
	UINT16 retvalue;
	retvalue = m_cop_mcu_ram[offset];


	switch (offset)
	{

		/* these two controls facing direction in Godzilla opponents (only vs.) - x value compare? */
		case 0x182/2:
			return (m_cop_hit_val_y);

		case 0x184/2:
			return (m_cop_hit_val_x);

		/* Legionnaire only - z value compare? */
		case 0x186/2:
			return (m_cop_hit_val_z);





		default:
			seibu_cop_log("%06x: COPX unhandled read returning %04x from offset %04x\n", space.device().safe_pc(), retvalue, offset*2);
			return retvalue;
	}
}

WRITE16_MEMBER( seibu_cop_legacy_device::generic_cop_w )
{
	COMBINE_DATA(&m_cop_mcu_ram[offset]);

	switch (offset)
	{
		default:
			seibu_cop_log("%06x: COPX unhandled write data %04x at offset %04x\n", space.device().safe_pc(), data, offset*2);
			break;



	

		case (0x010/2):
		{
			if(data)
				printf("Warning: COP RAM 0x410 used with %04x\n",data);
			else
			{
				/* guess */
				m_raiden2cop->cop_regs[4]+=8;
				m_cop_sprite_dma_src+=6;

				m_raiden2cop->m_cop_sprite_dma_size--;

				if(m_raiden2cop->m_cop_sprite_dma_size > 0)
					m_raiden2cop->cop_status &= ~2;
				else
					m_raiden2cop->cop_status |= 2;
			}
			break;
		}

		case (0x012/2):
		case (0x014/2):
			m_cop_sprite_dma_src = (m_cop_mcu_ram[0x014/2]) | (m_cop_mcu_ram[0x012/2] << 16);
			break;

		/* triggered before 0x6200 in Seibu Cup, looks like an angle value ... */
		case (0x01c/2): m_legacycop_angle_compare = UINT16(m_cop_mcu_ram[0x1c/2]);  break;
		case (0x01e/2): m_legacycop_angle_mod_val = UINT16(m_cop_mcu_ram[0x1e/2]); break;




			
			

		
		case (0x03e/2):
			/*
			0 in all 68k based games
			0xffff in raiden2 / raidendx
			0x2000 in zeroteam / xsedae
			it's always setted up just before the 0x474 register
			*/
			break;

		case (0x046/2): { m_cop_rom_addr_unk = data & 0xffff; break; }
		case (0x048/2): { m_cop_rom_addr_lo = data & 0xffff; break; }
		case (0x04a/2): { m_cop_rom_addr_hi = data & 0xffff; break; }

	
		/* DMA / layer clearing section */
		case (0x074/2):
			/*
			This sets up a DMA mode of some sort
			    0x0e00: grainbow, cupsoc
			    0x0a00: legionna, godzilla, denjinmk
			    0x0600: heatbrl
			    0x1e00: zeroteam, xsedae
			raiden2 and raidendx doesn't set this up, this could indicate that this is related to the non-private buffer DMAs
			(both only uses 0x14 and 0x15 as DMAs)
			*/
			break;

		








		case (0x08c/2): m_cop_sprite_dma_abs_y = (m_cop_mcu_ram[0x08c/2]); break;
		case (0x08e/2): m_cop_sprite_dma_abs_x = (m_cop_mcu_ram[0x08e/2]); break;

	

		case (0x100/2):
		case (0x102/2):
		case (0x104/2):
		{
			int command;


			logerror("%06x: COPX execute table macro command %04x %04x | regs %08x %08x %08x %08x %08x\n", space.device().safe_pc(), data, m_cop_mcu_ram[offset], m_raiden2cop->cop_regs[0], m_raiden2cop->cop_regs[1], m_raiden2cop->cop_regs[2], m_raiden2cop->cop_regs[3], m_raiden2cop->cop_regs[4]);


			command = m_raiden2cop->find_trigger_match(m_cop_mcu_ram[offset], 0xff00);


			if (command==-1)
			{
				return;
			}
			UINT16 funcval,funcmask;
			// this is pointless.. all we use it for is comparing against the same value
			funcval = m_raiden2cop->get_func_value(command);
			funcmask = m_raiden2cop->get_func_mask(command);
			//printf("%04x %04x %04x\n",m_cop_mcu_ram[offset],funcval,funcmask);

			/*
			Macro notes:
			- endianess changes from/to Raiden 2:
			  dword ^= 0
			  word ^= 2
			  byte ^= 3
			- some macro commands here have a commented algorithm, it's how Seibu Cup Bootleg version handles maths inside the 14/15 roms.
			  The ROMs map tables in the following arrangement:
			  0x00000 - 0x1ffff Sine math results
			  0x20000 - 0x3ffff Cosine math results
			  0x40000 - 0x7ffff Division math results
			  0x80000 - 0xfffff Pythagorean theorem, hypotenuse length math results
			  Surprisingly atan maths are nowhere to be found from the roms.
			*/

			int executed = 0;

			/* "automatic" movement, 0205 */
			if(m_raiden2cop->check_command_matches(command, 0x188,0x282,0x082,0xb8e,0x98e,0x000,0x000,0x000,6,0xffeb))
			{
				executed = 1;
				UINT8 offs;

				offs = (offset & 3) * 4;
				int ppos = space.read_dword(m_raiden2cop->cop_regs[0] + 4 + offs);
				int npos = ppos + space.read_dword(m_raiden2cop->cop_regs[0] + 0x10 + offs);
				int delta = (npos >> 16) - (ppos >> 16);

				space.write_dword(m_raiden2cop->cop_regs[0] + 4 + offs, npos);
				space.write_word(m_raiden2cop->cop_regs[0] + 0x1c + offs, space.read_word(m_raiden2cop->cop_regs[0] + 0x1c + offs) + delta);
				return;
			}

			/* "automatic" movement, for arcs in Legionnaire / Zero Team (expression adjustment) 0905 */
			if(m_raiden2cop->check_command_matches(command, 0x194,0x288,0x088,0x000,0x000,0x000,0x000,0x000,6,0xfbfb))
			{
				executed = 1;
				UINT8 offs;

				offs = (offset & 3) * 4;

				/* read 0x28 + offs */
				/* add 0x10 + offs */
				/* write 0x10 + offs */

				space.write_dword(m_raiden2cop->cop_regs[0] + 0x10 + offs, space.read_dword(m_raiden2cop->cop_regs[0] + 0x10 + offs) + space.read_dword(m_raiden2cop->cop_regs[0] + 0x28 + offs));
				return;
			}

			/* SINE math - 0x8100 */
			/*
			     00000-0ffff:
			       amp = x/256
			       ang = x & 255
			       s = sin(ang*2*pi/256)
			       val = trunc(s*amp)
			       if(s<0)
			         val--
			       if(s == 192)
			         val = -2*amp
			*/
			if(m_raiden2cop->check_command_matches(command, 0xb9a,0xb88,0x888,0x000,0x000,0x000,0x000,0x000,7,0xfdfb))
			{
				executed = 1;
				int raw_angle = (space.read_word(m_raiden2cop->cop_regs[0]+(0x34^2)) & 0xff);
				double angle = raw_angle * M_PI / 128;
				double amp = (65536 >> 5)*(space.read_word(m_raiden2cop->cop_regs[0]+(0x36^2)) & 0xff);
				int res;

				/* TODO: up direction, why? */
				if(raw_angle == 0xc0)
					amp*=2;

				res = int(amp*sin(angle)) << m_raiden2cop->cop_scale;

				space.write_dword(m_raiden2cop->cop_regs[0] + 0x10, res);
				return;
			}

			/* COSINE math - 0x8900 */
			/*
			 10000-1ffff:
			   amp = x/256
			   ang = x & 255
			   s = cos(ang*2*pi/256)
			   val = trunc(s*amp)
			   if(s<0)
			     val--
			   if(s == 128)
			     val = -2*amp
			*/
			if(m_raiden2cop->check_command_matches(command, 0xb9a,0xb8a,0x88a,0x000,0x000,0x000,0x000,0x000,7,0xfdfb))
			{
				executed = 1;
				int raw_angle = (space.read_word(m_raiden2cop->cop_regs[0]+(0x34^2)) & 0xff);
				double angle = raw_angle * M_PI / 128;
				double amp = (65536 >> 5)*(space.read_word(m_raiden2cop->cop_regs[0]+(0x36^2)) & 0xff);
				int res;

				/* TODO: left direction, why? */
				if(raw_angle == 0x80)
					amp*=2;

				res = int(amp*cos(angle)) << m_raiden2cop->cop_scale;

				space.write_dword(m_raiden2cop->cop_regs[0] + 20, res);
				return;
			}

			/* 0x130e / 0x138e */
			if(m_raiden2cop->check_command_matches(command, 0x984,0xaa4,0xd82,0xaa2,0x39b,0xb9a,0xb9a,0xa9a,5,0xbf7f))
			{
				executed = 1;
				int dy = space.read_dword(m_raiden2cop->cop_regs[1]+4) - space.read_dword(m_raiden2cop->cop_regs[0]+4);
				int dx = space.read_dword(m_raiden2cop->cop_regs[1]+8) - space.read_dword(m_raiden2cop->cop_regs[0]+8);

				m_raiden2cop->cop_status = 7;
				if(!dx) {
					m_raiden2cop->cop_status |= 0x8000;
					m_raiden2cop->cop_angle = 0;
				} else {
					m_raiden2cop->cop_angle = atan(double(dy)/double(dx)) * 128.0 / M_PI;
					if(dx<0)
						m_raiden2cop->cop_angle += 0x80;
				}

				m_r0 = dy;
				m_r1 = dx;

				//printf("%d %d %f %04x\n",dx,dy,atan(double(dy)/double(dx)) * 128 / M_PI,m_raiden2cop->cop_angle);

				if(m_cop_mcu_ram[offset] & 0x80)
					space.write_word(m_raiden2cop->cop_regs[0]+(0x34^2), m_raiden2cop->cop_angle);
				return;
			}

			/* Pythagorean theorem, hypotenuse direction - 130e / 138e */
			//(heatbrl)  | 5 | bf7f | 138e | 984 aa4 d82 aa2 39b b9a b9a b9a
			if(m_raiden2cop->check_command_matches(command, 0x984,0xaa4,0xd82,0xaa2,0x39b,0xb9a,0xb9a,0xb9a,5,0xbf7f))
			{
				executed = 1;
				int dy = space.read_dword(m_raiden2cop->cop_regs[1]+4) - space.read_dword(m_raiden2cop->cop_regs[0]+4);
				int dx = space.read_dword(m_raiden2cop->cop_regs[1]+8) - space.read_dword(m_raiden2cop->cop_regs[0]+8);

				m_raiden2cop->cop_status = 7;
				if(!dx) {
					m_raiden2cop->cop_status |= 0x8000;
					m_raiden2cop->cop_angle = 0;
				} else {
					m_raiden2cop->cop_angle = atan(double(dy)/double(dx)) * 128.0 / M_PI;
					if(dx<0)
						m_raiden2cop->cop_angle += 0x80;
				}

				m_raiden2cop->cop_angle-=0x80;
				m_r0 = dy;
				m_r1 = dx;

				if(m_cop_mcu_ram[offset] & 0x80)
					space.write_word(m_raiden2cop->cop_regs[0]+(0x34^2), m_raiden2cop->cop_angle);
				return;
			}

			/* Pythagorean theorem, hypotenuse length - 0x3bb0 */
			//(grainbow) | 4 | 007f | 3bb0 | f9c b9c b9c b9c b9c b9c b9c 99c
			/*
			 40000-7ffff:
			   v1 = (x / 32768)*64
			   v2 = (x & 255)*32767/255
			   val = sqrt(v1*v1+v2*v2) (unsigned)
			*/
			if(m_raiden2cop->check_command_matches(command, 0xf9c,0xb9c,0xb9c,0xb9c,0xb9c,0xb9c,0xb9c,0x99c,4,0x007f))
			{
				executed = 1;
				int dy = m_r0;
				int dx = m_r1;

				dx >>= 16;
				dy >>= 16;
				m_raiden2cop->cop_dist = sqrt((double)(dx*dx+dy*dy));

				if(m_cop_mcu_ram[offset] & 0x80)
					space.write_word(m_raiden2cop->cop_regs[0]+(0x38), m_raiden2cop->cop_dist);
				return;
			}

			/* Division - 0x42c2 */
			/*
			 20000-2ffff:
			   v1 = x / 1024
			   v2 = x & 1023
			   val = !v1 ? 32767 : trunc(v2/v1+0.5)
			 30000-3ffff:
			   v1 = x / 1024
			   v2 = (x & 1023)*32
			   val = !v1 ? 32767 : trunc(v2/v1+0.5)
			*/
			if(m_raiden2cop->check_command_matches(command, 0xf9a,0xb9a,0xb9c,0xb9c,0xb9c,0x29c,0x000,0x000,5,0xfcdd))
			{
				executed = 1;
				int dy = m_r0;
				int dx = m_r1;
				int div = space.read_word(m_raiden2cop->cop_regs[0]+(0x36^2));
				int res;
				int cop_dist_raw;

				if(!div)
				{
					printf("divide by zero?\n");
					div = 1;
				}

				/* TODO: calculation of this one should occur at 0x3b30/0x3bb0 I *think* */
				/* TODO: recheck if m_raiden2cop->cop_scale still masks at 3 with this command */
				dx >>= 11 + m_raiden2cop->cop_scale;
				dy >>= 11 + m_raiden2cop->cop_scale;
				cop_dist_raw = sqrt((double)(dx*dx+dy*dy));

				res = cop_dist_raw;
				res /= div;

				m_raiden2cop->cop_dist = (1 << (5 - m_raiden2cop->cop_scale)) / div;

				/* TODO: bits 5-6-15 */
				m_raiden2cop->cop_status = 7;

				space.write_word(m_raiden2cop->cop_regs[0]+(0x38^2), res);
				return;
			}

			/*
			    collision detection:

			    int dy_0 = space.read_dword(m_raiden2cop->cop_regs[0]+4);
			    int dx_0 = space.read_dword(m_raiden2cop->cop_regs[0]+8);
			    int dy_1 = space.read_dword(m_raiden2cop->cop_regs[1]+4);
			    int dx_1 = space.read_dword(m_raiden2cop->cop_regs[1]+8);
			    int hitbox_param1 = space.read_dword(m_raiden2cop->cop_regs[2]);
			    int hitbox_param2 = space.read_dword(m_raiden2cop->cop_regs[3]);

			    TODO: we are ignoring the funcval / funcmask params for now
			*/

			if(m_raiden2cop->check_command_matches(command, 0xb80,0xb82,0xb84,0xb86,0x000,0x000,0x000,0x000,funcval,funcmask))
			{
				executed = 1;
				m_cop_collision_info[0].y = (space.read_dword(m_raiden2cop->cop_regs[0]+4));
				m_cop_collision_info[0].x = (space.read_dword(m_raiden2cop->cop_regs[0]+8));
				return;
			}

			//(heatbrl)  | 9 | ffff | b080 | b40 bc0 bc2
			if(m_raiden2cop->check_command_matches(command, 0xb40,0xbc0,0xbc2,0x000,0x000,0x000,0x000,0x000,funcval,funcmask))
			{
				executed = 1;
				m_cop_collision_info[0].hitbox = space.read_word(m_raiden2cop->cop_regs[2]);
				m_cop_collision_info[0].hitbox_y = space.read_word((m_raiden2cop->cop_regs[2]&0xffff0000)|(m_cop_collision_info[0].hitbox));
				m_cop_collision_info[0].hitbox_x = space.read_word(((m_raiden2cop->cop_regs[2]&0xffff0000)|(m_cop_collision_info[0].hitbox))+2);

				/* do the math */
				cop_take_hit_box_params(0);
				m_raiden2cop->cop_hit_status = cop_calculate_collsion_detection();

				return;
			}

			if(m_raiden2cop->check_command_matches(command, 0xba0,0xba2,0xba4,0xba6,0x000,0x000,0x000,0x000,funcval,funcmask))
			{
				executed = 1;
				m_cop_collision_info[1].y = (space.read_dword(m_raiden2cop->cop_regs[1]+4));
				m_cop_collision_info[1].x = (space.read_dword(m_raiden2cop->cop_regs[1]+8));
				return;
			}

			//(heatbrl)  | 6 | ffff | b880 | b60 be0 be2
			if(m_raiden2cop->check_command_matches(command, 0xb60,0xbe0,0xbe2,0x000,0x000,0x000,0x000,0x000,funcval,funcmask))
			{
				executed = 1;
				m_cop_collision_info[1].hitbox = space.read_word(m_raiden2cop->cop_regs[3]);
				m_cop_collision_info[1].hitbox_y = space.read_word((m_raiden2cop->cop_regs[3]&0xffff0000)|(m_cop_collision_info[1].hitbox));
				m_cop_collision_info[1].hitbox_x = space.read_word(((m_raiden2cop->cop_regs[3]&0xffff0000)|(m_cop_collision_info[1].hitbox))+2);

				/* do the math */
				cop_take_hit_box_params(1);
				m_raiden2cop->cop_hit_status = cop_calculate_collsion_detection();
				return;
			}

			// grainbow 0d | a | fff3 | 6980 | b80 ba0
			if(m_raiden2cop->check_command_matches(command, 0xb80,0xba0,0x000,0x000,0x000,0x000,0x000,0x000,10,0xfff3))
			{
				executed = 1;
				UINT8 offs;
				int abs_x,abs_y,rel_xy;

				offs = (offset & 3) * 4;

				/* TODO: I really suspect that following two are actually taken from the 0xa180 macro command then internally loaded */
				abs_x = space.read_word(m_raiden2cop->cop_regs[0] + 8) - m_cop_sprite_dma_abs_x;
				abs_y = space.read_word(m_raiden2cop->cop_regs[0] + 4) - m_cop_sprite_dma_abs_y;
				rel_xy = space.read_word(m_cop_sprite_dma_src + 4 + offs);

				//if(rel_xy & 0x0706)
				//  printf("sprite rel_xy = %04x\n",rel_xy);

				if(rel_xy & 1)
					space.write_word(m_raiden2cop->cop_regs[4] + offs + 4,0xc0 + abs_x - (rel_xy & 0xf8));
				else
					space.write_word(m_raiden2cop->cop_regs[4] + offs + 4,(((rel_xy & 0x78) + (abs_x) - ((rel_xy & 0x80) ? 0x80 : 0))));

				space.write_word(m_raiden2cop->cop_regs[4] + offs + 6,(((rel_xy & 0x7800) >> 8) + (abs_y) - ((rel_xy & 0x8000) ? 0x80 : 0)));
				return;
			}

			// grainbow 18 | a | ff00 | c480 | 080 882
			if(m_raiden2cop->check_command_matches(command, 0x080,0x882,0x000,0x000,0x000,0x000,0x000,0x000,10,0xff00))
			{
				executed = 1;
				UINT8 offs;

				offs = (offset & 3) * 4;

				space.write_word(m_raiden2cop->cop_regs[4] + offs + 0,space.read_word(m_cop_sprite_dma_src + offs) + (m_raiden2cop->m_cop_sprite_dma_param & 0x3f));
				//space.write_word(m_raiden2cop->cop_regs[4] + offs + 2,space.read_word(m_cop_sprite_dma_src+2 + offs));
				return;
			}

			// cupsoc 1b | 5 | 7ff7 | dde5 | f80 aa2 984 0c2
			/* radar x/y positions */
			/* FIXME: x/ys are offsetted */
			/* FIXME: uses 0x10044a for something */
			if(m_raiden2cop->check_command_matches(command, 0xf80,0xaa2,0x984,0x0c2,0x000,0x000,0x000,0x000,5,0x7ff7))
			{
				executed = 1;
				UINT8 offs;
				int div;
				INT16 dir_offset;
//              INT16 offs_val;

				/* TODO: [4-7] could be mirrors of [0-3] (this is the only command so far that uses 4-7 actually)*/
				/* ? 0 + [4] */
				/* sub32 4 + [5] */
				/* write16h 8 + [4] */
				/* addmem16 4 + [6] */

				// these two are obvious ...
				// 0xf x 16 = 240
				// 0x14 x 16 = 320
				// what are these two instead? scale factor? offsets? (edit: offsets to apply from the initial sprite data)
				// 0xfc69 ?
				// 0x7f4 ?
				//printf("%08x %08x %08x %08x %08x %08x %08x\n",m_raiden2cop->cop_regs[0],m_raiden2cop->cop_regs[1],m_raiden2cop->cop_regs[2],m_raiden2cop->cop_regs[3],m_raiden2cop->cop_regs[4],m_raiden2cop->cop_regs[5],m_raiden2cop->cop_regs[6]);

				offs = (offset & 3) * 4;

				div = space.read_word(m_raiden2cop->cop_regs[4] + offs);
				dir_offset = space.read_word(m_raiden2cop->cop_regs[4] + offs + 8);
//              offs_val = space.read_word(m_raiden2cop->cop_regs[3] + offs);
				//420 / 180 = 500 : 400 = 30 / 50 = 98 / 18
				
				/* TODO: this probably trips a cop status flag */
				if(div == 0) { div = 1; }
				
				
				space.write_word((m_raiden2cop->cop_regs[6] + offs + 4), ((space.read_word(m_raiden2cop->cop_regs[5] + offs + 4) + dir_offset) / div));
				return;
			}

			//(cupsoc)   | 8 | f3e7 | 6200 | 3a0 3a6 380 aa0 2a6
			if(m_raiden2cop->check_command_matches(command, 0x3a0,0x3a6,0x380,0xaa0,0x2a6,0x000,0x000,0x000,8,0xf3e7))
			{
				executed = 1;
				UINT8 cur_angle;
				UINT16 flags;
				
				/* 0 [1] */
				/* 0xc [1] */
				/* 0 [0] */
				/* 0 [1] */
				/* 0xc [1] */

				cur_angle = space.read_byte(m_raiden2cop->cop_regs[1] + (0xc ^ 3));
				flags = space.read_word(m_raiden2cop->cop_regs[1]);
				//space.write_byte(m_raiden2cop->cop_regs[1] + (0^3),space.read_byte(m_raiden2cop->cop_regs[1] + (0^3)) & 0xfb); //correct?

				m_legacycop_angle_compare &= 0xff;
				m_legacycop_angle_mod_val &= 0xff;
				flags &= ~0x0004;
				
				int delta = cur_angle - m_legacycop_angle_compare;
				if(delta >= 128)
					delta -= 256;
				else if(delta < -128)
					delta += 256;
				if(delta < 0)
				{
					if(delta >= -m_legacycop_angle_mod_val)
					{
						cur_angle = m_legacycop_angle_compare;
						flags |= 0x0004;
					} 
					else
						cur_angle += m_legacycop_angle_mod_val;
				} 
				else
				{
					if(delta <= m_legacycop_angle_mod_val)
					{
						cur_angle = m_legacycop_angle_compare;
						flags |= 0x0004;
					} 
					else
						cur_angle -= m_legacycop_angle_mod_val;
				}

				space.write_byte(m_raiden2cop->cop_regs[1] + (0 ^ 2),flags);
				space.write_byte(m_raiden2cop->cop_regs[1] + (0xc ^ 3),cur_angle);
				return;
			}

			//(grainbow) | 8 | f3e7 | 6200 | 380 39a 380 a80 29a
			/* search direction, used on SD Gundam homing weapon */
			/* FIXME: still doesn't work ... */
			if(m_raiden2cop->check_command_matches(command, 0x380,0x39a,0x380,0xa80,0x29a,0x000,0x000,0x000,8,0xf3e7))
			{
				executed = 1;
				UINT8 cur_angle;
				UINT16 flags;
				
				cur_angle = space.read_byte(m_raiden2cop->cop_regs[0] + (0x34 ^ 3));
				flags = space.read_word(m_raiden2cop->cop_regs[0] + (0 ^ 2));
				//space.write_byte(m_raiden2cop->cop_regs[1] + (0^3),space.read_byte(m_raiden2cop->cop_regs[1] + (0^3)) & 0xfb); //correct?

				m_legacycop_angle_compare &= 0xff;
				m_legacycop_angle_mod_val &= 0xff;
				flags &= ~0x0004;
				
				int delta = cur_angle - m_legacycop_angle_compare;
				if(delta >= 128)
					delta -= 256;
				else if(delta < -128)
					delta += 256;
				if(delta < 0)
				{
					if(delta >= -m_legacycop_angle_mod_val)
					{
						cur_angle = m_legacycop_angle_compare;
						flags |= 0x0004;
					} 
					else
						cur_angle += m_legacycop_angle_mod_val;
				} 
				else
				{
					if(delta <= m_legacycop_angle_mod_val)
					{
						cur_angle = m_legacycop_angle_compare;
						flags |= 0x0004;
					} 
					else
						cur_angle -= m_legacycop_angle_mod_val;
				}

				space.write_byte(m_raiden2cop->cop_regs[0] + (0 ^ 3),flags);
				space.write_word(m_raiden2cop->cop_regs[0] + (0x34 ^ 3),cur_angle);
				return;
			}

			//(cupsoc) 1c | 5 | b07f | e38e | 984 ac4 d82 ac2 39b b9a b9a a9a
			if(m_raiden2cop->check_command_matches(command, 0x984,0xac4,0xd82,0xac2,0x39b,0xb9a,0xb9a,0xa9a,5,0xb07f))
			{
				executed = 1;
				int dy = space.read_dword(m_raiden2cop->cop_regs[2]+4) - space.read_dword(m_raiden2cop->cop_regs[0]+4);
				int dx = space.read_dword(m_raiden2cop->cop_regs[2]+8) - space.read_dword(m_raiden2cop->cop_regs[0]+8);

				m_raiden2cop->cop_status = 7;
				if(!dx) {
					m_raiden2cop->cop_status |= 0x8000;
					m_raiden2cop->cop_angle = 0;
				} else {
					m_raiden2cop->cop_angle = atan(double(dy)/double(dx)) * 128.0 / M_PI;
					if(dx<0)
						m_raiden2cop->cop_angle += 0x80;
				}

				m_r0 = dy;
				m_r1 = dx;

				//printf("%d %d %f %04x\n",dx,dy,atan(double(dy)/double(dx)) * 128 / M_PI,m_raiden2cop->cop_angle);

				if(m_cop_mcu_ram[offset] & 0x80)
					space.write_word(m_raiden2cop->cop_regs[0]+(0x34^2), m_raiden2cop->cop_angle);
				return;
			}

			//(cupsoc) 1a | 5 | fffb | d104 | ac2 9e0 0a2
			/* controls player vs. player collision detection, 0xf105 controls player vs. ball */
			if(m_raiden2cop->check_command_matches(command, 0xac2,0x9e0,0x0a2,0x000,0x000,0x000,0x000,0x000,5,0xfffb))
			{
				executed = 1;
				UINT8 *ROM = space.machine().root_device().memregion("maincpu")->base();
				UINT32 rom_addr = (m_cop_rom_addr_hi << 16 | m_cop_rom_addr_lo) & ~1;
				UINT16 rom_data = (ROM[rom_addr + 0]) | (ROM[rom_addr + 1]<<8);

				/* writes to some unemulated COP registers, then puts the result in here, adding a parameter taken from ROM */
				//space.write_word(m_raiden2cop->cop_regs[0]+(0x44 + offset * 4), rom_data);

				printf("%04x%04x %04x %04x\n",m_cop_rom_addr_hi,m_cop_rom_addr_lo,m_cop_rom_addr_unk,rom_data);
				return;
			}

			if (executed==0) printf("did not execute %04x\n",m_cop_mcu_ram[offset]);
			break;
		}





	}
}
