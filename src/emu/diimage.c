// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic
/***************************************************************************

    diimage.c

    Device image interfaces.

***************************************************************************/

#include "emu.h"
#include "emuopts.h"
#include "drivenum.h"
#include "ui/ui.h"
#include "zippath.h"
#include "ui/filesel.h"
#include "ui/swlist.h"
#include "ui/imgcntrl.h"

//**************************************************************************
//  DEVICE CONFIG IMAGE INTERFACE
//**************************************************************************
const image_device_type_info device_image_interface::m_device_info_array[] =
	{
		{ IO_UNKNOWN,   "unknown",      "unkn" },
		{ IO_CARTSLOT,  "cartridge",    "cart" }, /*  0 */
		{ IO_FLOPPY,    "floppydisk",   "flop" }, /*  1 */
		{ IO_HARDDISK,  "harddisk",     "hard" }, /*  2 */
		{ IO_CYLINDER,  "cylinder",     "cyln" }, /*  3 */
		{ IO_CASSETTE,  "cassette",     "cass" }, /*  4 */
		{ IO_PUNCHCARD, "punchcard",    "pcrd" }, /*  5 */
		{ IO_PUNCHTAPE, "punchtape",    "ptap" }, /*  6 */
		{ IO_PRINTER,   "printer",      "prin" }, /*  7 */
		{ IO_SERIAL,    "serial",       "serl" }, /*  8 */
		{ IO_PARALLEL,  "parallel",     "parl" }, /*  9 */
		{ IO_SNAPSHOT,  "snapshot",     "dump" }, /* 10 */
		{ IO_QUICKLOAD, "quickload",    "quik" }, /* 11 */
		{ IO_MEMCARD,   "memcard",      "memc" }, /* 12 */
		{ IO_CDROM,     "cdrom",        "cdrm" }, /* 13 */
		{ IO_MAGTAPE,   "magtape",      "magt" }, /* 14 */
		{ IO_ROM,       "romimage",     "rom"  }, /* 15 */
		{ IO_MIDIIN,    "midiin",       "min"  }, /* 16 */
		{ IO_MIDIOUT,   "midiout",      "mout" }  /* 17 */
	};


//**************************************************************************
//  DEVICE IMAGE INTERFACE
//**************************************************************************

//-------------------------------------------------
//  device_image_interface - constructor
//-------------------------------------------------

device_image_interface::device_image_interface(const machine_config &mconfig, device_t &device)
	: device_interface(device),
		m_file(NULL),
		m_mame_file(NULL),
		m_software_info_ptr(NULL),
		m_software_part_ptr(NULL),
		m_readonly(false),
		m_created(false),
		m_is_loading(FALSE)
{
}


//-------------------------------------------------
//  ~device_image_interface - destructor
//-------------------------------------------------

device_image_interface::~device_image_interface()
{
}

//-------------------------------------------------
//  find_device_type - search trough list of
//  device types to extract data
//-------------------------------------------------

const image_device_type_info *device_image_interface::find_device_type(iodevice_t type)
{
	int i;
	for (i = 0; i < ARRAY_LENGTH(device_image_interface::m_device_info_array); i++)
	{
		if (m_device_info_array[i].m_type == type)
			return &m_device_info_array[i];
	}
	return NULL;
}

//-------------------------------------------------
//  device_typename - retrieves device type name
//-------------------------------------------------

const char *device_image_interface::device_typename(iodevice_t type)
{
	const image_device_type_info *info = find_device_type(type);
	return (info != NULL) ? info->m_name : NULL;
}

//-------------------------------------------------
//  device_brieftypename - retrieves device
//  brief type name
//-------------------------------------------------

const char *device_image_interface::device_brieftypename(iodevice_t type)
{
	const image_device_type_info *info = find_device_type(type);
	return (info != NULL) ? info->m_shortname : NULL;
}

//-------------------------------------------------
//  device_typeid - retrieves device type id
//-------------------------------------------------

iodevice_t device_image_interface::device_typeid(const char *name)
{
	int i;
	for (i = 0; i < ARRAY_LENGTH(device_image_interface::m_device_info_array); i++)
	{
		if (!core_stricmp(name, m_device_info_array[i].m_name) || !core_stricmp(name, m_device_info_array[i].m_shortname))
			return m_device_info_array[i].m_type;
	}
	return (iodevice_t)-1;
}

/*-------------------------------------------------
    device_compute_hash - compute a hash,
    using this device's partial hash if appropriate
-------------------------------------------------*/

void device_image_interface::device_compute_hash(hash_collection &hashes, const void *data, size_t length, const char *types) const
{
	/* retrieve the partial hash func */
	device_image_partialhash_func partialhash = get_partial_hash();

	/* compute the hash */
	if (partialhash)
		partialhash(hashes, (const unsigned char*)data, length, types);
	else
		hashes.compute(reinterpret_cast<const UINT8 *>(data), length, types);
}

