/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectFrame.h:
 * frame declarations
 */
#ifndef __fileDissectFrame_h_
#define __fileDissectFrame_h_

#include "fileDissect.h"		// wxWidgets base

// #include <wx/stdpaths.h>		// standard paths
// #include <wx/filedlg.h>		// file dialog
// #include <wx/file.h>			// file handling
// #include <wx/gdicmn.h>
// #include <wx/filename.h>		// filename handling

/* classes used in this app */
class fileDissectFrame;
class fileDissectApp;

#include "fileDissectFmts.h"
#include "fileDissectTree.h"
#include "wxFileMap.h"
#include "wxHexView.h"

/*
 * the main frame of the application
 */
class fileDissectFrame : public wxFrame
{
public:
	fileDissectFrame(const wxString &title);
	~fileDissectFrame(void);
   
	// init stuff
	void InitGUI(const wxString &title);
	void InitFormats(void);

	// state maintenance code
	void UpdateTitle(void);
	void UpdateDisplay(void);
	void OpenFile(wxString& fname);
	void CloseFile(void);
	void HighlightItem(wxTreeItemId &id);

	// file menu event handlers
	// void OnFileNew(wxCommandEvent& event);
	void OnFileOpen(wxCommandEvent& event);
	void OnFileClose(wxCommandEvent& event);
	// void OnFileSave(wxCommandEvent& event);
	// void OnFileSaveAs(wxCommandEvent& event);
	void OnFileReload(wxCommandEvent& event);
	void OnFileExit(wxCommandEvent& event);

	// tools menu event handlers
	void OnToolsRescan(wxCommandEvent& event);

	// context menu event handlers
	void OnNodeExpandChildren(wxCommandEvent& event);
	void OnNodeCollapseChildren(wxCommandEvent& event);
	void OnNodeHighlight(wxCommandEvent& event); // probably not needed, since we do this on click

	// gui items (tree below as well)
	wxPanel *m_panel;				// main panel
	wxMenuBar *m_menubar;			// main menu bar
	wxMenu *m_mnuFile;				// file menu
	wxMenu *m_mnuTools;				// tools menu
	wxHexView *m_contents;			// hex viewer
	wxTextCtrl *m_warnings;			// log output gets put here

	// data items passed down to plugins
	wxFileMap *m_file;				// mapped file
	wxLogTextCtrl *m_log;			// logging object
	fileDissectTreeCtrl *m_tree;		// custom tree view control

 private:
	fileDissectFmts *m_formats;		// supported file formats
	fileDissectPlugin *m_plugin;		// the plugin for the currently open file
	wxString m_strWildcard;			// the wildcard data for file select dialog

	DECLARE_EVENT_TABLE()
};

#endif
