/*
 * Microsoft Office Excel Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * Workbook.h:
 * class declaration for Workbook cbffStreamPlugin class
 */

#ifndef __Workbook_h_
#define __Workbook_h_

#include "cbffStreamPlugin.h"
#include "WorkbookDefs.h"


// used for tracking nested pages (BOFs)
#include <wx/stack.h>
WX_DECLARE_STACK(wxTreeItemId, wxNodeStack);


class Workbook : public cbffStreamPlugin
{
public:
	Workbook(wxLog *plog, fileDissectTreeCtrl *tree);

	// plugin interface methods
	void MarkDesiredStreams(void);
	void Dissect(void);
	// XXX: determine if this is needed
	// void CloseFile(void);

private:
	BYTE *m_cur;
	BYTE *m_end;
	
	void DissectStream(cbffStream *pStream);

	struct WorkbookRecord *GetNextRecord(void);

	void AddBOFContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddBOUNDSHEETContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddFORMATContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddHEADERContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddINTERFACEHDRContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddMMSContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddWRITEACCESSContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddFONTContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddLABELSSTContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddSSTContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddEXTSSTContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddMULBLANKContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddMULRKContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);
	void AddRKContents(cbffStream *pStream, wxTreeItemId &parent, struct WorkbookRecord *prec);

	wxChar *HumanReadableBOFType(USHORT);
	wxChar *HumanReadableRecordTypeShort(USHORT);
	wxChar *HumanReadableRecordTypeLong(USHORT);
	wxChar *HumanReadableRKType(ULONG);

	void DecodeString(BYTE *, USHORT, USHORT, wxString &);
	size_t GetStringLength(USHORT, USHORT);

	double RKDecode(ULONG);
};

#endif
