/***********************************************************************************************************


    Game Boy Advance cart emulation
    (through slot devices)

 ***********************************************************************************************************/


#include "emu.h"
#include "gba_slot.h"

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type GBA_CART_SLOT = &device_creator<gba_cart_slot_device>;

//**************************************************************************
//    GBA cartridges Interface
//**************************************************************************

//-------------------------------------------------
//  device_gba_cart_interface - constructor
//-------------------------------------------------

device_gba_cart_interface::device_gba_cart_interface(const machine_config &mconfig, device_t &device)
	: device_slot_card_interface(mconfig, device),
		m_rom(NULL),
		m_rom_size(0)
{
}


//-------------------------------------------------
//  ~device_gba_cart_interface - destructor
//-------------------------------------------------

device_gba_cart_interface::~device_gba_cart_interface()
{
}

//-------------------------------------------------
//  nvram_alloc - alloc the space for the ram
//-------------------------------------------------

void device_gba_cart_interface::nvram_alloc(UINT32 size)
{
	m_nvram.resize(size/sizeof(UINT32));
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  gba_cart_slot_device - constructor
//-------------------------------------------------
gba_cart_slot_device::gba_cart_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
						device_t(mconfig, GBA_CART_SLOT, "Game Boy Advance Cartridge Slot", tag, owner, clock, "gba_cart_slot", __FILE__),
						device_image_interface(mconfig, *this),
						device_slot_interface(mconfig, *this),
						m_type(GBA_STD)
{
}


//-------------------------------------------------
//  gba_cart_slot_device - destructor
//-------------------------------------------------

gba_cart_slot_device::~gba_cart_slot_device()
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void gba_cart_slot_device::device_start()
{
	m_cart = dynamic_cast<device_gba_cart_interface *>(get_card_device());
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void gba_cart_slot_device::device_config_complete()
{
	// set brief and instance name
	update_names();
}


//-------------------------------------------------
//  GBA PCB
//-------------------------------------------------

struct gba_slot
{
	int                     pcb_id;
	const char              *slot_option;
};

// Here, we take the feature attribute from .xml (i.e. the PCB name) and we assign a unique ID to it
static const gba_slot slot_list[] =
{
	{ GBA_STD, "gba_rom" },
	{ GBA_SRAM, "gba_sram" },
	{ GBA_EEPROM, "gba_eeprom" },
	{ GBA_EEPROM4, "gba_eeprom_4k" },
	{ GBA_EEPROM64, "gba_eeprom_64k" },
	{ GBA_FLASH, "gba_flash" },
	{ GBA_FLASH512, "gba_flash_512" },
	{ GBA_FLASH1M, "gba_flash_1m" },
};

static int gba_get_pcb_id(const char *slot)
{
	for (int i = 0; i < ARRAY_LENGTH(slot_list); i++)
	{
		if (!core_stricmp(slot_list[i].slot_option, slot))
			return slot_list[i].pcb_id;
	}

	return 0;
}

static const char *gba_get_slot(int type)
{
	for (int i = 0; i < ARRAY_LENGTH(slot_list); i++)
	{
		if (slot_list[i].pcb_id == type)
			return slot_list[i].slot_option;
	}

	return "gba_rom";
}


/*-------------------------------------------------
 call load
 -------------------------------------------------*/

bool gba_cart_slot_device::call_load()
{
	if (m_cart)
	{
		UINT8 *ROM = (UINT8 *)m_cart->get_rom_base();
		UINT32 cart_size;

		if (software_entry() == NULL)
		{
			cart_size = length();
			if (cart_size > 0x2000000)
			{
				seterror(IMAGE_ERROR_UNSPECIFIED, "Attempted loading a cart larger than 32MB");
				return IMAGE_INIT_FAIL;
			}
			fread(ROM, cart_size);
			m_cart->set_rom_size(cart_size);    // we store the actual game size...

			m_type = get_cart_type(ROM, cart_size);
		}
		else
		{
			const char *pcb_name = get_feature("slot");

			cart_size = get_software_region_length("rom");
			if (cart_size > 0x2000000)
			{
				seterror(IMAGE_ERROR_UNSPECIFIED, "Attempted loading a cart larger than 32MB");
				return IMAGE_INIT_FAIL;
			}
			memcpy(ROM, get_software_region("rom"), cart_size);
			m_cart->set_rom_size(cart_size);    // we store the actual game size...

			if (pcb_name)
				m_type = gba_get_pcb_id(pcb_name);

			//printf("Type: %s\n", gba_get_slot(m_type));

			osd_printf_info("GBA: Detected (XML) %s\n", pcb_name ? pcb_name : "NONE");
		}

		if (m_type == GBA_SRAM)
			m_cart->nvram_alloc(0x10000);

		// mirror the ROM
		switch (cart_size)
		{
			case 2 * 1024 * 1024:
				memcpy(ROM + 0x200000, ROM, 0x200000);
				// intentional fall-through
			case 4 * 1024 * 1024:
				memcpy(ROM + 0x400000, ROM, 0x400000);
				// intentional fall-through
			case 8 * 1024 * 1024:
				memcpy(ROM + 0x800000, ROM, 0x800000);
				// intentional fall-through
			case 16 * 1024 * 1024:
				memcpy(ROM + 0x1000000, ROM, 0x1000000);
				break;
		}

		if (m_cart->get_nvram_size())
			battery_load(m_cart->get_nvram_base(), m_cart->get_nvram_size(), 0x00);

		return IMAGE_INIT_PASS;
	}

	return IMAGE_INIT_PASS;
}


/*-------------------------------------------------
 call_unload
 -------------------------------------------------*/

void gba_cart_slot_device::call_unload()
{
	if (m_cart && m_cart->get_nvram_size())
		battery_save(m_cart->get_nvram_base(), m_cart->get_nvram_size());
}



/*-------------------------------------------------
 call softlist load
 -------------------------------------------------*/

bool gba_cart_slot_device::call_softlist_load(software_list_device &swlist, const char *swname, const rom_entry *start_entry)
{
	load_software_part_region(*this, swlist, swname, start_entry);
	return TRUE;
}



/*-------------------------------------------------
 get_cart_type - code to detect NVRAM type from
 fullpath
 -------------------------------------------------*/

INLINE astring gba_chip_string( UINT32 chip )
{
	astring str;
	if (chip == 0) str += "NONE ";
	if (chip & GBA_CHIP_EEPROM) str += "EEPROM ";
	if (chip & GBA_CHIP_EEPROM_64K) str += "EEPROM_64K ";
	if (chip & GBA_CHIP_EEPROM_4K) str += "EEPROM_4K ";
	if (chip & GBA_CHIP_FLASH) str += "FLASH ";
	if (chip & GBA_CHIP_FLASH_1M) str += "FLASH_1M ";
	if (chip & GBA_CHIP_FLASH_512) str += "FLASH_512 ";
	if (chip & GBA_CHIP_SRAM) str += "SRAM ";
	if (chip & GBA_CHIP_RTC) str += "RTC ";
	return str.trimspace();
}


INLINE int gba_chip_has_conflict( UINT32 chip )
{
	int count1 = 0, count2 = 0;
	if (chip & GBA_CHIP_EEPROM) count1++;
	if (chip & GBA_CHIP_EEPROM_4K) count1++;
	if (chip & GBA_CHIP_EEPROM_64K) count1++;
	if (chip & GBA_CHIP_FLASH) count2++;
	if (chip & GBA_CHIP_FLASH_1M) count2++;
	if (chip & GBA_CHIP_FLASH_512) count2++;
	if (chip & GBA_CHIP_SRAM) count2++;
	return (count1 + count2) > 1; // if EEPROM + FLASH or EEPROM + SRAM carts exist, change to "(count1 > 1) || (count2 > 1)"
}


int gba_cart_slot_device::get_cart_type(UINT8 *ROM, UINT32 len)
{
	UINT32 chip = 0;
	int type = GBA_STD;

	// first detect nvram type based on strings inside the file
	for (int i = 0; i < len; i++)
	{
		if (!memcmp(&ROM[i], "EEPROM_V", 8))
			chip |= GBA_CHIP_EEPROM; // should be either GBA_CHIP_EEPROM_4K or GBA_CHIP_EEPROM_64K, but it is not yet possible to automatically detect which one
		else if ((!memcmp(&ROM[i], "SRAM_V", 6)) || (!memcmp(&ROM[i], "SRAM_F_V", 8))) // || (!memcmp(&data[i], "ADVANCEWARS", 11))) //advance wars 1 & 2 has SRAM, but no "SRAM_" string can be found inside the ROM space
			chip |= GBA_CHIP_SRAM;
		else if (!memcmp(&ROM[i], "FLASH1M_V", 9))
			chip |= GBA_CHIP_FLASH_1M;
		else if (!memcmp(&ROM[i], "FLASH512_V", 10))
			chip |= GBA_CHIP_FLASH_512;
		else if (!memcmp(&ROM[i], "FLASH_V", 7))
			chip |= GBA_CHIP_FLASH;
		else if (!memcmp(&ROM[i], "SIIRTC_V", 8))
			chip |= GBA_CHIP_RTC;
	}
	osd_printf_info("GBA: Detected (ROM) %s\n", gba_chip_string(chip).cstr());

	// fix for games which return more than one kind of chip: either it is one of the known titles, or we default to no battery
	if (gba_chip_has_conflict(chip))
	{
		char game_code[5] = { 0 };
		bool resolved = FALSE;

		if (len >= 0xac + 4)
			memcpy(game_code, ROM + 0xac, 4);

		osd_printf_info("GBA: Game Code \"%s\"\n", game_code);

		chip &= ~(GBA_CHIP_EEPROM | GBA_CHIP_EEPROM_4K | GBA_CHIP_EEPROM_64K | GBA_CHIP_FLASH | GBA_CHIP_FLASH_1M | GBA_CHIP_FLASH_512 | GBA_CHIP_SRAM);

		// search if it is one of the known titles with NVRAM conflicts
		for (int i = 0; i < sizeof(gba_chip_fix_conflict_list) / sizeof(gba_chip_fix_conflict_item); i++)
		{
			const gba_chip_fix_conflict_item *item = &gba_chip_fix_conflict_list[i];
			if (!strcmp(game_code, item->game_code))
			{
				chip |= item->chip;
				resolved = TRUE;
				break;
			}
		}
		if (!resolved)
			osd_printf_warning("GBA: NVRAM is disabled because multiple NVRAM chips were detected!\n");
	}

	// fix for games which are known to require an eeprom with 14-bit addressing (64 kbit)
	if (chip & GBA_CHIP_EEPROM)
	{
		char game_code[5] = { 0 };

		if (len >= 0xac + 4)
			memcpy(game_code, ROM + 0xac, 4);

		osd_printf_info("GBA: Game Code \"%s\"\n", game_code);

		for (int i = 0; i < sizeof(gba_chip_fix_eeprom_list) / sizeof(gba_chip_fix_eeprom_item); i++)
		{
			const gba_chip_fix_eeprom_item *item = &gba_chip_fix_eeprom_list[i];
			if (!strcmp(game_code, item->game_code))
			{
				chip = (chip & ~GBA_CHIP_EEPROM) | GBA_CHIP_EEPROM_64K;
				break;
			}
		}
	}

	if (chip & GBA_CHIP_RTC)
	{
		osd_printf_info("game has RTC - not emulated at the moment\n");
		chip &= ~GBA_CHIP_RTC;
	}

	osd_printf_info("GBA: Emulate %s\n", gba_chip_string(chip).cstr());

	switch (chip)
	{
		case GBA_CHIP_SRAM:
			type = GBA_SRAM;
			break;
		case GBA_CHIP_EEPROM:
			type = GBA_EEPROM;
			break;
		case GBA_CHIP_EEPROM_4K:
			type = GBA_EEPROM4;
			break;
		case GBA_CHIP_EEPROM_64K:
			type = GBA_EEPROM64;
			break;
		case GBA_CHIP_FLASH:
			type = GBA_FLASH;
			break;
		case GBA_CHIP_FLASH_512:
			type = GBA_FLASH512;
			break;
		case GBA_CHIP_FLASH_1M:
			type = GBA_FLASH1M;
			break;
		default:
			break;
	}

	return type;
}
/*-------------------------------------------------
 get default card software
 -------------------------------------------------*/

void gba_cart_slot_device::get_default_card_software(astring &result)
{
	if (open_image_file(mconfig().options()))
	{
		const char *slot_string = "gba_rom";
		UINT32 len = core_fsize(m_file);
		dynamic_buffer rom(len);
		int type;

		core_fread(m_file, rom, len);

		type = get_cart_type(rom, len);
		slot_string = gba_get_slot(type);

		//printf("type: %s\n", slot_string);
		clear();

		result.cpy(slot_string);
		return;
	}

	software_get_default_slot(result, "gba_rom");
}

/*-------------------------------------------------
 read
 -------------------------------------------------*/

READ32_MEMBER(gba_cart_slot_device::read_rom)
{
	if (m_cart)
		return m_cart->read_rom(space, offset, mem_mask);
	else
		return 0xffffffff;
}

READ32_MEMBER(gba_cart_slot_device::read_ram)
{
	if (m_cart)
		return m_cart->read_ram(space, offset, mem_mask);
	else
		return 0xffffffff;
}


/*-------------------------------------------------
 write
 -------------------------------------------------*/

WRITE32_MEMBER(gba_cart_slot_device::write_ram)
{
	if (m_cart)
		m_cart->write_ram(space, offset, data, mem_mask);
}


/*-------------------------------------------------
 Internal header logging
 -------------------------------------------------*/

void gba_cart_slot_device::internal_header_logging(UINT8 *ROM, UINT32 len)
{
}


/*-------------------------------------------------
 Install ROM - directly point system address map
 to the cart ROM region so to avoid the memory
 system additional load
 -------------------------------------------------*/

void gba_cart_slot_device::install_rom()
{
	if (m_cart)
	{
		astring tempstring;
		address_space &space = machine().device<cpu_device>("maincpu")->space(AS_PROGRAM);
		space.install_read_bank(0x08000000, 0x09ffffff, 0, 0, "rom1");
		space.install_read_bank(0x0a000000, 0x0bffffff, 0, 0, "rom2");
		space.install_read_bank(0x0c000000, 0x0cffffff, 0, 0, "rom3");
		machine().root_device().membank("rom1")->set_base(machine().root_device().memregion(m_cart->device().subtag(tempstring, "cartridge"))->base());
		machine().root_device().membank("rom2")->set_base(machine().root_device().memregion(m_cart->device().subtag(tempstring, "cartridge"))->base());
		machine().root_device().membank("rom3")->set_base(machine().root_device().memregion(m_cart->device().subtag(tempstring, "cartridge"))->base());
	}
}
