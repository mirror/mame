// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    devfind.h

    Device finding template helpers.

***************************************************************************/

#pragma once

#ifndef __EMU_H__
#error Dont include this file directly; include emu.h instead.
#endif

#ifndef __DEVFIND_H__
#define __DEVFIND_H__

#define FINDER_DUMMY_TAG "finder_dummy_tag"

//**************************************************************************
//  IOPORT ARRAY MACROS
//**************************************************************************

// these macros can be used to initialize an ioport_array with
// individual port names, instead of a base name + numeric suffix

#define IOPORT_ARRAY_MEMBER(name) const char * const name[] =
#define DECLARE_IOPORT_ARRAY(name) static const char * const name[]

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> finder_base

// helper class to request auto-object discovery in the constructor of a derived class
class finder_base
{
	friend class device_t;

public:
	// construction/destruction
	finder_base(device_t &base, const char *tag);
	virtual ~finder_base();

	// getters
	virtual bool findit(bool isvalidation = false) = 0;

	// setter for setting the object
	void set_tag(const char *tag) { m_tag = tag; }

protected:
	// helpers
	void *find_memory(UINT8 width, size_t &bytes, bool required);
	bool report_missing(bool found, const char *objname, bool required);

	// internal state
	finder_base *m_next;
	device_t &m_base;
	const char *m_tag;
};


// ======================> object_finder_base

// helper class to find objects of a particular type
template<class _ObjectClass>
class object_finder_base : public finder_base
{
public:
	// construction/destruction
	object_finder_base(device_t &base, const char *tag)
		: finder_base(base, tag),
			m_target(NULL) { }

	// operators to make use transparent
	operator _ObjectClass *() const { return m_target; }
	_ObjectClass *operator->() const { assert(m_target != NULL); return m_target; }

	// getter for explicit fetching
	_ObjectClass *target() const { return m_target; }

	// setter for setting the object
	void set_target(_ObjectClass *target) { m_target = target; }

protected:
	// internal state
	_ObjectClass *m_target;
};


// ======================> device_finder

// device finder template
template<class _DeviceClass, bool _Required>
class device_finder : public object_finder_base<_DeviceClass>
{
public:
	// construction/destruction
	device_finder(device_t &base, const char *tag)
		: object_finder_base<_DeviceClass>(base, tag) { }

	// make reference use transparent as well
	operator _DeviceClass &() { assert(object_finder_base<_DeviceClass>::m_target != NULL); return *object_finder_base<_DeviceClass>::m_target; }

	// finder
	virtual bool findit(bool isvalidation = false)
	{
		device_t *device = this->m_base.subdevice(this->m_tag);
		this->m_target = dynamic_cast<_DeviceClass *>(device);
		if (device != NULL && this->m_target == NULL)
		{
			void osd_printf_warning(const char *format, ...) ATTR_PRINTF(1,2);
			osd_printf_warning("Device '%s' found but is of incorrect type (actual type is %s)\n", this->m_tag, device->name());
		}
		return this->report_missing(this->m_target != NULL, "device", _Required);
	}
};

// optional device finder
template<class _DeviceClass>
class optional_device : public device_finder<_DeviceClass, false>
{
public:
	optional_device(device_t &base, const char *tag = FINDER_DUMMY_TAG) : device_finder<_DeviceClass, false>(base, tag) { }
};

// required devices are similar but throw an error if they are not found
template<class _DeviceClass>
class required_device : public device_finder<_DeviceClass, true>
{
public:
	required_device(device_t &base, const char *tag = FINDER_DUMMY_TAG) : device_finder<_DeviceClass, true>(base, tag) { }
};


// ======================> memory_region_finder

// device finder template
template<bool _Required>
class memory_region_finder : public object_finder_base<memory_region>
{
public:
	// construction/destruction
	memory_region_finder(device_t &base, const char *tag)
		: object_finder_base<memory_region>(base, tag) { }

	// make reference use transparent as well
	operator memory_region &() { assert(object_finder_base<memory_region>::m_target != NULL); return *object_finder_base<memory_region>::m_target; }

	// finder
	virtual bool findit(bool isvalidation = false)
	{
		if (isvalidation) return true;
		m_target = m_base.memregion(m_tag);
		return this->report_missing(m_target != NULL, "memory region", _Required);
	}
};

