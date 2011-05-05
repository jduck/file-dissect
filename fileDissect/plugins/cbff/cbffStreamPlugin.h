/*
 * Windows Compound Binary File Format implementation
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * cbffStreamPlugin.h:
 * class declaration for cbffStreamPlugin
 */
#ifndef __cbffStreamPlugin_h_
#define __cbffStreamPlugin_h_

#include "fileDissect.h"		// wxWidgets base
#include "fileDissectTree.h"	// custom tree
#include "cbffStream.h"

#define CBF_PLUGIN_VERSION		0x0001

#ifdef __WXMSW__
#define DECLARE_CBF_PLUGIN(class_name) \
	extern "C" __declspec (dllexport) cbffStreamPlugin *create_instance(wxLog *plog, fileDissectTreeCtrl *tree) \
	{ return (cbffStreamPlugin *)new class_name(plog, tree); }
#else
#define DECLARE_CBF_PLUGIN(class_name) \
	extern "C" cbffStreamPlugin *create_instance(wxLog *plog, fileDissectTreeCtrl *tree) \
	{ return (cbffStreamPlugin *)new class_name(plog, tree); }
#endif


class cbffStreamPlugin
{
public:
	cbffStreamPlugin(void) 
		: m_description(0), 
		  m_log(0), 
		  m_tree(0),
		  m_version(CBF_PLUGIN_VERSION)
	{
	};
	// allow derivatives to implement a destructor
	virtual ~cbffStreamPlugin(void)
	{
	};

	// plugin interface methods
	virtual void MarkDesiredStreams(void) = 0;
	virtual void Dissect(void) = 0;
	virtual void CloseFile(void)
	{
		m_streams = 0;
	};

	wxChar *m_description;

	// passed from app -> plugin
	wxLog *m_log;
	fileDissectTreeCtrl *m_tree;
	cbffStreamList *m_streams;

private:
	unsigned long m_version;
};

#endif
