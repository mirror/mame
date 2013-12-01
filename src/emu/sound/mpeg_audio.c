/***************************************************************************

    MPEG audio support.  Only layer2 and variants for now.

***************************************************************************/

#include "emu.h"
#include "mpeg_audio.h"

mpeg_audio::mpeg_audio(const void *_base, unsigned int _accepted, bool lsb_first, int _position_align)
{
	base = (const UINT8 *)_base;
	accepted = _accepted;
	do_gb = lsb_first ? do_gb_lsb : do_gb_msb;
	position_align = _position_align ? _position_align - 1 : 0;

	memset(audio_buffer, 0, sizeof(audio_buffer));
	audio_buffer_pos[0] = 16*32;
	audio_buffer_pos[1] = 16*32;
}


bool mpeg_audio::decode_buffer(int &pos, int limit, short *output,
								int &output_samples, int &sample_rate, int &channels)
{
	if(limit - pos < 16)
		return false;

	// Scan for the sync mark
	//
	// Avoid the exception dance at the point where going out of bound
	// is the most probable and easily avoidable

	current_pos = pos;
	current_limit = limit;
	unsigned short sync = do_gb(base, current_pos, 12);

	retry_sync:
	while(sync != 0xfff && current_pos < limit)
		sync = ((sync << 1) | do_gb(base, current_pos, 1)) & 0xfff;

	if(limit - current_pos < 4)
		return false;

	int layer = 0;
	int variant = do_gb(base, current_pos, 3);
	switch(variant) {
	case 2:
		if(accepted & L2_5)
			layer = 2;
		else if(accepted & AMM)
			layer = 4;
		break;

	case 5:
		if(accepted & L3)
			layer = 3;
		break;

	case 6:
		if(accepted & (L2|L2_5))
			layer = 2;
		else if(accepted & AMM)
			layer = 4;
		break;

	case 7:
		if(accepted & L1)
			layer = 1;
		break;
	}

	if(!layer) {
		current_pos -= 3;
		sync = ((sync << 1) | do_gb(base, current_pos, 1)) & 0xfff;
		goto retry_sync;
	}

	switch(layer) {
	case 1:
		abort();
	case 2:
		try {
			read_header_mpeg2(variant == 2);
			read_data_mpeg2();
			decode_mpeg2(output, output_samples);
		} catch(limit_hit) {
			return false;
		}
		break;
	case 3:
		abort();
	case 4:
		try {
			if (!read_header_amm(variant == 2))
				return false;
			read_data_mpeg2();
			decode_mpeg2(output, output_samples);
		} catch(limit_hit) {
			return false;
		}
		break;
	}

	if(position_align)
		current_pos = (current_pos + position_align) & ~position_align;

	pos = current_pos;
	sample_rate = sample_rates[sampling_rate];
	channels = channel_count;
	return true;
}

bool mpeg_audio::read_header_amm(bool layer25)
{
	gb(1); // unused
	int full_packets_count = gb(4); // max 12
	int srate_index = gb(2); // max 2
	sampling_rate = srate_index + 4 * layer25;
	int last_packet_frame_id = gb(2); // max 2
	last_frame_number = 3*full_packets_count + last_packet_frame_id;
	int stereo_mode = gb(2);
	int stereo_mode_ext = gb(2);
	param_index = gb(3);
	gb(1); // must be zero
	
	if (last_frame_number == 0)
		return false;

	channel_count = stereo_mode != 3 ? 2 : 1;

	total_bands = total_band_counts[param_index];
	joint_bands = total_bands;
	if(stereo_mode == 1) // joint stereo
		joint_bands = joint_band_counts[stereo_mode_ext];
	if(joint_bands > total_bands )
		joint_bands = total_bands;
	
	return true;
}

void mpeg_audio::read_header_mpeg2(bool layer25)
{
	int prot = gb(1);
	int bitrate_index = gb(4);
	sampling_rate = gb(2);
	gb(1); // padding
	gb(1);
	last_frame_number = 36;
	int stereo_mode = gb(2);
	int stereo_mode_ext = gb(2);
	gb(2); // copyright, original
	gb(2); // emphasis
	if(!prot)
		gb(16); // crc

	channel_count = stereo_mode != 3 ? 2 : 1;

	param_index = layer2_param_index[channel_count-1][sampling_rate][bitrate_index];
	assert(param_index != -1);

	total_bands = total_band_counts[param_index];
	joint_bands = total_bands;
	if(stereo_mode == 1) // joint stereo
		joint_bands = joint_band_counts[stereo_mode_ext];
	if(joint_bands > total_bands )
		joint_bands = total_bands;
}

