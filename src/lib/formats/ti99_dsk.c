// license:BSD-3-Clause
// copyright-holders:Michael Zapf
/*********************************************************************
 *
 * formats/ti99_dsk.c
 *
 * TI-99 family disk images
 *
 * used by TI-99/4, TI-99/4A, TI-99/8, SGCPU ("TI-99/4P"), and Geneve
 *
 * Sector Dump Format, aka "v9t9" format
 *    customized wd177x_format
 *    no track data
 *
 * Track Dump Format, aka "pc99" format
 *    contains all track information, but no clock patterns
 *
 * Both formats allow for a broad range of medium sizes. All sectors are 256
 * bytes long. The most common formats are 9 sectors per track, single-sided,
 * 40 tracks, which yields 90 KiB of sector data (known as SSSD), and 18
 * sectors per track, double-sided, and 40 tracks, which is 360 KiB (known as
 * DSDD). There are rare occurances of 8/16 sectors/track
 * (prototypical TI double-density controller) and 35 track media. Newer
 * controllers and ROMs allow for up to 36 sectors per track and 80 tracks on
 * both sides, which is 1,44 MiB (DSHD80).
 *
 * Double stepping
 * --------------
 * When using a 40-track disk in an 80-track drive, it seems as if each
 * track appears twice in sequence (0,0,1,1,2,2,...,39,39).
 * The system will write to each second track and leave the others untouched.
 * When we write back that image, there is no simple way to know that
 * the 80 tracks are in fact doubled 40 tracks. We will actually write
 * all 80 tracks, doubling the size of the image file. This disk image will
 * now become unusable in a 40-track drive - which is exactly what happens in reality.
 *
 * -------------------------------------------------------------------
 * The second half of this file contains the legacy implementation.
 *
 * Michael Zapf, Sep 2014
 *
 ********************************************************************/

#include "emu.h"
#include <string.h>
#include <assert.h>
#include <time.h>

#include "imageutl.h"
#include "ti99_dsk.h"

#define SECTOR_SIZE 256

// Debugging
#define TRACE 0

// ====================================================
//  Common methods for both formats.
// ====================================================

/*
    Find out whether there are IDAMs and DAMs (without having clock bits).
    As said above, let's not spend too much effort allowing format deviations.
    If the image does not exactly adhere to the format, give up.
*/
bool ti99_floppy_format::check_for_address_marks(UINT8* trackdata, int encoding)
{
	int i=0;

	if (encoding==floppy_image::FM)
	{
		// Check 5 sectors of track 0
		while (i < 5)
		{
			if (trackdata[16 + 6 + i*334] != 0xfe) break;
			if (trackdata[16 + 30 + i*334] != 0xfb && trackdata[16 + 30 + i*334] != 0xf8) break;
			i++;
		}
	}
	else
	{
		// Try MFM
		i = 0;
		while (i < 5)
		{
			if (trackdata[40 + 13 + i*340] != 0xfe) break;
			if (trackdata[40 + 57 + i*340] != 0xfb && trackdata[40 + 57 + i*334] != 0xf8) break;
			i++;
		}
	}
	return (i==5);
}

int ti99_floppy_format::get_encoding(int cell_size)
{
	return (cell_size==4000)? floppy_image::FM : floppy_image::MFM;
}

/*
    Load the image from disk and convert it into a sequence of flux levels.
*/
bool ti99_floppy_format::load(io_generic *io, UINT32 form_factor, floppy_image *image)
{
	int cell_size = 0;
	int sector_count = 0;
	int heads = 0;
	determine_sizes(io, cell_size, sector_count, heads);

	if (cell_size == 0) return false;

	// Be ready to hold a track of up to 36 sectors (with gaps and marks)
	UINT8 trackdata[13000];

	int maxtrack, maxheads;
	image->get_maximal_geometry(maxtrack, maxheads);

	int file_size = io_generic_size(io);
	int track_size = get_track_size(cell_size, sector_count);
	int track_count = file_size / (track_size*heads);

	if (TRACE) logerror("ti99_dsk: track count = %d\n", track_count);

	if (track_count > maxtrack)
	{
		logerror("ti99_dsk: Floppy disk has too many tracks for this drive.\n");
		return false;
	}

	bool doubletracks = (track_count * 2 <= maxtrack);

	if (doubletracks) logerror("ti99_dsk: 40-track image in an 80-track drive. On save, image size will double.\n");

	// Read the image
	for(int head=0; head < heads; head++)
	{
		for(int track=0; track < track_count; track++)
		{
			load_track(io, trackdata, head, track, sector_count, track_count, cell_size);

			if (doubletracks)
			{
				// Create two tracks from each medium track. This reflects the
				// fact that the drive head will find the same data after
				// a single step
				if (get_encoding(cell_size)==floppy_image::FM)
				{
					generate_track_fm(track*2, head, cell_size, trackdata, image);
					generate_track_fm(track*2+1, head, cell_size, trackdata, image);
				}
				else
				{
					generate_track_mfm(track*2, head, cell_size, trackdata, image);
					generate_track_mfm(track*2+1, head, cell_size, trackdata, image);
				}
			}
			else
			{
				// Normal tracks
				if (get_encoding(cell_size)==floppy_image::FM)
					generate_track_fm(track, head, cell_size, trackdata, image);
				else
					generate_track_mfm(track, head, cell_size, trackdata, image);
			}
		}
	}
	return true;
}

bool ti99_floppy_format::save(io_generic *io, floppy_image *image)
{
	int act_track_size = 0;

	UINT8 bitstream[500000/8];
	UINT8 trackdata[9216];   // max size

	int cellsizes[] = { 2000, 4000, 1000, 2000 };

	// Do we use double-stepping?
	// If our image was loaded into a 80-track drive, we will always write 80 tracks.
	int maxtrack, maxheads;
	image->get_maximal_geometry(maxtrack, maxheads);

	if (maxtrack > 80) maxtrack = 80;
	else
	{
		if (maxtrack > 35 && maxtrack < 80) maxtrack = 40;
		else maxtrack = 35;
	}

	int attempt = 0;
	int sector[36];
	int maxsect = 18;
	bool write = true;

	// We expect a bitstream of length 50000 for FM and 100000 for MFM
	for(int head=0; head < 2; head++)
	{
		int track = 0;
		while (track < maxtrack)
		{
			int cell_size = cellsizes[attempt];
			int encoding = get_encoding(cell_size);
			int track_size = get_track_size(cell_size, 36); // max number of sectors

			// Retrieve the cells from the flux sequence
			generate_bitstream_from_track(track, head, cell_size, bitstream, act_track_size, image);

			// Maybe the track has become longer due to moving splices
			if (act_track_size > 200000000/cell_size) act_track_size = 200000000/cell_size;

			int marks = decode_bitstream(bitstream, trackdata, sector, act_track_size, encoding, (encoding==floppy_image::FM)? 0xff:0x4e, track_size);

			if (track==0)
			{
				if (head==0)
				{
					// Find the highest sector in the track
					// This is only needed for the SDF format
					int i=35;
					while (i>=0 && sector[i]==-1) i--;

					if (i>18) maxsect = 36;
					else
					{
						if (i>16) maxsect = 18;
						else
						{
							if (i>9) maxsect = 16;
							else maxsect = 9;
						}
					}
					if (TRACE) logerror("ti99_dsk: Sectors/track: %d\n", maxsect);

					// We try different cell sizes until we find a fitting size.
					// If this fails, we fall back to a size of 2000 ns
					// The criterion for a successful decoding is that we find at least
					// 6 ID or data address marks on the track. It is highly unlikely
					// to find so many marks with a wrong cell size.
					if (marks < 6 && attempt < 4)
					{
						if (TRACE) logerror("ti99_dsk: Decoding with cell size %d failed.\n", cell_size);
						attempt++;
						write = false;
					}
					else write = true;
				}
				else
				{
					if (marks < 6)
					{
						if (min_heads()==1)
						{
							if (TRACE) logerror("ti99_dsk: We don't have a second side and the format allows for single-sided recording.\n");
							return true;
						}
						else
						{
							logerror("ti99_dsk: No second side, but this format requires two-sided recording. Saving empty tracks.\n");
						}
					}
				}
			}

			if (write)
			{
				if (TRACE)
				{
					if (head == 0 && track == 0)
					{
						if (marks >=6) { if (TRACE) logerror("ti99_dsk: Decoding with cell size %d successful.\n", cell_size); }
						else logerror("ti99_dsk: No address marks found on track 0. Assuming MFM format.\n");
					}
				}
				// Save to the file
				write_track(io, trackdata, sector, track, head, maxsect, maxtrack, track_size);
				track++;
			}
		}
	}

	return true;
}

void ti99_floppy_format::generate_track_fm(int track, int head, int cell_size, UINT8* trackdata, floppy_image *image)
{
	int track_size_cells = 200000000/cell_size;
	UINT32 *buffer = global_alloc_array_clear(UINT32, 200000000/cell_size);

	// The TDF has a long track lead-out that makes the track length sum up
	// to 3253; this is too long for the number of cells in the real track.
	// This was either an error when that format was defined,
	// or it is due to the fact that when reading a track via
	// the controller, after the track has been read, the controller still
	// delivers some FF values until it times out.

	// Accordingly, we limit the track size to cell_number / 16,
	// which means 3125 for FM
	// This also means that Gap 4 (lead-out) is not 231 bytes long but only 103 bytes

	int track_size = track_size_cells / 16;

	int offset = 0;
	short crc1, crc2, found_crc;
	int start = 16;

	if (check_for_address_marks(trackdata, floppy_image::FM)==false)
	{
		if (head==0 && track==0) logerror("ti99_dsk: Cannot find FM address marks on track %d, head %d; likely broken or unformatted.\n", track, head);
		global_free_array(buffer);
		return;
	}

	// Found a track; we now know where the address marks are:
	// (start is positioned at the pre-id gap)
	// IDAM: start + 6 + n*334
	// DAM:  start + 30 + n*334
	// and the CRCs are at
	// CRC1: start + 11 + n*334
	// CRC2: start + 287 + n*334
	// If the CRCs are F7F7, we recalculate them.
	for (int i=0; i < track_size; i++)
	{
		if (((i-start-6)%334==0) && (i < start + 9*334))
		{
			// IDAM
			raw_w(buffer, offset, 16, 0xf57e);
		}
		else
		{
			if (((i-start-30)%334==0) && (i < start + 9*334))
			{
				// DAM
				raw_w(buffer, offset, 16, 0xf56f);
			}
			else
			{
				if (((i-start-11)%334==0) && (i < start + 9*334))
				{
					// CRC1
					crc1 = ccitt_crc16(0xffff, &trackdata[i-5], 5);
					found_crc = (trackdata[i]<<8 | trackdata[i+1]);
					if ((found_crc & 0xffff) == 0xf7f7)
					{
						// PC99 format: no real CRC; replace it
						// Also, when converting from SDF we let this method create the proper CRC.
						// logerror("Warning: PC99 format using pseudo CRC1; replace by %04x\n", crc1);
						trackdata[i] = (crc1 >> 8) & 0xff;
						trackdata[i+1] = crc1 & 0xff;
						found_crc = crc1;
					}
					if (crc1 != found_crc)
					{
						logerror("ti99_dsk: Warning: CRC1 does not match (track=%d, head=%d). Found = %04x, calc = %04x\n", track, head, found_crc& 0xffff, crc1& 0xffff);
					}
				}
				else
				{
					if (((i-start-287)%334==0) && (i < start + 9*334))
					{
						// CRC2
						crc2 = ccitt_crc16(0xffff, &trackdata[i-SECTOR_SIZE-1], SECTOR_SIZE+1);
						found_crc = (trackdata[i]<<8 | trackdata[i+1]);
						if ((found_crc & 0xffff) == 0xf7f7)
						{
							// PC99 format: no real CRC; replace it
							// logerror("Warning: PC99 format using pseudo CRC2; replace by %04x\n", crc2);
							trackdata[i] = (crc2 >> 8) & 0xff;
							trackdata[i+1] = crc2 & 0xff;
							found_crc = crc2;
						}
						if (crc2 != found_crc)
						{
							logerror("ti99_dsk: Warning: CRC2 does not match (track=%d, head=%d). Found = %04x, calc = %04x\n", track, head, found_crc& 0xffff, crc2& 0xffff);
						}
					}
				}
				fm_w(buffer, offset, 8, trackdata[i]);
			}
		}
	}

	generate_track_from_levels(track, head, buffer, track_size_cells, 0, image);

	global_free_array(buffer);
}

