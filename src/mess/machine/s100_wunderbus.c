/**********************************************************************

    Morrow Designs Wunderbus I/O card emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "s100_wunderbus.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define I8259A_TAG		"13b"
#define INS8250_1_TAG	"6d"
#define INS8250_2_TAG	"5d"
#define INS8250_3_TAG	"4d"
#define UPD1990C_TAG	"12a"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type S100_WUNDERBUS = &device_creator<s100_wunderbus_device>;


//-------------------------------------------------
//  pic8259_interface pic_intf
//-------------------------------------------------

/*

    bit     description

    IR0     S-100 VI0
    IR1     S-100 VI1
    IR2     S-100 VI2
    IR3     Serial Device 1
    IR4     Serial Device 2
    IR5     Serial Device 3
    IR6     Daisy PWR line
    IR7     RT Clock TP line

*/

WRITE_LINE_MEMBER( s100_wunderbus_device::pic_int_w )
{
	m_s100->int_w(state);
}

static struct pic8259_interface pic_intf =
{
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, s100_wunderbus_device, pic_int_w),
	DEVCB_LINE_VCC,
	DEVCB_NULL
};


//-------------------------------------------------
//  ins8250_interface ace1_intf
//-------------------------------------------------
/*
static INS8250_TRANSMIT( ace1_transmit )
{
    s100_wunderbus_device *wunderbus = downcast<s100_wunderbus_device *>(device->owner());

    wunderbus->m_s100->terminal_transmit_w(data);
}
*/
//-------------------------------------------------
//  s100_terminal_w - terminal write
//-------------------------------------------------

void s100_wunderbus_device::s100_terminal_w(UINT8 data)
{
//  ins8250_receive(m_ace1, data);
}

static ins8250_interface ace1_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir3_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  ins8250_interface ace2_intf
//-------------------------------------------------

static ins8250_interface ace2_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir4_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  ins8250_interface ace3_intf
//-------------------------------------------------

static ins8250_interface ace3_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE(I8259A_TAG, pic8259_ir5_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  UPD1990A_INTERFACE( rtc_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( s100_wunderbus_device::rtc_tp_w )
{
	if (state)
	{
		m_rtc_tp = state;
		pic8259_ir7_w(m_pic, m_rtc_tp);
	}
}

static UPD1990A_INTERFACE( rtc_intf )
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, s100_wunderbus_device, rtc_tp_w)
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( s100_wunderbus )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( s100_wunderbus )
	MCFG_PIC8259_ADD(I8259A_TAG, pic_intf)
	MCFG_INS8250_ADD(INS8250_1_TAG, ace1_intf, XTAL_18_432MHz/10)
	MCFG_INS8250_ADD(INS8250_2_TAG, ace2_intf, XTAL_18_432MHz/10)
	MCFG_INS8250_ADD(INS8250_3_TAG, ace3_intf, XTAL_18_432MHz/10)
	MCFG_UPD1990A_ADD(UPD1990C_TAG, XTAL_32_768kHz, rtc_intf)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor s100_wunderbus_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( s100_wunderbus );
}


//-------------------------------------------------
//  INPUT_PORTS( wunderbus )
//-------------------------------------------------

