/*
 * wxHexView
 *
 * a wxWidgets based Hex Viewer
 *
 * (c) 2007-2009 Joshua J. Drake
 */
#include "wxHexView.h"

BEGIN_EVENT_TABLE(wxHexView, wxWindow)
	EVT_SIZE(wxHexView::OnSize)
    EVT_PAINT(wxHexView::OnPaint)
    EVT_COMMAND_SCROLL(IDC_VSCROLL, wxHexView::OnVScroll)
    EVT_MOUSEWHEEL(wxHexView::OnMouseWheel)
END_EVENT_TABLE()


#include <wx/listimpl.cpp>
WX_DEFINE_LIST(selectionList);


wxHexView::wxHexView(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style)
	: wxWindow(parent, id, pos, size, style)
{
	// delete the items we destroy the list
	m_selection.DeleteContents(true);

	m_data = NULL;
	m_datalen = 0;
	m_curLine = 0;
	m_numLines = 0;
	m_visibleLines = 0;

	m_width = m_height = 0;

	m_cliBitmap = NULL;

	m_txtbg = new wxColor((unsigned long)0xffffff); // white
	m_txtfg = new wxColor((unsigned long)0); // black

	// XXX: TODO: allow font choice
	m_font = wxSystemSettings::GetFont(wxSYS_ANSI_FIXED_FONT);
	m_font.SetPointSize(8);
	wxWindow::SetFont(m_font);

	m_vscroll = NULL;
	m_vscroll = new wxScrollBar(this, IDC_VSCROLL, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);

	// set the new window size
	int x_size = (8 + 1 + (8 * 3) + 1 + (8 * 3) + 1 + 16);
	m_charheight = wxWindow::GetCharHeight();
	m_charwidth = wxWindow::GetCharWidth() - 1;
	x_size *= m_charwidth;

#ifdef USE_HEXDIGITBITMAP_OPTIMIZATION
	// init the hexdigits bitmap
	int i, hw;
	wxString str(wxT('0'));
	hw = m_charwidth * 16;
	m_hexdigits = new wxBitmap(hw, m_charheight);
	m_hexDC.SelectObject(*m_hexdigits);
	// clear the background
	m_hexDC.SetBrush(*wxTheBrushList->FindOrCreateBrush(*m_txtbg));
	m_hexDC.SetPen(*wxThePenList->FindOrCreatePen(*m_txtbg, 1, wxSOLID));
	m_hexDC.DrawRectangle(0, 0, hw, m_charheight);
	// set the foreground pen/font
	m_hexDC.SetPen(*wxThePenList->FindOrCreatePen(*m_txtfg, 1, wxSOLID));
	m_hexDC.SetFont(m_font);
	// draw the digits
	for (i = 0; i < 16; i++)
	{
		if (i < 10)
			str.SetChar(0, i + '0');
		else
			str.SetChar(0, i - 10 + 'a');
		m_hexDC.DrawText(str, (i * m_charwidth), 0);
	}
#endif

	// set size
	wxSize sz(x_size + 18, -1);
	SetMinSize(sz);
	SetSize(sz);

	// set scroll bar position
	wxPoint pt(x_size, 0);
	m_vscroll->SetPosition(pt);
	m_vscroll->SetScrollbar(0, 0, 0, 0);
}

wxHexView::~wxHexView()
{
	// caller is responsible for free'n data
	m_data = NULL;

	if (m_cliBitmap)
		delete m_cliBitmap;
	delete m_txtbg;
	delete m_txtfg;
}

