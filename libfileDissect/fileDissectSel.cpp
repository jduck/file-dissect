/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectSel.cpp:
 * a list class of selected bytes in a file
 */
#include "fileDissectSel.h"

// actually define the internal list class
#include <wx/listimpl.cpp>
WX_DEFINE_LIST(fileDissectSelListInt);


fileDissectSel::fileDissectSel(wxFileOffset start, wxFileOffset end)
	: m_start(start), m_end(end)
{
}


fileDissectSelList::fileDissectSelList(void)
{
	DeleteContents(true);
}


fileDissectSelList::~fileDissectSelList(void)
{
}


fileDissectSelList::fileDissectSelList(wxFileOffset start, wxFileOffset end)
{
	DeleteContents(true);
	push_back(new fileDissectSel(start, end));
}


void fileDissectSelList::AddToSelection(wxFileOffset start, wxFileOffset end)
{
	push_back(new fileDissectSel(start, end));
}

