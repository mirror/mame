/***************************************************************************

    ui/imgcntrl.c

    MESS's clunky built-in file manager

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "emu.h"
#include "ui/ui.h"
#include "ui/menu.h"
#include "ui/imgcntrl.h"
#include "ui/filesel.h"
#include "ui/swlist.h"
#include "zippath.h"


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

//-------------------------------------------------
//  ctor
//-------------------------------------------------

ui_menu_control_device_image::ui_menu_control_device_image(running_machine &machine, render_container *container, device_image_interface *_image)
	: ui_menu(machine, container)
{
	image = _image;

	sld = 0;
	if (image->software_list_name()) {
		software_list_device_iterator iter(machine.config().root_device());
		for (software_list_device *swlist = iter.first(); swlist != NULL; swlist = iter.next())
		{
			if (strcmp(swlist->list_name(),image->software_list_name())==0) sld = swlist;
		}
	}
	swi = image->software_entry();
	swp = image->part_entry();

	if(swi)
	{
		state = START_OTHER_PART;
		current_directory.cpy(image->working_directory());
	}
	else
	{
		state = START_FILE;

		/* if the image exists, set the working directory to the parent directory */
		if (image->exists())
		{
			current_file.cpy(image->filename());
			zippath_parent(current_directory, current_file);
		} else
			current_directory.cpy(image->working_directory());

		/* check to see if the path exists; if not clear it */
		if (zippath_opendir(current_directory, NULL) != FILERR_NONE)
			current_directory.reset();
	}
}


//-------------------------------------------------
//  dtor
//-------------------------------------------------

ui_menu_control_device_image::~ui_menu_control_device_image()
{
}


//-------------------------------------------------
//  test_create - creates a new disk image
//-------------------------------------------------

void ui_menu_control_device_image::test_create(bool &can_create, bool &need_confirm)
{
	astring path;
	osd_directory_entry *entry;
	osd_dir_entry_type file_type;

	/* assemble the full path */
	zippath_combine(path, current_directory, current_file);

	/* does a file or a directory exist at the path */
	entry = osd_stat(path);
	file_type = (entry != NULL) ? entry->type : ENTTYPE_NONE;

	switch(file_type)
	{
		case ENTTYPE_NONE:
			/* no file/dir here - always create */
			can_create = true;
			need_confirm = false;
			break;

		case ENTTYPE_FILE:
			/* a file exists here - ask for permission from the user */
			can_create = true;
			need_confirm = true;
			break;

		case ENTTYPE_DIR:
			/* a directory exists here - we can't save over it */
			machine().ui().popup_time(5, "Cannot save over directory");
			can_create = false;
			need_confirm = false;
			break;

		default:
			fatalerror("Unexpected\n");
			can_create = false;
			need_confirm = false;
			break;
	}

	if (entry != NULL)
		osd_free(entry);
}


//-------------------------------------------------
//  load_software_part
//-------------------------------------------------

void ui_menu_control_device_image::load_software_part()
{
	astring temp_name(sld->list_name(), ":", swi->shortname(), ":", swp->name());
	hook_load(temp_name, true);
}


//-------------------------------------------------
//  hook_load
//-------------------------------------------------

void ui_menu_control_device_image::hook_load(astring name, bool softlist)
{
	image->load(name);
	ui_menu::stack_pop(machine());
}


//-------------------------------------------------
//  populate
//-------------------------------------------------

void ui_menu_control_device_image::populate()
{
}


//-------------------------------------------------
//  handle
//-------------------------------------------------

void ui_menu_control_device_image::handle()
{
	switch(state) {
	case START_FILE: {
		bool can_create = false;
		if(image->is_creatable()) {
			zippath_directory *directory = NULL;
			file_error err = zippath_opendir(current_directory, &directory);
			can_create = err == FILERR_NONE && !zippath_is_zip(directory);
			if(directory)
				zippath_closedir(directory);
		}
		submenu_result = -1;
		ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_file_selector(machine(), container, image, current_directory, current_file, true, image->image_interface()!=NULL, can_create, &submenu_result)));
		state = SELECT_FILE;
		break;
	}

	case START_SOFTLIST:
		sld = 0;
		ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_software(machine(), container, image->image_interface(), &sld)));
		state = SELECT_SOFTLIST;
		break;

	case START_OTHER_PART: {
		submenu_result = -1;
		ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_software_parts(machine(), container, swi, swp->interface(), &swp, true, &submenu_result)));
		state = SELECT_OTHER_PART;
		break;
	}

	case SELECT_SOFTLIST:
		if(!sld) {
			ui_menu::stack_pop(machine());
			break;
		}
		software_info_name = "";
		ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_software_list(machine(), container, sld, image->image_interface(), software_info_name)));
		state = SELECT_PARTLIST;
		break;

	case SELECT_PARTLIST:
		swi = sld->find(software_info_name);
		if (!swi)
			state = START_SOFTLIST;	
		else if(swi->has_multiple_parts(image->image_interface())) 
		{
			submenu_result = -1;
			swp = 0;
			ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_software_parts(machine(), container, swi, image->image_interface(), &swp, false, &submenu_result)));
			state = SELECT_ONE_PART;
		} 
		else 
		{
			swp = swi->first_part();
			load_software_part();
			ui_menu::stack_pop(machine());
		}
		break;

	case SELECT_ONE_PART:
		switch(submenu_result) {
		case ui_menu_software_parts::T_ENTRY: {
			load_software_part();
			ui_menu::stack_pop(machine());
			break;
		}

		case -1: // return to list
			state = SELECT_SOFTLIST;
			break;

		}
		break;

	case SELECT_OTHER_PART:
		switch(submenu_result) {
		case ui_menu_software_parts::T_ENTRY: {
			load_software_part();
			break;
		}

		case ui_menu_software_parts::T_FMGR:
			state = START_FILE;
			handle();
			break;

		case -1: // return to system
			ui_menu::stack_pop(machine());
			break;

		}
		break;

	case SELECT_FILE:
		switch(submenu_result) {
		case ui_menu_file_selector::R_EMPTY:
			image->unload();
			ui_menu::stack_pop(machine());
			break;

		case ui_menu_file_selector::R_FILE:
			hook_load(current_file, false);
			break;

		case ui_menu_file_selector::R_CREATE:
			ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_file_create(machine(), container, image, current_directory, current_file)));
			state = CREATE_FILE;
			break;

		case ui_menu_file_selector::R_SOFTLIST:
			state = START_SOFTLIST;
			handle();
			break;

		case -1: // return to system
			ui_menu::stack_pop(machine());
			break;
		}
		break;

	case CREATE_FILE: {
		bool can_create, need_confirm;
		test_create(can_create, need_confirm);
		if(can_create) {
			if(need_confirm) {
				ui_menu::stack_push(auto_alloc_clear(machine(), ui_menu_confirm_save_as(machine(), container, &create_confirmed)));
				state = CREATE_CONFIRM;
			} else {
				state = DO_CREATE;
				handle();
			}
		} else {
			state = START_FILE;
			handle();
		}
		break;
	}

	case CREATE_CONFIRM: {
		state = create_confirmed ? DO_CREATE : START_FILE;
		handle();
		break;
	}

	case DO_CREATE: {
		astring path;
		zippath_combine(path, current_directory, current_file);
		int err = image->create(path, 0, NULL);
		if (err != 0)
			popmessage("Error: %s", image->error());
		ui_menu::stack_pop(machine());
		break;
	}
	}
}
