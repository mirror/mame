#ifndef GENPIN_H_
#define GENPIN_H_


#include "emu.h"
#include "sound/samples.h"
#include "machine/nvram.h"


const char *const genpin_sample_names[] =
{
	"*genpin",
	"bumper",
	"chime1",
	"chime2",
	"chime3",
	"chime4",
	"hole",
	"knocker",
	"sling",
	"coinin",
	0   /* end of array */
};

MACHINE_CONFIG_EXTERN( genpin_audio );


class genpin_class : public driver_device
{
public:
	genpin_class(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_samples(*this, "samples")
	{ }

	required_device<samples_device> m_samples;
};


#endif /* GENPIN_H_ */
