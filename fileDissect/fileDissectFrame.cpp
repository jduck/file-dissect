/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectFrame.cpp:
 * Main frame code
 */
#include "fileDissectApp.h"		// for APP_NAME only
#include "fileDissectFrame.h"
#include "fileDissectGUI.h"
#include "fileDissectDnD.h"


BEGIN_EVENT_TABLE(fileDissectFrame, wxFrame)
  // EVT_MENU(IDM_FILE_NEW, fileDissectFrame::OnFileNew)
  EVT_MENU(IDM_FILE_OPEN, fileDissectFrame::OnFileOpen)
  EVT_MENU(IDM_FILE_CLOSE, fileDissectFrame::OnFileClose)
  // EVT_MENU(IDM_FILE_SAVE, fileDissectFrame::OnFileSave)
  // EVT_MENU(IDM_FILE_SAVE_AS, fileDissectFrame::OnFileSaveAs)
  EVT_MENU(IDM_FILE_EXIT, fileDissectFrame::OnFileExit)
  EVT_MENU(IDM_TOOLS_RESCAN, fileDissectFrame::OnToolsRescan)
  EVT_MENU(IDM_NODE_EXPANDBELOW, fileDissectFrame::OnNodeExpandChildren)
  EVT_MENU(IDM_NODE_HIGHLIGHT, fileDissectFrame::OnNodeHighlight)
END_EVENT_TABLE();


/*
 * application main frame..
 */
fileDissectFrame::fileDissectFrame (const wxString &title)
  : wxFrame (NULL, wxID_ANY, title)
{
	m_file = NULL;
	m_plugin = NULL;
	m_formats = NULL;

	InitGUI(title);
	InitFormats();
}

void fileDissectFrame::InitGUI(const wxString& WXUNUSED(title))
{
	// file menu
	m_mnuFile = new wxMenu;
	// m_mnuFile->Append(IDM_FILE_NEW, wxT("&New..\tCtrl-N"));
	m_mnuFile->Append(IDM_FILE_OPEN, wxT("&Open..\tCtrl-O"));
	m_mnuFile->Append(IDM_FILE_CLOSE, wxT("&Close..\tCtrl-W"));
	// m_mnuFile->Append(IDM_FILE_SAVE, wxT("&Save..\tCtrl-S"));
	// m_mnuFile->Append(IDM_FILE_SAVE_AS, wxT("Save &As.."));
	m_mnuFile->Enable(IDM_FILE_CLOSE, false);
	m_mnuFile->AppendSeparator();
	m_mnuFile->Append(IDM_FILE_EXIT, wxT("E&xit"));

	// tools menu
	m_mnuTools = new wxMenu;
	m_mnuTools->Append(IDM_TOOLS_RESCAN, wxT("&Reload Modules"));

	// menu bar
	m_menubar = new wxMenuBar;
	m_menubar->Append(m_mnuFile, wxT("&File"));
	m_menubar->Append(m_mnuTools, wxT("&Tools"));
	SetMenuBar(m_menubar);

	// init the panel..
	m_panel = new wxPanel(this);

	// initialize the tree
	m_tree = new fileDissectTreeCtrl(m_panel, IDC_TREE,
				  wxDefaultPosition, wxDefaultSize,
				  wxSUNKEN_BORDER | wxTR_HAS_VARIABLE_ROW_HEIGHT | wxTR_HAS_BUTTONS);

	m_contents = new wxHexView(m_panel, IDC_HEXVIEW,
		wxDefaultPosition, wxDefaultSize);

	// m_contents->AdjustSize();

	// initialize the static text box
	m_warnings = new wxTextCtrl(m_panel, IDC_LOG, _T(""),
		wxDefaultPosition, wxSize::wxSize(-1, 100),
		wxTE_MULTILINE);
	m_warnings->SetEditable(false);

	m_log = new wxLogTextCtrl(m_warnings);
	wxLog::SetActiveTarget(m_log);

	// set the icon
	// SetIcon(wxICON(fileDissect));

	// set up the horizontal sizer
	wxSizer *sizerTop = new wxBoxSizer(wxHORIZONTAL);
	sizerTop->Add(m_tree, 1, wxEXPAND | (wxTOP|wxLEFT|wxRIGHT), 8);
	sizerTop->Add(m_contents, 0, wxEXPAND | (wxTOP|wxRIGHT), 8);

	wxSizer *sizerVert = new wxBoxSizer(wxVERTICAL);
	sizerVert->Add(sizerTop, 2, wxEXPAND, 0);
	sizerVert->Add(m_warnings, 1, wxEXPAND | wxALL, 8);
	m_panel->SetSizer(sizerVert);

	/* set window size */
	wxSize ssz = wxGetDisplaySize();
	// int sh = ssz.GetHeight();
	// int sw = ssz.GetWidth();
	SetSize(1024 - 40, 768 - 40);

	// center the window
	wxSize wsz = GetClientSize();
	wxPoint pt;
	pt.x = (ssz.x / 2) - (wsz.x / 2);
	pt.y = (ssz.y / 2) - (wsz.y / 2);
	SetPosition(pt);

#if wxUSE_DRAG_AND_DROP
	SetDropTarget(new fileDissectDnD(this));
#endif
}