void mpeg_audio::read_data_mpeg2()
{
	read_band_params();
	read_scfci();
	read_band_amplitude_params();
}

void mpeg_audio::decode_mpeg2(short *output, int &output_samples)
{
	output_samples = 0;
	build_amplitudes();

	// Supposed to stop at last_frame_number when it's not 12*3+2 = 38
	int frame_number = 0;
	for(int upper_step = 0; upper_step<3; upper_step++)
		for(int middle_step = 0; middle_step < 4; middle_step++) {
			build_next_segments(upper_step);
			for(int lower_step = 0; lower_step < 3; lower_step++) {
				retrieve_subbuffer(lower_step);

				for(int chan=0; chan<channel_count; chan++) {
					double resynthesis_buffer[32];
					idct32(subbuffer[chan], audio_buffer[chan] + audio_buffer_pos[chan]);
					resynthesis(audio_buffer[chan] + audio_buffer_pos[chan] + 16, resynthesis_buffer);
					scale_and_clamp(resynthesis_buffer, output + chan, channel_count);
					audio_buffer_pos[chan] -= 32;
					if(audio_buffer_pos[chan]<0) {
						memmove(audio_buffer[chan]+17*32, audio_buffer[chan], 15*32*sizeof(audio_buffer[chan][0]));
						audio_buffer_pos[chan] = 16*32;
					}
				}
				output += 32*channel_count;
				output_samples += 32;
				frame_number++;
				if(frame_number == last_frame_number)
					return;
			}
		}
}

const int mpeg_audio::sample_rates[8] = { 44100, 48000, 32000, 0, 22050, 24000, 16000, 0 };

