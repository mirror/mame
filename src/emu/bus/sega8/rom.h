#ifndef __SEGA8_ROM_H
#define __SEGA8_ROM_H

#include "sega8_slot.h"
#include "machine/eepromser.h"

// ======================> sega8_rom_device

class sega8_rom_device : public device_t,
						public device_sega8_cart_interface
{
public:
	// construction/destruction
	sega8_rom_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source);
	sega8_rom_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	virtual void late_bank_setup();

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper);

protected:
	UINT8 m_rom_bank_base[3];
	UINT8 m_ram_base;
	int m_ram_enabled;
};




// ======================> sega8_cardcatch_device

class sega8_cardcatch_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_cardcatch_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper) {}

	virtual machine_config_constructor device_mconfig_additions() const;

protected:
	required_device<sega8_card_slot_device> m_card;
};


// ======================> sega8_othello_device

class sega8_othello_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_othello_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper) {}
};


// ======================> sega8_castle_device

class sega8_castle_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_castle_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper) {}
};


// ======================> sega8_basic_l3_device

class sega8_basic_l3_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_basic_l3_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper) {}

	// has internal RAM which overwrites the system one!
	virtual DECLARE_READ8_MEMBER(read_ram);
	virtual DECLARE_WRITE8_MEMBER(write_ram);
};


// ======================> sega8_music_editor_device

class sega8_music_editor_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_music_editor_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper) {}

	// has internal RAM which overwrites the system one!
	virtual DECLARE_READ8_MEMBER(read_ram);
	virtual DECLARE_WRITE8_MEMBER(write_ram);
};


// ======================> sega8_terebi_device

class sega8_terebi_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_terebi_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual ioport_constructor device_input_ports() const;
	virtual void device_reset();

	required_ioport m_tvdraw_x;
	required_ioport m_tvdraw_y;
	required_ioport m_tvdraw_pen;

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper) {}

protected:
	UINT8 m_tvdraw_data;
};


// ======================> sega8_dahjee_typea_device

class sega8_dahjee_typea_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_dahjee_typea_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper) {}

	// has internal RAM which overwrites the system one!
	virtual DECLARE_READ8_MEMBER(read_ram);
	virtual DECLARE_WRITE8_MEMBER(write_ram);
};


// ======================> sega8_dahjee_typeb_device

class sega8_dahjee_typeb_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_dahjee_typeb_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart) {}
	virtual DECLARE_WRITE8_MEMBER(write_mapper) {}

	// has internal RAM which overwrites the system one!
	virtual DECLARE_READ8_MEMBER(read_ram);
	virtual DECLARE_WRITE8_MEMBER(write_ram);
};




// ======================> sega8_eeprom_device

class sega8_eeprom_device : public device_t,
							public device_sega8_cart_interface
{
public:
	// construction/destruction
	sega8_eeprom_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual void device_reset();

	virtual void late_bank_setup();

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper);

protected:
	UINT8 m_rom_bank_base[3];

	required_device<eeprom_serial_93cxx_device> m_eeprom;
	int m_93c46_enabled;
	UINT8 m_93c46_lines;
};


// ======================> sega8_codemasters_device

class sega8_codemasters_device : public device_t,
								public device_sega8_cart_interface
{
public:
	// construction/destruction
	sega8_codemasters_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	virtual void late_bank_setup();

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	// no mapper write for this!

protected:
	UINT8 m_rom_bank_base[3];
	UINT8 m_ram_base;
	int m_ram_enabled;
};


// ======================> sega8_4pak_device

class sega8_4pak_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_4pak_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);

private:
	UINT8 m_reg[3];
};


// ======================> sega8_zemina_device

class sega8_zemina_device : public device_t,
							public device_sega8_cart_interface
{
public:
	// construction/destruction
	sega8_zemina_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source);
	sega8_zemina_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	virtual void late_bank_setup();

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	// no mapper write for this!

protected:
	UINT8 m_rom_bank_base[6];
	UINT8 m_ram_base;
	int m_ram_enabled;
};


// ======================> sega8_nemesis_device

class sega8_nemesis_device : public sega8_zemina_device
{
public:
	// construction/destruction
	sega8_nemesis_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual void late_bank_setup();
};


// ======================> sega8_janggun_device

class sega8_janggun_device : public device_t,
							public device_sega8_cart_interface
{
public:
	// construction/destruction
	sega8_janggun_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start() { save_item(NAME(m_rom_bank_base)); }

	virtual void late_bank_setup();

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_cart);
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper);

protected:
	UINT8 m_rom_bank_base[6];
};


// ======================> sega8_korean_device

class sega8_korean_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_korean_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual void late_bank_setup();

	// reading and writing
	virtual DECLARE_WRITE8_MEMBER(write_cart);
	virtual DECLARE_WRITE8_MEMBER(write_mapper) {}
};


// ======================> sega8_korean_nb_device

class sega8_korean_nb_device : public sega8_rom_device
{
public:
	// construction/destruction
	sega8_korean_nb_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// reading and writing
	virtual DECLARE_WRITE8_MEMBER(write_mapper) {}
};



// device type definition
extern const device_type SEGA8_ROM_STD;
extern const device_type SEGA8_ROM_CARDCATCH;
extern const device_type SEGA8_ROM_OTHELLO;
extern const device_type SEGA8_ROM_CASTLE;
extern const device_type SEGA8_ROM_BASIC_L3;
extern const device_type SEGA8_ROM_MUSIC_EDITOR;
extern const device_type SEGA8_ROM_DAHJEE_TYPEA;
extern const device_type SEGA8_ROM_DAHJEE_TYPEB;
extern const device_type SEGA8_ROM_EEPROM;
extern const device_type SEGA8_ROM_TEREBI;
extern const device_type SEGA8_ROM_CODEMASTERS;
extern const device_type SEGA8_ROM_4PAK;
extern const device_type SEGA8_ROM_ZEMINA;
extern const device_type SEGA8_ROM_NEMESIS;
extern const device_type SEGA8_ROM_JANGGUN;
extern const device_type SEGA8_ROM_KOREAN;
extern const device_type SEGA8_ROM_KOREAN_NB;

#endif
