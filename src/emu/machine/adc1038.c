/***************************************************************************

    National Semiconductor ADC1038

    10-Bit Serial I/O A/D Converters with Analog Multiplexer and
    Track/hold Function

***************************************************************************/

#include "emu.h"
#include "adc1038.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _adc1038_state adc1038_state;
struct _adc1038_state
{
	int cycle;
	int clk;
	int adr;
	int data_in;
	int data_out;
	int adc_data;
	int sars;
	adc1038_input_read_func input_callback_r;

	int gticlub_hack;
};

/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE adc1038_state *adc1038_get_safe_token( device_t *device )
{
	assert(device != NULL);
	assert(device->type() == ADC1038);

	return (adc1038_state *)downcast<legacy_device_base *>(device)->token();
}

INLINE const adc1038_interface *adc1038_get_interface( device_t *device )
{
	assert(device != NULL);
	assert((device->type() == ADC1038));
	return (const adc1038_interface *) device->static_config();
}

/*****************************************************************************
    DEVICE HANDLERS
*****************************************************************************/

READ_LINE_DEVICE_HANDLER( adc1038_do_read )
{
	adc1038_state *adc1038 = adc1038_get_safe_token(device);

	//printf("ADC DO\n");
	return adc1038->data_out;
}

WRITE_LINE_DEVICE_HANDLER( adc1038_di_write )
{
	adc1038_state *adc1038 = adc1038_get_safe_token(device);

	adc1038->data_in = state;
}

WRITE_LINE_DEVICE_HANDLER( adc1038_clk_write )
{
	adc1038_state *adc1038 = adc1038_get_safe_token(device);

	// GTI Club doesn't sync on SARS
	if (adc1038->gticlub_hack)
	{
		if (adc1038->clk == 0 && state == 0)
		{
			adc1038->cycle = 0;

			/* notice that adc1038->adr is always < 7! */
			adc1038->adc_data = adc1038->input_callback_r(device, adc1038->adr);
		}
	}

	if (state == 1)
	{
		//printf("ADC CLK, DI = %d, cycle = %d\n", adc1038->data_in, adc1038->cycle);

		if (adc1038->cycle == 0)			// A2
		{
			adc1038->adr = 0;
			adc1038->adr |= (adc1038->data_in << 2);
		}
		else if (adc1038->cycle == 1)	// A1
		{
			adc1038->adr |= (adc1038->data_in << 1);
		}
		else if (adc1038->cycle == 2)	// A0
		{
			adc1038->adr |= (adc1038->data_in << 0);
		}

		adc1038->data_out = (adc1038->adc_data & 0x200) ? 1 : 0;
		adc1038->adc_data <<= 1;

		adc1038->cycle++;
	}

	adc1038->clk = state;
}

READ_LINE_DEVICE_HANDLER( adc1038_sars_read )
{
	adc1038_state *adc1038 = adc1038_get_safe_token(device);

	adc1038->cycle = 0;

	/* notice that adc1038->adr is always < 7! */
	adc1038->adc_data = adc1038->input_callback_r(device, adc1038->adr);

	adc1038->data_out = (adc1038->adc_data & 0x200) ? 1 : 0;
	adc1038->adc_data <<= 1;

	adc1038->sars ^= 1;
	return adc1038->sars;
}


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( adc1038 )
{
	adc1038_state *adc1038 = adc1038_get_safe_token(device);
	const adc1038_interface *intf = adc1038_get_interface(device);

	adc1038->gticlub_hack = intf->gticlub_hack;
	adc1038->input_callback_r = intf->input_callback_r;

	device->save_item(NAME(adc1038->cycle));
	device->save_item(NAME(adc1038->clk));
	device->save_item(NAME(adc1038->adr));
	device->save_item(NAME(adc1038->data_in));
	device->save_item(NAME(adc1038->data_out));
	device->save_item(NAME(adc1038->adc_data));
	device->save_item(NAME(adc1038->sars));
}

static DEVICE_RESET( adc1038 )
{
	adc1038_state *adc1038 = adc1038_get_safe_token(device);

	adc1038->cycle = 0;
	adc1038->clk = 0;
	adc1038->adr = 0;
	adc1038->data_in = 0;
	adc1038->data_out = 0;
	adc1038->adc_data = 0;
	adc1038->sars = 1;
}

/*-------------------------------------------------
    device definition
-------------------------------------------------*/

DEVICE_GET_INFO(adc1038)
{
 switch (state)
 {
  case DEVINFO_INT_TOKEN_BYTES: info->i = sizeof(adc1038_state); break;
 
  case DEVINFO_FCT_START: info->start = DEVICE_START_NAME(adc1038); break;

  case DEVINFO_FCT_RESET: info->reset = DEVICE_RESET_NAME(adc1038); break;
  
  case DEVINFO_STR_NAME: strcpy(info->s, "A/D Converters 1038"); break;
 }
}

DEFINE_LEGACY_DEVICE(ADC1038, adc1038);