/*-------------------------------------------------
    set_image_filename - specifies the filename of
    an image
-------------------------------------------------*/

image_error_t device_image_interface::set_image_filename(const char *filename)
{
	m_image_name = filename;
	zippath_parent(m_working_directory, filename);
	m_basename.cpy(m_image_name);

	int loc1 = m_image_name.rchr(0,'\\');
	int loc2 = m_image_name.rchr(0,'/');
	int loc3 = m_image_name.rchr(0,':');
	int loc = MAX(loc1,MAX(loc2,loc3));
	if (loc!=-1) {
		if (loc == loc3)
		{
			// temp workaround for softlists now that m_image_name contains the part name too (e.g. list:gamename:cart)
			astring tmpstr = astring(m_basename.substr(0,loc));
			int tmploc = tmpstr.rchr(0,':');
			m_basename = m_basename.substr(tmploc + 1,loc-tmploc);
		}
		else
			m_basename = m_basename.substr(loc + 1,m_basename.len()-loc);
	}
	m_basename_noext = m_basename.cpy(m_basename);
	m_filetype = "";
	loc = m_basename_noext.rchr(0,'.');
	if (loc!=-1) {
		m_basename_noext = m_basename_noext.substr(0,loc);
		m_filetype = m_basename.cpy(m_basename);
		m_filetype = m_filetype.substr(loc + 1,m_filetype.len()-loc);
	}

	return IMAGE_ERROR_SUCCESS;
}

/****************************************************************************
    CREATION FORMATS
****************************************************************************/

/*-------------------------------------------------
    device_get_named_creatable_format -
    accesses a specific image format available for
    image creation by name
-------------------------------------------------*/

const image_device_format *device_image_interface::device_get_named_creatable_format(const char *format_name)
{
	for (const image_device_format *format = m_formatlist.first(); format != NULL; format = format->next())
		if (strcmp(format->name(), format_name) == 0)
			return format;
	return NULL;
}

/****************************************************************************
    ERROR HANDLING
****************************************************************************/

/*-------------------------------------------------
    image_clear_error - clear out any specified
    error
-------------------------------------------------*/

void device_image_interface::clear_error()
{
	m_err = IMAGE_ERROR_SUCCESS;
	if (m_err_message)
	{
		m_err_message.reset();
	}
}



/*-------------------------------------------------
    error - returns the error text for an image
    error
-------------------------------------------------*/
static const char *const messages[] =
{
	"",
	"Internal error",
	"Unsupported operation",
	"Out of memory",
	"File not found",
	"Invalid image",
	"File already open",
	"Unspecified error"
};

const char *device_image_interface::error()
{
	return (m_err_message) ? m_err_message.cstr() : messages[m_err];
}



/*-------------------------------------------------
    seterror - specifies an error on an image
-------------------------------------------------*/

void device_image_interface::seterror(image_error_t err, const char *message)
{
	clear_error();
	m_err = err;
	if (message != NULL)
	{
		m_err_message = message;
	}
}



/*-------------------------------------------------
    message - used to display a message while
    loading
-------------------------------------------------*/

void device_image_interface::message(const char *format, ...)
{
	va_list args;
	char buffer[256];

	/* format the message */
	va_start(args, format);
	vsnprintf(buffer, ARRAY_LENGTH(buffer), format, args);
	va_end(args);

	/* display the popup for a standard amount of time */
	device().machine().ui().popup_time(5, "%s: %s",
		basename(),
		buffer);
}


/***************************************************************************
    WORKING DIRECTORIES
***************************************************************************/

/*-------------------------------------------------
    try_change_working_directory - tries to change
    the working directory, but only if the directory
    actually exists
-------------------------------------------------*/
bool device_image_interface::try_change_working_directory(const char *subdir)
{
	osd_directory *directory;
	const osd_directory_entry *entry;
	bool success = FALSE;
	bool done = FALSE;

	directory = osd_opendir(m_working_directory.cstr());
	if (directory != NULL)
	{
		while(!done && (entry = osd_readdir(directory)) != NULL)
		{
			if (!core_stricmp(subdir, entry->name))
			{
				done = TRUE;
				success = entry->type == ENTTYPE_DIR;
			}
		}

		osd_closedir(directory);
	}

	/* did we successfully identify the directory? */
	if (success)
		zippath_combine(m_working_directory, m_working_directory, subdir);

	return success;
}
/*-------------------------------------------------
    setup_working_directory - sets up the working
    directory according to a few defaults
-------------------------------------------------*/

void device_image_interface::setup_working_directory()
{
	char *dst = NULL;

	osd_get_full_path(&dst,".");
	/* first set up the working directory to be the starting directory */
	m_working_directory = dst;

	/* now try browsing down to "software" */
	if (try_change_working_directory("software"))
	{
		/* now down to a directory for this computer */
		int gamedrv = driver_list::find(device().machine().system());
		while(gamedrv != -1 && !try_change_working_directory(driver_list::driver(gamedrv).name))
		{
			gamedrv = driver_list::compatible_with(gamedrv);
		}
	}
	osd_free(dst);
}

