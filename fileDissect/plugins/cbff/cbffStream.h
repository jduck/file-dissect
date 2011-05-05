/*
 * Windows Compound Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * cbffStream.h:
 * class declaration for cbffStream (passed to stream plugins)
 */
#ifndef __cbffStream_h_
#define __cbffStream_h_

#include "fileDissect.h"
#include <wx/treectrl.h>
#include "cbff_defs.h"

class cbffStream
{
public:
	cbffStream(wxString &name);
	~cbffStream(void);

	wxFileOffset GetFileOffset(wxFileOffset streamOffset);

	wxString m_name;

	// for the query process
	bool m_wanted;

	// for processing
	BYTE *m_data;
	ULONG m_length;		// limited by file format to ULONG
	wxTreeItemId m_id;

	SECT m_start;
	wxFileOffset *m_offsets;
	ULONG m_sectCnt;

	// from parent (cbff)
	struct StructuredStorageHeader *m_phdr;
	ULONG m_sectorSize;
	ULONG m_miniSectorSize;
};


// list of streams in the file (for stream plugin to dissect)
#include <wx/list.h>
WX_DECLARE_LIST(cbffStream, cbffStreamList);

#endif
