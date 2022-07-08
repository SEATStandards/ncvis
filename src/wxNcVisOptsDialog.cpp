///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisOptsDialog.cpp
///	\author  Paul Ullrich
///	\version July 6, 2022
///

#include "wxNcVisOptsDialog.h"

////////////////////////////////////////////////////////////////////////////////

enum {
	ID_GRIDLINES = 1,
	ID_OVERLAYS = 2,
	ID_CLOSE = 3
};

////////////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxNcVisOptsDialog, wxDialog)
	EVT_CLOSE(wxNcVisOptsDialog::OnClose)
	EVT_BUTTON(ID_CLOSE, wxNcVisOptsDialog::OnCloseClicked)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

wxNcVisOptsDialog::wxNcVisOptsDialog(
	const wxString & title,
	const wxPoint & pos,
	const wxSize & size,
	const std::string & strNcVisResourceDir
) :
	wxDialog(NULL, wxID_ANY, title, pos, size, wxDEFAULT_DIALOG_STYLE),
	m_strNcVisResourceDir(strNcVisResourceDir)
{
	InitializeWindow();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisOptsDialog::InitializeWindow() {
	//wxPanel * panel = new wxPanel(this, -1);
	wxBoxSizer * vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * hbox = new wxBoxSizer(wxHORIZONTAL);

	wxCheckBox * wxCheckboxGridLines = new wxCheckBox(this, ID_GRIDLINES, wxT("Show major grid lines"));
	wxComboBox * wxComboBoxOverlays = new wxComboBox(this, ID_OVERLAYS, wxT("Overlays"));

	//wxButton * okButton = new wxButton(this, -1, wxT("Ok"), wxDefaultPosition, wxSize(70, 30));
	wxButton * closeButton = new wxButton(this, ID_CLOSE, wxT("Close"), wxDefaultPosition, wxSize(70, 30));

	hbox->Add(closeButton, 1, wxLEFT, 5);

	vbox->Add(wxCheckboxGridLines, 0, wxEXPAND | wxALL, 4);
	vbox->Add(wxComboBoxOverlays, 0, wxEXPAND | wxALL, 4);
	vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

	SetSizer(vbox);

	Centre();
	Show();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisOptsDialog::OnClose(
	wxCloseEvent & event
) {
	Hide();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisOptsDialog::OnCloseClicked(
	wxCommandEvent & event
) {
	Close();
}

////////////////////////////////////////////////////////////////////////////////

