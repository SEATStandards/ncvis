///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisExportDialog.cpp
///	\author  Paul Ullrich
///	\version July 20, 2022
///

#include "wxNcVisExportDialog.h"
#include "Exception.h"
#include <wx/stdpaths.h>
#include <wx/filefn.h> 
#include <string>
#include <cctype>

////////////////////////////////////////////////////////////////////////////////

enum {
	ID_EXPORT = 1,
	ID_CANCEL = 2,
	ID_EXPORTCOUNTRADIO1 = 3,
	ID_EXPORTCOUNTRADIO2 = 4,
	ID_FILENAMETEXTCTRL = 5,
	ID_FILENAMEELLIPSES = 6,
	ID_FILEPATHTEXTCTRL = 7,
	ID_FILEPATHELLIPSES = 8,
	ID_PATTERNTEXTCTRL = 9,
	ID_IMAGEWIDTHCTRL = 10,
	ID_IMAGEHEIGHTCTRL = 11,
	ID_IMAGESIZECHECKBOX = 12,
	ID_DIMNAME = 100,
	ID_DIMSTART = 200,
	ID_DIMEND = 300
};

////////////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxNcVisExportDialog, wxDialog)
	EVT_CLOSE(wxNcVisExportDialog::OnClose)
	EVT_BUTTON(ID_EXPORT, wxNcVisExportDialog::OnExportClicked)
	EVT_BUTTON(ID_CANCEL, wxNcVisExportDialog::OnCancelClicked)
	EVT_RADIOBUTTON(ID_EXPORTCOUNTRADIO1, wxNcVisExportDialog::OnExportCountRadio)
	EVT_RADIOBUTTON(ID_EXPORTCOUNTRADIO2, wxNcVisExportDialog::OnExportCountRadio)
	EVT_BUTTON(ID_FILENAMEELLIPSES, wxNcVisExportDialog::OnFilenameEllipsesClicked)
	EVT_BUTTON(ID_FILEPATHELLIPSES, wxNcVisExportDialog::OnFilePathEllipsesClicked)
	EVT_CHECKBOX(ID_IMAGESIZECHECKBOX, wxNcVisExportDialog::OnUseCurrentImageSizeClicked)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