// optional device finder
class optional_memory_region : public memory_region_finder<false>
{
public:
	optional_memory_region(device_t &base, const char *tag = FINDER_DUMMY_TAG) : memory_region_finder<false>(base, tag) { }
};

// required devices are similar but throw an error if they are not found
class required_memory_region : public memory_region_finder<true>
{
public:
	required_memory_region(device_t &base, const char *tag = FINDER_DUMMY_TAG) : memory_region_finder<true>(base, tag) { }
};


// ======================> memory_bank_finder

// device finder template
template<bool _Required>
class memory_bank_finder : public object_finder_base<memory_bank>
{
public:
	// construction/destruction
	memory_bank_finder(device_t &base, const char *tag)
		: object_finder_base<memory_bank>(base, tag) { }

	// make reference use transparent as well
	operator memory_bank &() { assert(object_finder_base<memory_bank>::m_target != NULL); return *object_finder_base<memory_bank>::m_target; }

	// finder
	virtual bool findit(bool isvalidation = false)
	{
		if (isvalidation) return true;
		m_target = m_base.membank(m_tag);
		return this->report_missing(m_target != NULL, "memory bank", _Required);
	}
};

// optional device finder
class optional_memory_bank : public memory_bank_finder<false>
{
public:
	optional_memory_bank(device_t &base, const char *tag = FINDER_DUMMY_TAG) : memory_bank_finder<false>(base, tag) { }
};

// required devices are similar but throw an error if they are not found
class required_memory_bank : public memory_bank_finder<true>
{
public:
	required_memory_bank(device_t &base, const char *tag = FINDER_DUMMY_TAG) : memory_bank_finder<true>(base, tag) { }
};


// ======================> ioport_finder

// device finder template
template<bool _Required>
class ioport_finder : public object_finder_base<ioport_port>
{
public:
	// construction/destruction
	ioport_finder(device_t &base, const char *tag)
		: object_finder_base<ioport_port>(base, tag) { }

	// make reference use transparent as well
	operator ioport_port &() { assert(object_finder_base<ioport_port>::m_target != NULL); return *object_finder_base<ioport_port>::m_target; }

	// finder
	virtual bool findit(bool isvalidation = false)
	{
		if (isvalidation) return true;
		m_target = m_base.ioport(m_tag);
		return this->report_missing(m_target != NULL, "I/O port", _Required);
	}
};

// optional device finder
class optional_ioport : public ioport_finder<false>
{
public:
	optional_ioport(device_t &base, const char *tag = FINDER_DUMMY_TAG) : ioport_finder<false>(base, tag) { }
};

// required devices are similar but throw an error if they are not found
class required_ioport : public ioport_finder<true>
{
public:
	required_ioport(device_t &base, const char *tag = FINDER_DUMMY_TAG) : ioport_finder<true>(base, tag) { }
};


// ======================> ioport_array_finder

// ioport array finder template
template<int _Count, bool _Required>
class ioport_array_finder
{
	typedef ioport_finder<_Required> ioport_finder_type;

public:
	// construction/destruction
	ioport_array_finder(device_t &base, const char *basetag)
	{
		for (int index = 0; index < _Count; index++)
			m_array[index].reset(global_alloc(ioport_finder_type(base, m_tag[index].format("%s.%d", basetag, index))));
	}

	ioport_array_finder(device_t &base, const char * const *tags)
	{
		for (int index = 0; index < _Count; index++)
			m_array[index].reset(global_alloc(ioport_finder_type(base, tags[index])));
	}

	// array accessors
	const ioport_finder_type &operator[](int index) const { assert(index < _Count); return *m_array[index]; }
	ioport_finder_type &operator[](int index) { assert(index < _Count); return *m_array[index]; }

protected:
	// internal state
	auto_pointer<ioport_finder_type> m_array[_Count];
	astring m_tag[_Count];
};

// optional ioport array finder
template<int _Count>
class optional_ioport_array: public ioport_array_finder<_Count, false>
{
public:
	optional_ioport_array(device_t &base, const char *basetag) : ioport_array_finder<_Count, false>(base, basetag) { }
	optional_ioport_array(device_t &base, const char * const *tags) : ioport_array_finder<_Count, false>(base, tags) { }
};