void wxHexView::OnPaint(wxPaintEvent& WXUNUSED(evt))
{	
	wxPaintDC dc(this);
    wxMemoryDC memdc;
	memdc.SelectObject(*m_cliBitmap);

	if (m_repaint)
	{
		// draw the text background
		memdc.SetBrush(*wxTheBrushList->FindOrCreateBrush(*m_txtbg));
		memdc.SetPen(*wxThePenList->FindOrCreatePen(*m_txtbg, 1, wxSOLID));
		memdc.DrawRectangle(0, 0, m_width, m_height);

#ifndef USE_HEXDIGITBITMAP_OPTIMIZATION
		// draw the text
		memdc.SetPen(*wxThePenList->FindOrCreatePen(*m_txtfg, 1, wxSOLID));
		memdc.SetFont(m_font);

		wxString str;
#endif

		/* process all lines */
		int offset = m_curLine * 16;
		wxCoord out_y;
		int idx;

		// are we starting with selected text?
		bool selected = false;
		if (offset > 0)
		{
			for (selectionList::const_iterator i = m_selection.begin();
				i != m_selection.end();
				i++)
			{
				wxHexViewSelection *p = (*i);
				if (p->m_start < offset)
					selected = true;
				if (p->m_end <= offset)
					selected = false;
				if (p->m_start >= offset
					&& p->m_end >= offset)
					// neither are less.. we found out!
					break;
			}
		}

#ifdef USE_HEXDIGITBITMAP_OPTIMIZATION
		int tmp_off, char_off;
#endif
		for (out_y = 0; out_y < m_height; out_y += m_charheight)
		{
#ifdef USE_HEXDIGITBITMAP_OPTIMIZATION
			// paint the offset hex digits
			tmp_off = offset;
			for (idx = 7; idx >= 0; --idx)
			{
				char_off = tmp_off & 0xf;
				memdc.Blit((idx * m_charwidth), out_y, 
					m_charwidth, m_charheight, 
					&m_hexDC, 
					(char_off * m_charwidth), 0);
				tmp_off >>= 4;
			}

			// not much faster, gave up on this
#else
			/* for each line, draw the offset... */
			str = wxString::Format(wxT("%08x "), offset);
			wxCoord hex_start = m_charwidth * 9;
			wxCoord ascii_start = hex_start + (((3 * 8) * 2) + 1) * m_charwidth;

			/* must have data to continue */
			if (m_data && m_datalen > 0)
			{
				/* ...the hex data... */
				for (idx = 0; idx < 16; idx++)
				{
					if (offset + idx < m_datalen)
						str += wxString::Format(wxT("%02x "), m_data[offset + idx]);
					else
						str += wxT("   ");
					// add a space after the first 8
					if (idx == 7)
						str.Append(wxT(' '));
				}

				/* ...and the ascii representation */
				wxString tmp = wxT(".");
				for (idx = 0; idx < 16; idx++)
				{
					if (offset + idx < m_datalen)
					{
						wxByte ch = m_data[offset + idx];
						/*
						 * this is the old way, this causes alignment problems with some chars
						 *
						tmp.SetChar(0, ch);
						str += tmp;
						 */
						if (ch > 0x1f && ch < 0x7f)
							str.Append(ch);
						else
							str.Append(wxT('.'));
					}
				}
			}

			memdc.DrawText(str, 0, out_y);

			// invert highlighted parts in this line
			if (m_data && m_datalen > 0)
			{
				// start and stop selection characters for this line
				int selChLeft = -1;
				int selChRight = -1;

				// if we are already selecting at line start, note it
				if (selected)
					selChLeft = 0;

				// see if we have selections starting/stopping on this line
				for (selectionList::const_iterator i = m_selection.begin();
					i != m_selection.end();
					i++)
				{
					wxHexViewSelection *p = (*i);

					// XXX: what about multiple start/top in a line?

					// is the start of the selection in this line?
					if (selChLeft == -1)
						if (p->m_start >= offset && p->m_start < offset + 16)
							selChLeft = p->m_start - offset;

					// does it stop on this line?
					if (selChRight == -1)
						if (p->m_end > offset && p->m_end <= offset + 16)
							selChRight = p->m_end - offset;
				}

				// what kind of selection actions are on this line?
				if (selChLeft != -1 && selChRight != -1)			// both stop and start!
				{
					// start first, or stop first?
					if (selChLeft > selChRight)
					{
						// stop first (two selection bits)
						wxCoord l, r;

						// select 0 -> new end

						// hex part
						l = hex_start;
						r = hex_start + (selChRight * 3 * m_charwidth);
						if (selChRight <= 8)
							r -= m_charwidth;
						memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);

						// ascii part
						l = ascii_start;
						r = ascii_start + (selChRight * m_charwidth);
						memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);

						// select new start -> end

						// hex part
						l = hex_start + (selChLeft * 3 * m_charwidth);
						if (selChLeft > 7)
							l += m_charwidth;
						r = ascii_start - m_charwidth;
						memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);

						// ascii part
						l = ascii_start + (selChLeft * m_charwidth);
						r = ascii_start + (16 * m_charwidth);
						memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);
					}
					else if (selChLeft < selChRight)
					{
						// start first, select start -> end
						wxCoord l, r;

						// hex part
						l = hex_start + (selChLeft * 3 * m_charwidth);
						if (selChLeft > 7)
							l += m_charwidth;
						r = hex_start + (selChRight * 3 * m_charwidth);
						if (selChRight <= 8)
							r -= m_charwidth;
						memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);

						// ascii part
						l = ascii_start + (selChLeft * m_charwidth);
						r = ascii_start + (selChRight * m_charwidth);
						memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);

						// not selected anymore
						selected = false;
					}
					// ==?  no action.
				}
				else if (!selected && selChLeft != -1 && selChRight == -1)	// start only
				{
					// select from sel start to end
					wxCoord l, r;

					// hex part
					l = hex_start + (selChLeft * 3 * m_charwidth);
					if (selChLeft > 7)
						l += m_charwidth;
					r = ascii_start - m_charwidth;
					memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);

					// ascii part
					l = ascii_start + (selChLeft * m_charwidth);
					r = ascii_start + (16 * m_charwidth);
					memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);

					selected = true;
				}
				else if (selected && selChLeft == -1 && selChRight != -1)	// stop only
				{
					// select from start to sel stop
					wxCoord l, r;

					// hex part
					l = hex_start;
					r = hex_start + (selChRight * 3 * m_charwidth);
					if (selChRight <= 8)
						r -= m_charwidth;
					memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);

					// ascii part
					l = ascii_start;
					r = ascii_start + (selChRight * m_charwidth);
					memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);

					selected = false;
				}
				else if (selected)
				{
					// if we're selected, select the entire line
					wxCoord l, r;

					// hex part
					l = hex_start;
					r = ascii_start - m_charwidth;
					memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);

					// ascii part
					l = ascii_start;
					r = ascii_start + (16 * m_charwidth);
					memdc.Blit(l, out_y, r-l, m_charheight, &memdc, 0, 0, wxINVERT);
				}
			}