void ti99_floppy_format::generate_track_mfm(int track, int head, int cell_size, UINT8* trackdata, floppy_image *image)
{
	int track_size_cells = 200000000/cell_size;
	UINT32 *buffer = global_alloc_array_clear(UINT32, 200000000/cell_size);

	// See above
	// We limit the track size to cell_number / 16, which means 6250 for MFM
	// Here, Gap 4 is actually only 90 bytes long
	// (not 712 as specified in the TDF format)
	int track_size = track_size_cells / 16;

	int offset = 0;
	short crc1, crc2, found_crc;
	int start = 40;

	if (check_for_address_marks(trackdata, floppy_image::MFM)==false)
	{
		if (track==0 && head==0) logerror("ti99_dsk: Cannot find MFM address marks on track %d, head %d; likely broken or unformatted.\n", track, head);
		global_free_array(buffer);
		return;
	}

	// Found a track; we now know where the address marks are:
	// (start is positioned at the pre-id gap)
	// IDAM: start + 10 + n*340 (starting at first a1)
	// DAM:  start + 54 + n*340 (starting at first a1)
	// and the CRCs are at
	// CRC1: start + 18 + n*340
	// CRC2: start + 314 + n*334

	for (int i=0; i < track_size; i++)
	{
		if (((i-start-10)%340==0) && (i < start + 18*340))
		{
			// IDAM
			for (int j=0; j < 3; j++)
				raw_w(buffer, offset, 16, 0x4489);  // 3 times A1
			mfm_w(buffer, offset, 8, 0xfe);
			i += 3;
		}
		else
		{
			if (((i-start-54)%340==0) && (i < start + 18*340))
			{
				// DAM
				for (int j=0; j < 3; j++)
					raw_w(buffer, offset, 16, 0x4489);  // 3 times A1
				mfm_w(buffer, offset, 8, 0xfb);
				i += 3;
			}
			else
			{
				if (((i-start-18)%340==0) && (i < start + 18*340))
				{
					// CRC1
					// The CRC also covers the three A1 bytes!
					crc1 = ccitt_crc16(0xffff, &trackdata[i-8], 8);
					found_crc = (trackdata[i]<<8 | trackdata[i+1]);
					if ((found_crc & 0xffff) == 0xf7f7)
					{
						// PC99 format: pseudo CRC; replace it
						// logerror("Warning: PC99 format using pseudo CRC1; replace by %04x\n", crc1);
						trackdata[i] = (crc1 >> 8) & 0xff;
						trackdata[i+1] = crc1 & 0xff;
						found_crc = crc1;
					}
					if (crc1 != found_crc)
					{
						logerror("ti99_dsk: Warning: CRC1 does not match (track=%d, head=%d). Found = %04x, calc = %04x\n", track, head, found_crc & 0xffff, crc1& 0xffff);
					}
				}
				else
				{
					if ((i > 340) && ((i-start-314)%340==0) && (i < start + 18*340))
					{
						// CRC2
						crc2 = ccitt_crc16(0xffff, &trackdata[i-SECTOR_SIZE-4], SECTOR_SIZE+4);
						found_crc = (trackdata[i]<<8 | trackdata[i+1]);
						if ((found_crc & 0xffff) == 0xf7f7)
						{
							// PC99 format: pseudo CRC; replace it
							// logerror("Warning: PC99 format using pseudo CRC2; replace by %04x\n", crc2);
							trackdata[i] = (crc2 >> 8) & 0xff;
							trackdata[i+1] = crc2 & 0xff;
							found_crc = crc2;
						}
						if (crc2 != found_crc)
						{
							logerror("ti99_dsk: Warning: CRC2 does not match (track=%d, head=%d). Found = %04x, calc = %04x\n", track, head, found_crc& 0xffff,  crc2& 0xffff);
						}
					}
				}
				mfm_w(buffer, offset, 8, trackdata[i]);
			}
		}
	}

	generate_track_from_levels(track, head, buffer, track_size_cells, 0, image);

	global_free_array(buffer);
}

// States for decoding the bitstream
enum
{
	FMIDAM,
	MFMIDAM,
	HEADER,
	FMDAM,
	MFMDAM,
	DATA
};

/*
    Decodes the bitstream into a TDF track image.
    Returns the number of detected marks.
*/
int ti99_floppy_format::decode_bitstream(const UINT8 *bitstream, UINT8 *trackdata, int* sector, int cell_count, int encoding, UINT8 gapbytes, int track_size)
{
	int databytes = 0;
	int a1count = 0;
	int lastpos = 0;
	int headerbytes = 0;
	int curpos = 0;
	UINT8 curbyte = 0;
	UINT16 shift_reg = 0;
	int tpos = 0;
	int pos = 0;
	int state;
	bool good = false;
	int marks = 0;
	int current_sector = 0;

	// Init track
	memset(trackdata, 0x00, track_size);

	for (int i=0; i < 36; i++) sector[i] = -1;

	state = (encoding==floppy_image::MFM)? MFMIDAM : FMIDAM;

	while (pos < cell_count && tpos < track_size)
	{
		shift_reg = (shift_reg << 1) & 0xffff;
		if ((pos & 0x07)==0) curbyte = bitstream[curpos++];
		if ((curbyte & 0x80) != 0) shift_reg |= 1;
		curbyte <<= 1;

		switch (state)
		{
		case FMIDAM:
			if (shift_reg == 0xf57e)
			{
				marks++;
				// Found a header
				headerbytes = 5;
				// Create GAP0 at the beginning or GAP3 later
				if (tpos==0)
					tpos += 16;
				else
					for (int i=0; i < 45; i++) trackdata[tpos++] = gapbytes;

				// IDAM sync bytes
				tpos += 6;
				trackdata[tpos++] = 0xfe;
				state = HEADER;
				lastpos = pos;
			}
			break;

		case MFMIDAM:
			// Count three subsequent A1
			if (shift_reg == 0x4489)
			{
				if (lastpos > 0)
				{
					if (pos - lastpos == 16) a1count++;
					else a1count = 1;
				}
				else a1count = 1;

				lastpos = pos;
			}
			if (a1count == 3)
			{
				marks++;
				// Found a header
				headerbytes = 6;
				// Create GAP0 at the beginning or GAP3 later
				if (tpos==0)
					for (int i=0; i < 40; i++) trackdata[tpos++] = gapbytes;
				else
					for (int i=0; i < 24; i++) trackdata[tpos++] = gapbytes;

				// IDAM sync bytes
				tpos += 10;
				state = HEADER;
				trackdata[tpos++] = 0xa1;
				trackdata[tpos++] = 0xa1;
				trackdata[tpos++] = 0xa1;
			}
			break;

		case HEADER:
			if (pos - lastpos == 16)
			{
				// Transfer header bytes
				trackdata[tpos] = get_data_from_encoding(shift_reg);

				if (headerbytes == 3) current_sector = trackdata[tpos];
				tpos++;

				if (headerbytes == 0)
				{
					state = (encoding==floppy_image::MFM)? MFMDAM : FMDAM;
					a1count = 0;
				}
				else headerbytes--;
				lastpos = pos;
			}
			break;

		case FMDAM:
			if (shift_reg == 0xf56a || shift_reg == 0xf56f)
			{
				if (pos - lastpos > 400)
				{
					// Too far apart
					state = FMIDAM;
					// Abort this sector, skip to the next
					tpos += 321;
					a1count = 1;
					break;
				}
				marks++;
				// Add GAP2 and DAM sync
				for (int i=0; i < 11; i++) trackdata[tpos++] = 0xff;
				tpos += 6;
				state = DATA;
				databytes = 257;
				trackdata[tpos++] = (shift_reg==0xf56a)? 0xf8 : 0xfb;
				lastpos = pos;
				sector[current_sector] = tpos;
			}
			break;

		case MFMDAM:
			// Count three subsequent A1
			if (shift_reg == 0x4489)
			{
				if (pos - lastpos > 560)
				{
					// Too far apart
					state = MFMIDAM;
					// Abort this sector, skip to the next
					tpos += 320;
					a1count = 1;
					break;
				}

				if (lastpos > 0)
				{
					if (pos - lastpos == 16) a1count++;
					else a1count = 1;
				}
				else a1count = 1;

				lastpos = pos;
			}
			if (a1count == 3)
			{
				marks++;
				// Add GAP2 and DAM sync
				for (int i=0; i < 22; i++) trackdata[tpos++] = gapbytes;
				tpos += 12;
				trackdata[tpos++] = 0xa1;
				trackdata[tpos++] = 0xa1;
				trackdata[tpos++] = 0xa1;
				state = DATA;
				databytes = 258;
				sector[current_sector] = tpos+1;
			}
			break;

		case DATA:
			if (pos - lastpos == 16)
			{
				// Ident byte
				trackdata[tpos++] = get_data_from_encoding(shift_reg);
				if (databytes > 0) databytes--;
				else
				{
					state = (encoding==floppy_image::MFM)? MFMIDAM : FMIDAM;
					a1count = 0;
				}
				lastpos = pos;
			}
			break;
		}
		pos++;
	}
	return marks;
}

UINT8 ti99_floppy_format::get_data_from_encoding(UINT16 raw)
{
	return (raw & 0x4000 ? 0x80 : 0x00) |
			(raw & 0x1000 ? 0x40 : 0x00) |
			(raw & 0x0400 ? 0x20 : 0x00) |
			(raw & 0x0100 ? 0x10 : 0x00) |
			(raw & 0x0040 ? 0x08 : 0x00) |
			(raw & 0x0010 ? 0x04 : 0x00) |
			(raw & 0x0004 ? 0x02 : 0x00) |
			(raw & 0x0001 ? 0x01 : 0x00);
}

/*
    Sector Dump Format
    ------------------
    The Sector Dump Format is also known as v9t9 format (named after the first
    TI emulator to use this format). It is a contiguous sequence of sector
    contents without track data. The first sector of the disk is located at
    the start of the image, while the last sector is at its end. The sectors
    are ordered by their logical number as used in the TI file system.

    In this implementation, the sector dump format is just a kind of
    wd177x_format with minor customizations, which allows us to keep the code
    small. The difference is the logical ordering of tracks and sectors.

    The TI file system orders all tracks on side 0 as going inwards,
    and then all tracks on side 1 going outwards.

        00 01 02 03 ... 38 39     side 0
        79 78 77 76 ... 41 40     side 1

    The SDF format stores the tracks and their sectors in logical order
        00 01 02 03 ... 38 39 [40 41 ... 79]

    There is also a variant of the SDF which adds three sectors at the end
    containing a map of bad sectors. This was introduced by a tool to read
    real TI floppy disks on a PC. As other emulators tolerate this additional
    bad sector map, we just check whether there are 3 more sectors and ignore
    them.
*/
const char *ti99_sdf_format::name() const
{
	return "sdf";
}

const char *ti99_sdf_format::description() const
{
	return "TI99 sector dump floppy disk image";
}

const char *ti99_sdf_format::extensions() const
{
	return "dsk";
}

int ti99_sdf_format::identify(io_generic *io, UINT32 form_factor)
{
	UINT64 file_size = io_generic_size(io);
	int vote = 0;

	// Adding support for another sector image format which adds 768 bytes
	// as a bad sector map
	if ((file_size / SECTOR_SIZE) % 10 == 3)
	{
		if (TRACE) logerror("ti99_dsk: Stripping map of bad sectors at image end\n");
		file_size -= SECTOR_SIZE*3;
	}

	// Formats with 9 or 18 sectors per track (multiples of SSSD)
	// 40-track formats: SSSD/9/40 (1), DSSD/9/40 (2), SSDD/18/40 (2), DSDD/18/40 (4)
	// 80-track formats: SSSD/9/80 (2), DSSD/9/80 (4), SSDD/18/80 (4), DSDD/18/80,(8)
	// High density: DSQD/36/80 (16) (experimental)

	// All formats apply for 5.25" and 3.5" form factors

	// Default formats (when the geometry cannot be determined from the VIB)
	// (1)  -> SSSD
	// (2)  -> DSSD
	// (4)  -> DSDD
	// (8)  -> DSDD80
	// (16) -> DSQD

	if ((file_size % 92160)==0)
	{
		int multiple = file_size/92160;
		if ((multiple == 1) || (multiple == 2) || (multiple == 4) || (multiple == 8) || (multiple == 16)) vote = 50;
	}

	// Formats with 16 sectors per track (rare, SSDD/16/40, DSDD/16/40).
	if (file_size == 163840) vote = 50;
	if (file_size == 327680) vote = 50;

	if (vote > 0)
	{
		// Read first sector (Volume Information Block)
		ti99vib vib;
		io_generic_read(io, &vib, 0, sizeof(ti99vib));

		// Check from contents
		if ((vib.id[0]=='D')&&(vib.id[1]=='S')&&(vib.id[2]=='K'))
		{
			if (TRACE) logerror("ti99_dsk: Found formatted SDF disk medium\n");
			vote = 100;
		}
		else
		{
			if (TRACE) logerror("ti99_dsk: No valid VIB found; disk may be unformatted\n");
		}
	}
	else if (TRACE) logerror("ti99_dsk: Disk image is not a SDF image\n");
	return vote;
}

