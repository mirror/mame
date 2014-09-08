// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    validity.h

    Validity checks

***************************************************************************/

#pragma once

#ifndef __VALIDITY_H__
#define __VALIDITY_H__

#include "emu.h"
#include "drivenum.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// forward declarations
class machine_config;


// core validity checker class
class validity_checker
{
	// internal map types
	typedef tagmap_t<const game_driver *> game_driver_map;
	typedef tagmap_t<FPTR> int_map;

public:
	validity_checker(emu_options &options);
	~validity_checker();

	// getters
	int errors() const { return m_errors; }
	int warnings() const { return m_warnings; }

	// operations
	void check_driver(const game_driver &driver);
	void check_shared_source(const game_driver &driver);
	void check_all();

	// helpers for devices
	void validate_tag(const char *tag);
	int region_length(const char *tag) { return m_region_map.find(tag); }

	// generic registry of already-checked stuff
	bool already_checked(const char *string) { return (m_already_checked.add(string, 1, false) == TMERR_DUPLICATE); }

private:
	// internal helpers
	const char *ioport_string_from_index(UINT32 index);
	int get_defstr_index(const char *string, bool suppress_error = false);

	// core helpers
	void validate_begin();
	void validate_end();
	void validate_one(const game_driver &driver);

	// internal sub-checks
	void validate_core();
	void validate_inlines();
	void validate_driver();
	void validate_roms();
	void validate_analog_input_field(ioport_field &field);
	void validate_dip_settings(ioport_field &field);
	void validate_condition(ioport_condition &condition, device_t &device, int_map &port_map);
	void validate_inputs();
	void validate_devices();

	// output helpers
	void build_output_prefix(astring &string);
	void error_output(const char *format, va_list argptr);
	void warning_output(const char *format, va_list argptr);
	void output_via_delegate(output_delegate &delegate, const char *format, ...) ATTR_PRINTF(3,4);

	// internal driver list
	driver_enumerator       m_drivlist;

	// error tracking
	int                     m_errors;
	int                     m_warnings;
	astring                 m_error_text;
	astring                 m_warning_text;

	// maps for finding duplicates
	game_driver_map         m_names_map;
	game_driver_map         m_descriptions_map;
	game_driver_map         m_roms_map;
	int_map                 m_defstr_map;

	// current state
	const game_driver *     m_current_driver;
	const machine_config *  m_current_config;
	const device_t *        m_current_device;
	const char *            m_current_ioport;
	int_map                 m_region_map;
	tagmap_t<UINT8>         m_already_checked;

	// callbacks
	output_delegate         m_saved_error_output;
	output_delegate         m_saved_warning_output;
};

#endif
