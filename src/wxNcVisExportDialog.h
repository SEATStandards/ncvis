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
///		A class that manages the NcVis export dialog.
///	</summary>
class wxNcVisExportDialog : public wxDialog {

public:
	///	<summary>
	///		Command issued for export.
	///	</summary>
	enum ExportCommand {
		ExportCommand_Cancel,
		ExportCommand_OneFrame,
		ExportCommand_MultipleFrames
	};

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	wxNcVisExportDialog(
		wxWindow * parent,
		const wxString & title,
		const wxPoint & pos,
		const wxSize & size,
		const std::vector< wxString > & vecDimNames,
		const std::vector< std::pair<long,long> > & vecDimBounds,
		size_t sCurrentWindowWidth,
		size_t sCurrentWindowHeight
	);

	///	<summary>
	///		Initialize the wxNcVisExportDialog.
	///	</summary>
	void InitializeWindow();

	///	<summary>
	///		Event triggered when the dialog is closed.
	///	</summary>
	void OnClose(wxCloseEvent & event);

	///	<summary>
	///		Callback triggered when filename ellipses is clicked.
	///	</summary>
	void OnExportCountRadio(wxCommandEvent & event);

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
	///		Callback triggered when the export button is clicked.
	///	</summary>
	void OnExportClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the close button is clicked.
	///	</summary>
	void OnCancelClicked(wxCommandEvent & event);

public:
	///	<summary>
	///		Get the export command.
	///	</summary>
	ExportCommand GetExportCommand() const {
		return m_eExportCommand;
	}

	///	<summary>
	///		Get export image width.
	///	</summary>
	size_t GetExportWidth() const {
		return m_sExportWidth;
	}

	///	<summary>
	///		Get export image height.
	///	</summary>
	size_t GetExportHeight() const {
		return m_sExportHeight;
	}

	///	<summary>
	///		Get filename for export.
	///	</summary>
	const wxString & GetExportFilename() const {
		return m_wxstrExportFilename;
	}

	///	<summary>
	///		Get file path for export.
	///	</summary>
	const wxString & GetExportFilePath() const {
		return m_wxstrExportFilePath;
	}

	///	<summary>
	///		Get file pattern for export.
	///	</summary>
	const wxString & GetExportFilePattern() const {
		return m_wxstrExportFilePattern;
	}

	///	<summary>
	///		Get dimension name for export.
	///	</summary>
	const wxString & GetExportDimName() const {
		return m_strExportDim;
	}

	///	<summary>
	///		Get dimension start index for export.
	///	</summary>
	int GetExportDimBegin() const {
		return m_iExportDimBegin;
	}

	///	<summary>
	///		Get dimension end index for export.
	///	</summary>
	int GetExportDimEnd() const {
		return m_iExportDimEnd;
	}

protected:
	///	<summary>
	///		Export one frame radio button.
	///	</summary>
	wxRadioButton * m_wxExportOneFrameRadio;

	///	<summary>
	///		Export filename.
	///	</summary>
	wxStaticText * m_wxFilenameText;

	///	<summary>
	///		Export filename.
	///	</summary>
	wxTextCtrl * m_wxFilenameCtrl;

	///	<summary>
	///		Export filename button.
	///	</summary>
	wxButton * m_wxFilenameButton;

	///	<summary>
	///		Export multiple frames radio button.
	///	</summary>
	wxRadioButton * m_wxExportMultFrameRadio;

	///	<summary>
	///		Export file path static text.
	///	</summary>
	wxStaticText * m_wxDirText;

	///	<summary>
	///		Export file path.
	///	</summary>
	wxTextCtrl * m_wxFilePathCtrl;

	///	<summary>
	///		Export file path directory button.
	///	</summary>
	wxButton * m_wxDirButton;

	///	<summary>
	///		Export file pattern static text.
	///	</summary>
	wxStaticText * m_wxPatternText;

	///	<summary>
	///		Export file pattern control.
	///	</summary>
	wxTextCtrl * m_wxPatternCtrl;

	///	<summary>
	///		Export dimension radio buttons.
	///	</summary>
	std::vector<wxRadioButton *> m_vecExportDimRadioButtons;

	///	<summary>
	///		Export dimension text controls for start frame.
	///	</summary>
	std::vector<wxTextCtrl *> m_vecExportDimStartCtrl;

	///	<summary>
	///		Export dimension text controls for end frame.
	///	</summary>
	std::vector<wxTextCtrl *> m_vecExportDimEndCtrl;

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

public:
	///	<summary>
	///		Dimension names.
	///	</summary>
	std::vector< wxString > m_vecDimNames;

	///	<summary>
	///		Dimension bounds
	///	</summary>
	std::vector< std::pair<long,long> > m_vecDimBounds;

	///	<summary>
	///		Export command issued.
	///	</summary>
	ExportCommand m_eExportCommand;

	///	<summary>
	///		Filename for export.
	///	</summary>
	wxString m_wxstrExportFilename;

	///	<summary>
	///		File path for export.
	///	</summary>
	wxString m_wxstrExportFilePath;

	///	<summary>
	///		File pattern for export.
	///	</summary>
	wxString m_wxstrExportFilePattern;

	///	<summary>
	///		Dimension name for export.
	///	</summary>
	wxString m_strExportDim;

	///	<summary>
	///		Dimension start index for export.
	///	</summary>
	int m_iExportDimBegin;

	///	<summary>
	///		Dimension end index for export.
	///	</summary>
	int m_iExportDimEnd;

	///	<summary>
	///		Current window width.
	///	</summary>
	size_t m_sCurrentWindowWidth;

	///	<summary>
	///		Current window height.
	///	</summary>
	size_t m_sCurrentWindowHeight;

	///	<summary>
	///		Export width.
	///	</summary>
	size_t m_sExportWidth;

	///	<summary>
	///		Export height.
	///	</summary>
	size_t m_sExportHeight;

	wxDECLARE_EVENT_TABLE();
};

////////////////////////////////////////////////////////////////////////////////

#endif // _WXNCVISEXPORTDIALOG_H_
