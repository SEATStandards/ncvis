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

	if ((argc < 1) || (argc > 2)) {
		std::cout << "Usage: " << argv[0] << " [filename]" << std::endl;
		return false;
	}

	if (argc == 1) {
		wxNcVisFrame * frame =
			new wxNcVisFrame(
				"NcVis",
				wxPoint(50, 50),
				wxSize(450, 340));

		frame->Show( true );

	} else {
		wxNcVisFrame * frame =
			new wxNcVisFrame(
				"NcVis",
				wxPoint(50, 50),
				wxSize(450, 340),
				argv[1]);

		frame->Show( true );
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