void fileDissectFrame::InitFormats(void)
{
	m_formats = new fileDissectFmts();
	m_formats->LoadPlugins(m_log, m_tree);
	m_strWildcard = wxT("");

	// build the file list based on modules
	wxString strExts = wxT("");
	for (fileDissectFmts::iterator i = m_formats->begin();
		i != m_formats->end();
		i++)
	{
		fileDissectPlugin *p = (*i)->m_instance;

		if (p->m_extensions)
		{
			if (!strExts.IsEmpty())
				strExts += wxT(";");
			strExts += p->m_extensions;
		}
		else
			strExts += wxT("*.*");

		if (p->m_description)
		{
			if (!m_strWildcard.empty())
				m_strWildcard += wxT("|");
			m_strWildcard += p->m_description;
			m_strWildcard += wxT("(");
			m_strWildcard += strExts;
			m_strWildcard += wxT(")|");
			m_strWildcard += strExts;
		}
	}
}

fileDissectFrame::~fileDissectFrame(void)
{
	delete m_panel;
	if (m_file)
		delete m_file;
	if (m_formats)
		delete m_formats;
}


#if 0
void
fileDissectFrame::OnFileNew(wxCommandEvent& WXUNUSED(event))
{
}
#endif


void fileDissectFrame::OnFileOpen(wxCommandEvent& WXUNUSED(event))
{
	// present them with a file dialog to choose the project
	wxString prjFile = ::wxFileSelector(wxT("Select a file to dissect..."), 
										NULL, 
										NULL, 
										NULL,
										m_strWildcard,
										wxFD_OPEN,
										this);
	if (!prjFile.empty())
	{
		CloseFile();
		OpenFile(prjFile);
	}
}


void fileDissectFrame::OpenFile(wxString& fname)
{
	wxLogMessage(wxT("Opening file \"%s\""), fname.GetData());

	m_file = new wxFileMap();
	if (!m_file->Open(fname.c_str()))
		// error message is logged
		return;

	wxFileOffset len = m_file->Length();
	m_contents->SetData(m_file->GetAddress(), len);

	// update the close menu item
	m_mnuFile->Enable(IDM_FILE_CLOSE, true);

	// update the window
	UpdateDisplay();
}


void fileDissectFrame::OnFileClose(wxCommandEvent& WXUNUSED(event))
{
	CloseFile();
}

void fileDissectFrame::CloseFile(void)
{
	if (m_file)
	{
		wxLogMessage(wxT("Closing file."));

		delete m_file;
		m_file = NULL;
		m_tree->DeleteAllItems();
		m_contents->SetData(NULL, 0);
		if (m_plugin)
			m_plugin->CloseFile();
	}
}


#if 0
void
fileDissectFrame::OnFileSave(wxCommandEvent& WXUNUSED(event))
{
}


void
fileDissectFrame::OnFileSaveAs(wxCommandEvent& event)
{
	// present them with a file dialog to choose
	wxString prjFile = ::wxFileSelector(wxT("Save as..."), 
										NULL, 
										NULL, 
										wxT("xls"), 
										wxT("BIFF files (*.xls)|*.xls"), 
										wxSAVE,
										this);
	if (!prjFile.empty())
	{
		OnFileSave(event);
	}
}
#endif

void fileDissectFrame::OnFileExit(wxCommandEvent& WXUNUSED(event))
{
   Close(true);
}


void fileDissectFrame::OnNodeExpandChildren(wxCommandEvent& WXUNUSED(event))
{
	wxTreeItemId id = m_tree->GetSelection();

	m_tree->ExpandAllChildren(id);
}


void fileDissectFrame::OnNodeHighlight(wxCommandEvent& WXUNUSED(event))
{
	// find node data
	wxTreeItemId id = m_tree->GetSelection();
	HighlightItem(id);
}

void fileDissectFrame::HighlightItem(wxTreeItemId &id)
{
	// clear the hexview selection
	m_contents->ClearSelection();

	fdTIData *pTID = (fdTIData *)m_tree->GetItemData(id);
	if (!pTID)
	{
		m_contents->Redraw();
		return;
	}

	// update the selection based on the node data
	wxFileOffset start = m_file->Length();
	for (fileDissectSelList::iterator i = pTID->m_selected.begin();
		i != pTID->m_selected.end();
		i++)
	{
		fileDissectSel *pSel = *i;

		if (pSel->m_start < start)
			start = pSel->m_start;
		m_contents->AddToSelection(pSel->m_start, pSel->m_end);
	}
	m_contents->GotoOffset(start);

	// refresh hexview
	m_contents->Redraw();
}


// set the window title
void fileDissectFrame::UpdateTitle(void)
{
	wxString new_title(APP_NAME);
	if (m_file)
	{
		new_title += wxT(" - ");
		new_title += m_file->m_filename;
	}
	this->SetTitle(new_title);
}


void fileDissectFrame::UpdateDisplay(void)
{
	UpdateTitle();

	// refresh the tree
	m_tree->DeleteAllItems();
	if (!m_file)
		return;

	// see if we have a plugin that can dissect this file extension
	wxFileName fname(m_file->m_filename);
	m_plugin = m_formats->FindPluginForExt(fname.GetExt().c_str());
	if (!m_plugin)
	{
		wxLogError(wxT("No plug-ins support this file extension!"));
		return;
	}

	// pass the file and some UI elements off to the plugin
	wxLogMessage(wxT("Dissecting using the \"%s\" plug-in."), m_plugin->m_description);
	m_plugin->m_file = m_file;
	m_plugin->Dissect();
}


void fileDissectFrame::OnToolsRescan(wxCommandEvent& WXUNUSED(event))
{
	m_formats->LoadPlugins(m_log, m_tree);
}
