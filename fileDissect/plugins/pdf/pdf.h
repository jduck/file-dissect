/*
 * Adobe Portable Document Format implementation
 * Joshua J. Drake <jdrake accuvant.com>
 * 
 * pdf.h:
 * class declaration for pdf class
 */
#ifndef __pdf_h_
#define __pdf_h_

#include "fileDissectPlugin.h"
#include "pdf_defs.h" // portable document format

#include "pdfObjects.h"


class pdf : public fileDissectPlugin
{
public:
	pdf(wxLog *plog, fileDissectTreeCtrl *tree);
	~pdf(void);

	// plugin member functions
	bool SupportsExtension(const wxChar *extension);
	void Dissect(void);
	void CloseFile(void);
	void Destroy(void);

private:
	void DestroyFileData(void);
	void InitFileData(void);

	// tree nodes for dissection output
	wxTreeItemId m_root_id;
	wxTreeItemId m_hdr_id;
	wxTreeItemId m_xref_id;
	wxTreeItemId m_indobj_id;

	// PDF data items
	pdfObjectsHashMap m_objects;
	wxFileOffset m_xref_off;

	// we use an indirect object here even though thats not *EXACTLY* what a trailer is...
	pdfIndirect *m_trailer;

	// additional dissection routines
	bool DissectHeader(void);
	bool DissectTrailer(void);
	bool DissectXref(wxFileOffset offset = wxInvalidOffset);
	bool DissectXrefStm(wxByte *ptr, wxByte *base);
	bool DissectObjects(void);
	bool DissectStream(pdfIndirect *pObj);

	bool DissectData(wxTreeItemId &parent, pdfIndirect *pObj);

	// private file format functionality
	bool ReadIndirect(pdfIndirect *pObj);
};

#endif
