///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisOptionsDialog.h
///	\author  Paul Ullrich
///	\version August 25, 2022
///

#ifndef _WXNCVISOPTIONSDIALOG_H_
#define _WXNCVISOPTIONSDIALOG_H_

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "NcVisPlotOptions.h"

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class that manages the NcVis options dialog.
///	</summary>
class wxNcVisOptionsDialog : public wxDialog {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	wxNcVisOptionsDialog(
		wxWindow * parent,
		const wxString & title,
		const wxPoint & pos,
		const wxSize & size,
		const NcVisPlotOptions & plotopts
	);

	///	<summary>
	///		Initialize the wxNcVisOptionsDialogDialog.
	///	</summary>
	void InitializeWindow();

	///	<summary>
	///		Event triggered when the dialog is closed.
	///	</summary>
	void OnClose(wxCloseEvent & event);

	///	<summary>
	///		Callback triggered when "use current image size" checkbox is clicked.
	///	</summary>
	void OnUseCurrentImageSizeClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the export button is clicked.
	///	</summary>
	void OnOkClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the close button is clicked.
	///	</summary>
	void OnCancelClicked(wxCommandEvent & event);

public:
	///	<summary>
	///		Return true of the ok button was clicked on dialog close.
	///	</summary>
	bool IsOkClicked() const {
		return m_fOkClicked;
	}

	///	<summary>
	///		Get the plot options.
	///	</summary>
	const NcVisPlotOptions & GetPlotOptions() const {
		return m_plotopts;
	}

protected:
	///	<summary>
	///		Show title checkbox.
	///	</summary>
	wxCheckBox * m_wxShowTitleCheckbox;

	///	<summary>
	///		Show tickmark labels checkbox.
	///	</summary>
	wxCheckBox * m_wxShowTickmarkLabelsCheckbox;

	///	<summary>
	///		Show grid checkbox.
	///	</summary>
	wxCheckBox * m_wxShowGridCheckbox;

protected:
	///	<summary>
	///		Ok button has been clicked.
	///	</summary>
	bool m_fOkClicked;

	///	<summary>
	///		Plot options.
	///	</summary>
	NcVisPlotOptions m_plotopts;

	wxDECLARE_EVENT_TABLE();
};

////////////////////////////////////////////////////////////////////////////////

#endif // _WXNCVISOPTIONSDIALOG_H_
