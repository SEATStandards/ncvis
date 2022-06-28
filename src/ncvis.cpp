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
	wxInitAllImageHandlers();

	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <filename> [filename] ... " << std::endl;
		return false;
	}

	std::vector<std::string> vecFilenames;
	for (int v = 1; v < argc; v++) {
		vecFilenames.push_back(std::string(argv[v]));
	}

	wxNcVisFrame * frame =
		new wxNcVisFrame(
			"NcVis",
			wxPoint(50, 50),
			wxSize(450, 340),
			vecFilenames);

	frame->Show( true );

	return true;
}

////////////////////////////////////////////////////////////////////////////////