void ti99_sdf_format::determine_sizes(io_generic *io, int& cell_size, int& sector_count, int& heads)
{
	UINT64 file_size = io_generic_size(io);
	ti99vib vib;

	cell_size = 0;
	sector_count = 0;
	heads = 2;

	bool have_vib = false;

	// See above
	if ((file_size / SECTOR_SIZE) % 10 == 3) file_size -= SECTOR_SIZE*3;

	// Read first sector
	io_generic_read(io, &vib, 0, sizeof(ti99vib));

	// Check from contents
	if ((vib.id[0]=='D')&&(vib.id[1]=='S')&&(vib.id[2]=='K'))
	{
		sector_count = vib.secspertrack;
		heads = vib.sides;

		// Find out more about the density. SSDD happens to be the same size
		// as DSSD in the sector dump format, so we need to ask the
		// VIB if available. Otherwise, we assume that we have a DSSD medium.
		if (vib.density < 2) cell_size = 4000;
		else
		{
			if (vib.density < 4) cell_size = 2000;
			else cell_size = 1000;
		}
		if (TRACE) logerror("ti99_dsk: VIB says that this disk is %s density with %d sectors per track, %d tracks, and %d heads\n", (cell_size==4000)? "single": ((cell_size==2000)? "double" : "high"), sector_count, vib.tracksperside, heads);
		have_vib = true;
	}

	// We're also checking the size of the image
	int cell_size1 = 0;
	int sector_count1 = 0;

	// 90 KiB -> SSSD, 9 sect, FM
	// 160 KiB -> SSDD, 16 sect, MFM
	// 180 KiB -> DSSD, 9 sect, FM
	// 320 KiB -> DSDD, 16 sect, MFM
	// 360 KiB -> DSDD, 18 sect, MFM
	// 720 KiB -> DSDD, 18 sect, MFM, 80 tracks
	// 1440 KiB-> DSQD, 36 sect, MFM, 80 tracks

	if ((file_size == 163840) || (file_size == 327680))
	{
		cell_size1 = 2000;
		sector_count1 = 16;
	}
	else
	{
		if (file_size < 300000) cell_size1 = 4000;
		else
		{
			if (file_size < 1000000) cell_size1 = 2000;
			else cell_size1 = 1000;
		}
		sector_count1 = 36000 / cell_size1;
	}

	if (have_vib)
	{
		if (sector_count == 16 && sector_count1 == 18)
		{
			logerror("ti99_dsk: Warning: Invalid 16-sector format. Assuming 18 sectors.\n");
			sector_count = 18;
		}
		else
		{
			if (heads == 2 && ((cell_size1 != cell_size) || (sector_count1 != sector_count)))
				logerror("ti99_dsk: Warning: Disk image size does not correspond with format information in VIB.\n");
		}
	}
	else
	{
		heads = (file_size < 100000)? 1 : 2;    // for SSSD
		cell_size = cell_size1;
		sector_count = sector_count1;
	}
}

int ti99_sdf_format::get_track_size(int cell_size, int sector_count)
{
	return sector_count * SECTOR_SIZE;
}

/*
    Load a SDF image track. Essentially, we want to end up in the same
    kind of track image as with the TDF, so the easiest thing is to produce
    a TDF image track from the sectors and process them as if it came from TDF.
*/
void ti99_sdf_format::load_track(io_generic *io, UINT8 *trackdata, int head, int track, int sectorcount, int trackcount, int cellsize)
{
	bool fm = (cellsize==4000);
	int tracksize = sectorcount * SECTOR_SIZE;

	// Calculate the track offset from the beginning of the image file
	int logicaltrack = head * trackcount;
	logicaltrack += ((head&1)==0)?  track : (trackcount - 1 - track);

	// Interleave and skew
	int interleave = fm? 4 : 5;
	int skew = fm? 6 : 0;

	int secsize = fm? 334 : 340;
	int secoff = fm? 33 : 58;
	int position = 0;
	int count = 0;

	memset(trackdata, 0x00, 9216);

	int secno = 0;
	secno = (track * skew) % sectorcount;

	// Gap 1
	int gap1 = fm? 16 : 40;
	for (int i=0; i < gap1; i++) trackdata[position+i] = fm? 0x00 : 0x4e;

	for (int i=0; i < sectorcount; i++)
	{
		position = secno * secsize + gap1;
		// Sync
		count = fm? 6 : 10;
		while (count-- > 0) trackdata[position++] = 0x00;

		if (!fm)
		{
			trackdata[position++] = 0xa1;
			trackdata[position++] = 0xa1;
			trackdata[position++] = 0xa1;
		}
		trackdata[position++] = 0xfe;   // IDAM / ident

		// Header
		trackdata[position++] = track;
		trackdata[position++] = head;
		trackdata[position++] = i;
		trackdata[position++] = 1;

		trackdata[position++] = 0xf7;
		trackdata[position++] = 0xf7;

		// Gap 2
		count = fm? 11 : 22;
		while (count-- > 0) trackdata[position++] = fm? 0xff : 0x4e;

		// Sync
		count = fm? 6 : 12;
		while (count-- > 0) trackdata[position++] = 0x00;

		if (!fm)
		{
			trackdata[position++] = 0xa1;
			trackdata[position++] = 0xa1;
			trackdata[position++] = 0xa1;
		}
		trackdata[position++] = 0xfb;

		io_generic_read(io, trackdata + position, logicaltrack * tracksize + i*SECTOR_SIZE, SECTOR_SIZE);

		position += SECTOR_SIZE;
		trackdata[position++] = 0xf7;
		trackdata[position++] = 0xf7;

		// Gap 3
		count = fm? 45 : 24;
		while (count-- > 0) trackdata[position++] = fm? 0xff : 0x4e;
		secno = (secno + interleave) % sectorcount;
	}

	// Gap 4
	count = fm? 231 : 712;
	position = sectorcount * secsize + gap1;
	while (count-- > 0) trackdata[position++] = fm? 0xff : 0x4e;

	// if (head==0 && track==0) showtrack(trackdata, 9216);
}

/*
    For debugging. Outputs the byte array in a xxd-like way.
*/
void ti99_floppy_format::showtrack(UINT8* trackdata, int length)
{
	for (int i=0; i < length; i+=16)
	{
		logerror("%07x: ", i);
		for (int j=0; j < 16; j++)
		{
			logerror("%02x", trackdata[i+j]);
			if ((j&1)==1) logerror(" ");
		}
		logerror(" ");
		for (int j=0; j < 16; j++)
		{
			if (trackdata[i+j] >= 32 && trackdata[i+j]<128) logerror("%c", trackdata[i+j]);
			else logerror(".");
		}
		logerror("\n");
	}
}

/*
    Write the data to the disk. We have a list of sector positions, so we
    just need to go through that list and save each sector in the track data.
*/
void ti99_sdf_format::write_track(io_generic *io, UINT8 *trackdata, int *sector, int track, int head, int maxsect, int maxtrack, int numbytes)
{
	int logicaltrack = head * maxtrack;
	logicaltrack += ((head&1)==0)?  track : (maxtrack - 1 - track);
	int trackoffset = logicaltrack * maxsect * SECTOR_SIZE;

//  if (head==0 && track==0) showtrack(trackdata, 9216);

	for (int i=0; i < maxsect; i++)
		io_generic_write(io, trackdata + sector[i], trackoffset + i * SECTOR_SIZE, SECTOR_SIZE);
}

const floppy_format_type FLOPPY_TI99_SDF_FORMAT = &floppy_image_format_creator<ti99_sdf_format>;

/*
    Track Dump Format
    -----------------
    The Track Dump Format is also known as pc99 (again, named after the first
    TI emulator to use this format). It is a contiguous sequence of track
    contents, containing all information including the data values of the
    address marks and CRC, but it does not contain clock signals.
    Therefore, the format requires that the address marks be at the same
    positions within each track.

    Different to earlier implementations of the TDF format, we stay much
    closer to the specification. Deviations (like different gap sizes) are
    automatically rectified. It does not make sense to have a format that
    pretends to be an almost precise track image and then fail to load in
    other emulations because of small deviations.
    For precise track images there are better suited formats.

    For FM recording, each track is exactly 3253 bytes long in this format.
    For MFM recording, track length is 6872 bytes.

    Accordingly, we get a multiple of these values as the image length.
    There are no single-sided images (when single-sided, the upper half of
    the image is empty).

    DSSD: 260240 bytes
    DSDD: 549760 bytes

    We do not support other geometries in this format. One exception: We accept
    images of double size which may be generated when a 40-track disk is
    modified in an 80-track drive. This will automatically cause the creation
    of an 80-track image.

    Tracks are stored from outside to inside of head 0, then outside to inside of head 1:

    (Head,Track): (0,0) (0,1) (0,2) ... (0,38) (0,39) (1,0) (1,1) ... (1,39)
*/
const char *ti99_tdf_format::name() const
{
	return "tdf";
}

const char *ti99_tdf_format::description() const
{
	return "TI99 track dump floppy disk image";
}

const char *ti99_tdf_format::extensions() const
{
	return "dsk,dtk";
}

/*
    Determine whether the image file can be interpreted as a track dump
*/
int ti99_tdf_format::identify(io_generic *io, UINT32 form_factor)
{
	UINT64 file_size = io_generic_size(io);
	int vote = 0;
	UINT8 trackdata[2000];

	// Do we have a plausible image size? From the size alone we only give a 50 percent vote.
	if ((file_size == 260240) || (file_size == 549760) || (file_size == 1099520))
		vote = 50;

	if (vote > 0)
	{
		if (TRACE) logerror("ti99_dsk: Image looks like a TDF image\n");

		// Get some bytes from track 0
		io_generic_read(io, trackdata, 0, 2000);

		if (check_for_address_marks(trackdata, floppy_image::FM)==true || check_for_address_marks(trackdata, floppy_image::MFM)==true) vote=100;
		else
		{
			if (TRACE) logerror("ti99_dsk: Image does not comply with TDF; may be broken or unformatted\n");
		}
	}
	else if (TRACE) logerror("ti99_dsk: Disk image is not a TDF image\n");
	return vote;
}

/*
    Find the proper format for a given image file. We determine the cell size,
    but we do not care about the sector size (only needed by the SDF converter).
*/
void ti99_tdf_format::determine_sizes(io_generic *io, int& cell_size, int& sector_count, int& heads)
{
	UINT64 file_size = io_generic_size(io);
	heads = 2;  // TDF only supports two-sided recordings

	if (file_size % get_track_size(2000, 0)==0) cell_size = 2000;
	else if (file_size % get_track_size(4000, 0)==0) cell_size = 4000;

	// We could check for the content, but if we find that the content is
	// FM and the size implies MFM, the calculated track count will be wrong.
}

/*
    For TDF this just amounts to loading the track from the image file.
*/
void ti99_tdf_format::load_track(io_generic *io, UINT8 *trackdata, int head, int track, int sectorcount, int trackcount, int cellsize)
{
	int offset = ((trackcount * head) + track) * get_track_size(cellsize, 0);
	io_generic_read(io, trackdata, offset, get_track_size(cellsize, 0));
}

/*
    Also here, we just need to write the complete track on the image file.
*/
void ti99_tdf_format::write_track(io_generic *io, UINT8 *trackdata, int *sector, int track, int head, int maxsect, int maxtrack, int numbytes)
{
	int offset = ((maxtrack * head) + track) * numbytes;
	io_generic_write(io, trackdata, offset, numbytes);
}

int ti99_tdf_format::get_track_size(int cell_size, int sector_count)
{
	if (cell_size == 4000) return 3253;
	if (cell_size == 2000) return 6872;
	return 0;
}

const floppy_format_type FLOPPY_TI99_TDF_FORMAT = &floppy_image_format_creator<ti99_tdf_format>;


//==========================================================================

/**************************************************************
 *
 *    Legacy implementation
 *
 * Different to earlier implementations, these format implementations provide
 * full track read/write capabilities. For the SDF format, the missing track
 * information is recreated, and a common interleave is assumed.
 *
 * Note that in principle, writing tracks may change track sizes.
 * As neither one of the formats contains meta-information about track lengths,
 * and as the image consists of a contiguous sequence of the tracks, we
 * cannot allow different track sizes currently. Accordingly, the track size
 * cannot be changed after the image has been created by imgtool.
 * Hence, also sector count per tracks and recording type (FM or MFM) cannot be
 * changed without breaking the image.
 * For example, MFM disks with 18 sectors per track can only be reformatted to
 * MFM/18. Moreover, while this could be handled in a future revision,
 * the number of tracks should also remain unchanged.
 *
 * You can use blank image files (filled with zeros or arbitrary data) and use
 * the Disk Manager cartridge to properly format them. Also, this can be done
 * by imgtool. The image file must match the format length (representing the
 * space on the physical medium).
 *
 * 80 track drive handling
 *
 * For the TI system there may be a situation where 40 track disks are used
 * in 80 track drives. In these cases, the disk driver applies double-stepping,
 * that is, it advances the head twice in order to seek the next track. We
 * emulate this by dividing the track number by 2, which means that two tracks
 * are mapped to one (0,1 -> 0; 2,3 -> 1; 4,5 -> 2; ...; 78,79 -> 39)
 *
 * Technical detail: The size of the track must be given before actually
 * starting write_track as the controller implementation uses buffered
 * transfer.
 *
 * Michael Zapf, Feb 2010
 *
 * TODO: Link to imgtool. imgtool still uses its own format creation
 *
 * FIXME: If image is broken with good first track and defect higher tracks,
 *        tdf_guess_geometry fails to define a geometry. The guessing should
 *        always be done.
**************************************************************/

