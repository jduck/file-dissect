/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectDnD.cpp:
 * drag and drop implementation
 */
#include "fileDissectDnD.h"

#if wxUSE_DRAG_AND_DROP

bool fileDissectDnD::OnDropFiles(wxCoord WXUNUSED(x), wxCoord WXUNUSED(y), const wxArrayString& filenames)
{
    size_t nFiles = filenames.GetCount();
	if (nFiles != 1)
		return false;

	wxString str = filenames[0];
	m_frame->CloseFile();
	m_frame->OpenFile(str);
    return true;
}

#endif
