/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectApp.h:
 * application declarations
 */
#ifndef __fileDissectApp_h_
#define __fileDissectApp_h_

#include "fileDissect.h"		// wxWidgets base
#include <wx/cmdline.h>			// command line

#include "fileDissectFrame.h"

#define APP_NAME 	wxT("fileDissect")

/* 
 * derive our application from wxApp
 */
class fileDissectApp : public wxApp
{
public:
	bool OnInit();

	// frame window
	fileDissectFrame *m_frame;
};


/* command line parameters */
static const wxCmdLineEntryDesc g_cmdLineDesc[] =
{
	{ wxCMD_LINE_PARAM, NULL, NULL, wxT("input file"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
	{ wxCMD_LINE_NONE, NULL, NULL, NULL, wxCMD_LINE_VAL_NONE, 0 }
};


DECLARE_APP(fileDissectApp);

#endif