#define TI99_IDAM_LENGTH 7

/*
    Determines whether we are using 80 track drives. This variable is
    necessary unless we get information about the dip
    switch settings. It is set by the disk controller implementations
    (bwg, hfdc, ti_fdc).
*/
static int use_80_track_drives = FALSE;

struct ti99dsk_geometry
{
	UINT8 sides;
	UINT8 tracksperside;
	UINT8 secspertrack;
	UINT8 density;
};

struct ti99dsk_tag
{
	int heads;
	int tracks;
	int sectors;
	int track_size;  // Complete track; to be calculated
	int format;
	int first_idam;
	int dam_offset;
};

/*
    Parametrizes the format to use 80 track drives. For TI systems we
    need to be able to emulate 40 track drives and 80 track drives.
    This is set in the configuration, but the format has no direct access.
    So as a preliminary solution we use this global function which sets the
    flag. This function is called by the disk controller implementations
    (bwg, hfdc, ti_fdc).
*/
void ti99_set_80_track_drives(int use80)
{
	use_80_track_drives = use80;
}

#define TI99_DSK_TAG    "ti99dsktag"
#define TI99DSK_BLOCKNOTFOUND -1

/*
    Searches a block containing number * byte, starting at the given
    position. Returns the position of the first byte of the block.
*/
static int find_block(const UINT8 *buffer, int start, int stop, UINT8 byte, size_t number)
{
	int i = start;
	size_t current = number;
	while (i < stop && current > 0)
	{
		if (buffer[i++] != byte)
		{
			current = number;
		}
		else
		{
			current--;
		}
	}
	if (current==0)
	{
		return i - number;
	}
	else
		return TI99DSK_BLOCKNOTFOUND;
}

/*
    Queried by flopdrv. Instead of the image tracks, we return the tracks
    of the floppy drive. We assume we always have two heads.
*/
static int ti99_get_heads_per_disk(floppy_image_legacy *floppy)
{
//  struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);
//  return tag->heads;
	return 2;
}

/*
    Queried by flopdrv. Instead of the image tracks, we return the tracks
    of the floppy drive.
*/
static int ti99_get_tracks_per_disk(floppy_image_legacy *floppy)
{
	int drivetracks = 42;

	// This may fail on startup; therefore we use a special function in
	// in the floppy controller implementations to explicitly set the geometry.
	if (use_80_track_drives)
	{
		drivetracks = 83;
	}
	return drivetracks;
}

static floperr_t ti99_get_sector_length(floppy_image_legacy *floppy, int head, int track, int sector, UINT32 *sector_length)
{
	*sector_length=SECTOR_SIZE;
	return FLOPPY_ERROR_SUCCESS;
}

/*
    Return the track size. We do not allow different track
    sizes, so we store the track size in the tag.
*/
static UINT32 ti99_get_track_size(floppy_image_legacy *floppy, int head, int track)
{
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);
	return tag->track_size;
}

/*
static void create_vib(UINT8 *sector0, option_resolution *params)
{
    const char *name = "UNNAMED   ";
    int sides, tracksperside,secspertrack,totalsectors,ausize,allocbytes,i;

    enum { NAME=0, TOTALSECS=10, SECSPERTRACK=11, SIG=12, PROT=16, TRACKS=17, SIDES=18, DENSITY=19, EXT=20, ALLOC=56 };

    sides       = option_resolution_lookup_int(params, PARAM_HEADS);
    tracksperside   = option_resolution_lookup_int(params, PARAM_TRACKS);
    secspertrack    = option_resolution_lookup_int(params, PARAM_SECTORS);

    totalsectors = sides * tracksperside * secspertrack;

    memcpy(&sector0[NAME], name, 10);

    sector0[TOTALSECS] = (totalsectors>>8)&0xff;
    sector0[TOTALSECS+1] = (totalsectors&0xff);

    sector0[SECSPERTRACK] = secspertrack;
    sector0[SIG] = 'D';
    sector0[SIG+1] = 'S';
    sector0[SIG+2] = 'K';

    sector0[PROT] = ' ';
    sector0[TRACKS] = tracksperside;
    sector0[SIDES] = sides;
    sector0[DENSITY] = (secspertrack<10)? 0x01 : 0x02;

    for (i=EXT; i < ALLOC; i++)
        sector0[i] = 0;

    ausize = (totalsectors / 1441) + 1;

    allocbytes = totalsectors / ausize / 8;

    for (i=ALLOC; i < ALLOC+allocbytes; i++)
        sector0[i] = 0;

    while (i++ < SECTOR_SIZE)
        sector0[i] = 0xff;
}
*/
/* =========================================================================
             Sector dump format
   =========================================================================*/

static floperr_t ti99_sdf_read_sector(floppy_image_legacy *floppy, int head, int track, int sector, void *buffer, size_t buflen);
static floperr_t ti99_sdf_write_sector(floppy_image_legacy *floppy, int head, int track, int sector, const void *buffer, size_t buflen, int ddam);

/* -----------------------------------------------------------------------
 * Guess the geometry of the sector dump disk.
 *
 * Sector size is always 256 bytes.
 *
 * Common formats:
 * 90 KiB = 40 tracks / 1 side / 9 sectors = SSSD
 * 180 KiB = 40 tracks / 2 side / 9 sectors = DSSD (most common)
 *         = 40 tracks / 1 side / 18 sectors = SSDD (rare)
 *         = 80 tracks / 1 side / 9 sectors = SSSD80 (rare)
 * 360 KiB = 40 tracks / 2 side / 18 sectors = DSDD (most common)
 *         = 80 tracks / 2 side / 9 sectors = DSSD80 (rare)
 *         = 80 tracks / 1 side / 18 sectors = SSDD80 (rare)
 *         = 40 tracks / 1 side / 36 sectors = SSHD (rare)
 * 720 KiB = 80 tracks / 2 side / 18 sectors = DSDD80 (most common)
 * 1440 KiB= 80 tracks / 2 side / 36 sectors = DSHD80 (most common)
 *
 * Moreover, there may be formats with 35 tracks (ancient) and 8 sectors/track.
 * We only check for the 8 sector formats.
 * 160 KiB = 40 tracks / 1 side / 16 sectors = SSDD8
 * 320 KiB = 40 tracks / 2 side / 16 sectors = DSDD8
 *
 * The Volume Information Block may contain suitable information, but not
 * before the disk is formatted and initialized.
 *
 * returns 100 if recognized, 0 if not recognized
 * ----------------------------------------------------------------------- */

static int ti99_sdf_guess_geometry(floppy_image_legacy *floppy, UINT64 size,
	struct ti99dsk_geometry *geometry)
{
	int totsecs;
	typedef struct ti99_vib
	{
		char    name[10];       // volume name (10 characters, pad with spaces)
		UINT8   totsecsMSB;     // disk length in sectors (big-endian) (usually 360, 720 or 1440)
		UINT8   totsecsLSB;
		UINT8   secspertrack;   // sectors per track (usually 9 (FM) or 18 (MFM))
		UINT8   id[3];          // String "DSK"
		UINT8   protection;     // 'P' if disk is protected, ' ' otherwise.
		UINT8   tracksperside;  // tracks per side (usually 40)
		UINT8   sides;          // sides (1 or 2)
		UINT8   density;        // 0,1 (FM) or 2,3,4 (MFM)
		UINT8   res[36];        // Empty for traditional disks, or up to 3 directory pointers
		UINT8   abm[200];       // allocation bitmap: a 1 for each sector in use (sector 0 is LSBit of byte 0,
								// sector 7 is MSBit of byte 0, sector 8 is LSBit of byte 1, etc.)
	} ti99_vib;

	ti99_vib vib;
	UINT32 file_size;
	struct ti99dsk_geometry dummy_geometry;

	if (geometry)
		memset(geometry, 0, sizeof(*geometry));
	else
		geometry = &dummy_geometry;

	if (size > 0xFFFFFFFF)
		return 0;

	file_size = (UINT32) size;

	// Read the Volume Information Block
	floppy_image_read(floppy, &vib, 0, sizeof(vib));

	// If we have read the sector successfully, let us parse it
	totsecs = (vib.totsecsMSB << 8) | vib.totsecsLSB;
	geometry->secspertrack = vib.secspertrack;
	if (geometry->secspertrack == 0)
		// Some images might be like this, because the original SSSD
		// TI controller always assumes 9.
		geometry->secspertrack = 9;
	geometry->tracksperside = vib.tracksperside;
	if (geometry->tracksperside == 0)
		// Some images are like this, because the original SSSD TI controller
		// always assumes 40.
		geometry->tracksperside = 40;
	geometry->sides = vib.sides;
	if (geometry->sides == 0)
		// Some images are like this, because the original SSSD TI controller
		// always assumes that tracks beyond 40 are on side 2. */
		geometry->sides = totsecs / (geometry->secspertrack * geometry->tracksperside);
	geometry->density = vib.density;
	// check that the format makes sense
	if (((geometry->secspertrack * geometry->tracksperside * geometry->sides) == totsecs)
		&& (geometry->density <= 4) && (totsecs >= 2) && (! memcmp(vib.id, "DSK", 3))
		&& (file_size == totsecs*256))
	{
		LOG_FORMATS("SDF/VIB consistent; tracks = %d, heads = %d, sectors = %d\n", geometry->tracksperside, geometry->sides, geometry->secspertrack);
		return 100;
	}
	LOG_FORMATS("SDF/VIB not consistent; guessing format\n");

	// So that was not consistent. We guess the size from the file size
	// and assume that the VIB did not contain reliable data. For the
	// ambiguous case we choose the most common format.

	// Adding support for another sector image format which adds 768 bytes
	// as a bad sector map
	if ((file_size / 256) % 10 == 3)
	{
		LOG_FORMATS("Stripping map of bad sectors at image end\n");
		file_size -= 768;
	}

	switch (file_size)
	{
	case 1*40*9*256:
		// 90kbytes: SSSD
	case 0:
	/*default:*/
		geometry->sides = 1;
		geometry->tracksperside = 40;
		geometry->secspertrack = 9;
		geometry->density = 1;
		break;

	case 2*40*9*256:
		// 180kbytes: either DSSD or 18-sector-per-track SSDD.
		// We assume DSSD since DSSD is more common and is supported by
		// the original TI SD disk controller.
		geometry->sides = 2;
		geometry->tracksperside = 40;
		geometry->secspertrack = 9;
		geometry->density = 1;
		break;

	case 1*40*16*256:
		// 160kbytes: 16-sector-per-track SSDD (standard format for TI
		// DD disk controller prototype, and the TI hexbus disk
		// controller?) */
		geometry->sides = 1;
		geometry->tracksperside = 40;
		geometry->secspertrack = 16;
		geometry->density = 2;
		break;

	case 2*40*16*256:
		// 320kbytes: 16-sector-per-track DSDD (standard format for TI
		// DD disk controller prototype, and TI hexbus disk
		// controller?)
		geometry->sides = 2;
		geometry->tracksperside = 40;
		geometry->secspertrack = 16;
		geometry->density = 2;
		break;

	case 2*40*18*256:
		//  360kbytes: 18-sector-per-track DSDD (standard format for most
		// third-party DD disk controllers, but reportedly not supported by
		// the original TI DD disk controller prototype)
		geometry->sides = 2;
		geometry->tracksperside = 40;
		geometry->secspertrack = 18;
		geometry->density = 2;
		break;

	case 2*80*18*256:
		// 720kbytes: 18-sector-per-track 80-track DSDD (Myarc only)
		geometry->sides = 2;
		geometry->tracksperside = 80;
		geometry->secspertrack = 18;
		geometry->density = 2;
		break;

	case 2*80*36*256:
		// 1.44Mbytes: DSHD (Myarc only)
		geometry->sides = 2;
		geometry->tracksperside = 80;
		geometry->secspertrack = 36;
		geometry->density = 3;
		break;

	default:
		LOG_FORMATS("Unrecognized disk image geometry\n");
		return 0;
	}

	LOG_FORMATS("SDF geometry guess: tracks = %d, heads = %d, sectors = %d\n", geometry->tracksperside, geometry->sides, geometry->secspertrack);
	return 100;
}

