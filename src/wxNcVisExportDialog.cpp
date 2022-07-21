///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisExportDialog.cpp
///	\author  Paul Ullrich
///	\version July 20, 2022
///

#include "wxNcVisExportDialog.h"
#include "Exception.h"

////////////////////////////////////////////////////////////////////////////////

enum {
	ID_EXPORT = 1,
	ID_CANCEL = 2,
	ID_FILENAMETEXTCTRL = 3,
	ID_FILENAMEELLIPSES = 4,
	ID_FILEPATHTEXTCTRL = 5,
	ID_FILEPATHELLIPSES = 6,
	ID_PATTERNTEXTCTRL = 7,
	ID_IMAGEWIDTHCTRL = 8,
	ID_IMAGEHEIGHTCTRL = 9,
	ID_IMAGESIZECHECKBOX = 10,
	ID_DIMNAME = 100,
	ID_DIMSTART = 200,
	ID_DIMEND = 300
};

////////////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxNcVisExportDialog, wxDialog)
	EVT_CLOSE(wxNcVisExportDialog::OnClose)
	EVT_BUTTON(ID_CANCEL, wxNcVisExportDialog::OnCancelClicked)
	EVT_BUTTON(ID_FILENAMEELLIPSES, wxNcVisExportDialog::OnFilenameEllipsesClicked)
	EVT_BUTTON(ID_FILEPATHELLIPSES, wxNcVisExportDialog::OnFilePathEllipsesClicked)
	EVT_CHECKBOX(ID_IMAGESIZECHECKBOX, wxNcVisExportDialog::OnUseCurrentImageSizeClicked)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

