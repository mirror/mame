// license:BSD-3-Clause
// copyright-holders:R. Belmont
/***************************************************************************

  a2bus.h - Apple II slot bus and card emulation

  by R. Belmont

***************************************************************************/

#pragma once

#ifndef __A2BUS_H__
#define __A2BUS_H__

#include "emu.h"

#define INH_SLOT_INVALID    (255)

//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_A2BUS_CPU(_cputag) \
	a2bus_device::static_set_cputag(*device, _cputag);

#define MCFG_A2BUS_OUT_IRQ_CB(_devcb) \
	devcb = &a2bus_device::set_out_irq_callback(*device, DEVCB_##_devcb);

#define MCFG_A2BUS_OUT_NMI_CB(_devcb) \
	devcb = &a2bus_device::set_out_nmi_callback(*device, DEVCB_##_devcb);

#define MCFG_A2BUS_OUT_INH_CB(_devcb) \
	devcb = &a2bus_device::set_out_inh_callback(*device, DEVCB_##_devcb);

#define MCFG_A2BUS_SLOT_ADD(_nbtag, _tag, _slot_intf, _def_slot) \
	MCFG_DEVICE_ADD(_tag, A2BUS_SLOT, 0) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, false) \
	a2bus_slot_device::static_set_a2bus_slot(*device, _nbtag, _tag);
#define MCFG_A2BUS_SLOT_REMOVE(_tag)    \
	MCFG_DEVICE_REMOVE(_tag)

#define MCFG_A2BUS_ONBOARD_ADD(_nbtag, _tag, _dev_type, _def_inp) \
	MCFG_DEVICE_ADD(_tag, _dev_type, 0) \
	MCFG_DEVICE_INPUT_DEFAULTS(_def_inp) \
	device_a2bus_card_interface::static_set_a2bus_tag(*device, _nbtag, _tag);

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class a2bus_device;

class a2bus_slot_device : public device_t,
							public device_slot_interface
{
public:
	// construction/destruction
	a2bus_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	a2bus_slot_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source);

	// device-level overrides
	virtual void device_start();

	// inline configuration
	static void static_set_a2bus_slot(device_t &device, const char *tag, const char *slottag);
protected:
	// configuration
	const char *m_a2bus_tag, *m_a2bus_slottag;
};

// device type definition
extern const device_type A2BUS_SLOT;


class device_a2bus_card_interface;
// ======================> a2bus_device
class a2bus_device : public device_t
{
	// multi-card devices need to access m_device_list, so they get friend'ed here.
	friend class a2bus_mcms2_device;
public:
	// construction/destruction
	a2bus_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	a2bus_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source);

	// inline configuration
	static void static_set_cputag(device_t &device, const char *tag);
	template<class _Object> static devcb_base &set_out_irq_callback(device_t &device, _Object object) { return downcast<a2bus_device &>(device).m_out_irq_cb.set_callback(object); }
	template<class _Object> static devcb_base &set_out_nmi_callback(device_t &device, _Object object) { return downcast<a2bus_device &>(device).m_out_nmi_cb.set_callback(object); }
	template<class _Object> static devcb_base &set_out_inh_callback(device_t &device, _Object object) { return downcast<a2bus_device &>(device).m_out_inh_cb.set_callback(object); }

	void add_a2bus_card(int slot, device_a2bus_card_interface *card);
	device_a2bus_card_interface *get_a2bus_card(int slot);
	UINT8 get_a2bus_irq_mask();
	UINT8 get_a2bus_nmi_mask();

	void set_irq_line(int state, int slot);
	void set_nmi_line(int state, int slot);
	void set_inh_slotnum(int slot);

	DECLARE_WRITE_LINE_MEMBER( irq_w );
	DECLARE_WRITE_LINE_MEMBER( nmi_w );

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	// internal state
	cpu_device   *m_maincpu;

	devcb_write_line    m_out_irq_cb;
	devcb_write_line    m_out_nmi_cb;
	devcb_write_line    m_out_inh_cb;

	device_a2bus_card_interface *m_device_list[8];
	const char *m_cputag;

	UINT8 m_slot_irq_mask;
	UINT8 m_slot_nmi_mask;
};


// device type definition
extern const device_type A2BUS;

// ======================> device_a2bus_card_interface

// class representing interface-specific live a2bus card
class device_a2bus_card_interface : public device_slot_card_interface
{
	friend class a2bus_device;
public:
	// construction/destruction
	device_a2bus_card_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_a2bus_card_interface();

	virtual UINT8 read_c0nx(address_space &space, UINT8 offset) { printf("a2bus: unhandled read at C0n%x\n", offset); return 0; }       // C0nX - /DEVSEL
	virtual void write_c0nx(address_space &space, UINT8 offset, UINT8 data) { printf("a2bus: unhandled write %02x to C0n%x\n", data, offset); }
	virtual UINT8 read_cnxx(address_space &space, UINT8 offset) { return 0; }       // CnXX - /IOSEL
	virtual void write_cnxx(address_space &space, UINT8 offset, UINT8 data) { printf("a2bus: unhandled write %02x to Cn%02x\n", data, offset); }
	virtual UINT8 read_c800(address_space &space, UINT16 offset) { return 0; }      // C800 - /IOSTB
	virtual void write_c800(address_space &space, UINT16 offset, UINT8 data) { printf("a2bus: unhandled write %02x to %04x\n", data, offset + 0xc800); }
	virtual bool take_c800() { return true; }   // override and return false if your card doesn't take over the c800 space
	virtual UINT8 read_inh_rom(address_space &space, UINT16 offset) { return 0; }
	virtual void write_inh_rom(address_space &space, UINT16 offset, UINT8 data) { }

	device_a2bus_card_interface *next() const { return m_next; }

	void set_a2bus_device();

	UINT32 get_slotromspace() { return 0xc000 | (m_slot<<8); }      // return Cn00 address for this slot
	UINT32 get_slotiospace() { return 0xc080 + (m_slot<<4); }       // return C0n0 address for this slot

	void raise_slot_irq() { m_a2bus->set_irq_line(ASSERT_LINE, m_slot); }
	void lower_slot_irq() { m_a2bus->set_irq_line(CLEAR_LINE, m_slot); }
	void raise_slot_nmi() { m_a2bus->set_nmi_line(ASSERT_LINE, m_slot); }
	void lower_slot_nmi() { m_a2bus->set_nmi_line(CLEAR_LINE, m_slot); }
	void raise_slot_inh() { m_a2bus->set_inh_slotnum(m_slot); }
	void lower_slot_inh() { m_a2bus->set_inh_slotnum(INH_SLOT_INVALID); }

	// inline configuration
	static void static_set_a2bus_tag(device_t &device, const char *tag, const char *slottag);
public:
	a2bus_device  *m_a2bus;
	const char *m_a2bus_tag, *m_a2bus_slottag;
	int m_slot;
	device_a2bus_card_interface *m_next;
};

#endif  /* __A2BUS_H__ */