/*
    Figure out the interleave for the sector arrangement in the track. For
    SDF format this is required for the read_track function.

    Interleave:
        0 7 5 3 1 8 6 4 2;
             start with 0, 3, 6, 0, 3, 6, ...
            0 11 4 15 8 1 12 5 16 9 2 13 6 17 10 3 14 7
             start with 0 for every track

    Rare formats: 8 sectors
        0 3 6 1 4 7 2 5   (i*3)%8

        16 sectors
        0 9 2 11 4 13 6 15 8 1 10 3 12 5 14 7  (i*9)%16

        36 sectors
        0 11 22 ... 3 14 25 (i*11)%36
*/
static void guess_interleave(int sectors, int *step, int *initial)
{
	switch (sectors)
	{
	case 8:
		*step = 3;
		/* initial is the offset from track to track+1 of the starting sector number */
		*initial = 0;
		break;
	case 9:
		*step = 7;
		*initial = 3;
		break;
	case 16:
		*step = 9;
		*initial = 0;
		break;
	case 18:
		*step = 11;
		*initial = 0;
		break;
	case 36:
		*step = 11;
		*initial = 0;
		break;
	default:
		*step = 11;
		*initial = 0;
		break;
	}
}

/*
    Delivers a whole track. As the image consists only of
    sector data, we must reconstruct the track data and
    then insert the sector data.

    The track layout is exactly the Track Dump Format, except that the
    CRC values must be correctly calculated.

    Total length (FM)  = 247 + sectors*334; (sectors=8 or 9)
    Total length (MFM) = 752 + sectors*340; (sectors=16 or 18)
*/
static floperr_t ti99_sdf_read_track(floppy_image_legacy *floppy, int head, int track, UINT64 offset, void *buffer, size_t buflen)
{
	int sector;
	int startstep;
	int step;
	int position=0;
	unsigned short crc;
	int i;
	floperr_t err;
	int imgtrack = track;

	UINT8 *trackdata = (UINT8*)buffer;

	/* Determine from the image geometry whether we have FM or MFM */
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);

	// Do we have a 40 track disk in an 80 track drive? In that case we
	// double the "width" of the tracks. */
	if (use_80_track_drives && tag->tracks<=40)
	{
		imgtrack = track / 2;
	}

	guess_interleave(tag->sectors, &step, &startstep);

	if (tag->sectors < 10)  /* must be FM */
	{
		/* Write lead-in. */
		memset(trackdata, 0x00, 16);
		position += 16;
		for (i=0; i < tag->sectors; i++)
		{
			/* Create interleaving */
			sector = (i*step + imgtrack*startstep) % tag->sectors;

			memset(&trackdata[position], 0x00, 6);
			position += 6;
			/* Set IDAM */
			trackdata[position++] = 0xfe;
			trackdata[position++] = imgtrack;
			trackdata[position++] = head;
			trackdata[position++] = sector;
			trackdata[position++] = 0x01;
			/* Calculate CRC16. Preset to 0xffff (what is 0xcdb4?) */
			crc = ccitt_crc16(0xffff, &trackdata[position-(TI99_IDAM_LENGTH-2)], TI99_IDAM_LENGTH - 2);
			trackdata[position++] = (crc>>8)&0xff;
			trackdata[position++] = crc & 0xff;

			/* Write Gap2 */
			memset(&trackdata[position], 0xff, 11);
			position += 11;
			memset(&trackdata[position], 0x00, 6);
			position += 6;
			/* Write DAM */
			trackdata[position++] = 0xfb;

			/* Locate the sector content in the image and load it. */
			err = ti99_sdf_read_sector(floppy, head, imgtrack, sector, &trackdata[position], SECTOR_SIZE);
			if (err)
				return err;

			position += SECTOR_SIZE;

			/* Set CRC16 */
			crc = ccitt_crc16(0xffff, &trackdata[position-(SECTOR_SIZE+1)], SECTOR_SIZE+1);
			trackdata[position++] = (crc>>8)&0xff;
			trackdata[position++] = crc & 0xff;

			/* Write Gap3 */
			memset(&trackdata[position], 0xff, 45);
			position += 45;
		}
		/* Write lead-out */
		memset(&trackdata[position], 0xff, 231);
		position += 231;
	}
	else /* we have MFM */
	{
		/* Write lead-in. */
		memset(trackdata, 0x4e, 40);
		position += 40;
		for (i=0; i < tag->sectors; i++)
		{
			sector = (i*step + imgtrack*startstep) % tag->sectors;

			memset(&trackdata[position], 0x00, 10);
			position += 10;
			/* Write sync */
			memset(&trackdata[position], 0xa1, 3);
			position += 3;
			/* Set IDAM */
			trackdata[position++] = 0xfe;
			trackdata[position++] = imgtrack;
			trackdata[position++] = head;
			trackdata[position++] = sector;
			trackdata[position++] = 0x01;
			/* Set CRC16 */
			crc = ccitt_crc16(0xffff, &trackdata[position-TI99_IDAM_LENGTH+2], TI99_IDAM_LENGTH - 2);
			trackdata[position++] = (crc>>8)&0xff;
			trackdata[position++] = crc & 0xff;

			/* Write Gap2 */
			memset(&trackdata[position], 0x4e, 22);
			position += 22;
			memset(&trackdata[position], 0x00, 12);
			position += 12;
			/* Write sync */
			memset(&trackdata[position], 0xa1, 3);
			position += 3;
			/* Write DAM */
			trackdata[position++] = 0xfb;

			/* Locate the sector content in the image and load it. */
			err = ti99_sdf_read_sector(floppy, head, track, sector, &trackdata[position], SECTOR_SIZE);
			if (err)
				return err;

			position += SECTOR_SIZE;

			/* Set CRC16 */
			crc = ccitt_crc16(0xffff, &trackdata[position-SECTOR_SIZE-1], SECTOR_SIZE+1);
			trackdata[position++] = (crc>>8)&0xff;
			trackdata[position++] = crc & 0xff;

			/* Write Gap3 */
			memset(&trackdata[position], 0x4e, 24);
			position += 24;
		}
		/* Write lead-out */
		memset(&trackdata[position], 0x4e, 712);
		position += 712;
	}
	return FLOPPY_ERROR_SUCCESS;
}

/*
    Writes a whole track. As the sector dump format does not include track
    data, we have to extract the sector contents and to write them
    in sequence into the image. All the rest (gaps, crc, sync bytes) are
    dropped here.
    Bytes written to locations beyond the image end will be droppped
    silently (as if the medium is unwritable from that point).
*/
static floperr_t ti99_sdf_write_track(floppy_image_legacy *floppy, int head, int track, UINT64 offset, const void *buffer, size_t buflen)
{
	int current_pos = 0;
	UINT8 *track_image;
	int leadin, gap1, gap2;
	int is_fm, found;
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);
	int imgtrack = track;

	// Find out if we are going to write an FM or MFM track. Note that the
	// original TI controller is known to have a bug in the formatting
	// routine (using a MOVB instead of a MOV instruction when setting the
	// pointer to the track data), outputting false bytes at the
	// beginning of the track. So we do not rely on absolute positions
	// (and neither does the controller).
	track_image = (UINT8*)buffer;

	if (use_80_track_drives && tag->tracks<=40)
	{
		imgtrack = track / 2;
	}

	/* Only search in the first 100 bytes for the start. */
	leadin = 40;
	gap1 = 10;
	gap2 = 12;
	is_fm = FALSE;

	current_pos = find_block(track_image, 0, 100, 0x4e, leadin);

	// In case of defect formats, we continue as far as possible. This
	// may lead to sectors not being written. */
	if (current_pos==TI99DSK_BLOCKNOTFOUND)
	{
		/* Not MFM, possibly FM image */
		/* Again, find Lead-in */
		leadin = 16;
		current_pos = find_block(track_image, 0, 100, 0x00, leadin);
		if (current_pos==TI99DSK_BLOCKNOTFOUND)
		{
			/* If neither, forget about this process completely. */
			LOG_FORMATS("Cannot find lead-in for track %d, head %d.\n", track, head);
			return FLOPPY_ERROR_INVALIDIMAGE;
		}
		gap1 = 6;
		gap2 = 6;
		is_fm = TRUE;
	}

	found = FALSE;
	/* Get behind lead-in */
	current_pos += leadin;
	while (current_pos < buflen)
	{
		/* We must find the address block to determine the sector. */
		int new_pos = find_block(track_image, current_pos, buflen, 0x00, gap1);
		if (new_pos==TI99DSK_BLOCKNOTFOUND)
		{
			/* Forget about the rest. */
			if (found) break;  /* we were already successful, so all ok */
			LOG_FORMATS("Cannot find gap1 for track %d, head %d.\n", track, head);
			return FLOPPY_ERROR_INVALIDIMAGE;
		}
		found = TRUE;

		if (!is_fm)
			new_pos += 3; /* skip sync bytes in MFM */

		if (track_image[new_pos + gap1]==0xfe)
		{
			current_pos = new_pos + gap1  + 1;
			/* IDAM found. */
			int wtrack = track_image[current_pos];
			int whead = track_image[current_pos+1];
			int sector = track_image[current_pos+2];
			if (wtrack == imgtrack && whead == head)
			{
				/* We skip the first part of gap2. */
				new_pos = find_block(track_image, current_pos, buflen, 0x00, gap2);
				if (current_pos==TI99DSK_BLOCKNOTFOUND)
				{
					LOG_FORMATS("Cannot find gap2 for track %d, head %d.\n", imgtrack, head);
					return FLOPPY_ERROR_INVALIDIMAGE;
				}
				else
				{
					new_pos += gap2;
					if (!is_fm)
						new_pos += 3; /* skip sync */
					current_pos = new_pos;
					if (track_image[current_pos]==0xfb)
					{
						/* DAM found. We now write the sector content to the image. */
						ti99_sdf_write_sector(floppy, head, track, sector, &track_image[current_pos+1], SECTOR_SIZE, 0);
						current_pos += SECTOR_SIZE;
					}
					else
						LOG_FORMATS("DAM not found. Not writing write sector %d\n", sector);
					/* else not found, ignore this sector. */
				}
			}
			else
			{
				// else the sector head data do not match the
				// current track and head. Ignore the sector.
				LOG_FORMATS("Wrong track: wtrack=%d, imgtrack=%d, whead=%d, imghead=%d\n",wtrack,imgtrack, whead,head);
			}
		}
		else
		{
			/* One step forward. Retry. */
			current_pos++;
		}
	}
	return FLOPPY_ERROR_SUCCESS;
}

/*
    This method creates a properly formatted image.

    For a proper image, we need to format all tracks, and to create a
    minimal filesystem:

    name = "UNNAMED   "
    number of sectors
    sectors/track
    "DSK"
    protection = 0x00
    tracks/side
    sides
    density (0,1 = FM, 2 = MFM)
    0x14 - 0x37 : 0x00
    Allocation bitmap: 0xFF except for the first #sectors/AU size

    AU size = 1 (360 - 1440 sectors)
            = 2 (1441 - 2880 sectors)
        = 4 (2881 - 5760 sectors)
        = 8 (5760 - 11520 sectors)

    sector 0 =
    sector 1 = 256* 0x00
*/
/* static floperr_t ti99_sdf_format_track(floppy_image_legacy *floppy, int head, int track, option_resolution *params)
{
    dynamic_buffer sector0(SECTOR_SIZE);
    create_vib(sector0, params);
    if (true)
    {
        printf("not implemented\n");
        return FLOPPY_ERROR_SEEKERROR;
    }
    return FLOPPY_ERROR_SUCCESS;
}
*/

/*
    Get the offset from the start of the image file. In this format, logical
    tracks on two-sided disks are arranged as follows (example for 40
    tracks; accordingly for 35 or 80)

    head-track
    0-0
    0-1
    0-2
    0-3
    ...
    0-39
    1-39
    1-38
    1-37
    ...
    1-0
    Note that we take the imgtrack, which may be half of the drive track.
*/
static floperr_t ti99_sdf_get_offset(floppy_image_legacy *floppy, int head, int imgtrack, int sector, UINT64 *offset)
{
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);

	if ((head < 0) || (head >= tag->heads)
		|| (imgtrack < 0) || (imgtrack >= tag->tracks)
		|| (sector < 0) || (sector > tag->sectors))
		return FLOPPY_ERROR_SEEKERROR;

	if (head == 0)  /* track numbers increasing towards inner track */
	{
		*offset = (imgtrack * tag->sectors + sector) * SECTOR_SIZE;
	}
	else        /* track numbers increasing towards outer track */
	{
		*offset = (((2*tag->tracks)-1-imgtrack) * tag->sectors + sector) * SECTOR_SIZE;
	}
	return FLOPPY_ERROR_SUCCESS;
}

