/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectSel.h:
 * common implmentation for selecting bytes in the file
 *
 */
#ifndef __fileDissectSel_h__
#define __fileDissectSel_h__

#ifdef __linux__
# define __declspec(x)
#endif

#include <wx/wxprec.h>
#ifdef __BORLANDC__
# pragma hdrstop
#endif
#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif
#include <wx/list.h>


// a single selection range
class fileDissectSel
{
public:
	fileDissectSel(wxFileOffset start, wxFileOffset end);

	wxFileOffset m_start;
	wxFileOffset m_end;
};


// internal list class
WX_DECLARE_LIST(fileDissectSel, fileDissectSelListInt);


// value added list class
class fileDissectSelList : public fileDissectSelListInt
{
public:
	~fileDissectSelList(void);
	fileDissectSelList(void);
	fileDissectSelList(wxFileOffset start, wxFileOffset end);

	void AddToSelection(wxFileOffset start, wxFileOffset end);
};

#endif