//-------------------------------------------------
//  working_directory - returns the working
//  directory to use for this image; this is
//  valid even if not mounted
//-------------------------------------------------

const char * device_image_interface::working_directory()
{
	/* check to see if we've never initialized the working directory */
	if (!m_working_directory)
		setup_working_directory();

	return m_working_directory;
}


/*-------------------------------------------------
    get_software_region
-------------------------------------------------*/

UINT8 *device_image_interface::get_software_region(const char *tag)
{
	char full_tag[256];

	if ( m_software_info_ptr == NULL || m_software_part_ptr == NULL )
		return NULL;

	sprintf( full_tag, "%s:%s", device().tag(), tag );
	return device().machine().root_device().memregion( full_tag )->base();
}


/*-------------------------------------------------
    image_get_software_region_length
-------------------------------------------------*/

UINT32 device_image_interface::get_software_region_length(const char *tag)
{
	char full_tag[256];

	sprintf( full_tag, "%s:%s", device().tag(), tag );
	return device().machine().root_device().memregion( full_tag )->bytes();
}


/*-------------------------------------------------
 image_get_feature
 -------------------------------------------------*/

const char *device_image_interface::get_feature(const char *feature_name)
{
	return (m_software_part_ptr == NULL) ? NULL : m_software_part_ptr->feature(feature_name);
}


//-------------------------------------------------
//  load_software_region -
//-------------------------------------------------

bool device_image_interface::load_software_region(const char *tag, optional_shared_ptr<UINT8> &ptr)
{
	size_t size = get_software_region_length(tag);

	if (size)
	{
		ptr.allocate(size);

		memcpy(ptr, get_software_region(tag), size);
	}

	return size > 0;
}

/****************************************************************************
  Hash info loading

  If the hash is not checked and the relevant info not loaded, force that info
  to be loaded
****************************************************************************/

void device_image_interface::run_hash(void (*partialhash)(hash_collection &, const unsigned char *, unsigned long, const char *),
	hash_collection &hashes, const char *types)
{
	UINT32 size;
	dynamic_buffer buf;

	hashes.reset();
	size = (UINT32) length();

	buf.resize_and_clear(size);

	/* read the file */
	fseek(0, SEEK_SET);
	fread(buf, size);

	if (partialhash)
		partialhash(hashes, buf, size, types);
	else
		hashes.compute(buf, size, types);

	/* cleanup */
	fseek(0, SEEK_SET);
}



void device_image_interface::image_checkhash()
{
	device_image_partialhash_func partialhash;

	/* only calculate CRC if it hasn't been calculated, and the open_mode is read only */
	UINT32 crcval;
	if (!m_hash.crc(crcval) && m_readonly && !m_created)
	{
		/* do not cause a linear read of 600 megs please */
		/* TODO: use SHA1 in the CHD header as the hash */
		if (image_type() == IO_CDROM)
			return;

		/* Skip calculating the hash when we have an image mounted through a software list */
		if ( m_software_info_ptr )
			return;

		/* retrieve the partial hash func */
		partialhash = get_partial_hash();

		run_hash(partialhash, m_hash, hash_collection::HASH_TYPES_ALL);
	}
	return;
}

UINT32 device_image_interface::crc()
{
	UINT32 crc = 0;

	image_checkhash();
	m_hash.crc(crc);

	return crc;
}

/****************************************************************************
  Battery functions

  These functions provide transparent access to battery-backed RAM on an
  image; typically for cartridges.
****************************************************************************/


/*-------------------------------------------------
    battery_load - retrieves the battery
    backed RAM for an image. The file name is
    created from the machine driver name and the
    image name.
-------------------------------------------------*/
void device_image_interface::battery_load(void *buffer, int length, int fill)
{
	astring fname(device().machine().system().name, PATH_SEPARATOR, m_basename_noext, ".nv");
	image_battery_load_by_name(device().machine().options(), fname, buffer, length, fill);
}

void device_image_interface::battery_load(void *buffer, int length, void *def_buffer)
{
	astring fname(device().machine().system().name, PATH_SEPARATOR, m_basename_noext, ".nv");
	image_battery_load_by_name(device().machine().options(), fname, buffer, length, def_buffer);
}

/*-------------------------------------------------
    battery_save - stores the battery
    backed RAM for an image. The file name is
    created from the machine driver name and the
    image name.
-------------------------------------------------*/
void device_image_interface::battery_save(const void *buffer, int length)
{
	astring fname(device().machine().system().name, PATH_SEPARATOR, m_basename_noext, ".nv");

	image_battery_save_by_name(device().machine().options(), fname, buffer, length);
}

//-------------------------------------------------
//  uses_file_extension - update configuration
//  based on completed device setup
//-------------------------------------------------