wxNcVisExportDialog::wxNcVisExportDialog(
	wxWindow * parent,
	const wxString & title,
	const wxPoint & pos,
	const wxSize & size,
	const std::vector< wxString > & vecDimNames,
	const std::vector< std::pair<long,long> > & vecDimBounds,
	size_t sCurrentWindowWidth,
	size_t sCurrentWindowHeight
) :
	wxDialog(parent, wxID_ANY, title, pos, size, wxDEFAULT_DIALOG_STYLE),
	m_vecDimNames(vecDimNames),
	m_vecDimBounds(vecDimBounds),
	m_eExportCommand(ExportCommand_Cancel),
	m_sCurrentWindowWidth(sCurrentWindowWidth),
	m_sCurrentWindowHeight(sCurrentWindowHeight),
	m_sExportWidth(1560),
	m_sExportHeight(720)
{
	InitializeWindow();
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

	m_wxExportOneFrameRadio =
		new wxRadioButton(this, ID_EXPORTCOUNTRADIO1, _T("Export one frame"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	m_wxExportOneFrameRadio->SetValue(true);
	wxOneFrameBox->Add(m_wxExportOneFrameRadio, 0, wxEXPAND | wxALL, 2);
	wxOneFrameBox->AddSpacer(4);

	m_wxFilenameText = new wxStaticText(this, -1, _T("Filename:"), wxDefaultPosition, wxSize(80, nCtrlHeight));
	m_wxFilenameCtrl = new wxTextCtrl(this, ID_FILENAMETEXTCTRL, _T("ncvis_output.png"), wxDefaultPosition, wxSize(100, nCtrlHeight));
	m_wxFilenameButton = new wxButton(this, ID_FILENAMEELLIPSES, _T("..."), wxDefaultPosition, wxSize(34, nCtrlHeight));

	wxBoxSizer * wxOneFrameFilenameSizer = new wxBoxSizer(wxHORIZONTAL);
	wxOneFrameFilenameSizer->AddSpacer(20);
	wxOneFrameFilenameSizer->Add(m_wxFilenameText, 0);
	wxOneFrameFilenameSizer->Add(m_wxFilenameCtrl, 1, wxEXPAND);
	wxOneFrameFilenameSizer->Add(m_wxFilenameButton, 0);
	wxOneFrameBox->Add(wxOneFrameFilenameSizer, 1, wxEXPAND | wxALL, 2);

	// Select directory and pattern for multiple output
	wxStaticBoxSizer * wxMultipleFrameBox = new wxStaticBoxSizer(wxVERTICAL, this);
	
	m_wxExportMultFrameRadio =
		new wxRadioButton(this, ID_EXPORTCOUNTRADIO2, _T("Export multiple frames"), wxDefaultPosition, wxDefaultSize);
	m_wxExportMultFrameRadio->SetValue(false);
	wxMultipleFrameBox->Add(m_wxExportMultFrameRadio, 0, wxEXPAND | wxALL, 2);
	wxMultipleFrameBox->AddSpacer(4);

	wxBoxSizer * wxMultFrameFilenameSizer = new wxBoxSizer(wxHORIZONTAL);
	wxMultFrameFilenameSizer->AddSpacer(20);
	m_wxDirText = new wxStaticText(this, -1, _T("Directory:"), wxDefaultPosition, wxSize(80, nCtrlHeight));
	m_wxFilePathCtrl = new wxTextCtrl(this, ID_FILEPATHTEXTCTRL, wxGetCwd(), wxDefaultPosition, wxSize(100, nCtrlHeight));
	m_wxDirButton = new wxButton(this, ID_FILEPATHELLIPSES, _T("..."), wxDefaultPosition, wxSize(34, nCtrlHeight));

	wxMultFrameFilenameSizer->Add(m_wxDirText, 0);
	wxMultFrameFilenameSizer->Add(m_wxFilePathCtrl, 1, wxEXPAND);
	wxMultFrameFilenameSizer->Add(m_wxDirButton, 0);
	wxMultipleFrameBox->Add(wxMultFrameFilenameSizer, 0, wxEXPAND | wxALL, 2);

	m_wxPatternText = new wxStaticText(this, -1, _T("Pattern:"), wxDefaultPosition, wxSize(80, nCtrlHeight));
	m_wxPatternCtrl = new wxTextCtrl(this, ID_PATTERNTEXTCTRL, _T("ncvis%06i.png"), wxDefaultPosition, wxSize(100, nCtrlHeight));

	wxBoxSizer * wxMultFramePatternSizer = new wxBoxSizer(wxHORIZONTAL);
	wxMultFramePatternSizer->AddSpacer(20);
	wxMultFramePatternSizer->Add(m_wxPatternText, 0);
	wxMultFramePatternSizer->Add(m_wxPatternCtrl, 1, wxEXPAND);
	wxMultipleFrameBox->Add(wxMultFramePatternSizer, 0, wxEXPAND | wxALL, 2);

	if (m_vecDimNames.size() == 0) {
		m_wxExportMultFrameRadio->Enable(false);

	} else {
		_ASSERT(m_vecDimNames.size() == m_vecDimBounds.size());
		for (size_t d = 0; d < m_vecDimNames.size(); d++) {
			wxBoxSizer * wxDimSizer = new wxBoxSizer(wxHORIZONTAL);
			wxDimSizer->AddSpacer(20);
			wxRadioButton * wxDimButton =
				new wxRadioButton(this, ID_DIMNAME + d, wxString(m_vecDimNames[d]), wxDefaultPosition, wxSize(80, nCtrlHeight), (d==0)?(wxRB_GROUP):(0));
			wxTextCtrl * wxDimStartCtrl =
				new wxTextCtrl(this, ID_DIMSTART + d, wxString::Format("%li", m_vecDimBounds[d].first), wxDefaultPosition, wxSize(80, nCtrlHeight), wxTE_CENTRE);
			wxTextCtrl * wxDimEndCtrl =
				new wxTextCtrl(this, ID_DIMEND + d, wxString::Format("%li", m_vecDimBounds[d].second), wxDefaultPosition, wxSize(80, nCtrlHeight), wxTE_CENTRE);
			if (d == 0) {
				wxDimButton->SetValue(true);
			}

			m_vecExportDimRadioButtons.push_back(wxDimButton);
			m_vecExportDimStartCtrl.push_back(wxDimStartCtrl);
			m_vecExportDimEndCtrl.push_back(wxDimEndCtrl);

			wxDimSizer->Add(wxDimButton, 0);
			wxDimSizer->Add(wxDimStartCtrl, 1, wxEXPAND);
			wxDimSizer->Add(wxDimEndCtrl, 1, wxEXPAND);
			wxMultipleFrameBox->Add(wxDimSizer, 0, wxEXPAND | wxALL, 2);
		}
	}

	m_wxDirText->Enable(false);
	m_wxFilePathCtrl->Enable(false);
	m_wxDirButton->Enable(false);
	m_wxPatternText->Enable(false);
	m_wxPatternCtrl->Enable(false);

	for (int d = 0; d < m_vecExportDimRadioButtons.size(); d++) {
		m_vecExportDimRadioButtons[d]->Enable(false);
		m_vecExportDimStartCtrl[d]->Enable(false);
		m_vecExportDimEndCtrl[d]->Enable(false);
	}

	// Image size
	wxStaticBoxSizer * wxImageSizeBox = new wxStaticBoxSizer(wxVERTICAL, this);

	m_wxImageWidthCtrl = new wxTextCtrl(this, -1, wxString::Format("%lu", m_sExportWidth), wxDefaultPosition, wxSize(100, nCtrlHeight), wxTE_CENTRE);
	m_wxImageHeightCtrl = new wxTextCtrl(this, -1, wxString::Format("%lu", m_sExportHeight), wxDefaultPosition, wxSize(100, nCtrlHeight), wxTE_CENTRE);
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

	//Centre();
	//Show();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::OnClose(
	wxCloseEvent & event
) {
	EndModal(0);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::OnExportCountRadio(
	wxCommandEvent & event
) {
	_ASSERT(m_vecExportDimRadioButtons.size() == m_vecExportDimStartCtrl.size());
	_ASSERT(m_vecExportDimRadioButtons.size() == m_vecExportDimEndCtrl.size());

	if (event.GetId() == ID_EXPORTCOUNTRADIO1) {
		m_wxFilenameText->Enable(true);
		m_wxFilenameCtrl->Enable(true);
		m_wxFilenameButton->Enable(true);

		m_wxDirText->Enable(false);
		m_wxFilePathCtrl->Enable(false);
		m_wxDirButton->Enable(false);
		m_wxPatternText->Enable(false);
		m_wxPatternCtrl->Enable(false);

		for (int d = 0; d < m_vecExportDimRadioButtons.size(); d++) {
			m_vecExportDimRadioButtons[d]->Enable(false);
			m_vecExportDimStartCtrl[d]->Enable(false);
			m_vecExportDimEndCtrl[d]->Enable(false);
		}

	} else {
		m_wxFilenameText->Enable(false);
		m_wxFilenameCtrl->Enable(false);
		m_wxFilenameButton->Enable(false);

		m_wxDirText->Enable(true);
		m_wxFilePathCtrl->Enable(true);
		m_wxDirButton->Enable(true);
		m_wxPatternText->Enable(true);
		m_wxPatternCtrl->Enable(true);

		for (int d = 0; d < m_vecExportDimRadioButtons.size(); d++) {
			m_vecExportDimRadioButtons[d]->Enable(true);
			m_vecExportDimStartCtrl[d]->Enable(true);
			m_vecExportDimEndCtrl[d]->Enable(true);
		}
	}
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

		m_wxImageWidthCtrl->SetValue(wxString::Format("%lu", m_sCurrentWindowWidth));
		m_wxImageHeightCtrl->SetValue(wxString::Format("%lu", m_sCurrentWindowHeight));

	} else {
		m_wxImageWidthCtrl->Enable(true);
		m_wxImageHeightCtrl->Enable(true);

		m_wxImageWidthCtrl->SetValue(wxString::Format("%lu", m_sExportWidth));
		m_wxImageHeightCtrl->SetValue(wxString::Format("%lu", m_sExportHeight));
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::OnExportClicked(
	wxCommandEvent & event
) {
	bool fAllowExport = true;

	// Export one frame radio button clicked
	if (m_wxExportOneFrameRadio->GetValue()) {
		m_eExportCommand = ExportCommand_OneFrame;
		m_wxstrExportFilename = m_wxFilenameCtrl->GetValue();

	// Export multiple frames radio button clicked
	} else if (m_wxExportMultFrameRadio->GetValue()) {
		m_eExportCommand = ExportCommand_MultipleFrames;
		m_wxstrExportFilePath = m_wxFilePathCtrl->GetValue();
		m_wxstrExportFilePattern = m_wxPatternCtrl->GetValue();

		long dActive = (-1);
		for (long d = 0; d < m_vecExportDimRadioButtons.size(); d++) {
			if (m_vecExportDimRadioButtons[d]->GetValue()) {
				m_strExportDim = m_vecDimNames[d];
				m_iExportDimBegin = std::stoi(m_vecExportDimStartCtrl[d]->GetValue().ToStdString());
				m_iExportDimEnd = std::stoi(m_vecExportDimEndCtrl[d]->GetValue().ToStdString());
				dActive = d;
				break;
			}
		}

		_ASSERT(dActive != (-1));

		// Error checking
		if ((m_iExportDimBegin < m_vecDimBounds[dActive].first) ||
		    (m_iExportDimBegin > m_vecDimBounds[dActive].second) ||
		    (m_iExportDimEnd < m_vecDimBounds[dActive].first) ||
		    (m_iExportDimEnd > m_vecDimBounds[dActive].second)
		) {
			wxMessageDialog wxResultDialog(
				this,
				wxString::Format("One or more indices for dimension \"%s\" out of range. Value must be between %li and %li.",
					m_strExportDim.ToStdString().c_str(),
					m_vecDimBounds[dActive].first,
					m_vecDimBounds[dActive].second),
				wxString::Format("Index out of range"),
				wxOK | wxCENTRE | wxICON_EXCLAMATION);
			wxResultDialog.ShowModal();
			fAllowExport = false;
		}

		if (m_iExportDimBegin > m_iExportDimEnd) {
			wxMessageDialog wxResultDialog(
				this,
				wxString::Format("Begin index (%li) for dimension \"%s\" exceeds end index (%li).",
					m_strExportDim.ToStdString().c_str(),
					m_iExportDimBegin,
					m_iExportDimEnd),
				wxString::Format("Invalid incides"),
				wxOK | wxCENTRE | wxICON_EXCLAMATION);
			wxResultDialog.ShowModal();
			fAllowExport = false;
		}

		// Check pattern
		{
			// Check for at most one escape character and get its location
			int iPercentIx = (-1);
			for (int i = 0; i < m_wxstrExportFilePattern.length(); i++) {
				if (m_wxstrExportFilePattern[i] == '%') {
					if (iPercentIx != (-1)) {
						wxMessageDialog wxResultDialog(
							this,
							wxString::Format("Only one escape character %% allowed in file pattern \"%s\".",
								m_wxstrExportFilePattern.ToStdString().c_str()),
							wxString::Format("Invalid file pattern"),
							wxOK | wxCENTRE | wxICON_EXCLAMATION);
						wxResultDialog.ShowModal();
						fAllowExport = false;
						break;

					} else {
						iPercentIx = i;
					}
				}
			}

			// Check for at least one escape character
			if (fAllowExport && (iPercentIx == (-1))) {
				wxMessageDialog wxResultDialog(
					this,
					wxString::Format("At least one escape character %% required in file pattern \"%s\".",
						m_wxstrExportFilePattern.ToStdString().c_str()),
					wxString::Format("Invalid file pattern"),
					wxOK | wxCENTRE | wxICON_EXCLAMATION);
				wxResultDialog.ShowModal();
				fAllowExport = false;
			}

			// Check for malformed file pattern
			if (fAllowExport) {
				bool fMalformedPatternError = false;

				if (iPercentIx == m_wxstrExportFilePattern.length()-1) {
					fMalformedPatternError = true;

				} else {
					for (int i = iPercentIx+1; i < m_wxstrExportFilePattern.length(); i++) {
						if (isdigit(m_wxstrExportFilePattern[i])) {
							continue;
						}
						if (m_wxstrExportFilePattern[i] == 'i') {
							break;
						}
						fMalformedPatternError = true;
					}
				}

				if (fMalformedPatternError) {
					wxMessageDialog wxResultDialog(
						this,
						wxString::Format("Malformed file pattern \"%s\".",
							m_wxstrExportFilePattern.ToStdString().c_str()),
						wxString::Format("Invalid file pattern"),
						wxOK | wxCENTRE | wxICON_EXCLAMATION);
					wxResultDialog.ShowModal();
					fAllowExport = false;
				}
			}
		}

	} else {
		_EXCEPTION();
	}

	if (m_wxUseCurrentImageCheckBox->IsChecked()) {
		m_sExportWidth = m_sCurrentWindowWidth;
		m_sExportHeight = m_sCurrentWindowHeight;
	} else {
		m_sExportWidth = std::stoi(m_wxImageWidthCtrl->GetValue().ToStdString());
		m_sExportHeight = std::stoi(m_wxImageHeightCtrl->GetValue().ToStdString());
	}

	if (fAllowExport) {
		if ((m_sExportWidth < 200) ||
		    (m_sExportWidth > 100000) ||
		    (m_sExportHeight < 80) ||
		    (m_sExportHeight > 100000)
		) {
			wxMessageDialog wxResultDialog(
				this,
				wxString::Format("Width must be between 200 and 100000 pixels.  Height must be between 80 and 100000 pixels."),
				wxString::Format("Invalid export image size"),
				wxOK | wxCENTRE | wxICON_EXCLAMATION);
			wxResultDialog.ShowModal();
			fAllowExport = false;
		}
	}

	if (fAllowExport) {
		Close();
	} else {
		m_eExportCommand = ExportCommand_Cancel;
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisExportDialog::OnCancelClicked(
	wxCommandEvent & event
) {
	m_eExportCommand = ExportCommand_Cancel;
	Close();
}

////////////////////////////////////////////////////////////////////////////////

