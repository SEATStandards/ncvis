///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisExportDialog.h
///	\author  Paul Ullrich
///	\version July 20, 2022
///

#ifndef _WXNCVISEXPORTDIALOG_H_
#define _WXNCVISEXPORTDIALOG_H_

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include <vector>

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class that manages the NcVis options dialog.
///	</summary>
class wxNcVisExportDialog : public wxDialog {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	wxNcVisExportDialog(
		const wxString & title,
		const wxPoint & pos,
		const wxSize & size,
		const std::string & strNcVisResourceDir
	);

	///	<summary>
	///		Set the available dimensions for looping and their bounds.
	///	</summary>
	void SetDimensionBounds(
		const std::vector< wxString > & vecDimNames,
		const std::vector< std::pair<long,long> > & vecDimBounds
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
	///		Callback triggered when filename ellipses is clicked.
	///	</summary>
	void OnFilenameEllipsesClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when file path ellipses is clicked.
	///	</summary>
	void OnFilePathEllipsesClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when "use current image size" checkbox is clicked.
	///	</summary>
	void OnUseCurrentImageSizeClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the close button is clicked.
	///	</summary>
	void OnCancelClicked(wxCommandEvent & event);

public:
	///	<summary>
	///		Export filename.
	///	</summary>
	wxTextCtrl * m_wxFilenameCtrl;

	///	<summary>
	///		Export file path.
	///	</summary>
	wxTextCtrl * m_wxFilePathCtrl;

	///	<summary>
	///		Text box specifying output width.
	///	</summary>
	wxTextCtrl * m_wxImageWidthCtrl;

	///	<summary>
	///		Text box specifying output height.
	///	</summary>
	wxTextCtrl * m_wxImageHeightCtrl;

	///	<summary>
	///		Use current image width checkbox.
	///	</summary>
	wxCheckBox * m_wxUseCurrentImageCheckBox;

private:
	///	<summary>
	///		Dimension names.
	///	</summary>
	std::vector< wxString > m_vecDimNames;

	///	<summary>
	///		Dimension bounds
	///	</summary>
	std::vector< std::pair<long,long> > m_vecDimBounds;

	///	<summary>
	///		Export width.
	///	</summary>
	int m_nExportWidth;

	///	<summary>
	///		Export height.
	///	</summary>
	int m_nExportHeight;

	wxDECLARE_EVENT_TABLE();
};

////////////////////////////////////////////////////////////////////////////////

#endif // _WXNCVISEXPORTDIALOG_H_