bool device_image_interface::uses_file_extension(const char *file_extension) const
{
	bool result = FALSE;

	if (file_extension[0] == '.')
		file_extension++;

	/* find the extensions */
	astring extensions(file_extensions());
	char *ext = strtok((char*)extensions.cstr(),",");
	while (ext != NULL)
	{
		if (!core_stricmp(ext, file_extension))
		{
			result = TRUE;
			break;
		}
		ext = strtok (NULL, ",");
	}
	return result;
}

/****************************************************************************
    IMAGE LOADING
****************************************************************************/

/*-------------------------------------------------
    is_loaded - quick check to determine whether an
    image is loaded
-------------------------------------------------*/

bool device_image_interface::is_loaded()
{
	return (m_file != NULL);
}

/*-------------------------------------------------
    load_image_by_path - loads an image with a
    specific path
-------------------------------------------------*/

image_error_t device_image_interface::load_image_by_path(UINT32 open_flags, const char *path)
{
	file_error filerr = FILERR_NOT_FOUND;
	image_error_t err = IMAGE_ERROR_FILENOTFOUND;
	astring revised_path;

	/* attempt to read the file */
	filerr = zippath_fopen(path, open_flags, m_file, revised_path);

	/* did the open succeed? */
	switch(filerr)
	{
		case FILERR_NONE:
			/* success! */
			m_readonly = (open_flags & OPEN_FLAG_WRITE) ? 0 : 1;
			m_created = (open_flags & OPEN_FLAG_CREATE) ? 1 : 0;
			err = IMAGE_ERROR_SUCCESS;
			break;

		case FILERR_NOT_FOUND:
		case FILERR_ACCESS_DENIED:
			/* file not found (or otherwise cannot open); continue */
			err = IMAGE_ERROR_FILENOTFOUND;
			break;

		case FILERR_OUT_OF_MEMORY:
			/* out of memory */
			err = IMAGE_ERROR_OUTOFMEMORY;
			break;

		case FILERR_ALREADY_OPEN:
			/* this shouldn't happen */
			err = IMAGE_ERROR_ALREADYOPEN;
			break;

		case FILERR_FAILURE:
		case FILERR_TOO_MANY_FILES:
		case FILERR_INVALID_DATA:
		default:
			/* other errors */
			err = IMAGE_ERROR_INTERNAL;
			break;
	}

	/* if successful, set the file name */
	if (filerr == FILERR_NONE)
		set_image_filename(revised_path);

	return err;
}

int device_image_interface::reopen_for_write(const char *path)
{
	if(m_file)
		core_fclose(m_file);

	file_error filerr = FILERR_NOT_FOUND;
	image_error_t err = IMAGE_ERROR_FILENOTFOUND;
	astring revised_path;

	/* attempt to open the file for writing*/
	filerr = zippath_fopen(path, OPEN_FLAG_READ|OPEN_FLAG_WRITE|OPEN_FLAG_CREATE, m_file, revised_path);

	/* did the open succeed? */
	switch(filerr)
	{
		case FILERR_NONE:
			/* success! */
			m_readonly = 0;
			m_created = 1;
			err = IMAGE_ERROR_SUCCESS;
			break;

		case FILERR_NOT_FOUND:
		case FILERR_ACCESS_DENIED:
			/* file not found (or otherwise cannot open); continue */
			err = IMAGE_ERROR_FILENOTFOUND;
			break;

		case FILERR_OUT_OF_MEMORY:
			/* out of memory */
			err = IMAGE_ERROR_OUTOFMEMORY;
			break;

		case FILERR_ALREADY_OPEN:
			/* this shouldn't happen */
			err = IMAGE_ERROR_ALREADYOPEN;
			break;

		case FILERR_FAILURE:
		case FILERR_TOO_MANY_FILES:
		case FILERR_INVALID_DATA:
		default:
			/* other errors */
			err = IMAGE_ERROR_INTERNAL;
			break;
	}

	/* if successful, set the file name */
	if (filerr == FILERR_NONE)
		set_image_filename(revised_path);

	return err;
}

/*-------------------------------------------------
    determine_open_plan - determines which open
    flags to use, and in what order
-------------------------------------------------*/

void device_image_interface::determine_open_plan(int is_create, UINT32 *open_plan)
{
	int i = 0;

	/* emit flags */
	if (!is_create && is_readable() && is_writeable())
		open_plan[i++] = OPEN_FLAG_READ | OPEN_FLAG_WRITE;
	if (!is_create && !is_readable() && is_writeable())
		open_plan[i++] = OPEN_FLAG_WRITE;
	if (!is_create && is_readable())
		open_plan[i++] = OPEN_FLAG_READ;
	if (is_writeable() && is_creatable())
		open_plan[i++] = OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE;
	open_plan[i] = 0;
}

/*-------------------------------------------------
    dump_wrong_and_correct_checksums - dump an
    error message containing the wrong and the
    correct checksums for a given software item
-------------------------------------------------*/

