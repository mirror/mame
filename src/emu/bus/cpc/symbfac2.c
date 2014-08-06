/*
 * symbfac2.c
 *   SYMBiFACE II expansion device
 *    - IDE
 *    - RTC (Dallas DS1287A)
 *    - PS/2 compatible mouse connector
 *    - 512kB RAM expansion
 *    - 512kB rewritable ROM
 *
 *  Created on: 2/08/2014
 *
 *  TODO:
 *    - expansion RAM
 *    - rewritable ROM
 *    - mouse controls still need some work
 */

#include "symbfac2.h"


const device_type CPC_SYMBIFACE2 = &device_creator<cpc_symbiface2_device>;

//**************************************************************************
//  DEVICE CONFIG INTERFACE
//**************************************************************************

// device machine config
static MACHINE_CONFIG_FRAGMENT( cpc_symbiface2 )
	MCFG_ATA_INTERFACE_ADD("ide",ata_devices,"hdd",NULL,false)
	MCFG_DS12885_ADD("rtc")
	// no pass-through
MACHINE_CONFIG_END

static INPUT_PORTS_START(cpc_symbiface2)
	PORT_START("sf2_mouse_x")
	PORT_BIT(0x3f , 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(50) PORT_KEYDELTA(0) PORT_REVERSE PORT_PLAYER(1) PORT_CODE(MOUSECODE_X) PORT_CHANGED_MEMBER(DEVICE_SELF,cpc_symbiface2_device,mouse_change_x,NULL)

	PORT_START("sf2_mouse_y")
	PORT_BIT(0x3f , 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(50) PORT_KEYDELTA(0) PORT_PLAYER(1) PORT_CODE(MOUSECODE_Y) PORT_CHANGED_MEMBER(DEVICE_SELF,cpc_symbiface2_device,mouse_change_x,NULL)

	PORT_START("sf2_mouse_buttons")
	PORT_BIT(0x00000001, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("PS/2 Mouse left button") PORT_CODE(MOUSECODE_BUTTON1) PORT_CHANGED_MEMBER(DEVICE_SELF,cpc_symbiface2_device,mouse_change_x,NULL)
	PORT_BIT(0x00000002, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("PS/2 Mouse right button") PORT_CODE(MOUSECODE_BUTTON3) PORT_CHANGED_MEMBER(DEVICE_SELF,cpc_symbiface2_device,mouse_change_x,NULL)
	PORT_BIT(0x00000004, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("PS/2 Mouse middle button") PORT_CODE(MOUSECODE_BUTTON2) PORT_CHANGED_MEMBER(DEVICE_SELF,cpc_symbiface2_device,mouse_change_x,NULL)
	PORT_BIT(0x00000008, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("PS/2 Mouse back button") PORT_CODE(MOUSECODE_BUTTON4) PORT_CHANGED_MEMBER(DEVICE_SELF,cpc_symbiface2_device,mouse_change_x,NULL)
	PORT_BIT(0x00000010, IP_ACTIVE_HIGH, IPT_OTHER) PORT_NAME("PS/2 Mouse forward button") PORT_CODE(MOUSECODE_BUTTON5) PORT_CHANGED_MEMBER(DEVICE_SELF,cpc_symbiface2_device,mouse_change_x,NULL)

	// TODO: mouse scroll wheel support
//	PORT_START("sf2_mouse_scroll")
//	PORT_BIT(0x1f , 0, IPT_TRACKBALL_Y)
//	PORT_SENSITIVITY(100)
//	PORT_KEYDELTA(10)
//	PORT_PLAYER(1)
INPUT_PORTS_END


machine_config_constructor cpc_symbiface2_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( cpc_symbiface2 );
}

ioport_constructor cpc_symbiface2_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( cpc_symbiface2 );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

cpc_symbiface2_device::cpc_symbiface2_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, CPC_SYMBIFACE2, "SYMBiFACE II", tag, owner, clock, "cpc_symf2", __FILE__),
	device_cpc_expansion_card_interface(mconfig, *this),
	m_ide(*this,"ide"),
	m_rtc(*this,"rtc"),
	m_mouse_x(*this,"sf2_mouse_x"),
	m_mouse_y(*this,"sf2_mouse_y"),
	m_mouse_buttons(*this,"sf2_mouse_buttons")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cpc_symbiface2_device::device_start()
{
	device_t* cpu = machine().device("maincpu");
	address_space& space = cpu->memory().space(AS_IO);

	space.install_readwrite_handler(0xfd00,0xfd07,0,0,read8_delegate(FUNC(cpc_symbiface2_device::ide_cs1_r),this),write8_delegate(FUNC(cpc_symbiface2_device::ide_cs1_w),this));
	space.install_readwrite_handler(0xfd08,0xfd0f,0,0,read8_delegate(FUNC(cpc_symbiface2_device::ide_cs0_r),this),write8_delegate(FUNC(cpc_symbiface2_device::ide_cs0_w),this));
	space.install_read_handler(0xfd10,0xfd10,0,0,read8_delegate(FUNC(cpc_symbiface2_device::mouse_r),this));
	space.install_readwrite_handler(0xfd14,0xfd15,0,0,read8_delegate(FUNC(cpc_symbiface2_device::rtc_r),this),write8_delegate(FUNC(cpc_symbiface2_device::rtc_w),this));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void cpc_symbiface2_device::device_reset()
{
	m_iohigh = false;
	m_mouse_state = PS2_MOUSE_IDLE;
	m_input_x = m_mouse_x->read() & 0x3f;
	m_input_y = m_mouse_y->read() & 0x3f;
}

// IDE controller (custom)
// #FD00 - CS1
// #FD08 - CS0
READ8_MEMBER(cpc_symbiface2_device::ide_cs0_r)
{
	// data is returned in words, so it must be buffered
	if(offset == 0x00) // data register
	{
		if(m_iohigh)
		{
			m_iohigh = false;
			return m_ide_data >> 8;
		}
		else
		{
			m_iohigh = true;
			m_ide_data = m_ide->read_cs0(space,offset);
			return m_ide_data & 0xff;
		}
	}
	else
		return m_ide->read_cs0(space,offset);
}

WRITE8_MEMBER(cpc_symbiface2_device::ide_cs0_w)
{
	m_ide->write_cs0(space,offset,data);
}

READ8_MEMBER(cpc_symbiface2_device::ide_cs1_r)
{
	return m_ide->read_cs1(space,offset);
}

WRITE8_MEMBER(cpc_symbiface2_device::ide_cs1_w)
{
	m_ide->write_cs1(space,offset,data);
}

// RTC (Dallas DS1287A)
// #FD15 (write only) register select
// #FD14 (read/write) read from or write into selected register
READ8_MEMBER(cpc_symbiface2_device::rtc_r)
{
	switch(offset & 0x01)
	{
	case 0x00:
		return m_rtc->read(space,1);
	case 0x01:
		return m_rtc->read(space,0);
	}
	return 0;
}

WRITE8_MEMBER(cpc_symbiface2_device::rtc_w)
{
	switch(offset & 0x01)
	{
	case 0x00:
		m_rtc->write(space,1,data);
		break;
	case 0x01:
		m_rtc->write(space,0,data);
		break;
	}
}

// PS/2 Mouse connector
// #FD10 (read only) read mouse status
/*
	Status byte
	Bit 76543210
	Use mmDDDDDD

	m: Mode
	D: Use-Data

	If read and...

	m = 00 -> no more data available, you can stop reading the status for a while
	m = 01 -> D = X offset (signed); you will receive positive values, if the user
									 is moving the mouse to the right
	m = 10 -> D = Y offset (signed); you will receive positive values, if the user
									 is moving the mouse upwards
	m = 11 -> D[bit5] = 0 -> D[bit0]   = left button
							 D[bit1]   = right button
							 D[bit2]   = middle button
							 D[bit3]   = forward button
							 D[bit4]   = backward button
			  D[bit5] = 1 -> D[bit0-4] = scroll wheel offset (signed)
 */
READ8_MEMBER(cpc_symbiface2_device::mouse_r)
{
	UINT8 ret = 0;
	int input;
	int input_diff;

	switch(m_mouse_state)
	{
	case PS2_MOUSE_IDLE:
		m_mouse_state = PS2_MOUSE_IDLE;
		ret = 0;
		break;
	case PS2_MOUSE_X:
		input = m_mouse_x->read() & 0x3f;
		input_diff = m_input_x - input;
		ret = 0x40 | (input_diff & 0x3f);
		m_input_x = input;
		m_mouse_state = PS2_MOUSE_Y;
		break;
	case PS2_MOUSE_Y:
		input = m_mouse_y->read() & 0x3f;
		input_diff = m_input_y - input;
		ret = 0x80 | (input_diff & 0x3f);
		m_input_y = input;
		m_mouse_state = PS2_MOUSE_BUTTONS;
		break;
	case PS2_MOUSE_BUTTONS:
		ret = 0xc0 | (m_mouse_buttons->read() & 0x1f);
		m_mouse_state = PS2_MOUSE_IDLE;
		break;
	case PS2_MOUSE_SCROLL:
		m_mouse_state = PS2_MOUSE_IDLE;
		break;  // TODO
	}
	//popmessage("Mouse: X: %02x  Y: %02x\n",m_input_x,m_input_y);
	return ret;
}

INPUT_CHANGED_MEMBER(cpc_symbiface2_device::mouse_change_x)
{
	m_mouse_state = PS2_MOUSE_X;
}

INPUT_CHANGED_MEMBER(cpc_symbiface2_device::mouse_change_y)
{
	m_mouse_state = PS2_MOUSE_Y;
}

INPUT_CHANGED_MEMBER(cpc_symbiface2_device::mouse_change_buttons)
{
	m_mouse_state = PS2_MOUSE_BUTTONS;
}

