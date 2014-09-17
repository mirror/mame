/***********************************************************************************************************

   Generic ROM / RAM Socket and Cartslot device
 
   This device offers basic RAM / ROM allocation and access
 
 ***********************************************************************************************************/


#include "emu.h"
#include "slot.h"

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type GENERIC_SOCKET = &device_creator<generic_slot_device>;


//-------------------------------------------------
//  device_generic_cart_interface - constructor
//-------------------------------------------------

device_generic_cart_interface::device_generic_cart_interface(const machine_config &mconfig, device_t &device)
	: device_slot_card_interface(mconfig, device),
		m_rom(NULL),
		m_rom_size(0)
{
}


//-------------------------------------------------
//  ~device_generic_cart_interface - destructor
//-------------------------------------------------

device_generic_cart_interface::~device_generic_cart_interface()
{
}

//-------------------------------------------------
//  rom_alloc - alloc the space for the cart
//-------------------------------------------------

void device_generic_cart_interface::rom_alloc(size_t size, int width, const char *tag)
{
	if (m_rom == NULL)
	{
		astring tempstring(tag);
		tempstring.cat(":cart:rom");
		m_rom = device().machine().memory().region_alloc(tempstring, size, width, ENDIANNESS_LITTLE)->base();
		m_rom_size = size;
	}
}


//-------------------------------------------------
//  ram_alloc - alloc the space for the ram
//-------------------------------------------------

void device_generic_cart_interface::ram_alloc(UINT32 size)
{
	if (m_ram == NULL)
	{
		m_ram.resize(size);
		device().save_item(NAME(m_ram));
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  generic_slot_device - constructor
//-------------------------------------------------
generic_slot_device::generic_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
						device_t(mconfig, GENERIC_SOCKET, "Generic ROM Socket / RAM Socket / Cartridge Slot", tag, owner, clock, "generic_socket", __FILE__),
						device_image_interface(mconfig, *this),
						device_slot_interface(mconfig, *this),
						m_interface(NULL),
						m_default_card("rom"),
						m_extensions("bin"),
						m_must_be_loaded(FALSE),
						m_width(GENERIC_ROM8_WIDTH),
						m_empty(TRUE)
{
}


//-------------------------------------------------
//  generic_slot_device - destructor
//-------------------------------------------------

generic_slot_device::~generic_slot_device()
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void generic_slot_device::device_start()
{
	m_cart = dynamic_cast<device_generic_cart_interface *>(get_card_device());
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void generic_slot_device::device_config_complete()
{
	// set brief and instance name
	update_names();
}


/*-------------------------------------------------
 call load
 -------------------------------------------------*/

bool generic_slot_device::call_load()
{
	if (m_cart)
	{		
		if (!m_device_image_load.isnull())
		{
			int result = m_device_image_load(*this);
			m_empty = (result == IMAGE_INIT_PASS) ? FALSE : TRUE;
			return result;
		}
		else
		{
			UINT32 len = (software_entry() == NULL) ? length() : get_software_region_length("rom");
			UINT8 *ROM;
			
			rom_alloc(len, m_width);			
			ROM = get_rom_base();
			
			if (software_entry() == NULL)
				fread(ROM, len);
			else
				memcpy(ROM, get_software_region("rom"), len);
			
			m_empty = FALSE;

			return IMAGE_INIT_PASS;
		}
	}

	return IMAGE_INIT_PASS;
}


/*-------------------------------------------------
 call_unload
 -------------------------------------------------*/

void generic_slot_device::call_unload()
{
	if (!m_device_image_unload.isnull())
		return m_device_image_unload(*this);
}


/*-------------------------------------------------
 call softlist load
 -------------------------------------------------*/

bool generic_slot_device::call_softlist_load(software_list_device &swlist, const char *swname, const rom_entry *start_entry)
{
	load_software_part_region(*this, swlist, swname, start_entry);
	return TRUE;
}



/*-------------------------------------------------
 get default card software
 -------------------------------------------------*/

void generic_slot_device::get_default_card_software(astring &result)
{
	software_get_default_slot(result, m_default_card);
}

/*-------------------------------------------------
 read_rom
 -------------------------------------------------*/

READ8_MEMBER(generic_slot_device::read_rom)
{
	if (m_cart)
		return m_cart->read_rom(space, offset);
	else
		return 0xff;
}

/*-------------------------------------------------
 read16_rom
 -------------------------------------------------*/

READ16_MEMBER(generic_slot_device::read16_rom)
{
	if (m_cart)
		return m_cart->read16_rom(space, offset, mem_mask);
	else
		return 0xffff;
}

/*-------------------------------------------------
 read_ram
 -------------------------------------------------*/

READ8_MEMBER(generic_slot_device::read_ram)
{
	if (m_cart)
		return m_cart->read_ram(space, offset);
	else
		return 0xff;
}

/*-------------------------------------------------
 write
 -------------------------------------------------*/

WRITE8_MEMBER(generic_slot_device::write_ram)
{
	if (m_cart)
		m_cart->write_ram(space, offset, data);
}