/*
    Read one sector at the specified position.
*/
static floperr_t ti99_sdf_read_sector(floppy_image_legacy *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	floperr_t err;
	UINT64 offset;
	int imgtrack = track;
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);

	if (use_80_track_drives && tag->tracks<=40)
	{
		imgtrack = track / 2;
	}

	err = ti99_sdf_get_offset(floppy, head, imgtrack, sector, &offset);
	if (err)
	{
		return err;
	}
	floppy_image_read(floppy, buffer, offset, SECTOR_SIZE);

	return FLOPPY_ERROR_SUCCESS;
}

/*
    For the SDF format, sectors are always numbered in ascending order,
    starting with the lowest numbered sector in each track. In other words,
    we have nothing to calculate here.
*/
static floperr_t ti99_sdf_read_indexed_sector(floppy_image_legacy *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	return ti99_sdf_read_sector(floppy, head, track, sector, buffer, buflen);
}

/*
    Write one sector at the specified position.
*/
static floperr_t ti99_sdf_write_sector(floppy_image_legacy *floppy, int head, int track, int sector, const void *buffer, size_t buflen, int ddam)
{
	UINT64 offset;
	floperr_t err;
	int imgtrack = track;

	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);

	if (use_80_track_drives && tag->tracks<=40)
	{
		imgtrack = track / 2;
	}

	err = ti99_sdf_get_offset(floppy, head, imgtrack, sector, &offset);
	if (err)
	{
		return err;
	}
	floppy_image_write(floppy, buffer, offset, SECTOR_SIZE);

	return FLOPPY_ERROR_SUCCESS;
}

/*
    See above; indexing and numbering coincide.
*/
static floperr_t ti99_sdf_write_indexed_sector(floppy_image_legacy *floppy, int head, int track, int sector, const void *buffer, size_t buflen, int ddam)
{
	return ti99_sdf_write_sector(floppy, head, track, sector, buffer, buflen, ddam);
}

/*
    Required for ReadAddress command
*/
static floperr_t ti99_sdf_get_indexed_sector_info(floppy_image_legacy *floppy, int head, int track, int sector_index, int *cylinder, int *side, int *sector, UINT32 *sector_length, unsigned long *flags)
{
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);

	if (use_80_track_drives && tag->tracks<=40)
	{
		track = track / 2;
	}

	if (sector_length)
		*sector_length=SECTOR_SIZE;
	if (cylinder)
		*cylinder = track;
	if (side)
		*side = head;
	if (sector)
		*sector = sector_index;
	if (flags)
		*flags = 0;
	if (sector_index >= tag->sectors)
	{
		return FLOPPY_ERROR_SEEKERROR;
	}

	return FLOPPY_ERROR_SUCCESS;
}

/*
    Try to identify the image as an SDF format image.
*/
static FLOPPY_IDENTIFY(ti99_sdf_identify)
{
	UINT64 size;
	size = floppy_image_size(floppy);
	*vote = ti99_sdf_guess_geometry(floppy, size, NULL);
	LOG_FORMATS("SDF voting %d\n", *vote);
	return FLOPPY_ERROR_SUCCESS;
}

/*
    Create the SDF data structures and set the callbacks.
*/
static FLOPPY_CONSTRUCT(ti99_sdf_construct)
{
	struct ti99dsk_geometry geometry;
	struct FloppyCallbacks *callbacks;
	struct ti99dsk_tag *tag;
	LOG_FORMATS("Reading image as SDF\n");

	if (params)
	{
		/* create */
		memset(&geometry, 0, sizeof(geometry));
		geometry.sides              = option_resolution_lookup_int(params, PARAM_HEADS);
		geometry.tracksperside          = option_resolution_lookup_int(params, PARAM_TRACKS);
		geometry.secspertrack           = option_resolution_lookup_int(params, PARAM_SECTORS);

		/* We don't have headers for geometry */
		/* check for usage in imgtool - we want to be able to create useful disks */
	}
	else
	{
		/* load */
		if (ti99_sdf_guess_geometry(floppy, floppy_image_size(floppy), &geometry)<50)
			return FLOPPY_ERROR_INVALIDIMAGE;
	}

	assert(geometry.sides);
	assert(geometry.tracksperside);
	assert(geometry.secspertrack);

	tag = (struct ti99dsk_tag *) floppy_create_tag(floppy, sizeof(struct ti99dsk_tag));

	if (!tag)
		return FLOPPY_ERROR_OUTOFMEMORY;

	tag->heads = geometry.sides;
	tag->tracks = geometry.tracksperside;
	tag->sectors = geometry.secspertrack;
	if (tag->sectors < 10)
		/* FM mode */
		tag->track_size = 16 + tag->sectors*334 + 231;
	else
		tag->track_size = 40 + tag->sectors*340 + 712;

	/* set up format callbacks */
	callbacks = floppy_callbacks(floppy);
	callbacks->read_sector = ti99_sdf_read_sector;
	callbacks->write_sector = ti99_sdf_write_sector;
	callbacks->read_indexed_sector = ti99_sdf_read_indexed_sector;
	callbacks->write_indexed_sector = ti99_sdf_write_indexed_sector;
	callbacks->read_track = ti99_sdf_read_track;
	callbacks->write_track = ti99_sdf_write_track;
/*  callbacks->format_track = ti99_sdf_format_track; */
	callbacks->get_track_size = ti99_get_track_size;
	/* post format unset */
	callbacks->get_heads_per_disk = ti99_get_heads_per_disk;
	callbacks->get_tracks_per_disk = ti99_get_tracks_per_disk;
	callbacks->get_sector_length = ti99_get_sector_length;
	callbacks->get_indexed_sector_info = ti99_sdf_get_indexed_sector_info;

	return FLOPPY_ERROR_SUCCESS;
}

/* -----------------------------------------------------------------------
 * Track Dump Format, aka PC99 format
 * This format includes all track data, including address marks and gaps.
 * However, CRC is not stored; instead, a f7f7 is stored at that location.
 * Two track images exist, one for FM and one for MFM format.
 *
 * In this implementation, however, we do calculate the CRC.
 *
 * All tracks have exactly the same size, so offsets are calculated easily.
 *
 * This format is a precise map of the data contents of the track;
 * however, it does not store clock patterns.
 *
 * The TDF format does not have a special index part, so any sector on it
 * must be searched (as done by the disk controller)
 *
 * Interesting detail: The original TI disk controller has a bug which has
 * just recently been discovered (using MESS!): The lead-in has another 9
 * bytes as a prefix; this is due to a wrongly set pointer in the format
 * routine. Strictly speaking, it produces invalid track images.
 * The original hardware does not care, however, so neither should we.
 * To keep the emulation realistic (and to allow for a
 * possible future extension with clock bits), we search for the
 * beginning of the track. From there on, however, we accept no further
 * deviations. This may make the resulting format unusable with the PC99
 * emulator, but it only appears when formatting with the TI disk
 * controller anyway. If the image is created with another disk controller or
 * using imgtool, the format is correctly created.
 *
 * FM track image
 *
 * Lead-in:           16*  00
 * Pre-ID gap:         6*  00    ---+
 *       IDAM:             fe       |
 *      Track:             tt       |
 *       Head:             hh       |
 *     Sector:             ss       |
 *       Size:             01       |
 *        CRC:         2*  f7       +--- repeat 9 times (8 in rare cases)
 *       Gap2:        11*  ff       |
 *                     6*  00       |
 *        DAM:             fb       |
 *    Content:       256*  xx       |
 *        CRC:         2*  f7       |
 *       Gap3:        45*  ff    ---+
 * Lead-out:         231*  ff
 *
 * --------------
 *
 * MFM track image
 *
 * Lead-in:           40*  4e
 * Pre-ID gap:        10*  00    ---+
 *       Sync:         3*  a1       |
 *       IDAM:             fe       |
 *      Track:             tt       |
 *       Head:             hh       |
 *     Sector:             ss       |
 *       Size:             01       |
 *        CRC:         2*  f7       +--- repeat 18 times (16 in rare cases)
 *       Gap2:        22*  4e       |
 *                    12*  00       |
 *       Sync:         3*  a1       |
 *        DAM:             fb       |
 *    Content:       256*  xx       |
 *        CRC:         2*  f7       |
 *       Gap3:        24*  4e    ---+
 * Lead-out:         712*  4e
 *
 * ------------------
 * The tracks are located on the image as follows (different to SDF!):
 *
 *   head-track
 *      0-0
 *      0-1
 *      0-2
 *      0-3
 *      ...
 *      0-39
 *      1-0
 *      1-1
 *      1-2
 *      ...
 *      1-39
 * ----------------------------------------------------------------------- */

#define TI99_FM 1
#define TI99_MFM 2

/* For the emulation of the clock pattern, we add a 9th bit. */
#define TI99_DAM 0x1fb
#define TI99_IDAM 0x1fe

#define TI99_FM_DAM_OFFSET 24
#define TI99_MFM_DAM_OFFSET 44

#define TI99_FM_TRACK_ADD 247
#define TI99_MFM_TRACK_ADD 752

#define TI99_FM_TOTAL_SECTOR 334
#define TI99_MFM_TOTAL_SECTOR 340

/*
    Determine offset of the first IDAM. This is the base of the simulation
    of the clock bytes. We use a heuristic to guess where the track begins
    and simply check whether we can locate the first IDAM and DAM. Return
    value is the offset to the first IDAM.
*/
static floperr_t determine_offset(int format, UINT8 *track, int *offset)
{
	floperr_t retval;
	int current_pos;

	if (format==TI99_FM)
	{
//      printf("Trying FM\n");
		// We are seeking the lead-in and pre-id gap from
		// position 0 to position 50. This is just to make sure that
		// if there are bad bytes at the beginning (original TI controller)
		// we will skip them.
		current_pos = find_block(track, 0, 50, 0x00, 22);
		if (current_pos==TI99DSK_BLOCKNOTFOUND)
		{
			LOG_FORMATS("Lead-in not found\n");
			retval = FLOPPY_ERROR_SEEKERROR;
		}
		else
		{
			current_pos += 22;
			if (track[current_pos]==0xfe /* IDAM */ &&
				track[current_pos+4]==0x01 /* sector length, always 256 for TI */ &&
				track[current_pos+24]==0xfb) /* DAM */
			{
				/* We're pretty sure this is the beginning. */
				*offset = current_pos;
				retval = FLOPPY_ERROR_SUCCESS;
			}
			else
			{
				LOG_FORMATS("IDAM or DAM not found\n");
				retval = FLOPPY_ERROR_SEEKERROR;
			}
		}
	}
	else
	{
		/* MFM */
//      printf("Trying MFM\n");
		int track_start = find_block(track, 0, 100, 0x4e, 40);
		if (track_start==TI99DSK_BLOCKNOTFOUND)
		{
			LOG_FORMATS("Lead-in not found\n");
			retval = FLOPPY_ERROR_SEEKERROR;
		}
		else
		{
			track_start += 40;
			current_pos = find_block(track, track_start, track_start+10, 0x00, 10);
			if (current_pos==TI99DSK_BLOCKNOTFOUND)
			{
				LOG_FORMATS("Pre-gap not found\n");
				retval = FLOPPY_ERROR_SEEKERROR;
			}
			else
			{
				current_pos += 10;
				if (track[current_pos] == 0xa1 /* First sync */ &&
					track[current_pos+3] == 0xfe /* IDAM */ &&
					track[current_pos+44] == 0xa1 /* First sync */ &&
					track[current_pos+47] == 0xfb) /* DAM */
				{
				/* We're pretty sure this is the beginning. */
					*offset = current_pos + 3;
					retval = FLOPPY_ERROR_SUCCESS;
				}
				else
				{
					LOG_FORMATS("Sync/IDAM/DAM not found\n");
					retval = FLOPPY_ERROR_SEEKERROR;
				}
			}
		}
	}
/*  printf("TDF offset = 0x%x\n", *offset); */
	return retval;
}

/*
    Gets a byte from the track. In this format we do not have a clock
    signal, so we will simulate it. Thus, this function returns a 9-bit
    value, with the first bit set as a marker for an address mark.

    This may be a little more complicated than necessary, but it correctly
    emulates the behavior of the controller (and it allows for an
    alternative format which actually supports clock and data).
*/
static int read_byte(int format, int first_idam, UINT8 *track, int position)
{
	int totalseclen;
	int dam_offset;
	int byte;

	/* Check whether we are at any IDAM or DAM in this track. */
	if (format==TI99_FM)
	{
		totalseclen = TI99_FM_TOTAL_SECTOR;
		dam_offset = TI99_FM_DAM_OFFSET;
	}
	else
	{
		totalseclen = TI99_MFM_TOTAL_SECTOR;
		dam_offset = TI99_MFM_DAM_OFFSET;
	}

	int curpos = (position - first_idam) % totalseclen;
	if (curpos==0 && track[position]==0xfe)
		byte = TI99_IDAM;
	else
	{
		if (curpos==dam_offset && track[position]==0xfb)
			byte = TI99_DAM;
		else
			byte = track[position] & 0xff;
	}
	return byte;
}

