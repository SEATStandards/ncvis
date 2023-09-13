///////////////////////////////////////////////////////////////////////////////
///
///	\file    ncvis.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "wxNcVisFrame.h"
#include "GridDataSampler.h"
#include "netcdfcpp.h"

#include <cstdlib>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Class reprsenting the application.
///	</summary>
class wxNcVisApp : public wxApp {
public:
	///	<summary>
	///		Callback triggered when app is initialized.
	///	</summary>
	virtual bool OnInit();
};

////////////////////////////////////////////////////////////////////////////////

wxIMPLEMENT_APP(wxNcVisApp);

////////////////////////////////////////////////////////////////////////////////

bool wxNcVisApp::OnInit() {

	// Turn off fatal errors in NetCDF
	NcError error(NcError::silent_nonfatal);

	// Process command line arguments
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " [options] <filename> [filename] ... " << std::endl;
		exit(-1);
	}

	int iarg = 1;

	std::map<wxString, wxString> mapOptions;
	for (; iarg < argc; iarg++) {
		if (argv[iarg][0] == '-') {
			if ((wxString("-g") == argv[iarg]) ||
			    (wxString("-uxc") == argv[iarg]) ||
			    (wxString("-uyc") == argv[iarg]) ||
				(wxString("-mcr") == argv[iarg])
			) {
				if (iarg+1 == argc) {
					std::cout << "Option " << argv[iarg] << " missing required parameter" << std::endl;
					exit(-1);
				}
				mapOptions.insert(
					std::pair<wxString, wxString>(
						argv[iarg], argv[iarg+1]));
				iarg++;

			} else {
				mapOptions.insert(
					std::pair<wxString, wxString>(
						wxString(argv[iarg]), wxString("")));
			}
		} else {
			break;
		}
	}

	std::vector<wxString> vecFilenames;
	for (; iarg < argc; iarg++) {
		vecFilenames.push_back(wxString(argv[iarg]));
	}

	if (vecFilenames.size() == 0) {
		std::cout << "ERROR: No filenames specified" << std::endl;
		std::cout << "Usage: " << argv[0] << " [options] <filename> [filename] ... " << std::endl;
		exit(-1);
	}

	wxString wxstrNcVisResourceDir = wxString(std::getenv("NCVIS_RESOURCE_DIR"));
	if (wxstrNcVisResourceDir.length() == 0) {
		wxFileName wxfn(wxStandardPaths::Get().GetExecutablePath());
		wxfn.AppendDir(_T("resources"));
		wxfn.MakeAbsolute();
		wxstrNcVisResourceDir = wxfn.GetPath();
		if (!wxfn.DirExists()) {
			std::cout << "ERROR: Cannot open resource directory \"" << wxstrNcVisResourceDir << "\"" << std::endl;
			std::cout << "Set environment variable NCVIS_RESOURCE_DIR instead" << std::endl;
			exit(-1);
		}
	}

	wxNcVisFrame * frame =
		new wxNcVisFrame(
			"NcVis",
			wxPoint(50, 50),
			wxSize(842, 462),
			wxstrNcVisResourceDir,
			mapOptions,
			vecFilenames);

	frame->Show( true );

	return true;
}

////////////////////////////////////////////////////////////////////////////////