static void dump_wrong_and_correct_checksums(const hash_collection &hashes, const hash_collection &acthashes)
{
	astring tempstr;
	osd_printf_error("    EXPECTED: %s\n", hashes.macro_string(tempstr));
	osd_printf_error("       FOUND: %s\n", acthashes.macro_string(tempstr));
}

/*-------------------------------------------------
    verify_length_and_hash - verify the length
    and hash signatures of a file
-------------------------------------------------*/

static int verify_length_and_hash(emu_file *file, const char *name, UINT32 explength, const hash_collection &hashes)
{
	int retVal = 0;
	if (file==NULL) return 0;

	/* verify length */
	UINT32 actlength = file->size();
	if (explength != actlength)
	{
		osd_printf_error("%s WRONG LENGTH (expected: %d found: %d)\n", name, explength, actlength);
		retVal++;
	}

	/* If there is no good dump known, write it */
	astring tempstr;
	hash_collection &acthashes = file->hashes(hashes.hash_types(tempstr));
	if (hashes.flag(hash_collection::FLAG_NO_DUMP))
	{
		osd_printf_error("%s NO GOOD DUMP KNOWN\n", name);
	}
	/* verify checksums */
	else if (hashes != acthashes)
	{
		/* otherwise, it's just bad */
		osd_printf_error("%s WRONG CHECKSUMS:\n", name);
		dump_wrong_and_correct_checksums(hashes, acthashes);
		retVal++;
	}
	/* If it matches, but it is actually a bad dump, write it */
	else if (hashes.flag(hash_collection::FLAG_BAD_DUMP))
	{
		osd_printf_error("%s NEEDS REDUMP\n",name);
	}
	return retVal;
}

/*-------------------------------------------------
    load_software - software image loading
-------------------------------------------------*/

bool device_image_interface::load_software(software_list_device &swlist, const char *swname, const rom_entry *start)
{
	astring locationtag, breakstr("%");
	const rom_entry *region;
	astring regiontag;
	bool retVal = FALSE;
	int warningcount = 0;
	for (region = start; region != NULL; region = rom_next_region(region))
	{
		/* loop until we hit the end of this region */
		const rom_entry *romp = region + 1;
		while (!ROMENTRY_ISREGIONEND(romp))
		{
			/* handle files */
			if (ROMENTRY_ISFILE(romp))
			{
				file_error filerr = FILERR_NOT_FOUND;

				UINT32 crc = 0;
				bool has_crc = hash_collection(ROM_GETHASHDATA(romp)).crc(crc);

				software_info *swinfo = swlist.find(swname);
				if (swinfo == NULL)
					return false;

				UINT32 supported = swinfo->supported();
				if (supported == SOFTWARE_SUPPORTED_PARTIAL)
					osd_printf_error("WARNING: support for software %s (in list %s) is only partial\n", swname, swlist.list_name());
				if (supported == SOFTWARE_SUPPORTED_NO)
					osd_printf_error("WARNING: support for software %s (in list %s) is only preliminary\n", swname, swlist.list_name());

				// attempt reading up the chain through the parents and create a locationtag astring in the format
				// " swlist % clonename % parentname "
				// below, we have the code to split the elements and to create paths to load from

				while (swinfo != NULL)
				{
					locationtag.cat(swinfo->shortname()).cat(breakstr);
					const char *parentname = swinfo->parentname();
					swinfo = (parentname != NULL) ? swlist.find(parentname) : NULL;
				}
				// strip the final '%'
				locationtag.del(locationtag.len() - 1, 1);


				// check if locationtag actually contains two locations separated by '%'
				// (i.e. check if we are dealing with a clone in softwarelist)
				astring tag2, tag3, tag4(locationtag), tag5;
				int separator = tag4.chr(0, '%');
				if (separator != -1)
				{
					// we are loading a clone through softlists, split the setname from the parentname
					tag5.cpysubstr(tag4, separator + 1, tag4.len() - separator + 1);
					tag4.del(separator, tag4.len() - separator);
				}

				// prepare locations where we have to load from: list/parentname & list/clonename
				astring tag1(swlist.list_name());
				tag1.cat(PATH_SEPARATOR);
				tag2.cpy(tag1.cat(tag4));
				tag1.cpy(swlist.list_name());
				tag1.cat(PATH_SEPARATOR);
				tag3.cpy(tag1.cat(tag5));

				if (tag5.chr(0, '%') != -1)
					fatalerror("We do not support clones of clones!\n");

				// try to load from the available location(s):
				// - if we are not using lists, we have regiontag only;
				// - if we are using lists, we have: list/clonename, list/parentname, clonename, parentname
				// try to load from list/setname
				if ((m_mame_file == NULL) && (tag2.cstr() != NULL))
					filerr = common_process_file(device().machine().options(), tag2.cstr(), has_crc, crc, romp, &m_mame_file);
				// try to load from list/parentname
				if ((m_mame_file == NULL) && (tag3.cstr() != NULL))
					filerr = common_process_file(device().machine().options(), tag3.cstr(), has_crc, crc, romp, &m_mame_file);
				// try to load from setname
				if ((m_mame_file == NULL) && (tag4.cstr() != NULL))
					filerr = common_process_file(device().machine().options(), tag4.cstr(), has_crc, crc, romp, &m_mame_file);
				// try to load from parentname
				if ((m_mame_file == NULL) && (tag5.cstr() != NULL))
					filerr = common_process_file(device().machine().options(), tag5.cstr(), has_crc, crc, romp, &m_mame_file);

				warningcount += verify_length_and_hash(m_mame_file,ROM_GETNAME(romp),ROM_GETLENGTH(romp),hash_collection(ROM_GETHASHDATA(romp)));

				if (filerr == FILERR_NONE)
				{
					m_file = *m_mame_file;
					retVal = TRUE;
				}

				break; // load first item for start
			}
			romp++; /* something else; skip */
		}
	}
	if (warningcount > 0)
	{
		osd_printf_error("WARNING: the software item might not run correctly.\n");
	}
	return retVal;
}

