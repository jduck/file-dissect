/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectFmts.h:
 * the list of found/loaded plugins supporting file formats
 */
#ifndef __fileDissectFmts_h__
#define __fileDissectFmts_h__

#include "fileDissectPlugin.h"
#include "wxPluginLoader.h"

WX_DECLARE_PLUGINLIST(fileDissectPlugin, fileDissectFmtsBase);

class fileDissectFmts : public fileDissectFmtsBase
{
public:
	fileDissectFmts(void);
	void LoadPlugins(wxLog *plog, fileDissectTreeCtrl *tree);
	fileDissectPlugin *FindPluginForExt(const wxChar *extension);

	wxLog *m_log;
	fileDissectTreeCtrl *m_tree;
};

#endif
