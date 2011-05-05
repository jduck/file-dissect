/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectDnD.h:
 * drag and drop declarations
 */
#ifndef __fileDissectDnD_h_
#define __fileDissectDnD_h_

#include "fileDissectFrame.h"

#if wxUSE_DRAG_AND_DROP
#include <wx/dnd.h>			// drag and drop

class fileDissectDnD : public wxFileDropTarget
{
public:
    fileDissectDnD(fileDissectFrame *pf) { m_frame = pf; }

    virtual bool OnDropFiles(wxCoord x, wxCoord y,
                             const wxArrayString& filenames);
private:
    fileDissectFrame *m_frame;
};
#endif

#endif