/*-------------------------------------------------
    load_internal - core image loading
-------------------------------------------------*/

bool device_image_interface::load_internal(const char *path, bool is_create, int create_format, option_resolution *create_args, bool just_load)
{
	UINT32 open_plan[4];
	int i;
	bool softload = FALSE;
	m_from_swlist = FALSE;

	// if the path contains no period, we are using softlists, so we won't create an image
	astring pathstr(path);
	bool filename_has_period = (pathstr.rchr(0, '.') != -1) ? TRUE : FALSE;

	/* first unload the image */
	unload();

	/* clear any possible error messages */
	clear_error();

	/* we are now loading */
	m_is_loading = TRUE;

	/* record the filename */
	m_err = set_image_filename(path);

	if (m_err)
		goto done;

	if (core_opens_image_file())
	{
		/* Check if there's a software list defined for this device and use that if we're not creating an image */
		if (!filename_has_period && !just_load)
		{
			softload = load_software_part(path, m_software_part_ptr);
			if (softload)
			{
				m_software_info_ptr = &m_software_part_ptr->info();
				m_software_list_name.cpy(m_software_info_ptr->list().list_name());
				m_full_software_name.cpy(m_software_part_ptr->info().shortname());

				// if we had launched from softlist with a specified part, e.g. "shortname:part"
				// we would have recorded the wrong name, so record it again based on software_info
				if (m_software_info_ptr && m_full_software_name)
					m_err = set_image_filename(m_full_software_name);

				// check if image should be read-only
				const char *read_only = get_feature("read_only");
				if (read_only && !strcmp(read_only, "true")) {
					make_readonly();
				}

				m_from_swlist = TRUE;
			}
		}

		if (is_create || filename_has_period)
		{
			/* determine open plan */
			determine_open_plan(is_create, open_plan);

			/* attempt to open the file in various ways */
			for (i = 0; !m_file && open_plan[i]; i++)
			{
				/* open the file */
				m_err = load_image_by_path(open_plan[i], path);
				if (m_err && (m_err != IMAGE_ERROR_FILENOTFOUND))
					goto done;
			}
		}

		/* Copy some image information when we have been loaded through a software list */
		if ( m_software_info_ptr )
		{
			// sanitize
			if (m_software_info_ptr->longname() == NULL || m_software_info_ptr->publisher() == NULL || m_software_info_ptr->year() == NULL)
				fatalerror("Each entry in an XML list must have all of the following fields: description, publisher, year!\n");

			// store
			m_longname = m_software_info_ptr->longname();
			m_manufacturer = m_software_info_ptr->publisher();
			m_year = m_software_info_ptr->year();
			//m_playable = m_software_info_ptr->supported();
		}

		/* did we fail to find the file? */
		if (!is_loaded() && !softload)
		{
			m_err = IMAGE_ERROR_FILENOTFOUND;
			goto done;
		}
	}

	/* call device load or create */
	m_create_format = create_format;
	m_create_args = create_args;

	if (m_init_phase==FALSE) {
		m_err = (image_error_t)finish_load();
		if (m_err)
			goto done;
	}
	/* success! */

done:
	if (just_load) {
		if(m_err) clear();
		return m_err ? IMAGE_INIT_FAIL : IMAGE_INIT_PASS;
	}
	if (m_err!=0) {
		if (!m_init_phase)
		{
			if (device().machine().phase() == MACHINE_PHASE_RUNNING)
				popmessage("Error: Unable to %s image '%s': %s", is_create ? "create" : "load", path, error());
			else
				osd_printf_error("Error: Unable to %s image '%s': %s\n", is_create ? "create" : "load", path, error());
		}
		clear();
	}
	else {
		/* do we need to reset the CPU? only schedule it if load/create is successful */
		if (device().machine().time() > attotime::zero && is_reset_on_load())
			device().machine().schedule_hard_reset();
		else
		{
			if (!m_init_phase)
			{
				if (device().machine().phase() == MACHINE_PHASE_RUNNING)
					popmessage("Image '%s' was successfully %s.", path, is_create ? "created" : "loaded");
				else
					osd_printf_info("Image '%s' was successfully %s.\n", path, is_create ? "created" : "loaded");
			}
		}
	}
	return m_err ? IMAGE_INIT_FAIL : IMAGE_INIT_PASS;
}



