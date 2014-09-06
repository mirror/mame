/*
 * svga_trident.h
 *
 *  Created on: 6/09/2014
 */

#ifndef SVGA_TRIDENT_H_
#define SVGA_TRIDENT_H_

#include "emu.h"
#include "isa.h"
#include "video/pc_vga.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> isa8_vga_device

class isa8_svga_tgui9680_device :
		public device_t,
		public device_isa8_card_interface
{
public:
		// construction/destruction
		isa8_svga_tgui9680_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

		// optional information overrides
		virtual machine_config_constructor device_mconfig_additions() const;
		virtual const rom_entry *device_rom_region() const;

		DECLARE_READ8_MEMBER(input_port_0_r);
protected:
		// device-level overrides
		virtual void device_start();
		virtual void device_reset();
private:
		trident_vga_device *m_vga;
};


// device type definition
extern const device_type ISA8_SVGA_TGUI9680;


#endif /* SVGA_TRIDENT_H_ */