wxNcVisExportDialog::wxNcVisExportDialog(
	const wxString & title,
	const wxPoint & pos,
	const wxSize & size,
	const std::string & strNcVisResourceDir
) :
	wxDialog(NULL, wxID_ANY, title, pos, size, wxDEFAULT_DIALOG_STYLE),
	m_nExportWidth(1440),
	m_nExportHeight(720)
{
	InitializeWindow();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::SetDimensionBounds(
	const std::vector< wxString > & vecDimNames,
	const std::vector< std::pair<long,long> > & vecDimBounds
) {
	m_vecDimNames = vecDimNames;
	m_vecDimBounds = vecDimBounds;
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::InitializeWindow() {

	// Ok cancel buttons
	wxBoxSizer * wxBottomButtons = new wxBoxSizer(wxHORIZONTAL);

	wxButton * wxOkButton = new wxButton(this, ID_EXPORT, wxT("Export"), wxDefaultPosition, wxDefaultSize);
	wxButton * wxCancelButton = new wxButton(this, ID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize);

	wxBottomButtons->Add(wxOkButton, 1, wxLEFT, 5);
	wxBottomButtons->Add(wxCancelButton, 1, wxLEFT, 5);

	int nCtrlHeight = wxOkButton->GetSize().GetHeight();

	// Select filename for single output
	wxStaticBoxSizer * wxOneFrameBox = new wxStaticBoxSizer(wxVERTICAL, this);

	wxRadioButton * wxExportOneFrameRadio =
		new wxRadioButton(this, -1, _T("Export one frame"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	wxExportOneFrameRadio->SetValue(true);
	wxOneFrameBox->Add(wxExportOneFrameRadio, 0, wxEXPAND | wxALL, 2);
	wxOneFrameBox->AddSpacer(4);

	m_wxFilenameCtrl = new wxTextCtrl(this, ID_FILENAMETEXTCTRL, _T(""), wxDefaultPosition, wxSize(100, nCtrlHeight));

	wxBoxSizer * wxOneFrameFilenameSizer = new wxBoxSizer(wxHORIZONTAL);
	wxOneFrameFilenameSizer->AddSpacer(20);
	wxOneFrameFilenameSizer->Add(new wxStaticText(this, -1, _T("Filename:"), wxDefaultPosition, wxSize(80, nCtrlHeight)), 0);
	wxOneFrameFilenameSizer->Add(m_wxFilenameCtrl, 1, wxEXPAND);
	wxOneFrameFilenameSizer->Add(new wxButton(this, ID_FILENAMEELLIPSES, _T("..."), wxDefaultPosition, wxSize(34, nCtrlHeight)), 0);
	wxOneFrameBox->Add(wxOneFrameFilenameSizer, 1, wxEXPAND | wxALL, 2);

	// Select directory and pattern for multiple output
	wxStaticBoxSizer * wxMultipleFrameBox = new wxStaticBoxSizer(wxVERTICAL, this);
	
	wxRadioButton * wxExportMultFrameRadio =
		new wxRadioButton(this, -1, _T("Export multiple frames"), wxDefaultPosition, wxDefaultSize);
	wxExportMultFrameRadio->SetValue(false);
	wxMultipleFrameBox->Add(wxExportMultFrameRadio, 0, wxEXPAND | wxALL, 2);
	wxMultipleFrameBox->AddSpacer(4);

	wxBoxSizer * wxMultFrameFilenameSizer = new wxBoxSizer(wxHORIZONTAL);
	wxMultFrameFilenameSizer->AddSpacer(20);
	wxStaticText * wxDirText = new wxStaticText(this, -1, _T("Directory:"), wxDefaultPosition, wxSize(80, nCtrlHeight));
	m_wxFilePathCtrl = new wxTextCtrl(this, ID_FILEPATHTEXTCTRL, _T(""), wxDefaultPosition, wxSize(100, nCtrlHeight));
	wxButton * wxDirButton = new wxButton(this, ID_FILEPATHELLIPSES, _T("..."), wxDefaultPosition, wxSize(34, nCtrlHeight));

	wxMultFrameFilenameSizer->Add(wxDirText, 0);
	wxMultFrameFilenameSizer->Add(m_wxFilePathCtrl, 1, wxEXPAND);
	wxMultFrameFilenameSizer->Add(wxDirButton, 0);
	wxMultipleFrameBox->Add(wxMultFrameFilenameSizer, 0, wxEXPAND | wxALL, 2);

	wxStaticText * wxPatternText = new wxStaticText(this, -1, _T("Pattern:"), wxDefaultPosition, wxSize(80, nCtrlHeight));
	wxTextCtrl * wxPatternCtrl = new wxTextCtrl(this, ID_PATTERNTEXTCTRL, _T(""), wxDefaultPosition, wxSize(100, nCtrlHeight));

	wxBoxSizer * wxMultFramePatternSizer = new wxBoxSizer(wxHORIZONTAL);
	wxMultFramePatternSizer->AddSpacer(20);
	wxMultFramePatternSizer->Add(wxPatternText, 0);
	wxMultFramePatternSizer->Add(wxPatternCtrl, 1, wxEXPAND);
	wxMultipleFrameBox->Add(wxMultFramePatternSizer, 0, wxEXPAND | wxALL, 2);

	m_vecDimNames.push_back("time");
	m_vecDimBounds.push_back(std::pair<long,long>(0,10));

	if (m_vecDimNames.size() == 0) {
		wxExportMultFrameRadio->Enable(false);
		wxDirText->Enable(false);
		m_wxFilePathCtrl->Enable(false);
		wxDirButton->Enable(false);
		wxPatternText->Enable(false);
		wxPatternCtrl->Enable(false);

	} else {
		_ASSERT(m_vecDimNames.size() == m_vecDimBounds.size());
		for (size_t d = 0; d < m_vecDimNames.size(); d++) {
			wxBoxSizer * wxDimSizer = new wxBoxSizer(wxHORIZONTAL);
			wxDimSizer->AddSpacer(20);
			wxRadioButton * wxDimButton =
				new wxRadioButton(this, ID_DIMNAME + d, wxString(m_vecDimNames[d]), wxDefaultPosition, wxSize(80, nCtrlHeight), (d==0)?(wxRB_GROUP):(0));
			if (d == 0) {
				wxDimButton->SetValue(true);
			}
			wxDimSizer->Add(wxDimButton, 0);
			wxDimSizer->Add(new wxTextCtrl(this, ID_DIMSTART + d, _T(""), wxDefaultPosition, wxSize(80, nCtrlHeight), wxTE_CENTRE), 1, wxEXPAND);
			wxDimSizer->Add(new wxTextCtrl(this, ID_DIMEND + d, _T(""), wxDefaultPosition, wxSize(80, nCtrlHeight), wxTE_CENTRE), 1, wxEXPAND);
			wxMultipleFrameBox->Add(wxDimSizer, 0, wxEXPAND | wxALL, 2);
		}
	}

	// Image size
	wxStaticBoxSizer * wxImageSizeBox = new wxStaticBoxSizer(wxVERTICAL, this);

	m_wxImageWidthCtrl = new wxTextCtrl(this, -1, wxString::Format("%i", m_nExportWidth), wxDefaultPosition, wxSize(100, nCtrlHeight), wxTE_CENTRE);
	m_wxImageHeightCtrl = new wxTextCtrl(this, -1, wxString::Format("%i", m_nExportHeight), wxDefaultPosition, wxSize(100, nCtrlHeight), wxTE_CENTRE);
	m_wxUseCurrentImageCheckBox = new wxCheckBox(this, ID_IMAGESIZECHECKBOX, _T("Use current window size"), wxDefaultPosition, wxDefaultSize);

	wxBoxSizer * wxImageSizeTextCtrlSizer = new wxBoxSizer(wxHORIZONTAL);
	wxImageSizeTextCtrlSizer->Add(new wxStaticText(this, -1, _T("Width:"), wxDefaultPosition, wxSize(50, nCtrlHeight)), 0);
	wxImageSizeTextCtrlSizer->Add(m_wxImageWidthCtrl, 1, wxEXPAND);
	wxImageSizeTextCtrlSizer->AddSpacer(8);
	wxImageSizeTextCtrlSizer->Add(new wxStaticText(this, -1, _T("Height:"), wxDefaultPosition, wxSize(50, nCtrlHeight)), 0);
	wxImageSizeTextCtrlSizer->Add(m_wxImageHeightCtrl, 1, wxEXPAND);

	wxImageSizeBox->Add(wxImageSizeTextCtrlSizer, 1, wxEXPAND | wxALL, 2);
	wxImageSizeBox->AddSpacer(4);
	wxImageSizeBox->Add(m_wxUseCurrentImageCheckBox);

	// Full frame
	wxBoxSizer * wxFrameSizer = new wxBoxSizer(wxVERTICAL);
	wxFrameSizer->Add(wxOneFrameBox, 0, wxEXPAND | wxALL, 4);
	wxFrameSizer->Add(wxMultipleFrameBox, 0, wxEXPAND | wxALL, 4);
	wxFrameSizer->Add(wxImageSizeBox, 0, wxEXPAND | wxALL, 4);
	wxFrameSizer->Add(wxBottomButtons, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

	SetSizerAndFit(wxFrameSizer);

	Centre();
	Show();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::OnFilenameEllipsesClicked(
	wxCommandEvent & event
) {
	wxFileDialog saveFileDialog(this, _T("Export Filename"), "", "", "PNG files (*.png)|*.png", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

	if (saveFileDialog.ShowModal() == wxID_CANCEL) {
		return;
	}

	m_wxFilenameCtrl->SetValue(saveFileDialog.GetPath());
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::OnFilePathEllipsesClicked(
	wxCommandEvent & event
) {
	wxDirDialog saveDirDialog(this, _T("Export File Path"), "", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

	if (saveDirDialog.ShowModal() == wxID_CANCEL) {
		return;
	}

	m_wxFilePathCtrl->SetValue(saveDirDialog.GetPath());
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::OnUseCurrentImageSizeClicked(
	wxCommandEvent & event
) {
	if (m_wxUseCurrentImageCheckBox->IsChecked()) {
		m_wxImageWidthCtrl->Enable(false);
		m_wxImageHeightCtrl->Enable(false);
	} else {
		m_wxImageWidthCtrl->Enable(true);
		m_wxImageHeightCtrl->Enable(true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::OnClose(
	wxCloseEvent & event
) {
	Hide();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::OnCancelClicked(
	wxCommandEvent & event
) {
	Close();
}

////////////////////////////////////////////////////////////////////////////////