/*-------------------------------------------------
    load - load an image into MESS
-------------------------------------------------*/

bool device_image_interface::load(const char *path)
{
	return load_internal(path, FALSE, 0, NULL, FALSE);
}

/*-------------------------------------------------
    open_image_file - opening plain image file
-------------------------------------------------*/

bool device_image_interface::open_image_file(emu_options &options)
{
	const char* path = options.value(instance_name());
	if (*path != 0)
	{
		set_init_phase();
		if (load_internal(path, FALSE, 0, NULL, TRUE)==IMAGE_INIT_PASS)
		{
			if (software_entry()==NULL) return true;
		}
	}
	return false;
}

/*-------------------------------------------------
    image_finish_load - special call - only use
    from core
-------------------------------------------------*/

bool device_image_interface::finish_load()
{
	bool err = IMAGE_INIT_PASS;

	if (m_is_loading)
	{
		image_checkhash();

		if (has_been_created())
		{
			err = call_create(m_create_format, m_create_args);
			if (err)
			{
				if (!m_err)
					m_err = IMAGE_ERROR_UNSPECIFIED;
			}
		}
		else
		{
			/* using device load */
			err = call_load();
			if (err)
			{
				if (!m_err)
					m_err = IMAGE_ERROR_UNSPECIFIED;
			}
		}
	}
	m_is_loading = FALSE;
	m_create_format = 0;
	m_create_args = NULL;
	m_init_phase = FALSE;
	return err;
}

/*-------------------------------------------------
    create - create a image
-------------------------------------------------*/

bool device_image_interface::create(const char *path, const image_device_format *create_format, option_resolution *create_args)
{
	int format_index = (create_format != NULL) ? m_formatlist.indexof(*create_format) : 0;
	return load_internal(path, TRUE, format_index, create_args, FALSE);
}


/*-------------------------------------------------
    clear - clear all internal data pertaining
    to an image
-------------------------------------------------*/

void device_image_interface::clear()
{
	if (m_mame_file)
	{
		global_free(m_mame_file);
		m_mame_file = NULL;
		m_file = NULL;
	} else {
		if (m_file)
		{
			core_fclose(m_file);
			m_file = NULL;
		}
	}

	m_image_name.reset();
	m_readonly = false;
	m_created = false;

	m_longname.reset();
	m_manufacturer.reset();
	m_year.reset();
	m_basename.reset();
	m_basename_noext.reset();
	m_filetype.reset();

	m_full_software_name.reset();
	m_software_info_ptr = NULL;
	m_software_part_ptr = NULL;
	m_software_list_name.reset();
}

/*-------------------------------------------------
    unload - main call to unload an image
-------------------------------------------------*/

void device_image_interface::unload()
{
	if (is_loaded() || m_software_info_ptr)
	{
		call_unload();
	}
	clear();
	clear_error();
}

/*-------------------------------------------------
    update_names - update brief and instance names
-------------------------------------------------*/

void device_image_interface::update_names(const device_type device_type, const char *inst, const char *brief)
{
	image_interface_iterator iter(device().mconfig().root_device());
	int count = 0;
	int index = -1;
	for (const device_image_interface *image = iter.first(); image != NULL; image = iter.next())
	{
		if (this == image)
			index = count;
		if ((image->image_type() == image_type() && device_type==NULL) || (device_type==image->device().type()))
			count++;
	}
	const char *inst_name = (device_type!=NULL) ? inst : device_typename(image_type());
	const char *brief_name = (device_type!=NULL) ? brief : device_brieftypename(image_type());
	if (count > 1)
	{
		m_instance_name.printf("%s%d", inst_name , index + 1);
		m_brief_instance_name.printf("%s%d", brief_name, index + 1);
	}
	else
	{
		m_instance_name = inst_name;
		m_brief_instance_name = brief_name;
	}
}

//-------------------------------------------------
//  software_name_split - helper that splits a
//  software_list:software:part string into
//  separate software_list, software, and part
//  strings.
//
//  str1:str2:str3  => swlist_name - str1, swname - str2, swpart - str3
//  str1:str2       => swlist_name - NULL, swname - str1, swpart - str2
//  str1            => swlist_name - NULL, swname - str1, swpart - NULL
//
//  Notice however that we could also have been
//  passed a string swlist_name:swname, and thus
//  some special check has to be performed in this
//  case.
//-------------------------------------------------

