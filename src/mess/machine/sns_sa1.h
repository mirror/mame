#ifndef __SNS_SA1_H
#define __SNS_SA1_H

#include "machine/sns_slot.h"
#include "cpu/g65816/g65816.h"


// ======================> sns_sa1_device

class sns_sa1_device : public device_t,
					  public device_sns_cart_interface
{
public:
	// construction/destruction
	sns_sa1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	
	// device-level overrides
	virtual void device_start();
	virtual machine_config_constructor device_mconfig_additions() const;

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_l);
	virtual DECLARE_READ8_MEMBER(read_h);
	virtual DECLARE_WRITE8_MEMBER(write_l);
	virtual DECLARE_WRITE8_MEMBER(write_h);
	
	// additional reading and writing
	virtual DECLARE_READ8_MEMBER(chip_read);
	virtual DECLARE_WRITE8_MEMBER(chip_write);

	DECLARE_READ8_MEMBER(sa1_lo_r);
	DECLARE_READ8_MEMBER(sa1_hi_r);
	DECLARE_WRITE8_MEMBER(sa1_lo_w);
	DECLARE_WRITE8_MEMBER(sa1_hi_w);
	
private:
	
	UINT8 read_regs(UINT32 offset);
	UINT8 read_iram(UINT32 offset);
	UINT8 read_bwram(UINT32 offset);
	void write_regs(UINT32 offset, UINT8 data);
	void write_iram(UINT32 offset, UINT8 data);
	void write_bwram(UINT32 offset, UINT8 data);
	
	required_device<_5a22_device> m_sa1;
	
	UINT8 m_internal_ram[0x800];

	// register related
	// $2220-$2223
	int m_bank_c_hi, m_bank_c_rom;
	int m_bank_d_hi, m_bank_d_rom;
	int m_bank_e_hi, m_bank_e_rom;
	int m_bank_f_hi, m_bank_f_rom;
	// $2224-$2225 & $223f
	UINT8 m_bwram_snes, m_bwram_sa1; 
	int m_bwram_sa1_source, m_bwram_sa1_format;
	// $2226-$2227
	int m_bwram_write_snes, m_bwram_write_sa1;
	// $2228
	UINT32 m_bwpa_sa1;
	// $2229-$222a
	UINT8 m_iram_write_snes, m_iram_write_sa1;
};


// device type definition
extern const device_type SNS_LOROM_SA1;

#endif