/*
    Determine the position of this track in the image.
*/
static floperr_t ti99_tdf_get_offset(floppy_image_legacy *floppy, int head, int imgtrack, UINT64 *offset)
{
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);

	if ((head < 0) || (head >= tag->heads) || (imgtrack < 0) || (imgtrack >= tag->tracks))
		return FLOPPY_ERROR_SEEKERROR;

	// For head 0, tracks increase from 0 to tracks-1; then for head 1,
	// tracks also increase from 0 to tracks-1. */
	*offset = head * (tag->tracks * tag->track_size) +  imgtrack * tag->track_size;

	return FLOPPY_ERROR_SUCCESS;
}

/*
    Reads a track. We just copy the contents from the format and recreate
    the CRC if it is the "blank" CRC as F7F7.
    This method assumes that the track division be done already.
*/
static floperr_t ti99_tdf_read_track_internal(floppy_image_legacy *floppy, int head, int imgtrack, UINT64 offset, void *buffer, size_t buflen)
{
	floperr_t err;
	UINT64 track_offset;
	int first_idam = 0;
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);
	int i, byte, crc;

	UINT8 *track_data = (UINT8*)buffer;

	err = ti99_tdf_get_offset(floppy, head, imgtrack, &track_offset);
	if (err)
		return err;

	floppy_image_read(floppy, buffer, track_offset, buflen);

	// Rebuild the CRCs. PC99 did not store the CRC but put
	// F7F7 in its place.
	// The first CRC is at position IDAM + 5
	// The second CRC is at position IDAM + 0x119 (FM) or +0x12d (MFM)
	// All this is repeated for each sector in the track

	if (determine_offset(tag->format, track_data, &first_idam)==FLOPPY_ERROR_SEEKERROR)
		return FLOPPY_ERROR_SEEKERROR;

	i = 0;
	while (i < buflen)
	{
		byte = read_byte(tag->format, first_idam, track_data, i);
		if (byte == TI99_IDAM)
		{
			// Do we have a valid CRCs already? Then this image
			// already contains proper CRCs handling. Do not recreate them.
			if (track_data[i+5] != 0xf7 || track_data[i+6] != 0xf7)
				return FLOPPY_ERROR_SUCCESS;

			crc = ccitt_crc16(0xffff, &track_data[i], 5);
			track_data[i+5] = (crc>>8) & 0xff;
			track_data[i+6] = (crc & 0xff);
		}
		else
		{
			if (byte == TI99_DAM)
			{
				crc = ccitt_crc16(0xffff, &track_data[i], SECTOR_SIZE+1);
				track_data[i+SECTOR_SIZE+1] = (crc>>8) & 0xff;
				track_data[i+SECTOR_SIZE+2] = (crc & 0xff);
			}
		}
		i++;
	}
//  dump_contents(track_data, buflen);
	return FLOPPY_ERROR_SUCCESS;
}

/*
    Reads a track. We just copy the contents from the format, without
    changes.
*/
static floperr_t ti99_tdf_read_track(floppy_image_legacy *floppy, int head, int track, UINT64 offset, void *buffer, size_t buflen)
{
	int imgtrack = track;

	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);
	if (use_80_track_drives && tag->tracks<=40)
	{
		imgtrack = track / 2;
	}
	return ti99_tdf_read_track_internal(floppy, head, imgtrack, offset, buffer, buflen);
}

/*
    Writes a track.
*/
static floperr_t ti99_tdf_write_track(floppy_image_legacy *floppy, int head, int track, UINT64 offset, const void *buffer, size_t buflen)
{
	floperr_t err;
	UINT64 track_offset;
	int imgtrack = track;

	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);
	if (use_80_track_drives && tag->tracks<=40)
	{
		imgtrack = track / 2;
	}

	err = ti99_tdf_get_offset(floppy, head, imgtrack, &track_offset);
	if (err)
		return err;

	// According to the TDF format, we don't need the CRC but replace it
	// with F7F7. For now we keep it and see whether PC99 can cope with
	// that. (Otherwise we would have to copy the const buffer). */
	floppy_image_write(floppy, buffer, offset + track_offset, buflen);
	return FLOPPY_ERROR_SUCCESS;
}

/*
static floperr_t ti99_tdf_format_track(floppy_image_legacy *floppy, int head, int track, option_resolution *params)
{
    int sectors;
    int sector_length;
    int interleave;
    int first_sector_id;
    sectors         = option_resolution_lookup_int(params, PARAM_SECTORS);
    sector_length   = option_resolution_lookup_int(params, PARAM_SECTOR_LENGTH);
    interleave      = option_resolution_lookup_int(params, PARAM_INTERLEAVE);
    first_sector_id = option_resolution_lookup_int(params, PARAM_FIRST_SECTOR_ID);
    return FLOPPY_ERROR_SUCCESS;
}
*/

/*
    Get a sector from a track. This function takes a complete track,
    searches for the sector, and passes a pointer to the sector contents
    back.
    As the original TI controller creates bad bytes at the start of a track
    we do *not* assume a simple layout. The strategy is to search the
    lead-in of the track, and from this offset, step through the track
    in search for the sector. Since we do not have index marks (we cannot
    distinguish them from ordinary data, as we do not store the clock), we
    do not attempt to detect IDAM or DAM.
*/
static floperr_t ti99_tdf_seek_sector_in_track(floppy_image_legacy *floppy, int head, int track, int sector, UINT8 *track_data, UINT8 **sector_data)
{
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);

	int byte;
	int state;
	enum { SEARCHIDAM, SEARCHDAM };

	state = SEARCHIDAM;
	if (tag->first_idam==0) /* should we check for every track? */
	{
		if (determine_offset(tag->format, track_data, &tag->first_idam)==FLOPPY_ERROR_SEEKERROR)
		{
			LOG_FORMATS("could not determine offset\n");
			return FLOPPY_ERROR_SEEKERROR;
		}
	}

	for (int i=0; i < tag->track_size; i++)
	{
		byte = read_byte(tag->format, tag->first_idam, track_data, i);

		switch (state)
		{
		case SEARCHIDAM:
			/* search for the IDAM */
			if (byte==TI99_IDAM)
			{
				if ((track_data[i+1]==track) && (track_data[i+2]==head) && (track_data[i+3]==sector))
					state = SEARCHDAM;
				else
					state = SEARCHIDAM;
			}
			break;
		case SEARCHDAM:
			if (byte==TI99_DAM)
			{
				*sector_data = &track_data[i+1];
				return FLOPPY_ERROR_SUCCESS;
			}
			break;
		}
	}
	LOG_FORMATS("Sector not found.\n");
	return FLOPPY_ERROR_SEEKERROR;
}

static floperr_t ti99_tdf_read_sector(floppy_image_legacy *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	floperr_t err;
	UINT8 *sector_data;
	int imgtrack = track;
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);
	dynamic_buffer track_data(tag->track_size);

	if (use_80_track_drives && tag->tracks<=40)
	{
		imgtrack = track / 2;
	}

	err = ti99_tdf_read_track_internal(floppy, head, imgtrack, 0, track_data, tag->track_size);
	if (err)
		return err;

	err = ti99_tdf_seek_sector_in_track(floppy, head, imgtrack, sector, track_data, &sector_data);
	if (err)
		return err;
	/* verify CRC? */

	memcpy(buffer, sector_data, SECTOR_SIZE);
	return FLOPPY_ERROR_SUCCESS;
}

/*
    Writes a sector into the track. Note that the indexed_write method
    may be used instead of this method, according to the implementation
    of flopimg/flopdrv.
*/
static floperr_t ti99_tdf_write_sector(floppy_image_legacy *floppy, int head, int track, int sector, const void *buffer, size_t buflen, int ddam)
{
	floperr_t err;
	UINT8 *sector_data;
	UINT8 *track_data;
	UINT64 track_offset;
	int imgtrack = track;
	UINT64 offset;
	UINT8 crc_field[2];
	int crc;

	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);

	track_data = (UINT8*)malloc(tag->track_size);
	if (use_80_track_drives && tag->tracks<=40)
	{
		imgtrack = track / 2;
	}

	err = ti99_tdf_read_track_internal(floppy, head, imgtrack, 0, track_data, tag->track_size);
	if (err)
		return err;
	err = ti99_tdf_seek_sector_in_track(floppy, head, imgtrack, sector, track_data, &sector_data);
	if (err)
		return err;

	err = ti99_tdf_get_offset(floppy, head, imgtrack, &track_offset);
	if (err)
		return err;

	offset = track_offset + (sector_data - track_data);

	floppy_image_write(floppy, buffer, offset, SECTOR_SIZE);

	// Recalculate CRC. We init the CRC with bf84 (DAM fb is
	// already included; would be ffff without)
	crc = ccitt_crc16(0xbf84, (UINT8*)buffer, SECTOR_SIZE);
	crc_field[0] = (crc>>8) & 0xff;
	crc_field[1] = (crc & 0xff);
	floppy_image_write(floppy, crc_field, offset+SECTOR_SIZE, 2);

	return FLOPPY_ERROR_SUCCESS;
}

static floperr_t ti99_tdf_write_indexed_sector(floppy_image_legacy *floppy, int head, int track, int sector_index, const void *buffer, size_t buflen, int ddam)
{
	/* Read track, head */
	int byte;
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);
	dynamic_buffer track_data(tag->track_size);
	UINT8 *sector_data;
	UINT64 track_offset;
	UINT64 offset;
	UINT8 crc_field[2];

	int crc;
	int imgtrack = track;

	int i;
	floperr_t err;

	if (use_80_track_drives && tag->tracks<=40)
	{
		imgtrack = track / 2;
	}

	err = ti99_tdf_read_track_internal(floppy, head, imgtrack, 0, track_data, tag->track_size);
	if (err)
	{
		return err;
	}

	/* Search for the sector_index-th sector. */
	if (tag->first_idam==0) /* should we check for every track? */
	{
		if (determine_offset(tag->format, track_data, &tag->first_idam)==FLOPPY_ERROR_SEEKERROR)
		{
			return FLOPPY_ERROR_SEEKERROR;
		}
	}

	/* Search for the IDAM of the n-th sector */
	i=0;
	while (i < tag->track_size && sector_index>=0)
	{
		byte = read_byte(tag->format, tag->first_idam, track_data, i);
		if (byte == TI99_IDAM)
		{
			sector_index--;
		}
		i++;
	}
	i--;

	/* Not that many sectors in this track? */
	if (sector_index>0)
	{
		return FLOPPY_ERROR_SEEKERROR;
	}

	/* Find the DAM. */
	while (i < tag->track_size)
	{
		byte = read_byte(tag->format, tag->first_idam, track_data, i);
		if (byte == TI99_DAM)
			break;
		else
			i++;
	}

	/* There was no DAM. Image seems to be broken. */
	if (i == tag->track_size)
	{
		return FLOPPY_ERROR_SEEKERROR;
	}

	/* Sector data is right after the DAM. */
	sector_data = &track_data[i+1];

	/* Get the position of the track in the image. */
	err = ti99_tdf_get_offset(floppy, head, imgtrack, &track_offset);
	if (err)
		return err;

	offset = track_offset + (sector_data - track_data);

	/* Write the sector data at that position. */
	floppy_image_write(floppy, buffer, offset, SECTOR_SIZE);

	// Recalculate CRC. We init the CRC with bf84 (DAM fb is
	// already included; would be ffff without)
	crc = ccitt_crc16(0xbf84, (UINT8*)buffer, SECTOR_SIZE);
	crc_field[0] = (crc>>8) & 0xff;
	crc_field[1] = (crc & 0xff);
	floppy_image_write(floppy, crc_field, offset+SECTOR_SIZE, 2);

	return FLOPPY_ERROR_SUCCESS;
}