#endif
			offset += 16; // bytes per line
		}
		m_repaint = false;
	}

	dc.Blit(0, 0, m_width, m_height, &memdc, 0, 0);
}

void wxHexView::OnSize(wxSizeEvent &evt)
{
	int w, h;

	GetClientSize(&w, &h);
	if (w != m_width
		|| h != m_height)
	{
		m_width = w;
		m_height = h;

		delete m_cliBitmap;
		m_cliBitmap = NULL;
	}
	m_visibleLines = m_height / m_charheight;

	if (!m_cliBitmap)
		m_cliBitmap = new wxBitmap(w, h);

	// GetSize(&w, &h);

	UpdateScrollBar();

	m_repaint = true;
	Refresh(false);

	evt.Skip();
}


void wxHexView::SetData(wxByte *ptr, wxFileOffset len)
{
	m_data = ptr;
	m_datalen = len;
	m_numLines = m_datalen / 16;
	if (m_datalen % 16)
		m_numLines++;
	UpdateScrollBar();

	// GotoLine refreshes the display
	GotoLine(0);
}


void wxHexView::GotoLine(unsigned int line)
{
	// don't move below the last "page"
	if (m_numLines > m_visibleLines)
	{
		if (line > m_numLines - m_visibleLines)
			line = m_numLines - m_visibleLines;
	}

	m_curLine = line;
	m_vscroll->SetThumbPosition(line);
	Redraw();
}


void wxHexView::GotoOffset(wxFileOffset offset)
{
	// don't move the screen if the line is already onscreen
	unsigned int line = offset / 16;
	if (line < m_curLine
		|| line > m_curLine + m_visibleLines)
		GotoLine(offset / 16);
}


// update the scroll bar
void wxHexView::UpdateScrollBar()
{
	// only update/show it if there's more lines than the visible height
	if (m_vscroll)
	{
		m_vscroll->SetSize(16, m_height);
		m_vscroll->SetScrollbar(0, m_visibleLines, m_numLines, m_visibleLines - 1);
		if (m_numLines > m_visibleLines)
			m_vscroll->Enable(true);
		else
			m_vscroll->Enable(false);
	}
}


void wxHexView::OnVScroll(wxScrollEvent &evt)
{
    unsigned int pos = evt.GetPosition();

	if (pos == m_curLine)
		return;
	GotoLine(pos);
}


void wxHexView::OnMouseWheel(wxMouseEvent &evt)
{
	int th_size = m_vscroll->GetThumbSize();
	int th_max = m_vscroll->GetRange();

	if (th_max <= th_size)
		return;

	int th_cur = m_vscroll->GetThumbPosition();

	if (evt.m_wheelRotation > 0)
	{
		// scroll up
		if (th_cur > th_size)
			th_cur -= th_size;
		else
			th_cur = 0;
	}
	else
	{
		// scroll down
		if (th_cur < th_max - (th_size * 2))
			th_cur += th_size;
		else
			th_cur = th_max - th_size;
	}
	if (th_cur == (int)m_curLine)
		return;

	GotoLine(th_cur);
}


void wxHexView::AddToSelection(wxFileOffset start, wxFileOffset end)
{
	m_selection.push_back(new wxHexViewSelection(start, end));
}

void wxHexView::ClearSelection(void)
{
	m_selection.clear();
}

void wxHexView::Redraw(void)
{
	m_repaint = true;
	Refresh();
}
