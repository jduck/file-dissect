/*
 * Windows Compound Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * cbffStreamPlugins.cpp:
 * implementation for cbffStreamPlugins class
 */
#include "cbffStreamPlugins.h"
#include <wx/filename.h>

#include <wx/listimpl.cpp>
WX_DEFINE_PLUGINLIST(cbffStreamPlugin);

// default class constructor
cbffStreamPlugins::cbffStreamPlugins(void)
{
	// destroy elements on instance destruction
	m_list.DeleteContents(true);
}

// scan the module directory
void cbffStreamPlugins::LoadPlugins(wxLog *plog, fileDissectTreeCtrl *tree)
{
	// scan for modules
	wxString path(wxGetCwd());
	path += wxFileName::GetPathSeparator();
	path += wxT("plugins");
	path += wxFileName::GetPathSeparator();
	path += wxT("cbff");
	path += wxFileName::GetPathSeparator();

	cbffStreamPluginsBase::LoadPlugins(path, plog, tree);
}
