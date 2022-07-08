///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisOptsDialog.h
///	\author  Paul Ullrich
///	\version July 6, 2022
///

#ifndef _WXNCVISOPTSDIALOG_H_
#define _WXNCVISOPTSDIALOG_H_

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class that manages the NcVis options dialog.
///	</summary>
class wxNcVisOptsDialog : public wxDialog {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	wxNcVisOptsDialog(
		const wxString & title,
		const wxPoint & pos,
		const wxSize & size,
		const std::string & strNcVisResourceDir
	);

	///	<summary>
	///		Initialize the wxNcVisFrame.
	///	</summary>
	void InitializeWindow();

	///	<summary>
	///		Event triggered when the dialog is closed.
	///	</summary>
	void OnClose(wxCloseEvent & event);

	///	<summary>
	///		Callback triggered when the close button is clicked.
	///	</summary>
	void OnCloseClicked(wxCommandEvent & event);

private:
	///	<summary>
	///		Folder containing NcVis resources.
	///	</summary>
	std::string m_strNcVisResourceDir;

	wxDECLARE_EVENT_TABLE();
};

////////////////////////////////////////////////////////////////////////////////

#endif // _WXNCVISOPTSDIALOG_H_
