/*
 * Windows Compound Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * cbffStreamPlugin.h:
 * class declaration for cbffStreamPlugin
 */
#ifndef __cbffStreamPlugins_h_
#define __cbffStreamPlugins_h_

#include "cbffStreamPlugin.h"
#include "wxPluginLoader.h"

// list of stream plugins
WX_DECLARE_PLUGINLIST(cbffStreamPlugin, cbffStreamPluginsBase);

class cbffStreamPlugins : public cbffStreamPluginsBase
{
public:
	cbffStreamPlugins(void);
	void LoadPlugins(wxLog *plog, fileDissectTreeCtrl *tree);
};


#endif