static floperr_t ti99_tdf_find_indexed_sector(floppy_image_legacy *floppy, int head, int track, int sector_index, int *cylinder, int *side, int *sector, UINT32 *sector_length, void *buffer, unsigned long *flags)
{
	/* Read track, head */
	int byte = 0;
	struct ti99dsk_tag *tag = (ti99dsk_tag*)floppy_tag(floppy);
	dynamic_buffer track_data(tag->track_size);
	int imgtrack = track;
	int i;
	floperr_t err;
	floperr_t retval;

	if (use_80_track_drives && tag->tracks<=40)
	{
		imgtrack = track / 2;
	}

	err = ti99_tdf_read_track_internal(floppy, head, imgtrack, 0, track_data, tag->track_size);
	if (err)
	{
		return err;
	}

	/* Search for the sector_index-th sector. */

	if (tag->first_idam==0) /* should we check for every track? */
	{
		if (determine_offset(tag->format, track_data, &tag->first_idam)==FLOPPY_ERROR_SEEKERROR)
		{
			return FLOPPY_ERROR_SEEKERROR;
		}
	}

	i=0;
	sector_index = sector_index+1;
	while (i++ < tag->track_size && sector_index>0)
	{
		byte = read_byte(tag->format, tag->first_idam, track_data, i);
		if (byte == TI99_IDAM)
		{
			sector_index--;
		}
	}

	retval = FLOPPY_ERROR_SEEKERROR;
	if (sector_index==0)
	{
		/* If desired, return the ID field */
		if (cylinder)
		{
			*cylinder = track_data[i];
			//      printf("cylinder=%d", *cylinder);

		}
		if (side)
		{
			*side = track_data[i+1];
			//      printf(", side=%d", *side);
		}
		if (sector)
		{
			*sector = track_data[i+2];
			//      printf(", sector=%d", *sector);
		}
		if (sector_length)
		{
			*sector_length = 128 << track_data[i+3];
			//      printf(", sector_length=%d\n", *sector_length);
		}
		if (flags)
		{
			*flags = 0;
		}
		retval = FLOPPY_ERROR_SUCCESS;

		/* If desired, return the sector contents. */
		if (buffer)
		{
			while (i++ < tag->track_size)
			{
				byte = read_byte(tag->format, tag->first_idam, track_data, i);
				if (byte == TI99_DAM)
					break;
			}
			if (byte == TI99_DAM)
			{
				memcpy(buffer, &track_data[i+1], SECTOR_SIZE);
			}
			else
			{
				retval = FLOPPY_ERROR_SEEKERROR;
			}
		}
	}
	return retval;
}

/*
    Required for ReadAddress command. This function gets the 1st, 2nd, 3rd,
    sector (and so on) from the track, not sector 1, 2, 3. That is, the
    returned sector info depends on the interleave and the start sector.
*/
static floperr_t ti99_tdf_get_indexed_sector_info(floppy_image_legacy *floppy, int head, int track, int sector_index, int *cylinder, int *side, int *sector, UINT32 *sector_length, unsigned long *flags)
{
	return ti99_tdf_find_indexed_sector(floppy, head, track, sector_index, cylinder, side, sector, sector_length, NULL, flags);
}

static floperr_t ti99_tdf_read_indexed_sector(floppy_image_legacy *floppy, int head, int track, int sector, void *buffer, size_t buflen)
{
	return ti99_tdf_find_indexed_sector(floppy, head, track, sector, (int*)NULL, (int*)NULL, (int*)NULL, (UINT32*)NULL, buffer, (unsigned long*)NULL);
}


/*
    Guess the geometry of the disk.
    1. Check whether the disk is formatted, and if so, whether it is
       a FM or a MFM disk
    2. If this fails, guess from the file size which format it should have

    SDF formats have sizes which are multiples of 10KiB.
    TDF: full sector size FM (MFM) = 334 (340) bytes
             additional track data FM (MFM) = 247 (752) bytes

         => check whether size is dividable by 40 (or 35) and by 2
         => get track size
         => subtract track data
         => divide by 334 or 340
         => number of sectors should be 8, 9, 16, 18, 32, or 36

     SSDD is 274880, but DSSD is 260240, so they differ (unlike in the SDF format)
     Still, we cannot tell apart single-sided 80 track formats from double-sided
     40 tracks. We assume the latter. Moreover, we only attempt to detect
     9/18/36 sectors (no 8/16) and 40 tracks (no 35).

     SSSD40 =  130120
     DSSD   =  260240
     SSDD   =  274880
     DSDD   =  549760
     DSHD   = 1039360
     DSDD80 = 1099520
     DSHD80 = 2078720
*/
static int ti99_tdf_guess_geometry(floppy_image_legacy *floppy, UINT64 size,
	struct ti99dsk_geometry *geometry)
{
	int idamcnt, state, track, head, i, totalseclen = 0, format = 0, byte, tracklength, trackadd = 0;
	int first_idam = 0;
	/* Allocate enough bytes to hold the longest supported track. */
	dynamic_buffer track_data(13000);
	/*int offset;*/

	struct ti99dsk_geometry dummy_geometry;

	if (geometry)
		memset(geometry, 0, sizeof(*geometry));
	else
		geometry = &dummy_geometry;

	floppy_image_read(floppy, track_data, 0, 13000);

	if (determine_offset(TI99_MFM, track_data, &first_idam)==FLOPPY_ERROR_SEEKERROR)
	{
		/* MFM failed. */
		if (determine_offset(TI99_FM, track_data, &first_idam)==FLOPPY_ERROR_SEEKERROR)
		{
			LOG_FORMATS("IDAM not found. Unformatted disk.\n");

			// FM failed as well. Disk is not formatted. We assume
			// a format that fits into the space provided by the file.
			// We only give a moderate vote in this case, so SDF
			// make take it.

			if (size < 130120 || size > 2078720)
			{
				LOG_FORMATS("Unknown format size: %d\n", (int)size);
				return 0;
			}
			if (size >= 130120 && size < 260240)
			{
				/* SSSD */
				geometry->sides = 1;
				geometry->tracksperside = 40;
				geometry->secspertrack = 9;
				geometry->density = 1;
				return 50;
			}
			if (size >= 260240 && size < 274880)
			{
				/* DSSD */
				geometry->sides = 2;
				geometry->tracksperside = 40;
				geometry->secspertrack = 9;
				geometry->density = 1;
				return 50;
			}
			if (size >= 274880 && size < 549760)
			{
				/* SSDD */
				geometry->sides = 1;
				geometry->tracksperside = 40;
				geometry->secspertrack = 18;
				geometry->density = 2;
				return 50;
			}
			if (size >= 549760 && size < 1039360)
			{
				/* DSDD */
				geometry->sides = 2;
				geometry->tracksperside = 40;
				geometry->secspertrack = 18;
				geometry->density = 2;
				return 50;
			}
			if (size >= 1039360 && size < 1099520)
			{
				/* DSHD */
				geometry->sides = 2;
				geometry->tracksperside = 40;
				geometry->secspertrack = 36;
				geometry->density = 2;
				return 50;
			}
			if (size >= 1099520 && size < 2078720)
			{
				/* DSDD80 */
				geometry->sides = 2;
				geometry->tracksperside = 80;
				geometry->secspertrack = 18;
				geometry->density = 2;
				return 50;
			}
			if (size == 2078720)
			{
				/* DSHD80 */
				geometry->sides = 2;
				geometry->tracksperside = 80;
				geometry->secspertrack = 36;
				geometry->density = 2;
				return 50;
			}
		}
		else
		{
			/* FM format */
			totalseclen = TI99_FM_TOTAL_SECTOR;
			trackadd = TI99_FM_TRACK_ADD; /* additional track data (gaps etc.) */
			geometry->density = 1;
			format = TI99_FM;
		}
	}
	else
	{
		/* MFM format */
		totalseclen = TI99_MFM_TOTAL_SECTOR;
		trackadd = TI99_MFM_TRACK_ADD;
		geometry->density = 2;
		format = TI99_MFM;
	}

	/* Count the number of IDAMs on track 0, head 0 */
	idamcnt = 0;
	state = 0;
	track = 0;
	head = 0;
	i=0;

	while (i++ < 13000 && head == 0 && track == 0)
	{
		byte = read_byte(format, first_idam, track_data, i);
		switch (state)
		{
		case 0:
			if (byte==TI99_IDAM)
				state = 1;
			break;
		case 1:
			track = byte;
			state = 2;
			break;
		case 2:
			head = byte;
			state = 3;
			break;
		case 3:
			idamcnt++;
			state = 0;
		}
	}
	LOG_FORMATS("Determined %d sectors\n", idamcnt);

	// Now calculate the geometry
	// The track size, in theory, may change due to reformatting,
	// and it may also vary between tracks, but the TDF does not support
	// varying track lengths. So every track has the same length as track 0.
	// Also, we don't yet know whether the drive and the disk have the
	// same track count. For 80 track drives and 40 track disks, we need
	// double-stepping; this will be known as soon as the controller is
	// initialized; we need to check that in the access methods.

	tracklength = idamcnt * totalseclen + trackadd;
	/*offset = 0;*/

	// Read the last track. We assume that all tracks are properly
	// formatted; otherwise, the above calculations would already fail.
	// That is, the last track in the image tells us whether it is for
	// head 0 or for head 1.

	geometry->sides = 1;
	floppy_image_read(floppy, track_data, size-tracklength, tracklength);
	if (determine_offset(format, track_data, &first_idam)==FLOPPY_ERROR_SEEKERROR)
	{
		/* error ... what now? */
		LOG_FORMATS("Error when reading last track. Image broken.\n");
		return 50;
	}
	else
	{
		if (track_data[first_idam+2]==0)
		{
			geometry->sides = 1;
		}
		else
		{
			if (track_data[first_idam+2]==1)
			{
				geometry->sides = 2;
			}
			else
			{
				LOG_FORMATS("Error: Last track has invalid first IDAM: head = %d.\n", track_data[first_idam+2]);
			}

		}
	}


	geometry->tracksperside = (size / tracklength) / geometry->sides;
	geometry->secspertrack = idamcnt;

	if (geometry->tracksperside < 35 || geometry->tracksperside > 80)
	{
		LOG_FORMATS("Unsupported track count: %d\n", geometry->tracksperside);
		return 0;
	}

	return 100;
}

static FLOPPY_IDENTIFY(ti99_tdf_identify)
{
	UINT64 size;
	size = floppy_image_size(floppy);
	*vote = ti99_tdf_guess_geometry(floppy, size, NULL);
	LOG_FORMATS("TDF voting %d\n", *vote);
	return FLOPPY_ERROR_SUCCESS;
}

static FLOPPY_CONSTRUCT(ti99_tdf_construct)
{
	struct ti99dsk_geometry geometry;
	struct FloppyCallbacks *callbacks;
	struct ti99dsk_tag *tag;
	LOG_FORMATS("Reading image as TDF\n");
	if (params)
	{
		/* create */
		memset(&geometry, 0, sizeof(geometry));
/*
        geometry.sides              = option_resolution_lookup_int(params, PARAM_HEADS);
        geometry.tracksperside          = option_resolution_lookup_int(params, PARAM_TRACKS);
        geometry.secspertrack           = option_resolution_lookup_int(params, PARAM_SECTORS);
*/
	}
	else
	{
		/* load */
		if (ti99_tdf_guess_geometry(floppy, floppy_image_size(floppy), &geometry)<50)
			return FLOPPY_ERROR_INVALIDIMAGE;
	}

	assert(geometry.sides);
	assert(geometry.tracksperside);
	assert(geometry.secspertrack);

	tag = (struct ti99dsk_tag *) floppy_create_tag(floppy, sizeof(struct ti99dsk_tag));

	tag->heads = geometry.sides;
	tag->tracks = geometry.tracksperside;

	if (geometry.density==TI99_FM)
		tag->track_size = geometry.secspertrack * TI99_FM_TOTAL_SECTOR + TI99_FM_TRACK_ADD;
	else
		tag->track_size = geometry.secspertrack * TI99_MFM_TOTAL_SECTOR + TI99_MFM_TRACK_ADD;

	tag->format = geometry.density;
	tag->first_idam = 0; /* let's find out later */
	tag->dam_offset = 0;

	/* set up format callbacks */
	callbacks = floppy_callbacks(floppy);
	callbacks->read_sector = ti99_tdf_read_sector;
	callbacks->write_sector = ti99_tdf_write_sector;
	callbacks->read_indexed_sector = ti99_tdf_read_indexed_sector;
	callbacks->write_indexed_sector = ti99_tdf_write_indexed_sector;
	callbacks->read_track = ti99_tdf_read_track;
	callbacks->write_track = ti99_tdf_write_track;
/*  callbacks->format_track = ti99_tdf_format_track; */
	callbacks->get_track_size = ti99_get_track_size;
	/* post format unset */
	callbacks->get_heads_per_disk = ti99_get_heads_per_disk;
	callbacks->get_tracks_per_disk = ti99_get_tracks_per_disk;
	callbacks->get_sector_length = ti99_get_sector_length;
	callbacks->get_indexed_sector_info = ti99_tdf_get_indexed_sector_info;

	return FLOPPY_ERROR_SUCCESS;
}

/* ----------------------------------------------------------------------- */

LEGACY_FLOPPY_OPTIONS_START( ti99 )
	LEGACY_FLOPPY_OPTION( ti99_sdf, "dsk",          "TI99 sector dump (v9t9)",  ti99_sdf_identify,  ti99_sdf_construct, NULL,
		HEADS([1]-2)
		TRACKS(35-[40]-80)
		SECTORS(8/9/16/[18]/36)
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID(0))
	LEGACY_FLOPPY_OPTION( ti99_tdf, "dsk,dtk",          "TI99 track dump (pc99)",   ti99_tdf_identify,  ti99_tdf_construct, NULL,
		TRACKS(35-[40]-80)
		SECTORS(8/9/16/[18]/36)
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID(0))
LEGACY_FLOPPY_OPTIONS_END
