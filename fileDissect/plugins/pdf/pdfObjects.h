/*
 * Adobe Portable Document Format implementation
 * Joshua J. Drake <jdrake accuvant.com>
 * 
 * pdfObject.h:
 * class declaration for pdfObject
 */
#ifndef __pdfObject_h_
#define __pdfObject_h_

#include "fileDissect.h"
#include <wx/string.h>
#include <wx/treectrl.h>
#include <wx/mstream.h>

#include "pdf_defs.h"


// this defines the most basic pdf object
class pdfObjectBase
{
public:
	pdfObjectBase(void);
	pdfObjectBase(wxTreeItemId &id, wxByte *ptr);
	pdfObjectBase(wxTreeItemId &id, wxByte *ptr, size_t len);

	static void Delete(pdfObjectBase *pObj);

	// internal object type, refers to PDF_OBJ_xxx macros
	pdfObjType m_type;

	// source data information
	wxByte *m_ptr;		// the pointer to the start in the filemap
	size_t m_len;       // the length of this object's source data

	// the node that holds this item in the tree
	wxTreeItemId m_id;
};


class pdfStream : public pdfObjectBase
{
public:
	pdfStream(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_STREAM;
	};
	pdfStream(wxTreeItemId &id, wxByte *ptr, size_t len) : pdfObjectBase(id, ptr, len)
	{
		m_type = PDF_OBJ_STREAM;
	};

	wxMemoryOutputStream m_decoded;
};


#include <wx/hashmap.h>
WX_DECLARE_STRING_HASH_MAP(pdfObjectBase *, pdfDictHashMap);

class pdfDictionary : public pdfObjectBase
{
public:
	~pdfDictionary(void);
	pdfDictionary(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_DICTIONARY;
	};

	pdfDictHashMap m_entries;
};


class pdfIndirect : public pdfObjectBase
{
public:
	~pdfIndirect(void);
	pdfIndirect(unsigned long num, unsigned long offset, unsigned long generation);

	// for processing
	ULONG m_length; // includes obj/endobj stuff

	// data portion
	wxFileOffset m_data_offset;
	BYTE *m_data;
	unsigned long m_datalen; // just the data portion

	// where this object supposedly lives in the file
	wxFileOffset m_offset;

	// indirect object specific data
	unsigned long m_number;
	unsigned long m_generation;

	pdfDictionary *m_dict;
	pdfObjectBase *m_obj;
	pdfStream *m_stream;
};


// used for tracking nested pdf objects (arrays, dictionaries)
#include <wx/list.h>
WX_DECLARE_LIST(pdfObjectBase, pdfObjectList);

class pdfArray : public pdfObjectBase
{
public:
	~pdfArray(void);
	pdfArray(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_ARRAY;
	};

	pdfObjectList m_list;
};


// this one is for storing all objects in the file
WX_DECLARE_STRING_HASH_MAP(pdfObjectList *, pdfObjectsHashMap);


class pdfLiteral : public pdfObjectBase
{
public:
	pdfLiteral(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_LITERAL;
	};
	pdfLiteral(wxTreeItemId &id, wxByte *ptr, size_t len) : pdfObjectBase(id, ptr, len)
	{
		m_type = PDF_OBJ_LITERAL;
		m_value = wxString::From8BitData((const char *)ptr, len);
	};

	wxString m_value;
};


class pdfHexString : public pdfObjectBase
{
public:
	pdfHexString(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_HEXSTRING;
	};
	pdfHexString(wxTreeItemId &id, wxByte *ptr, size_t len) : pdfObjectBase(id, ptr, len)
	{
		m_type = PDF_OBJ_HEXSTRING;
		m_value.From8BitData((const char *)ptr, len);
	};

	wxString m_value;
	wxString m_decoded;
};


class pdfReference : public pdfObjectBase
{
public:
	pdfReference(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_REFERENCE;
		m_refnum = m_refgen = 0xffffffff;
	};
	pdfReference(wxTreeItemId &id, wxByte *ptr, size_t len) : pdfObjectBase(id, ptr, len)
	{
		m_type = PDF_OBJ_REFERENCE;
		m_refnum = m_refgen = 0xffffffff;
	};

	unsigned long m_refnum;
	unsigned long m_refgen;
};


class pdfName : public pdfObjectBase
{
public:
	pdfName(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_NAME;
	};
	pdfName(wxTreeItemId &id, wxByte *ptr, size_t len) : pdfObjectBase(id, ptr, len)
	{
		m_type = PDF_OBJ_NAME;
		if (len > 0)
			m_value = wxString::From8BitData((const char *)ptr, len);
	};

	wxString m_value;
};


class pdfReal : public pdfObjectBase
{
public:
	pdfReal(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_REAL;
		m_value = 0;
	};
	pdfReal(wxTreeItemId &id, wxByte *ptr, size_t len) : pdfObjectBase(id, ptr, len)
	{
		m_type = PDF_OBJ_REAL;
		m_value = 0;
	};

	double m_value;
};


class pdfInteger : public pdfObjectBase
{
public:
	pdfInteger(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_INTEGER;
		m_value = 0;
	};
	pdfInteger(wxTreeItemId &id, wxByte *ptr, size_t len) : pdfObjectBase(id, ptr, len)
	{
		m_type = PDF_OBJ_INTEGER;
		m_value = 0;
	};

	long m_value;
};


class pdfBoolean : public pdfObjectBase
{
public:
	pdfBoolean(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_BOOLEAN;
		m_value = false;
	};
	pdfBoolean(wxTreeItemId &id, wxByte *ptr, size_t len) : pdfObjectBase(id, ptr, len)
	{
		m_type = PDF_OBJ_BOOLEAN;
		m_value = false;
	};

	bool m_value;
};


class pdfNull : public pdfObjectBase
{
public:
	pdfNull(wxTreeItemId &id, wxByte *ptr) : pdfObjectBase(id, ptr)
	{
		m_type = PDF_OBJ_NULL;
	};
	pdfNull(wxTreeItemId &id, wxByte *ptr, size_t len) : pdfObjectBase(id, ptr, len)
	{
		m_type = PDF_OBJ_NULL;
	};
};


#endif
