/*
 * fileDissect - a cross platform file dissection tool
 * Joshua J. Drake <jdrake idefense.com>
 *
 * fileDissectApp.cpp:
 * application launching code
 *
 * load plugins, process command line, create and show the frame
 */

#include "fileDissect.h"		// wxWidgets base
#include "fileDissectApp.h"


// implement it
IMPLEMENT_APP(fileDissectApp);


// the actual implementation
bool fileDissectApp::OnInit ()
{
	/* this only appears to process the command line...
	 * we do that ourselves later...
	 *
	if (!wxApp::OnInit())
		return false;
	 */

	// scanning for file format modules is done in frame the constructor

	// create application frame
	m_frame = new fileDissectFrame(APP_NAME);

	// check command line
	wxCmdLineParser parser(argc, argv);
   	parser.SetDesc(g_cmdLineDesc);
	parser.SetSwitchChars(wxT("-"));
	if (parser.Parse(false) == 0)
	{
		size_t pcount = parser.GetParamCount();

		// wxMessageBox(wxString::Format(wxT("Found %d parameters"), pcount), wxT("Information"));
		if (pcount > 0)
		{
			wxString param = parser.GetParam(0);
			m_frame->OpenFile(param);
		}
	}

	// show application frame
	m_frame->Show(true);
	SetTopWindow(m_frame);
	return true;
}