static INPUT_PORTS_START( wunderbus )
	PORT_START("7C")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("7C:1")
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x3e, 0x12, "BASE Port Address" ) PORT_DIPLOCATION("7C:2,3,4,5,6")
	PORT_DIPSETTING(    0x00, "00H" )
	PORT_DIPSETTING(    0x02, "08H" )
	PORT_DIPSETTING(    0x04, "10H" )
	PORT_DIPSETTING(    0x06, "18H" )
	PORT_DIPSETTING(    0x08, "20H" )
	PORT_DIPSETTING(    0x0a, "28H" )
	PORT_DIPSETTING(    0x0c, "30H" )
	PORT_DIPSETTING(    0x0e, "38H" )
	PORT_DIPSETTING(    0x10, "40H" )
	PORT_DIPSETTING(    0x12, "48H" )
	PORT_DIPSETTING(    0x14, "50H" )
	PORT_DIPSETTING(    0x16, "58H" )
	PORT_DIPSETTING(    0x18, "60H" )
	PORT_DIPSETTING(    0x1a, "68H" )
	PORT_DIPSETTING(    0x1c, "70H" )
	PORT_DIPSETTING(    0x1e, "78H" )
	PORT_DIPSETTING(    0x20, "80H" )
	PORT_DIPSETTING(    0x22, "88H" )
	PORT_DIPSETTING(    0x24, "90H" )
	PORT_DIPSETTING(    0x26, "98H" )
	PORT_DIPSETTING(    0x28, "A0H" )
	PORT_DIPSETTING(    0x2a, "A8H" )
	PORT_DIPSETTING(    0x2c, "B0H" )
	PORT_DIPSETTING(    0x2e, "B8H" )
	PORT_DIPSETTING(    0x30, "C0H" )
	PORT_DIPSETTING(    0x32, "C8H" )
	PORT_DIPSETTING(    0x34, "D0H" )
	PORT_DIPSETTING(    0x36, "D8H" )
	PORT_DIPSETTING(    0x38, "E0H" )
	PORT_DIPSETTING(    0x3a, "E8H" )
	PORT_DIPSETTING(    0x3c, "F0H" )
	PORT_DIPSETTING(    0x3e, "F8H" )
	PORT_DIPNAME( 0x40, 0x40, "FLAG2 Polarity" ) PORT_DIPLOCATION("7C:7")
	PORT_DIPSETTING(    0x40, "Negative" )
	PORT_DIPSETTING(    0x00, "Positive" )
	PORT_DIPNAME( 0x80, 0x80, "FLAG1 Polarity" ) PORT_DIPLOCATION("7C:8")
	PORT_DIPSETTING(    0x80, "Negative" )
	PORT_DIPSETTING(    0x00, "Positive" )

	PORT_START("10A")
	PORT_DIPNAME( 0x07, 0x00, "Baud Rate" ) PORT_DIPLOCATION("10A:1,2,3")
	PORT_DIPSETTING(    0x00, "Automatic" )
	PORT_DIPSETTING(    0x01, "19200" )
	PORT_DIPSETTING(    0x02, "9600" )
	PORT_DIPSETTING(    0x03, "4800" )
	PORT_DIPSETTING(    0x04, "2400" )
	PORT_DIPSETTING(    0x05, "1200" )
	PORT_DIPSETTING(    0x06, "300" )
	PORT_DIPSETTING(    0x07, "110" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("10A:4")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) ) PORT_DIPLOCATION("10A:5")
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unused ) ) PORT_DIPLOCATION("10A:6")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) ) PORT_DIPLOCATION("10A:7")
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) ) PORT_DIPLOCATION("10A:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor s100_wunderbus_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( wunderbus );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  s100_wunderbus_device - constructor
//-------------------------------------------------

s100_wunderbus_device::s100_wunderbus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, S100_WUNDERBUS, "Wunderbus I/O", tag, owner, clock),
	device_s100_card_interface(mconfig, *this),
	m_pic(*this, I8259A_TAG),
	m_ace1(*this, INS8250_1_TAG),
	m_ace2(*this, INS8250_2_TAG),
	m_ace3(*this, INS8250_3_TAG),
	m_rtc(*this, UPD1990C_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void s100_wunderbus_device::device_start()
{
	m_s100 = machine().device<s100_device>("s100");
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void s100_wunderbus_device::device_reset()
{
}

//-------------------------------------------------
//  s100_vi0_w - vectored interrupt 0
//-------------------------------------------------

void s100_wunderbus_device::s100_vi0_w(int state)
{
	pic8259_ir0_w(m_pic, state);
}


//-------------------------------------------------
//  s100_vi1_w - vectored interrupt 1
//-------------------------------------------------

void s100_wunderbus_device::s100_vi1_w(int state)
{
	pic8259_ir1_w(m_pic, state);
}


//-------------------------------------------------
//  s100_vi2_w - vectored interrupt 2
//-------------------------------------------------

void s100_wunderbus_device::s100_vi2_w(int state)
{
	pic8259_ir2_w(m_pic, state);
}


//-------------------------------------------------
//  s100_sinp_r - I/O read
//-------------------------------------------------

UINT8 s100_wunderbus_device::s100_sinp_r(address_space &space, offs_t offset)
{
	UINT8 address = (ioport("7C")->read() & 0x3e) << 2;
	if ((offset & 0xf8) != address) return 0;

	UINT8 data = 0;

	if ((offset & 0x07) < 7)
	{
		switch (m_group)
		{
		case 0:
			switch (offset & 0x07)
			{
			case 0: // DAISY 0 IN (STATUS)
				/*

                    bit     description

                    0       End of Ribbon
                    1       Paper Out
                    2       Cover Open
                    3       Paper Feed Ready
                    4       Carriage Ready
                    5       Print Wheel Ready
                    6       Check
                    7       Printer Ready

                */
				break;

			case 1: // Switch/Parallel port flags
				/*

                    bit     description

                    0       FLAG1
                    1       FLAG2
                    2       10A S6
                    3       10A S5
                    4       10A S4
                    5       10A S3
                    6       10A S2
                    7       10A S1

                */

				data = BITSWAP8(ioport("10A")->read(),0,1,2,3,4,5,6,7) & 0xfc;
				break;

			case 2: // R.T. Clock IN/RESET CLK. Int.
				/*

                    bit     description

                    0       1990 Data Out
                    1       1990 TP
                    2
                    3
                    4
                    5
                    6
                    7

                */

				data |= m_rtc->data_out_r();
				data |= m_rtc->tp_r() << 1;

				// reset clock interrupt
				m_rtc_tp = 0;
				pic8259_ir7_w(m_pic, m_rtc_tp);
				break;

			case 3: // Parallel data IN
				break;

			case 4: // 8259 0 register
			case 5: // 8259 1 register
				data = pic8259_r(m_pic, space, offset & 0x01);
				break;

			case 6: // not used
				break;
			}
			break;

		case 1:
			data = m_ace1->ins8250_r(machine().driver_data()->generic_space(), offset & 0x07);
			break;

		case 2:
			data = m_ace2->ins8250_r(machine().driver_data()->generic_space(), offset & 0x07);
			break;

		case 3:
			data = m_ace3->ins8250_r(machine().driver_data()->generic_space(), offset & 0x07);
			break;
		}
	}

	return data;
}


//-------------------------------------------------
//  s100_sout_w - I/O write
//-------------------------------------------------

void s100_wunderbus_device::s100_sout_w(address_space &space, offs_t offset, UINT8 data)
{
	UINT8 address = (ioport("7C")->read() & 0x3e) << 2;
	if ((offset & 0xf8) != address) return;

	if ((offset & 0x07) == 7)
	{
		m_group = data & 0x03;
	}
	else
	{
		switch (m_group)
		{
		case 0:
			switch (offset & 0x07)
			{
			case 0: // DAISY 0 OUT
				/*

                    bit     description

                    0       Data Bit 9
                    1       Data Bit 10
                    2       Data Bit 11
                    3       Data Bit 12
                    4       Paper Feed Strobe
                    5       Carriage Strobe
                    6       Print Wheel Strobe
                    7       Restore

                */
				break;

			case 1: // DAISY 1 OUT
				/*

                    bit     description

                    0       Data Bit 1
                    1       Data Bit 2
                    2       Data Bit 3
                    3       Data Bit 4
                    4       Data Bit 5
                    5       Data Bit 6
                    6       Data Bit 7
                    7       Data Bit 8

                */
				break;

			case 2: // R.T. Clock OUT
				/*

                    bit     description

                    0       1990 Data In
                    1       1990 Clk
                    2       1990 C0
                    3       1990 C1
                    4       1990 C2
                    5       1990 STB
                    6       Ribbon Lift
                    7       Select

                */

				m_rtc->data_in_w(BIT(data, 0));
				m_rtc->clk_w(BIT(data, 0));
				m_rtc->c0_w(BIT(data, 0));
				m_rtc->c1_w(BIT(data, 0));
				m_rtc->c2_w(BIT(data, 0));
				m_rtc->stb_w(BIT(data, 0));
				break;

			case 3: // Par. data OUT
				break;

			case 4: // 8259 0 register
			case 5: // 8259 1 register
				pic8259_w(m_pic, space, offset & 0x01, data);
				break;

			case 6: // Par. port cntrl.
				/*

                    bit     description

                    0       POE
                    1       _RST1
                    2       _RST2
                    3       _ATTN1
                    4       _ATTN2
                    5
                    6
                    7

                */
				break;
			}
			break;

		case 1:
			m_ace1->ins8250_w(machine().driver_data()->generic_space(), offset & 0x07, data);
			break;

		case 2:
			m_ace2->ins8250_w(machine().driver_data()->generic_space(), offset & 0x07, data);
			break;

		case 3:
			m_ace3->ins8250_w(machine().driver_data()->generic_space(), offset & 0x07, data);
			break;
		}
	}
}

