// license:BSD-3-Clause
// copyright-holders:R. Belmont
/**********************************************************************

    Commodore PET userport "CB2 sound" audio device emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __PETUSER_CB2__
#define __PETUSER_CB2__

#include "emu.h"
#include "user.h"
#include "sound/dac.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class pet_userport_cb2_sound_device : public device_t,
	public device_pet_user_port_interface
{
public:
	// construction/destruction
	pet_userport_cb2_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual machine_config_constructor device_mconfig_additions() const;

	virtual DECLARE_WRITE_LINE_MEMBER( input_m );

	required_device<dac_device> m_dac;

protected:
	// device-level overrides
	virtual void device_start();
};


// device type definition
extern const device_type PET_USERPORT_CB2_SOUND_DEVICE;

#endif