void device_image_interface::software_name_split(const char *swlist_swname, astring &swlist_name, astring &swname, astring &swpart)
{
	// reset all output parameters
	swlist_name.reset();
	swname.reset();
	swpart.reset();

	// if no colon, this is the swname by itself
	const char *split1 = strchr(swlist_swname, ':');
	if (split1 == NULL)
	{
		swname.cpy(swlist_swname);
		return;
	}

	// if one colon, it is the swname and swpart alone
	const char *split2 = strchr(split1 + 1, ':');
	if (split2 == NULL)
	{
		swname.cpy(swlist_swname, split1 - swlist_swname);
		swpart.cpy(split1 + 1);
		return;
	}

	// if two colons present, split into 3 parts
	swlist_name.cpy(swlist_swname, split1 - swlist_swname);
	swname.cpy(split1 + 1, split2 - (split1 + 1));
	swpart.cpy(split2 + 1);
}


software_part *device_image_interface::find_software_item(const char *path, bool restrict_to_interface)
{
	// split full software name into software list name and short software name
	astring swlist_name, swinfo_name, swpart_name;
	software_name_split(path, swlist_name, swinfo_name, swpart_name);

	// determine interface
	const char *interface = NULL;
	if (restrict_to_interface)
		interface = image_interface();

	// find the software list if explicitly specified
	software_list_device_iterator deviter(device().mconfig().root_device());
	for (software_list_device *swlistdev = deviter.first(); swlistdev != NULL; swlistdev = deviter.next())
	{
		if (swlist_name == swlistdev->list_name() || !(swlist_name.len() > 0))
		{
			software_info *info = swlistdev->find(swinfo_name);
			if (info != NULL)
			{
				software_part *part = info->find_part(swpart_name, interface);
				if (part != NULL)
					return part;
			}
		}

		if (swinfo_name == swlistdev->list_name())
		{
			// ad hoc handling for the case path = swlist_name:swinfo_name (e.g. 
			// gameboy:sml) which is not handled properly by software_name_split 
			// since the function cannot distinguish between this and the case 
			// path = swinfo_name:swpart_name
			software_info *info = swlistdev->find(swpart_name);
			if (info != NULL)
			{
				software_part *part = info->find_part(NULL, interface);
				if (part != NULL)
					return part;
			}
		}
	}

	// if explicitly specified and not found, just error here
	return NULL;
}


//-------------------------------------------------
//  load_software_part
//
//  Load a software part for a device. The part to
//  load is determined by the "path", software lists
//  configured for a driver, and the interface
//  supported by the device.
//
//  returns true if the software could be loaded,
//  false otherwise. If the software could be loaded
//  sw_info and sw_part are also set.
//-------------------------------------------------

bool device_image_interface::load_software_part(const char *path, software_part *&swpart)
{
	// if no match has been found, we suggest similar shortnames
	swpart = find_software_item(path, true);
	if (swpart == NULL)
	{
		software_list_device::display_matches(device().machine().config(), image_interface(), path);
		return false;
	}

	// Load the software part
	bool result = call_softlist_load(swpart->info().list(), swpart->info().shortname(), swpart->romdata());

	// Tell the world which part we actually loaded
	astring full_sw_name;
	full_sw_name.printf("%s:%s:%s", swpart->info().list().list_name(), swpart->info().shortname(), swpart->name());

	// check compatibility
	if (!swpart->is_compatible(swpart->info().list()))
		osd_printf_warning("WARNING! the set %s might not work on this system due to missing filter(s) '%s'\n", swpart->info().shortname(), swpart->info().list().filter());

	// check requirements and load those images
	const char *requirement = swpart->feature("requirement");
	if (requirement != NULL)
	{
		software_part *req_swpart = find_software_item(requirement, false);
		if (req_swpart != NULL)
		{
			image_interface_iterator imgiter(device().machine().root_device());
			for (device_image_interface *req_image = imgiter.first(); req_image != NULL; req_image = imgiter.next())
			{
				const char *interface = req_image->image_interface();
				if (interface != NULL)
				{
					if (req_swpart->matches_interface(interface))
					{
						const char *option = device().mconfig().options().value(req_image->brief_instance_name());
						// mount only if not already mounted
						if (strlen(option) == 0 && !req_image->filename())
						{
							req_image->set_init_phase();
							req_image->load(requirement);
						}
						break;
					}
				}
			}
		}
	}
	return result;
}

//-------------------------------------------------
//  software_get_default_slot
//-------------------------------------------------

void device_image_interface::software_get_default_slot(astring &result, const char *default_card_slot)
{
	const char *path = device().mconfig().options().value(instance_name());
	result.reset();
	if (strlen(path) > 0)
	{
		result.cpy(default_card_slot);
		software_part *swpart = find_software_item(path, true);
		if (swpart != NULL)
		{
			const char *slot = swpart->feature("slot");
			if (slot != NULL)
				result.cpy(slot);
		}
	}
}

/*-------------------------------------------------
    get_selection_menu - create the menu stack
    for ui-level image selection
-------------------------------------------------*/

ui_menu *device_image_interface::get_selection_menu(running_machine &machine, render_container *container)
{
	return auto_alloc_clear(machine, ui_menu_control_device_image(machine, container, this));
}
