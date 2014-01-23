/*
 * Adobe Portable Document Format implementation
 * Joshua J. Drake <jdrake accuvant.com>
 * 
 * pdfObject.cpp:
 * implementation for pdfObject class
 */
#include "pdfObjects.h"

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(pdfObjectList);


pdfObjectBase::pdfObjectBase(void)
{
	m_type = PDF_OBJ_BASE;
	m_len = 0;
	m_ptr = 0;
}

pdfObjectBase::pdfObjectBase(wxTreeItemId &id, wxByte *ptr)
{
	m_type = PDF_OBJ_BASE;
	m_len = 0; // dunno yet
	m_id = id;
	m_ptr = ptr;
}

pdfObjectBase::pdfObjectBase(wxTreeItemId &id, wxByte *ptr, size_t len)
{
	m_type = PDF_OBJ_BASE;
	m_len = len;
	m_id = id;
	m_ptr = ptr;
}


pdfArray::~pdfArray(void)
{
	pdfObjectList::iterator it, en;
    for (it = m_list.begin(), en = m_list.end(); it != en; ++it)
		Delete(*it);
	m_list.clear();
}


pdfDictionary::~pdfDictionary(void)
{
	pdfDictHashMap::iterator it = m_entries.begin();
	pdfDictHashMap::iterator en = m_entries.end();
    for (; it != en; ++it)
	{
		pdfObjectBase *pObj = (pdfObjectBase *)it->second;
		// For some reason, wxHashMap creates keys with NULL values whenever
		// you access an element that didn't exist. We can't delete NULL
		// so we just skip them..
		if (!pObj)
			continue;
		Delete(pObj);
	}
	m_entries.clear();
}


pdfIndirect::pdfIndirect(unsigned long num, unsigned long offset, unsigned long generation)
{
	m_type = PDF_OBJ_INDIRECT;
	m_length = 0; // dunno yet

	m_data_offset = wxInvalidOffset;
	m_data = 0;
	m_datalen = 0;

	m_offset = offset;

	m_number = num;
	m_generation = generation;

	m_dict = 0;
	m_stream = 0;
	m_obj = 0;
}

pdfIndirect::~pdfIndirect(void)
{
	if (m_dict)
		delete m_dict;
	if (m_stream)
		delete m_stream;
	if (m_obj)
		pdfObjectBase::Delete(m_obj);
}


void pdfObjectBase::Delete(pdfObjectBase *pObj)
{
	switch (pObj->m_type)
	{
	case PDF_OBJ_STREAM:
		delete (pdfStream *)pObj;
		break;

	case PDF_OBJ_DICTIONARY:
		delete (pdfDictionary *)pObj;
		break;

	case PDF_OBJ_INDIRECT:
		delete (pdfIndirect *)pObj;
		break;

	case PDF_OBJ_ARRAY:
		delete (pdfArray *)pObj;
		break;

	case PDF_OBJ_LITERAL:
		delete (pdfLiteral *)pObj;
		break;

	case PDF_OBJ_HEXSTRING:
		delete (pdfHexString *)pObj;
		break;

	case PDF_OBJ_REFERENCE:
		delete (pdfReference *)pObj;
		break;

	case PDF_OBJ_NAME:
		delete (pdfName *)pObj;
		break;

	case PDF_OBJ_REAL:
		delete (pdfReal *)pObj;
		break;

	case PDF_OBJ_INTEGER:
		delete (pdfInteger *)pObj;
		break;

	case PDF_OBJ_BOOLEAN:
		delete (pdfBoolean *)pObj;
		break;

	case PDF_OBJ_NULL:
		delete (pdfNull *)pObj;
		break;
	}
}
