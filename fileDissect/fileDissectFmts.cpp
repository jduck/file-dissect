/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectFmts.cpp:
 * the list of found/loaded plugins supporting file formats
 */
#include "fileDissectFmts.h"
#include <wx/filename.h>

#include <wx/listimpl.cpp>
WX_DEFINE_PLUGINLIST(fileDissectPlugin);


// default class constructor
fileDissectFmts::fileDissectFmts(void)
{
	m_list.DeleteContents(true);

	m_log = 0;
	m_tree = 0;
}

// scan the module directory
void fileDissectFmts::LoadPlugins(wxLog *plog, fileDissectTreeCtrl *tree)
{
	m_log = plog;
	m_tree = tree;

	// scan for modules
	wxString path(wxGetCwd());
	path += wxFileName::GetPathSeparator();
	path += wxT("plugins");
	path += wxFileName::GetPathSeparator();
	
	fileDissectFmtsBase::LoadPlugins(path, plog, tree);
}


// look for a plugin that reports to suppor this
fileDissectPlugin *fileDissectFmts::FindPluginForExt(const wxChar *extension)
{
#ifdef CHECK_FOR_DUPLICATE_EXTENSION_SUPPORT
	fileDissectPlugin *found = NULL;
#endif
	for (fileDissectFmtsBase::iterator i = m_list.begin();
		i != m_list.end();
		i++)
	{
		fileDissectPlugin *p = (*i)->m_instance;

		if (p->SupportsExtension(extension))
		{
#ifdef CHECK_FOR_DUPLICATE_EXTENSION_SUPPORT
			if (found)
				// wtf!
				;
#endif
			// make a new one, it will be deleted by the parent
			// return (*i)->m_create_instance(m_log, m_tree);

			return p;
		}
	}
	// not found
	return NULL;
}
