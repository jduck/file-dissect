/*
 * Windows Compound Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * cbffStream.cpp:
 * implementation for cbffStream class
 */
#include "cbffStream.h"

#include <wx/listimpl.cpp>
WX_DEFINE_LIST(cbffStreamList);

cbffStream::cbffStream(wxString &name)
	: m_name(name)
{
	m_wanted = false;

	m_data = 0;
	m_length = 0;

	m_start = CBFF_SECT_FREE;
	m_offsets = 0;
	m_sectCnt = 0;

	// m_id = .. 

	m_phdr = 0;
	m_miniSectorSize = 0;
	m_sectorSize = 0;
}

cbffStream::~cbffStream(void)
{
	if (m_data)
		free(m_data);
	if (m_offsets)
		free(m_offsets);
}

wxFileOffset cbffStream::GetFileOffset(wxFileOffset streamOffset)
{
	if (streamOffset > (wxFileOffset)m_length)
		// XXX: fix me!
		return m_length;

	ULONG mod;
	if (m_length < m_phdr->_ulMiniSectorCutoff)
		// its stored in the minifat
		mod = m_miniSectorSize;
	else
		// its stored in the fat
		mod = m_sectorSize;

	ULONG sect = streamOffset / mod;
	ULONG rem = streamOffset % mod;
	if (sect >= m_sectCnt)
		return m_length;

	return m_offsets[sect] + rem;
}
