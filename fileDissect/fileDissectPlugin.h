/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectPlugin.h:
 * common implmentation for file format handling plug-ins
 *
 * plug-ins will derive from this class and export a single "create_instance" function 
 * that returns a pointer to a new instance
 */
#ifndef __fileDissectPlugin_h__
#define __fileDissectPlugin_h__

#include "fileDissect.h"		// wxWidgets base
#include "fileDissectTree.h"	// custom TreeCtrl
#include "wxFileMap.h"			// memory mapped files


#define FD_PLUGIN_VERSION			0x0001

#ifdef __WXMSW__
#define DECLARE_FD_PLUGIN(class_name) \
	extern "C" __declspec (dllexport) fileDissectPlugin *create_instance(wxLog *plog, fileDissectTreeCtrl *tree) \
	{ return (fileDissectPlugin *)new class_name(plog, tree); }
#else
#define DECLARE_FD_PLUGIN(class_name) \
	extern "C" fileDissectPlugin *create_instance(wxLog *plog, fileDissectTreeCtrl *tree) \
	{ return (fileDissectPlugin *)new class_name(plog, tree); }
#endif


class fileDissectPlugin
{
public:
	fileDissectPlugin(void)
		: m_description(0), 
		  m_extensions(0), 
		  m_log(0), 
		  m_tree(0), 
		  m_file(0),
		  m_version(FD_PLUGIN_VERSION)
	{
	};
	virtual ~fileDissectPlugin(void)
	{
	};

	// do we support this file?
	virtual bool SupportsExtension(const wxChar *extension) = 0;
	virtual void Dissect(void) = 0;
	virtual void CloseFile(void) = 0;

	wxChar *m_description;
	wxChar *m_extensions;

	// passed from app
	wxLog *m_log;
	fileDissectTreeCtrl *m_tree;
	wxFileMap *m_file;

private:
	unsigned long m_version;
};

#endif
