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
		return false;
	}

	int iarg = 1;

	std::map<std::string, std::string> mapOptions;
	for (; iarg < argc; iarg++) {
		if (argv[iarg][0] == '-') {
			if (iarg+1 < argc) {
				mapOptions.insert(
					std::pair<std::string, std::string>(
						argv[iarg], argv[iarg+1]));
				iarg++;
			}
		} else {
			break;
		}
	}

	std::vector<std::string> vecFilenames;
	for (; iarg < argc; iarg++) {
		vecFilenames.push_back(std::string(argv[iarg]));
	}

	if (vecFilenames.size() == 0) {
		std::cout << "ERROR: No filenames specified" << std::endl;
		std::cout << "Usage: " << argv[0] << " [options] <filename> [filename] ... " << std::endl;
		return false;
	}

	char * szNcVisResourceDir = std::getenv("NCVIS_RESOURCE_DIR");
	if (szNcVisResourceDir == NULL) {
		std::cout << "ERROR: Please set environment variable \"NCVIS_RESOURCE_DIR\"" << std::endl;
		return false;
	}

	wxNcVisFrame * frame =
		new wxNcVisFrame(
			"NcVis",
			wxPoint(50, 50),
			wxSize(842, 462),
			szNcVisResourceDir,
			mapOptions,
			vecFilenames);

	frame->Show( true );

	return true;
}

////////////////////////////////////////////////////////////////////////////////

