/***************************************************************************

    m740.c

    Mitsubishi M740 series (M507xx/M509xx)

****************************************************************************

    Copyright Olivier Galibert
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in
          the documentation and/or other materials provided with the
          distribution.
        * Neither the name 'MAME' nor the names of its contributors may be
          used to endorse or promote products derived from this software
          without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY OLIVIER GALIBERT ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
    STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
    IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#include "emu.h"
#include "m740.h"

const device_type M740 = &device_creator<m740_device>;

m740_device::m740_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	m6502_device(mconfig, M740, "M740", tag, owner, clock, "m740", __FILE__)
{
}

m740_device::m740_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source) :
	m6502_device(mconfig, type, name, tag, owner, clock, shortname, source)
{
}

offs_t m740_device::disasm_disassemble(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, UINT32 options)
{
	return disassemble_generic(buffer, pc, oprom, opram, options, disasm_entries);
}

void m740_device::device_start()
{
	m6502_device::device_start();

	save_item(NAME(m_irq_multiplex));
	save_item(NAME(m_irq_vector));
}

void m740_device::device_reset()
{
	inst_state_base = 0;
	inst_state = STATE_RESET;
	inst_substate = 0;
	nmi_state = false;
	irq_state = false;
	m_irq_multiplex = 0;
	m_irq_vector = 0xfffc;
	apu_irq_state = false;
	irq_taken = false;
	v_state = false;
	sync = false;
	inhibit_interrupts = false;
	SP = 0x00ff;
}

void m740_device::state_string_export(const device_state_entry &entry, astring &string)
{
	switch(entry.index()) {
	case STATE_GENFLAGS:
	case M6502_P:
		string.printf("%c%c%c%c%c%c%c",
						P & F_N ? 'N' : '.',
						P & F_V ? 'V' : '.',
						P & F_T ? 'T' : '.',
						P & F_D ? 'D' : '.',
						P & F_I ? 'I' : '.',
						P & F_Z ? 'Z' : '.',
						P & F_C ? 'C' : '.');
		break;
	}
}

UINT8 m740_device::do_clb(UINT8 in, UINT8 bit)
{
	return in & ~(1<<bit);
}

UINT8 m740_device::do_seb(UINT8 in, UINT8 bit)
{
	return in | (1<<bit);
}

// swap the two nibbles of the input (Rotate Right Four bits)
// doesn't affect the flags
UINT8 m740_device::do_rrf(UINT8 in)
{
		return ((in&0xf)<<4) | ((in&0xf0)>>4);
}

void m740_device::do_sbc_dt(UINT8 val)
{
	UINT8 c = P & F_C ? 0 : 1;
	P &= ~(F_N|F_V|F_Z|F_C);
	UINT16 diff = TMP2 - val - c;
	UINT8 al = (TMP2 & 15) - (val & 15) - c;
	if(INT8(al) < 0)
		al -= 6;
	UINT8 ah = (TMP2 >> 4) - (val >> 4) - (INT8(al) < 0);
	if(!UINT8(diff))
		P |= F_Z;
	else if(diff & 0x80)
		P |= F_N;
	if((TMP2^val) & (TMP2^diff) & 0x80)
		P |= F_V;
	if(!(diff & 0xff00))
		P |= F_C;
	if(INT8(ah) < 0)
		ah -= 6;
	TMP2 = (ah << 4) | (al & 15);
}

void m740_device::do_sbc_ndt(UINT8 val)
{
	UINT16 diff = TMP2 - val - (P & F_C ? 0 : 1);
	P &= ~(F_N|F_V|F_Z|F_C);
	if(!UINT8(diff))
		P |= F_Z;
	else if(INT8(diff) < 0)
		P |= F_N;
	if((TMP2^val) & (TMP2^diff) & 0x80)
		P |= F_V;
	if(!(diff & 0xff00))
		P |= F_C;
	TMP2 = diff;
}

void m740_device::do_sbct(UINT8 val)
{
	if(P & F_D)
		do_sbc_dt(val);
	else
		do_sbc_ndt(val);
}

void m740_device::do_adc_dt(UINT8 val)
{
	UINT8 c = P & F_C ? 1 : 0;
	P &= ~(F_N|F_V|F_Z|F_C);
	UINT8 al = (TMP2 & 15) + (val & 15) + c;
	if(al > 9)
		al += 6;
	UINT8 ah = (TMP2 >> 4) + (val >> 4) + (al > 15);
	if(!UINT8(TMP2 + val + c))
		P |= F_Z;
	else if(ah & 8)
		P |= F_N;
	if(~(TMP2^val) & (TMP2^(ah << 4)) & 0x80)
		P |= F_V;
	if(ah > 9)
		ah += 6;
	if(ah > 15)
		P |= F_C;
	TMP2 = (ah << 4) | (al & 15);
}

void m740_device::do_adc_ndt(UINT8 val)
{
	UINT16 sum;
	sum = TMP2 + val + (P & F_C ? 1 : 0);
	P &= ~(F_N|F_V|F_Z|F_C);
	if(!UINT8(sum))
		P |= F_Z;
	else if(INT8(sum) < 0)
		P |= F_N;
	if(~(TMP2^val) & (TMP2^sum) & 0x80)
		P |= F_V;
	if(sum & 0xff00)
		P |= F_C;
	TMP2 = sum;
}

void m740_device::do_adct(UINT8 val)
{
	if(P & F_D)
		do_adc_dt(val);
	else
		do_adc_ndt(val);
}

void m740_device::execute_set_input(int inputnum, int state)
{
	switch(inputnum)
	{
		case M740_INT0_LINE:
		case M740_INT1_LINE:
		case M740_INT2_LINE:
		case M740_INT3_LINE:
		case M740_INT4_LINE:
		case M740_INT5_LINE:
		case M740_INT6_LINE:
		case M740_INT7_LINE:
		case M740_INT8_LINE:
		case M740_INT9_LINE:
		case M740_INT10_LINE:
		case M740_INT11_LINE:
		case M740_INT12_LINE:
		case M740_INT13_LINE:
		case M740_INT14_LINE:   // 37450 has 15 IRQ lines, no other known variant has that many
			set_irq_line(inputnum - M740_INT0_LINE, state);
			break;

		case V_LINE:
			if(!v_state && state == ASSERT_LINE)
			{
				P |= F_V;
			}
			v_state = state == ASSERT_LINE;
			break;
	}
}

void m740_device::set_irq_line(int line, int state)
{
	assert(line > 0);
	assert(line <= M740_MAX_INT_LINE);

	if (state == ASSERT_LINE)
	{
		m_irq_multiplex  |= (1<<line);
	}
	else
	{
		m_irq_multiplex &= ~(1<<line);
	}

	irq_state = (m_irq_multiplex != 0);

	if (irq_state)
	{
		for (int i = 0; i < M740_MAX_INT_LINE; i++)
		{
			if (m_irq_multiplex & (1 << i))
			{
				m_irq_vector = 0xfffc - (UINT16)(2 * i);
				break;
			}
		}
	}

//  printf("M740 single IRQ state is %d (MPX %08x, vector %x)\n", irq_state, m_irq_multiplex, m_irq_vector);
}

#include "cpu/m6502/m740.inc"
