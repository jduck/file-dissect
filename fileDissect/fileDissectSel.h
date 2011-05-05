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
	fileDissectSel(wxFileOffset start, wxFileOffset end)
		: m_start(start), m_end(end)
	{
	};

	wxFileOffset m_start;
	wxFileOffset m_end;
};


// internal list class
WX_DECLARE_LIST(fileDissectSel, fileDissectSelListInt);


// value added list class
class fileDissectSelList
{
public:
	fileDissectSelList(void)
	{
		m_list.DeleteContents(true);
	};

	fileDissectSelList(wxFileOffset start, wxFileOffset end)
	{
		m_list.DeleteContents(true);
		m_list.push_back(new fileDissectSel(start, end));
	};

	void AddToSelection(wxFileOffset start, wxFileOffset end)
	{
		m_list.push_back(new fileDissectSel(start, end));
	};

	typedef fileDissectSelListInt::iterator iterator;
	iterator begin(void) { return m_list.begin(); };
	iterator end(void) { return m_list.end(); };

	fileDissectSelListInt m_list;
};

#endif
