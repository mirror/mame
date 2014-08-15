// license:BSD-3-Clause
// copyright-holders:Sandro Ronco
/***************************************************************************

    K806 Mouse module

***************************************************************************/

#include "emu.h"
#include "k806.h"


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

ROM_START( dmv_k806 )
	ROM_REGION( 0x0400, "mcu", 0 )
	ROM_LOAD( "dmv_mouse_8741a.bin", 0x0000, 0x0400, CRC(2163737a) SHA1(b82c14dba6c25cb1f60cf623989ca8c0c1ee4cc3))
ROM_END

static ADDRESS_MAP_START( k806_io, AS_IO, 8, dmv_k806_device )
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_READWRITE(port1_r, port1_w)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_READ(portt1_r)
ADDRESS_MAP_END

static MACHINE_CONFIG_FRAGMENT( dmv_k806 )
	MCFG_CPU_ADD("mcu", I8741, XTAL_6MHz)
	MCFG_CPU_IO_MAP(k806_io)
MACHINE_CONFIG_END

static INPUT_PORTS_START( dmv_k806 )
	PORT_START("JUMPERS")
	PORT_DIPNAME( 0x7f, 0x24, "K806 IFSEL" )  PORT_DIPLOCATION("J:!1,J:!2,J:!3,J:!4,J:!5,J:!6,J:!7")
	PORT_DIPSETTING( 0x21, "0A" )
	PORT_DIPSETTING( 0x41, "0B" )
	PORT_DIPSETTING( 0x22, "1A" )
	PORT_DIPSETTING( 0x42, "1B" )
	PORT_DIPSETTING( 0x24, "2A" )   // default
	PORT_DIPSETTING( 0x44, "2B" )
	PORT_DIPSETTING( 0x28, "3A" )
	PORT_DIPSETTING( 0x48, "3B" )
	PORT_DIPSETTING( 0x30, "4A" )
	PORT_DIPSETTING( 0x50, "4B" )
	PORT_DIPNAME( 0x380, 0x00, "K806 Mouse" )  PORT_DIPLOCATION("J:!8,J:!9,J:!10")
	PORT_DIPSETTING( 0x000, "Hawley, Alps" )
	PORT_DIPSETTING( 0x380, "Depraz" )
INPUT_PORTS_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type DMV_K806 = &device_creator<dmv_k806_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  dmv_k806_device - constructor
//-------------------------------------------------

dmv_k806_device::dmv_k806_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
		: device_t(mconfig, DMV_K806, "K806 mouse", tag, owner, clock, "dmv_k806", __FILE__),
		device_dmvslot_interface( mconfig, *this ),
		m_mcu(*this, "mcu"),
		m_jumpers(*this, "JUMPERS")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void dmv_k806_device::device_start()
{
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void dmv_k806_device::device_reset()
{
}

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor dmv_k806_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( dmv_k806 );
}

//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor dmv_k806_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( dmv_k806 );
}

//-------------------------------------------------
//  device_rom_region
//-------------------------------------------------

const rom_entry *dmv_k806_device::device_rom_region() const
{
	return ROM_NAME( dmv_k806 );
}

void dmv_k806_device::io_read(address_space &space, int ifsel, offs_t offset, UINT8 &data)
{
	UINT8 jumpers = m_jumpers->read();
	if (BIT(jumpers, ifsel) && ((!BIT(offset, 3) && BIT(jumpers, 5)) || (BIT(offset, 3) && BIT(jumpers, 6))))
		data = m_mcu->upi41_master_r(space, offset & 1);
}

void dmv_k806_device::io_write(address_space &space, int ifsel, offs_t offset, UINT8 data)
{
	UINT8 jumpers = m_jumpers->read();
	if (BIT(jumpers, ifsel) && ((!BIT(offset, 3) && BIT(jumpers, 5)) || (BIT(offset, 3) && BIT(jumpers, 6))))
		m_mcu->upi41_master_w(space, offset & 1, data);
}

READ8_MEMBER( dmv_k806_device::port1_r )
{
	// TODO
	return 0xff;
}

READ8_MEMBER( dmv_k806_device::portt1_r )
{
	return BIT(m_jumpers->read(), 7) ? 0 : 1;
}

WRITE8_MEMBER( dmv_k806_device::port1_w )
{
	// TODO
};