// required ioport array finder
template<int _Count>
class required_ioport_array: public ioport_array_finder<_Count, true>
{
public:
	required_ioport_array(device_t &base, const char *basetag) : ioport_array_finder<_Count, true>(base, basetag) { }
	required_ioport_array(device_t &base, const char * const *tags) : ioport_array_finder<_Count, true>(base, tags) { }
};


// ======================> shared_ptr_finder

// shared pointer finder template
template<typename _PointerType, bool _Required>
class shared_ptr_finder : public object_finder_base<_PointerType>
{
public:
	// construction/destruction
	shared_ptr_finder(device_t &base, const char *tag, UINT8 width = sizeof(_PointerType) * 8)
		: object_finder_base<_PointerType>(base, tag),
			m_bytes(0),
			m_width(width) { }

	// operators to make use transparent
	_PointerType operator[](int index) const { return this->m_target[index]; }
	_PointerType &operator[](int index) { return this->m_target[index]; }

	// getter for explicit fetching
	UINT32 bytes() const { return m_bytes; }
	UINT32 mask() const { return m_bytes - 1; }

	// setter for setting the object
	void set_target(_PointerType *target, size_t bytes) { this->m_target = target; m_bytes = bytes; }

	// dynamic allocation of a shared pointer
	void allocate(UINT32 entries)
	{
		assert(m_allocated.count() == 0);
		m_allocated.resize(entries);
		this->m_target = m_allocated;
		m_bytes = entries * sizeof(_PointerType);
		this->m_base.save_item(this->m_allocated, this->m_tag);
	}

	// finder
	virtual bool findit(bool isvalidation = false)
	{
		if (isvalidation) return true;
		this->m_target = reinterpret_cast<_PointerType *>(this->find_memory(m_width, m_bytes, _Required));
		return this->report_missing(this->m_target != NULL, "shared pointer", _Required);
	}

protected:
	// internal state
	size_t m_bytes;
	UINT8 m_width;
	dynamic_array<_PointerType> m_allocated;
};

// optional shared pointer finder
template<class _PointerType>
class optional_shared_ptr : public shared_ptr_finder<_PointerType, false>
{
public:
	optional_shared_ptr(device_t &base, const char *tag = FINDER_DUMMY_TAG, UINT8 width = sizeof(_PointerType) * 8) : shared_ptr_finder<_PointerType, false>(base, tag, width) { }
};

// required shared pointer finder
template<class _PointerType>
class required_shared_ptr : public shared_ptr_finder<_PointerType, true>
{
public:
	required_shared_ptr(device_t &base, const char *tag = FINDER_DUMMY_TAG, UINT8 width = sizeof(_PointerType) * 8) : shared_ptr_finder<_PointerType, true>(base, tag, width) { }
};


// ======================> shared_ptr_array_finder

// shared pointer array finder template
template<typename _PointerType, int _Count, bool _Required>
class shared_ptr_array_finder
{
	typedef shared_ptr_finder<_PointerType, _Required> shared_ptr_type;

public:
	// construction/destruction
	shared_ptr_array_finder(device_t &base, const char *basetag, UINT8 width = sizeof(_PointerType) * 8)
	{
		for (int index = 0; index < _Count; index++)
			m_array[index].reset(global_alloc(shared_ptr_type(base, m_tag[index].format("%s.%d", basetag, index), width)));
	}

	// array accessors
	const shared_ptr_type &operator[](int index) const { assert(index < _Count); return *m_array[index]; }
	shared_ptr_type &operator[](int index) { assert(index < _Count); return *m_array[index]; }

protected:
	// internal state
	auto_pointer<shared_ptr_type> m_array[_Count];
	astring m_tag[_Count];
};

// optional shared pointer array finder
template<class _PointerType, int _Count>
class optional_shared_ptr_array : public shared_ptr_array_finder<_PointerType, _Count, false>
{
public:
	optional_shared_ptr_array(device_t &base, const char *tag, UINT8 width = sizeof(_PointerType) * 8) : shared_ptr_array_finder<_PointerType, _Count, false>(base, tag, width) { }
};

// required shared pointer array finder
template<class _PointerType, int _Count>
class required_shared_ptr_array : public shared_ptr_array_finder<_PointerType, _Count, true>
{
public:
	required_shared_ptr_array(device_t &base, const char *tag, UINT8 width = sizeof(_PointerType) * 8) : shared_ptr_array_finder<_PointerType, _Count, true>(base, tag, width) { }
};


#endif  /* __DEVFIND_H__ */
