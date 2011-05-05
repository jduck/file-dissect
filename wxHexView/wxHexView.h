/*
 * wxHexView
 *
 * a wxWidgets based Hex Viewer
 *
 * (c) 2007-2009 Joshua J. Drake
 */ 
#ifndef __wxHexView_h_
#define __wxHexView_h_

#include <wx/wxprec.h>
#ifdef __BORLANDC__
# pragma hdrstop
#endif
#ifndef WX_PRECOMP
# include <wx/wx.h>
#endif
#include <wx/list.h>


class wxHexViewSelection
{
public:
	wxHexViewSelection(wxFileOffset start, wxFileOffset end)
		: m_start(start), m_end(end)
	{
	};

	wxFileOffset m_start;
	wxFileOffset m_end;
};

// list of selected bytes
WX_DECLARE_LIST(wxHexViewSelection, selectionList);


class wxHexView : public wxWindow
{
public:
    wxHexView(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxSIMPLE_BORDER|wxWANTS_CHARS);
    ~wxHexView(void);
	void SetData(wxByte *ptr, wxFileOffset len);
	void GotoLine(unsigned int line);
	void GotoOffset(wxFileOffset offset);
	void AddToSelection(wxFileOffset start, wxFileOffset end);
	void ClearSelection(void);
	void Redraw(void);

private:
	// the painted area
	bool m_repaint;
	wxBitmap *m_cliBitmap;
	int m_width;
	int m_height;

	// font stuff
	wxFont m_font;
	int m_charheight;
	int m_charwidth;

#ifdef USE_HEXDIGITBITMAP_OPTIMIZATION
	// hexdigit painting optimization
	wxBitmap *m_hexdigits;
	wxMemoryDC m_hexDC;
#endif

	// scroll bars!
	wxScrollBar *m_vscroll;

	// colors!
	wxColor *m_txtbg;
	wxColor *m_txtfg;

	// the data
	wxByte *m_data;
	wxFileOffset m_datalen;
	unsigned int m_curLine;
	unsigned int m_numLines;
	unsigned int m_visibleLines;

	// selection info
	selectionList m_selection;

protected:
    void OnPaint(wxPaintEvent &evt);
	void OnSize(wxSizeEvent &evt);
	void OnVScroll(wxScrollEvent &evt);
	void OnMouseWheel(wxMouseEvent &evt);

	void UpdateScrollBar(void);

private:
	DECLARE_EVENT_TABLE()
};

#define IDC_VSCROLL 5000

#endif
