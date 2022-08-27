///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisOptionsDialog.cpp
///	\author  Paul Ullrich
///	\version August 25, 2022
///

#include "wxNcVisOptionsDialog.h"
#include "Exception.h"
#include <wx/stdpaths.h>
#include <wx/filefn.h> 
#include <string>
#include <cctype>

////////////////////////////////////////////////////////////////////////////////

enum {
	ID_OK = 1,
	ID_CANCEL = 2,
};

////////////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxNcVisOptionsDialog, wxDialog)
	EVT_CLOSE(wxNcVisOptionsDialog::OnClose)
	EVT_BUTTON(ID_OK, wxNcVisOptionsDialog::OnOkClicked)
	EVT_BUTTON(ID_CANCEL, wxNcVisOptionsDialog::OnCancelClicked)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

wxNcVisOptionsDialog::wxNcVisOptionsDialog(
	wxWindow * parent,
	const wxString & title,
	const wxPoint & pos,
	const wxSize & size,
	const NcVisPlotOptions & plotopts
) :
	wxDialog(parent, wxID_ANY, title, pos, size, wxDEFAULT_DIALOG_STYLE),
	m_fOkClicked(false),
	m_plotopts(plotopts)
{
	InitializeWindow();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisOptionsDialog::InitializeWindow() {

	// Ok cancel buttons
	wxBoxSizer * wxBottomButtons = new wxBoxSizer(wxHORIZONTAL);

	wxButton * wxOkButton = new wxButton(this, ID_OK, wxT("Ok"), wxDefaultPosition, wxDefaultSize);
	wxButton * wxCancelButton = new wxButton(this, ID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize);

	wxBottomButtons->Add(wxOkButton, 1, wxLEFT, 5);
	wxBottomButtons->Add(wxCancelButton, 1, wxLEFT | wxRIGHT, 5);

	int nCtrlHeight = wxOkButton->GetSize().GetHeight();

	// Visualization options
	wxStaticBoxSizer * wxVisualOptions = new wxStaticBoxSizer(wxVERTICAL, this);

	m_wxShowTitleCheckbox = new wxCheckBox(this, -1, _T("Show title"));
	m_wxShowTitleCheckbox->SetValue(m_plotopts.m_fShowTitle);

	m_wxShowTickmarkLabelsCheckbox = new wxCheckBox(this, -1, _T("Show tickmark labels"));
	m_wxShowTickmarkLabelsCheckbox->SetValue(m_plotopts.m_fShowTickmarkLabels);

	m_wxShowGridCheckbox = new wxCheckBox(this, -1, _T("Show grid"));
	m_wxShowGridCheckbox->SetValue(m_plotopts.m_fShowGrid);

	wxVisualOptions->Add(m_wxShowTitleCheckbox, 0, wxEXPAND | wxALL, 2);
	wxVisualOptions->Add(m_wxShowTickmarkLabelsCheckbox, 0, wxEXPAND | wxALL, 2);
	wxVisualOptions->Add(m_wxShowGridCheckbox, 0, wxEXPAND | wxALL, 2);

	// Full frame
	wxBoxSizer * wxFrameSizer = new wxBoxSizer(wxVERTICAL);
	wxFrameSizer->Add(wxVisualOptions, 0, wxALIGN_CENTER | wxALL, 4);
	wxFrameSizer->Add(wxBottomButtons, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

	SetSizerAndFit(wxFrameSizer);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisOptionsDialog::OnClose(
	wxCloseEvent & event
) {
	EndModal(0);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisOptionsDialog::OnOkClicked(
	wxCommandEvent & event
) {
	m_fOkClicked = true;

	m_plotopts.m_fShowTitle = m_wxShowTitleCheckbox->IsChecked();

	m_plotopts.m_fShowTickmarkLabels = m_wxShowTickmarkLabelsCheckbox->IsChecked();

	m_plotopts.m_fShowGrid = m_wxShowGridCheckbox->IsChecked();

	Close();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisOptionsDialog::OnCancelClicked(
	wxCommandEvent & event
) {
	m_fOkClicked = false;

	Close();
}

////////////////////////////////////////////////////////////////////////////////