const int mpeg_audio::layer2_param_index[2][4][16] = {
	{
		{  1,  2,  2,  0,  0,  0,  1,  1,  1,  1,  1, -1, -1, -1, -1, -1 },
		{  0,  2,  2,  0,  0,  0,  0,  0,  0,  0,  0, -1, -1, -1, -1, -1 },
		{  1,  3,  3,  0,  0,  0,  1,  1,  1,  1,  1, -1, -1, -1, -1, -1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	},
	{
		{  1, -1, -1, -1,  2, -1,  2,  0,  0,  0,  1,  1,  1,  1,  1, -1 },
		{  0, -1, -1, -1,  2, -1,  2,  0,  0,  0,  0,  0,  0,  0,  0, -1 },
		{  1, -1, -1, -1,  3, -1,  3,  0,  0,  0,  1,  1,  1,  1,  1, -1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	}
};

const int mpeg_audio::band_parameter_indexed_values[5][32][17] = {
	{
		{  0,  1,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, -1, },
		{  0,  1,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, -1, },
		{  0,  1,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
	},
	{
		{  0,  1,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, -1, },
		{  0,  1,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, -1, },
		{  0,  1,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 17, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  3,  4,  5,  6, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
	},
	{
		{  0,  1,  2,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
	},
	{
		{  0,  1,  2,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
	},
	{
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, -1, },
		{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4,  5,  6,  7,  8, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0,  1,  2,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
		{  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, },
	}
};

const int mpeg_audio::band_parameter_index_bits_count[5][32] = {
	{ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0, 0, 0, 0, 0, },
	{ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 0, 0, },
	{ 4, 4, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
	{ 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
	{ 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, },
};

const int mpeg_audio::total_band_counts[5] = { 27, 30, 8, 12, 30 };

const int mpeg_audio::joint_band_counts[4] = { 4, 8, 12, 16 };

const mpeg_audio::band_info mpeg_audio::band_infos[18] = {
	{ 0x0000,  0.00,  0,  0, 0,  0,           0,          0,                 0,         0 },
	{ 0x0003,  7.00,  2,  5, 3,  9, 1-1.0/    4, -1.0/    4,   1/(1-1.0/    4), 1.0/    2 },
	{ 0x0005, 11.00,  3,  7, 5, 25, 1-3.0/    8, -3.0/    8,   1/(1-3.0/    8), 1.0/    2 },
	{ 0x0007, 16.00,  3,  9, 0,  0, 1-1.0/    8, -1.0/    8,   1/(1-1.0/    8), 1.0/    4 },
	{ 0x0009, 20.84,  4, 10, 9, 81, 1-7.0/   16, -7.0/   16,   1/(1-7.0/   16), 1.0/    2 },
	{ 0x000f, 25.28,  4, 12, 0,  0, 1-1.0/   16, -1.0/   16,   1/(1-1.0/   16), 1.0/    8 },
	{ 0x001f, 31.59,  5, 15, 0,  0, 1-1.0/   32, -1.0/   32,   1/(1-1.0/   32), 1.0/   16 },
	{ 0x003f, 37.75,  6, 18, 0,  0, 1-1.0/   64, -1.0/   64,   1/(1-1.0/   64), 1.0/   32 },
	{ 0x007f, 43.84,  7, 21, 0,  0, 1-1.0/  128, -1.0/  128,   1/(1-1.0/  128), 1.0/   64 },
	{ 0x00ff, 49.89,  8, 24, 0,  0, 1-1.0/  256, -1.0/  256,   1/(1-1.0/  256), 1.0/  128 },
	{ 0x01ff, 55.93,  9, 27, 0,  0, 1-1.0/  512, -1.0/  512,   1/(1-1.0/  512), 1.0/  256 },
	{ 0x03ff, 61.96, 10, 30, 0,  0, 1-1.0/ 1024, -1.0/ 1024,   1/(1-1.0/ 1024), 1.0/  512 },
	{ 0x07ff, 67.98, 11, 33, 0,  0, 1-1.0/ 2048, -1.0/ 2048,   1/(1-1.0/ 2048), 1.0/ 1024 },
	{ 0x0fff, 74.01, 12, 36, 0,  0, 1-1.0/ 4096, -1.0/ 4096,   1/(1-1.0/ 4096), 1.0/ 2048 },
	{ 0x1fff, 80.03, 13, 39, 0,  0, 1-1.0/ 8192, -1.0/ 8192,   1/(1-1.0/ 8192), 1.0/ 4096 },
	{ 0x3fff, 86.05, 14, 42, 0,  0, 1-1.0/16384, -1.0/16384,   1/(1-1.0/16384), 1.0/ 8192 },
	{ 0x7fff, 92.01, 15, 45, 0,  0, 1-1.0/32768, -1.0/32768,   1/(1-1.0/32768), 1.0/16384 },
	{ 0xffff, 98.01, 16, 48, 0,  0, 1-1.0/65536, -1.0/65536,   1/(1-1.0/65536), 1.0/32768 },
};

const double mpeg_audio::scalefactors[64] = {
	2.00000000000000, 1.58740105196820, 1.25992104989487, 1.00000000000000,
	0.79370052598410, 0.62996052494744, 0.50000000000000, 0.39685026299205,
	0.31498026247372, 0.25000000000000, 0.19842513149602, 0.15749013123686,
	0.12500000000000, 0.09921256574801, 0.07874506561843, 0.06250000000000,
	0.04960628287401, 0.03937253280921, 0.03125000000000, 0.02480314143700,
	0.01968626640461, 0.01562500000000, 0.01240157071850, 0.00984313320230,
	0.00781250000000, 0.00620078535925, 0.00492156660115, 0.00390625000000,
	0.00310039267963, 0.00246078330058, 0.00195312500000, 0.00155019633981,
	0.00123039165029, 0.00097656250000, 0.00077509816991, 0.00061519582514,
	0.00048828125000, 0.00038754908495, 0.00030759791257, 0.00024414062500,
	0.00019377454248, 0.00015379895629, 0.00012207031250, 0.00009688727124,
	0.00007689947814, 0.00006103515625, 0.00004844363562, 0.00003844973907,
	0.00003051757812, 0.00002422181781, 0.00001922486954, 0.00001525878906,
	0.00001211090890, 0.00000961243477, 0.00000762939453, 0.00000605545445,
	0.00000480621738, 0.00000381469727, 0.00000302772723, 0.00000240310869,
	0.00000190734863, 0.00000151386361, 0.00000120155435, 0.00000000000000
};

const double mpeg_audio::synthesis_filter[512] = {
	+0.000000000, -0.000015259, -0.000015259, -0.000015259, -0.000015259, -0.000015259, -0.000015259, -0.000030518,
	-0.000030518, -0.000030518, -0.000030518, -0.000045776, -0.000045776, -0.000061035, -0.000061035, -0.000076294,
	-0.000076294, -0.000091553, -0.000106812, -0.000106812, -0.000122070, -0.000137329, -0.000152588, -0.000167847,
	-0.000198364, -0.000213623, -0.000244141, -0.000259399, -0.000289917, -0.000320435, -0.000366211, -0.000396729,
	-0.000442505, -0.000473022, -0.000534058, -0.000579834, -0.000625610, -0.000686646, -0.000747681, -0.000808716,
	-0.000885010, -0.000961304, -0.001037598, -0.001113892, -0.001205444, -0.001296997, -0.001388550, -0.001480103,
	-0.001586914, -0.001693726, -0.001785278, -0.001907349, -0.002014160, -0.002120972, -0.002243042, -0.002349854,
	-0.002456665, -0.002578735, -0.002685547, -0.002792358, -0.002899170, -0.002990723, -0.003082275, -0.003173828,
	+0.003250122, +0.003326416, +0.003387451, +0.003433228, +0.003463745, +0.003479004, +0.003479004, +0.003463745,
	+0.003417969, +0.003372192, +0.003280640, +0.003173828, +0.003051758, +0.002883911, +0.002700806, +0.002487183,
	+0.002227783, +0.001937866, +0.001617432, +0.001266479, +0.000869751, +0.000442505, -0.000030518, -0.000549316,
	-0.001098633, -0.001693726, -0.002334595, -0.003005981, -0.003723145, -0.004486084, -0.005294800, -0.006118774,
	-0.007003784, -0.007919312, -0.008865356, -0.009841919, -0.010848999, -0.011886597, -0.012939453, -0.014022827,
	-0.015121460, -0.016235352, -0.017349243, -0.018463135, -0.019577026, -0.020690918, -0.021789550, -0.022857666,
	-0.023910522, -0.024932861, -0.025909424, -0.026840210, -0.027725220, -0.028533936, -0.029281616, -0.029937744,
	-0.030532837, -0.031005860, -0.031387330, -0.031661987, -0.031814575, -0.031845093, -0.031738280, -0.031478880,
	+0.031082153, +0.030517578, +0.029785156, +0.028884888, +0.027801514, +0.026535034, +0.025085450, +0.023422241,
	+0.021575928, +0.019531250, +0.017257690, +0.014801025, +0.012115479, +0.009231567, +0.006134033, +0.002822876,
	-0.000686646, -0.004394531, -0.008316040, -0.012420654, -0.016708374, -0.021179200, -0.025817871, -0.030609130,
	-0.035552980, -0.040634155, -0.045837402, -0.051132202, -0.056533813, -0.061996460, -0.067520140, -0.073059080,
	-0.078628540, -0.084182740, -0.089706420, -0.095169070, -0.100540160, -0.105819700, -0.110946655, -0.115921020,
	-0.120697020, -0.125259400, -0.129562380, -0.133590700, -0.137298580, -0.140670780, -0.143676760, -0.146255500,
	-0.148422240, -0.150115970, -0.151306150, -0.151962280, -0.152069090, -0.151596070, -0.150497440, -0.148773200,
	-0.146362300, -0.143264770, -0.139450070, -0.134887700, -0.129577640, -0.123474120, -0.116577150, -0.108856200,
	+0.100311280, +0.090927124, +0.080688480, +0.069595340, +0.057617188, +0.044784546, +0.031082153, +0.016510010,
	+0.001068115, -0.015228271, -0.032379150, -0.050354004, -0.069168090, -0.088775635, -0.109161380, -0.130310060,
	-0.152206420, -0.174789430, -0.198059080, -0.221984860, -0.246505740, -0.271591200, -0.297210700, -0.323318480,
	-0.349868770, -0.376800540, -0.404083250, -0.431655880, -0.459472660, -0.487472530, -0.515609740, -0.543823240,
	-0.572036740, -0.600219700, -0.628295900, -0.656219500, -0.683914200, -0.711318970, -0.738372800, -0.765029900,
	-0.791214000, -0.816864000, -0.841949460, -0.866363500, -0.890090940, -0.913055400, -0.935195900, -0.956481930,
	-0.976852400, -0.996246340, -1.014617900, -1.031936600, -1.048156700, -1.063217200, -1.077117900, -1.089782700,
	-1.101211500, -1.111373900, -1.120224000, -1.127746600, -1.133926400, -1.138763400, -1.142211900, -1.144287100,
	+1.144989000, +1.144287100, +1.142211900, +1.138763400, +1.133926400, +1.127746600, +1.120224000, +1.111373900,
	+1.101211500, +1.089782700, +1.077117900, +1.063217200, +1.048156700, +1.031936600, +1.014617900, +0.996246340,
	+0.976852400, +0.956481930, +0.935195900, +0.913055400, +0.890090940, +0.866363500, +0.841949460, +0.816864000,
	+0.791214000, +0.765029900, +0.738372800, +0.711318970, +0.683914200, +0.656219500, +0.628295900, +0.600219700,
	+0.572036740, +0.543823240, +0.515609740, +0.487472530, +0.459472660, +0.431655880, +0.404083250, +0.376800540,
	+0.349868770, +0.323318480, +0.297210700, +0.271591200, +0.246505740, +0.221984860, +0.198059080, +0.174789430,
	+0.152206420, +0.130310060, +0.109161380, +0.088775635, +0.069168090, +0.050354004, +0.032379150, +0.015228271,
	-0.001068115, -0.016510010, -0.031082153, -0.044784546, -0.057617188, -0.069595340, -0.080688480, -0.090927124,
	+0.100311280, +0.108856200, +0.116577150, +0.123474120, +0.129577640, +0.134887700, +0.139450070, +0.143264770,
	+0.146362300, +0.148773200, +0.150497440, +0.151596070, +0.152069090, +0.151962280, +0.151306150, +0.150115970,
	+0.148422240, +0.146255500, +0.143676760, +0.140670780, +0.137298580, +0.133590700, +0.129562380, +0.125259400,
	+0.120697020, +0.115921020, +0.110946655, +0.105819700, +0.100540160, +0.095169070, +0.089706420, +0.084182740,
	+0.078628540, +0.073059080, +0.067520140, +0.061996460, +0.056533813, +0.051132202, +0.045837402, +0.040634155,
	+0.035552980, +0.030609130, +0.025817871, +0.021179200, +0.016708374, +0.012420654, +0.008316040, +0.004394531,
	+0.000686646, -0.002822876, -0.006134033, -0.009231567, -0.012115479, -0.014801025, -0.017257690, -0.019531250,
	-0.021575928, -0.023422241, -0.025085450, -0.026535034, -0.027801514, -0.028884888, -0.029785156, -0.030517578,
	+0.031082153, +0.031478880, +0.031738280, +0.031845093, +0.031814575, +0.031661987, +0.031387330, +0.031005860,
	+0.030532837, +0.029937744, +0.029281616, +0.028533936, +0.027725220, +0.026840210, +0.025909424, +0.024932861,
	+0.023910522, +0.022857666, +0.021789550, +0.020690918, +0.019577026, +0.018463135, +0.017349243, +0.016235352,
	+0.015121460, +0.014022827, +0.012939453, +0.011886597, +0.010848999, +0.009841919, +0.008865356, +0.007919312,
	+0.007003784, +0.006118774, +0.005294800, +0.004486084, +0.003723145, +0.003005981, +0.002334595, +0.001693726,
	+0.001098633, +0.000549316, +0.000030518, -0.000442505, -0.000869751, -0.001266479, -0.001617432, -0.001937866,
	-0.002227783, -0.002487183, -0.002700806, -0.002883911, -0.003051758, -0.003173828, -0.003280640, -0.003372192,
	-0.003417969, -0.003463745, -0.003479004, -0.003479004, -0.003463745, -0.003433228, -0.003387451, -0.003326416,
	+0.003250122, +0.003173828, +0.003082275, +0.002990723, +0.002899170, +0.002792358, +0.002685547, +0.002578735,
	+0.002456665, +0.002349854, +0.002243042, +0.002120972, +0.002014160, +0.001907349, +0.001785278, +0.001693726,
	+0.001586914, +0.001480103, +0.001388550, +0.001296997, +0.001205444, +0.001113892, +0.001037598, +0.000961304,
	+0.000885010, +0.000808716, +0.000747681, +0.000686646, +0.000625610, +0.000579834, +0.000534058, +0.000473022,
	+0.000442505, +0.000396729, +0.000366211, +0.000320435, +0.000289917, +0.000259399, +0.000244141, +0.000213623,
	+0.000198364, +0.000167847, +0.000152588, +0.000137329, +0.000122070, +0.000106812, +0.000106812, +0.000091553,
	+0.000076294, +0.000076294, +0.000061035, +0.000061035, +0.000045776, +0.000045776, +0.000030518, +0.000030518,
	+0.000030518, +0.000030518, +0.000015259, +0.000015259, +0.000015259, +0.000015259, +0.000015259, +0.000015259,
};

int mpeg_audio::do_gb_msb(const unsigned char *data, int &pos, int count)
{
	int v = 0;
	for(int i=0; i != count; i++) {
		v <<= 1;
		if(data[pos >> 3] & (0x80 >> (pos & 7)))
			v |= 1;
		pos++;
	}
	return v;
}

int mpeg_audio::do_gb_lsb(const unsigned char *data, int &pos, int count)
{
	int v = 0;
	for(int i=0; i != count; i++) {
		v <<= 1;
		if(data[pos >> 3] & (0x01 << (pos & 7)))
			v |= 1;
		pos++;
	}
	return v;
}

int mpeg_audio::get_band_param(int band)
{
	int bit_count = band_parameter_index_bits_count[param_index][band];
	int index = gb(bit_count);
	return band_parameter_indexed_values[param_index][band][index];
}

void mpeg_audio::read_band_params()
{
	int band = 0;

	while(band < joint_bands) {
		for(int chan=0; chan < channel_count; chan++)
			band_param[chan][band] = get_band_param(band);
		band++;
	}

	while(band < total_bands) {
		int val = get_band_param(band);
		band_param[0][band] = val;
		band_param[1][band] = val;
		band++;
	}

	while(band < 32) {
		band_param[0][band] = 0;
		band_param[1][band] = 0;
		band++;
	}
}

void mpeg_audio::read_scfci()
{
	memset(scfsi, 0, sizeof(scfsi));
	for(int band=0; band < total_bands; band++)
		for(int chan=0; chan < channel_count; chan++)
			if(band_param[chan][band])
				scfsi[chan][band] = gb(2);
}

void mpeg_audio::read_band_amplitude_params()
{
	memset(scf, 0, sizeof(scf));
	for(int band=0; band < total_bands; band++)
		for(int chan=0; chan<channel_count; chan++)
			if(band_param[chan][band]) {
				switch(scfsi[chan][band]) {
				case 0:
					scf[chan][0][band] = gb(6);
					scf[chan][1][band] = gb(6);
					scf[chan][2][band] = gb(6);
					break;

				case 1: {
					int val = gb(6);
					scf[chan][0][band] = val;
					scf[chan][1][band] = val;
					scf[chan][2][band] = gb(6);
					break;
				}

				case 2: {
					int val = gb(6);
					scf[chan][0][band] = val;
					scf[chan][1][band] = val;
					scf[chan][2][band] = val;
					break;
				}

				case 3: {
					scf[chan][0][band] = gb(6);
					int val = gb(6);
					scf[chan][1][band] = val;
					scf[chan][2][band] = val;
					break;
				}
				}
			}
}

void mpeg_audio::build_amplitudes()
{
	memset(amp_values, 0, sizeof(amp_values));

	for(int band=0; band < total_bands; band++)
		for(int chan=0; chan<channel_count; chan++)
			if(band_param[chan][band])
				for(int step=0; step<3; step++)
					amp_values[chan][step][band] = scalefactors[scf[chan][step][band]];
}

void mpeg_audio::read_band_value_triplet(int chan, int band)
{
	double buffer[3];

	int band_idx = band_param[chan][band];
	switch(band_idx) {
	case 0:
		bdata[chan][0][band] = 0;
		bdata[chan][1][band] = 0;
		bdata[chan][2][band] = 0;
		return;

	case 1:
	case 2:
	case 4: {
		int modulo = band_infos[band_idx].modulo;
		int val  = gb(band_infos[band_idx].cube_bits);
		buffer[0] = val % modulo;
		val = val / modulo;
		buffer[1] = val % modulo;
		val = val / modulo;
		buffer[2] = val % modulo;
		break;
	}

	default: {
		int bits = band_infos[band_idx].bits;
		buffer[0] = gb(bits);
		buffer[1] = gb(bits);
		buffer[2] = gb(bits);
		break;
	}
	}

	double scale = 1 << (band_infos[band_idx].bits - 1);

	bdata[chan][0][band] = ((buffer[0] - scale) / scale + band_infos[band_idx].offset) * band_infos[band_idx].scale;
	bdata[chan][1][band] = ((buffer[1] - scale) / scale + band_infos[band_idx].offset) * band_infos[band_idx].scale;
	bdata[chan][2][band] = ((buffer[2] - scale) / scale + band_infos[band_idx].offset) * band_infos[band_idx].scale;
}

void mpeg_audio::build_next_segments(int step)
{
	int band = 0;
	while(band < joint_bands) {
		for(int chan=0; chan<channel_count; chan++) {
			read_band_value_triplet(chan, band);
			double amp = amp_values[chan][step][band];
			bdata[chan][0][band] *= amp;
			bdata[chan][1][band] *= amp;
			bdata[chan][2][band] *= amp;
		}
		band++;
	}

	while(band < joint_bands) {
		read_band_value_triplet(0, band);
		bdata[1][0][band] = bdata[0][0][band];
		bdata[1][1][band] = bdata[0][1][band];
		bdata[1][2][band] = bdata[0][2][band];

		for(int chan=0; chan<channel_count; chan++) {
			double amp = amp_values[chan][step][band];
			bdata[chan][0][band] *= amp;
			bdata[chan][1][band] *= amp;
			bdata[chan][2][band] *= amp;
		}
		band++;
	}

	while(band < 32) {
		bdata[0][0][band] = 0;
		bdata[0][1][band] = 0;
		bdata[0][2][band] = 0;
		bdata[1][0][band] = 0;
		bdata[1][1][band] = 0;
		bdata[1][2][band] = 0;
		band++;
	}
}

void mpeg_audio::retrieve_subbuffer(int step)
{
	for(int chan=0; chan<channel_count; chan++)
		memcpy(subbuffer[chan], bdata[chan][step], 32*sizeof(subbuffer[0][0]));
}

void mpeg_audio::idct32(const double *input, double *output)
{
	// Simplest idct32 ever, non-fast at all
	for(int i=0; i<32; i++) {
		double s = 0;
		for(int j=0; j<32; j++)
			s += input[j] * cos(i*(2*j+1)*M_PI/64);
		output[i] = s;
	}
}

void mpeg_audio::resynthesis(const double *input, double *output)
{
	memset(output, 0, 32*sizeof(output[0]));
	for(int j=0; j<64*8; j+=64) {
		for(int i=0; i<16; i++)
			output[i] += input[   i+j]*synthesis_filter[i+j] - input[32-i+j]*synthesis_filter[32+i+j];
		output[16] -= input[16+j]*synthesis_filter[32+16+j];
		for(int i=17; i<32; i++)
			output[i] -= input[32-i+j]*synthesis_filter[i+j] + input[   i+j]*synthesis_filter[32+i+j];
	}
}

void mpeg_audio::scale_and_clamp(const double *input, short *output, int step)
{
	for(int i=0; i<32; i++) {
		double val = input[i]*32768 + 0.5;
		short cval;
		if(val <= -32768)
			cval = -32768;
		else if(val >= 32767)
			cval = 32767;
		else
			cval = int(val);
		*output = cval;
		output += step;
	}
}
