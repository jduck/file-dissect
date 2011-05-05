/*
 * Basic plugin loader class -- derive from me!
 * Joshua J. Drake <jdrake idefense.com>
 * 
 * wxPluginLoader.h:
 * 
 */
#ifndef __wxPluginLoader_h_
#define __wxPluginLoader_h_

#include <wx/string.h>
#include <wx/list.h>
#include <wx/filename.h>
#include <wx/dir.h>

#ifdef __WXMSW__
typedef HMODULE				libhandle_t;
#define	WXPLL_CLOSE			FreeLibrary
#define WXPLL_EXT			wxT("dll");
#define WXPLL_LOAD(p)		LoadLibrary(p)
#define WXPLL_LOOKUP		GetProcAddress
#define WXPLL_LOADERRMSG(x) 	wxString errMsg = wxT("Unknown error")
#else
#include <dlfcn.h>

typedef void *				libhandle_t;
#define WXPLL_CLOSE			dlclose
#define WXPLL_EXT			wxT("so");
#define WXPLL_LOAD(p)		dlopen((const char *)p,RTLD_NOW)
#define WXPLL_LOOKUP		dlsym
#define WXPLL_LOADERRMSG(x) 	const char *perr = (const char *)dlerror(); \
   				if (!perr) perr = "Unknown error"; \
   				wxString errMsg(perr, wxConvLibc, wxSTRING_MAXLEN)
#endif




#define WX_DECLARE_PLUGINLIST(apiclass, lcname) \
	typedef apiclass *(*pfn##apiclass)(wxLog *plog, fileDissectTreeCtrl *tree); \
	\
	class apiclass##__Module \
	{ \
	public: \
		apiclass##__Module(wxString &fname, libhandle_t h, pfn##apiclass ci, apiclass *ph) \
			: m_filename(fname), m_handle(h), m_create_instance(ci), m_instance(ph) { }; \
		~apiclass##__Module(void) { \
			if (m_instance) delete m_instance; \
			if (m_handle) WXPLL_CLOSE(m_handle); \
		}; \
		wxString m_filename; \
		libhandle_t m_handle; \
		pfn##apiclass m_create_instance; \
		apiclass *m_instance; \
	}; \
	WX_DECLARE_LIST(apiclass##__Module, apiclass##__List); \
	class lcname \
	{ \
	public: \
		lcname(void) { \
			m_list.DeleteContents(true); \
			wxString path = wxT("."); \
		}; \
		void LoadPlugins(wxString &path, wxLog *plog, fileDissectTreeCtrl *tree) \
		{ \
			m_list.clear(); \
			wxDir dir(path); \
			if (!dir.IsOpened()) return; \
			wxString filespec(wxT("*.")); \
			filespec += WXPLL_EXT; \
			wxString fname; \
			bool cont = dir.GetFirst(&fname, filespec, wxDIR_FILES | wxDIR_HIDDEN); \
			while (cont) { \
				wxString fullpath = path + fname; \
				pfn##apiclass pfn; \
				apiclass *ph = 0; \
				libhandle_t h; \
				if (!(h = WXPLL_LOAD(fullpath.fn_str()))) \
				{ \
				   	WXPLL_LOADERRMSG(); \
					wxLogError(wxT("Unable to load plugin \"%s\": %s"), \
						fname.c_str(), errMsg.c_str()); \
				   	cont = dir.GetNext(&fname); \
				   	continue; \
				} \
			   	if (!(pfn = (pfn##apiclass)WXPLL_LOOKUP(h, "create_instance"))) \
				{ \
					WXPLL_CLOSE(h); \
					wxLogError(wxT("Invalid plugin \"%s\": Unable to lookup export"), fname.c_str()); \
				   	cont = dir.GetNext(&fname); \
				   	continue; \
				} \
				if (!(ph = pfn(plog, tree))) \
				{ \
					WXPLL_CLOSE(h); \
					wxLogError(wxT("Invalid plugin \"%s\": Object factory failed"), fname.c_str()); \
				   	cont = dir.GetNext(&fname); \
				   	continue; \
				} \
				m_list.push_back(new apiclass##__Module(fname, h, pfn, ph)); \
			       	wxLogMessage(wxT("Registered support for \"%s\" (%s)"), \
		       			ph->m_description, fname.c_str()); \
				\
				cont = dir.GetNext(&fname); \
			} \
		}; \
		typedef apiclass##__List::iterator iterator; \
		iterator begin() { return m_list.begin(); } \
		iterator end() { return m_list.end(); } \
		apiclass##__List m_list; \
    };

#define WX_DEFINE_PLUGINLIST(apiclass) \
	WX_DEFINE_LIST(apiclass##__List);

#endif
