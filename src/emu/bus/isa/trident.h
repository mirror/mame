/*
 * trident.h
 *
 */

#ifndef TRIDENT_H_
#define TRIDENT_H_

#include "video/pc_vga.h"

// ======================> trident_vga_device

class trident_vga_device :  public svga_device
{
public:
	// construction/destruction
	trident_vga_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual READ8_MEMBER(port_03c0_r);
	virtual WRITE8_MEMBER(port_03c0_w);
	virtual READ8_MEMBER(port_03d0_r);
	virtual WRITE8_MEMBER(port_03d0_w);
	DECLARE_READ8_MEMBER(port_83c6_r);
	DECLARE_WRITE8_MEMBER(port_83c6_w);
	DECLARE_READ8_MEMBER(vram_r) { if (svga.rgb8_en || svga.rgb15_en || svga.rgb16_en || svga.rgb32_en) return vga.memory[offset % vga.svga_intf.vram_size]; else return 0xff; }
	DECLARE_WRITE8_MEMBER(vram_w) { if (svga.rgb8_en || svga.rgb15_en || svga.rgb16_en || svga.rgb32_en) vga.memory[offset % vga.svga_intf.vram_size] = data; }
	virtual READ8_MEMBER(mem_r);
	virtual WRITE8_MEMBER(mem_w);
	virtual UINT16 offset();

	DECLARE_READ8_MEMBER(accel_r);
	DECLARE_WRITE8_MEMBER(accel_w);

protected:
	virtual void device_start();
	virtual void device_reset();
	struct
	{
		UINT8 sr0c;
		UINT8 sr0d_old;
		UINT8 sr0d_new;
		UINT8 sr0e_old;
		UINT8 sr0e_new;
		UINT8 sr0f;
		UINT8 gc0e;
		UINT8 gc0f;
		UINT8 gc2f;
		UINT8 cr1e;
		UINT8 cr1f;
		UINT8 cr21;
		UINT8 cr29;
		UINT8 cr39;
		UINT8 cr50;
		UINT8 dac;
		bool new_mode;
		bool port_3c3;
		UINT8 clock;
		UINT8 pixel_depth;
		UINT8 revision;
		bool dac_active;
		UINT8 dac_count;
		UINT32 linear_address;
		bool linear_active;
		bool mmio_active;

		// 2D acceleration
		UINT16 accel_opermode;
		UINT8 accel_command;
		UINT8 accel_fmix;
		UINT32 accel_drawflags;
		UINT32 accel_fgcolour;
		UINT32 accel_bgcolour;
		UINT16 accel_pattern_loc;
		INT16 accel_source_x;
		INT16 accel_source_y;
		INT16 accel_dest_x;
		INT16 accel_dest_y;
		INT16 accel_dim_x;
		INT16 accel_dim_y;
		UINT32 accel_style;
		UINT32 accel_ckey;
		INT16 accel_source_x_clip;
		INT16 accel_source_y_clip;
		INT16 accel_dest_x_clip;
		INT16 accel_dest_y_clip;
		UINT32 accel_fg_pattern_colour;
		UINT32 accel_bg_pattern_colour;
		UINT8 accel_pattern[0x80];
		bool accel_busy;
		bool accel_memwrite_active;  // true when writing to VRAM will push data to an ongoing command (SRCMONO/PATMONO)
		INT16 accel_mem_x;
		INT16 accel_mem_y;
		UINT32 accel_transfer;
	} tri;
private:
	UINT8 trident_seq_reg_read(UINT8 index);
	void trident_seq_reg_write(UINT8 index, UINT8 data);
	void trident_define_video_mode();
	UINT8 trident_crtc_reg_read(UINT8 index);
	void trident_crtc_reg_write(UINT8 index, UINT8 data);
	UINT8 trident_gc_reg_read(UINT8 index);
	void trident_gc_reg_write(UINT8 index, UINT8 data);

	void accel_command();
	void accel_bitblt();
	void accel_line();
	void accel_data_write(UINT32 data);
	UINT8 READPIXEL8(INT16 x, INT16 y);
	UINT16 READPIXEL15(INT16 x, INT16 y);
	UINT16 READPIXEL16(INT16 x, INT16 y);
	UINT32 READPIXEL32(INT16 x, INT16 y);
	void WRITEPIXEL8(INT16 x, INT16 y, UINT8 data);
	void WRITEPIXEL15(INT16 x, INT16 y, UINT16 data);
	void WRITEPIXEL16(INT16 x, INT16 y, UINT16 data);
	void WRITEPIXEL32(INT16 x, INT16 y, UINT32 data);
	UINT32 READPIXEL(INT16 x,INT16 y);
	void WRITEPIXEL(INT16 x,INT16 y, UINT32 data);
	UINT32 handle_rop(UINT32 src, UINT32 dst);
};


// device type definition
extern const device_type TRIDENT_VGA;

#endif /* TRIDENT_H_ */
