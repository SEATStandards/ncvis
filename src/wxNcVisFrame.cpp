///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisFrame.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#include "wxNcVisFrame.h"

////////////////////////////////////////////////////////////////////////////////

enum {
	ID_Hello = 1,
	ID_COLORMAP = 2,
	ID_DATATRANS = 3,
	ID_VARSELECTOR = 4,
	ID_BOUNDS = 5,
	ID_ZOOMOUT = 6,
	ID_DIMEDIT = 100,
	ID_DIMDOWN = 200,
	ID_DIMUP = 300,
};

////////////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxNcVisFrame, wxFrame)
	EVT_MENU(ID_Hello,   wxNcVisFrame::OnHello)
	EVT_MENU(wxID_EXIT,  wxNcVisFrame::OnExit)
	EVT_MENU(wxID_ABOUT, wxNcVisFrame::OnAbout)
	EVT_BUTTON(ID_COLORMAP, wxNcVisFrame::OnColorMapClicked)
	EVT_BUTTON(ID_DATATRANS, wxNcVisFrame::OnDataTransClicked)
	EVT_COMBOBOX(ID_VARSELECTOR, wxNcVisFrame::OnVariableSelected)
	EVT_TEXT(ID_BOUNDS, wxNcVisFrame::OnBoundsChanged)
	EVT_BUTTON(ID_ZOOMOUT, wxNcVisFrame::OnZoomOutClicked)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

wxNcVisFrame::wxNcVisFrame(
	const wxString & title,
	const wxPoint & pos,
	const wxSize & size
) :
	wxFrame(NULL, wxID_ANY, title, pos, size),
	m_strFilename(""),
	m_pncfile(NULL),
	m_wxColormapButton(NULL),
	m_imagepanel(NULL),
	m_sColorMap(0)
{
	InitializeWindow();
}

////////////////////////////////////////////////////////////////////////////////

wxNcVisFrame::wxNcVisFrame(
	const wxString & title,
	const wxPoint & pos,
	const wxSize & size,
	const wxString & strFilename
) :
	wxFrame(NULL, wxID_ANY, title, pos, size),
	m_strFilename(""),
	m_wxColormapButton(NULL),
	m_imagepanel(NULL),
	m_sColorMap(0)
{
	OpenFile(strFilename);

	InitializeWindow();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::InitializeWindow() {

	// Create menu
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_Hello, "&Hello...\tCtrl-H",
					 "Help string shown in status bar for this menu item");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, "&File" );
	menuBar->Append( menuHelp, "&Help" );

	SetMenuBar( menuBar );

	// Create a top-level panel to hold all the contents of the frame
    wxPanel * panel = new wxPanel(this, wxID_ANY);

	// Vertical structure
	wxSizer * topsizer = new wxBoxSizer(wxVERTICAL);
/*
	// Info box
	wxTextCtrl *infobox = new wxTextCtrl(this, -1,
      wxT("Hi!"), wxDefaultPosition, wxDefaultSize,
      wxTE_MULTILINE | wxTE_RICH , wxDefaultValidator, wxTextCtrlNameStr);
      Maximize();
*/
	// First line of controls
	wxBoxSizer *ctrlsizer = new wxBoxSizer(wxHORIZONTAL);

	ColorMapLibrary colormaplib;
	m_wxColormapButton = new wxButton(this, ID_COLORMAP, colormaplib.GetColorMapName(0));
	m_wxDataTransButton = new wxButton(this, ID_DATATRANS, _T("Linear"));

	ctrlsizer->Add(m_wxColormapButton, 0, wxEXPAND | wxALL, 2);
	ctrlsizer->Add(m_wxDataTransButton, 0, wxEXPAND | wxALL, 2);
	ctrlsizer->Add(new wxButton(this, -1, _T("Axes")), 0, wxEXPAND | wxALL, 2);
	ctrlsizer->Add(new wxButton(this, -1, _T("Range")), 0, wxEXPAND | wxALL, 2);
	ctrlsizer->Add(new wxButton(this, -1, _T("Auto (qt)")), 0, wxEXPAND | wxALL, 2);
	ctrlsizer->Add(new wxButton(this, -1, _T("Export")), 0, wxEXPAND | wxALL, 2);

	ctrlsizer->SetSizeHints(this);

	// Variable selector
	wxBoxSizer *varsizer = new wxBoxSizer(wxHORIZONTAL);

	for (int vc = 0; vc < 10; vc++) {
		m_vecwxVarSelector[vc] = NULL;
	}
	for (int vc = 0; vc < 10; vc++) {
		if (m_vecVarNames[vc].size() == 0) {
			continue;
		}

		char szName[16];
		snprintf(szName, 16, "(%lu) %iD vars", m_vecVarNames[vc].size(), vc);
		m_vecwxVarSelector[vc] = new wxComboBox(this, ID_VARSELECTOR, szName);
		m_vecwxVarSelector[vc]->SetEditable(false);
		for (int v = 0; v < m_vecVarNames[vc].size(); v++) {
			m_vecwxVarSelector[vc]->Append(wxString(m_vecVarNames[vc][v]));
		}
		varsizer->Add(m_vecwxVarSelector[vc], 0, wxEXPAND | wxALL, 2);
	}

	// Dimensions
	m_dimsizer = new wxBoxSizer(wxHORIZONTAL);
	m_dimsizer->Add(new wxStaticText(this, -1, ""), 0, wxALL, 4);

	// Displayed bounds
	wxBoxSizer *boundssizer = new wxBoxSizer(wxHORIZONTAL);

	for (int i = 0; i < 4; i++) {
		m_vecwxImageBounds[i] = new wxTextCtrl(this, ID_BOUNDS, "", wxDefaultPosition, wxSize(100, 22), wxTE_CENTRE);
	}
	boundssizer->Add(new wxStaticText(this, -1, "X:"), 0, wxEXPAND | wxALL, 4);
	boundssizer->Add(m_vecwxImageBounds[0], 0, wxEXPAND | wxALL, 2);
	boundssizer->Add(m_vecwxImageBounds[1], 0, wxEXPAND | wxALL, 2);
	boundssizer->Add(new wxStaticText(this, -1, "Y:"), 0, wxEXPAND | wxALL, 4);
	boundssizer->Add(m_vecwxImageBounds[2], 0, wxEXPAND | wxALL, 2);
	boundssizer->Add(m_vecwxImageBounds[3], 0, wxEXPAND | wxALL, 2);
	boundssizer->Add(new wxButton(this, ID_ZOOMOUT, _T("Zoom Out")), 0, wxEXPAND | wxALL, 2);

	// Image panel
	m_imagepanel = new wxImagePanel(this, &m_gdsqt, &m_data);

	//topsizer->Add(infobox, 0, wxALIGN_CENTER, 0);
	topsizer->Add(ctrlsizer, 0, wxALIGN_CENTER, 0);
	topsizer->Add(varsizer, 0, wxALIGN_CENTER, 0);
	topsizer->Add(m_dimsizer, 0, wxALIGN_CENTER, 0);
	topsizer->Add(boundssizer, 0, wxALIGN_CENTER, 0);
	topsizer->Add(m_imagepanel, 1, wxALIGN_CENTER | wxSHAPED);

	CreateStatusBar();

	// Status bar
	SetStatusText( _T("NcVis 2022.06.26") );

	SetSizerAndFit(topsizer);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OpenFile(
	const wxString & strFilename
) {
	// Open the NetCDF file
	m_pncfile = new NcFile(strFilename.c_str());
	if (!m_pncfile->is_valid()) {
		std::cout << "Error: Unable to open file \"" << strFilename << "\"" << std::endl;
		_EXCEPTION();
	}
	for (long v = 0; v < m_pncfile->num_vars(); v++) {
		NcVar * var = m_pncfile->get_var(v);
		size_t sVarDims = static_cast<size_t>(var->num_dims());
		if (var->num_dims() >= 10) {
			std::cout << "Error: Only variables of dimension <= 10 supported" << std::endl;
			_EXCEPTION();
		}
		m_vecVarNames[sVarDims].push_back(std::string(var->name()));
	}

	m_strFilename = strFilename;

	// Initialize the GridDataSampler
	m_gdsqt.Initialize(strFilename.ToStdString());
	//m_gdskd.Initialize(strFilename.ToStdString());

	// Allocate space for the data
	NcVar * varLon = m_pncfile->get_var("lon");
	if (varLon->num_dims() != 1) {
		std::cout << "Error: Only 1D \"lon\" variable supported" << std::endl;
		_EXCEPTION();
	}
	m_data.Allocate(varLon->get_dim(0)->size());
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::LoadData() {
	m_varActive->set_cur(&(m_lVarActiveDims[0]));
	m_varActive->get(&(m_data[0]), 1, m_varActive->get_dim(1)->size());
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::SetDisplayedBounds(
	double dX0,
	double dX1,
	double dY0,
	double dY1
) {
	char szBounds[16];

	snprintf(szBounds, 16, "%f", dX0);
	m_vecwxImageBounds[0]->ChangeValue(wxString(szBounds));

	snprintf(szBounds, 16, "%f", dX1);
	m_vecwxImageBounds[1]->ChangeValue(wxString(szBounds));

	snprintf(szBounds, 16, "%f", dY0);
	m_vecwxImageBounds[2]->ChangeValue(wxString(szBounds));

	snprintf(szBounds, 16, "%f", dY1);
	m_vecwxImageBounds[3]->ChangeValue(wxString(szBounds));
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::SetStatusMessage(
	const wxString & strMessage,
	bool fIncludeVersion
) {
	if (fIncludeVersion) {
		wxString strMessageBak = _T("NcVis 2022.06.26");
		strMessageBak += strMessage;
		SetStatusText( strMessageBak );
	} else {
		SetStatusText( strMessage );
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnHello(
	wxCommandEvent & event
) {
	wxLogMessage("Hello world from wxWidgets!");
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnExit(
	wxCommandEvent & event
) {
	Close( true );
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnAbout(
	wxCommandEvent & event
) {
	wxMessageBox( "This is a wxWidgets' Hello world sample",
				  "About Hello World", wxOK | wxICON_INFORMATION );
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnColorMapClicked(
	wxCommandEvent & event
) {
	std::cout << "COLOR MAP CLICKED" << std::endl;

	ColorMapLibrary colormaplib;

	size_t sColorMapCount = colormaplib.GetColorMapCount();

	m_sColorMap++;
	if (m_sColorMap >= sColorMapCount) {
		m_sColorMap = 0;
	}

	std::string strColorMapName = colormaplib.GetColorMapName(m_sColorMap);

	m_wxColormapButton->SetLabel(wxString(strColorMapName));

	m_imagepanel->SetColorMap(strColorMapName, true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnDataTransClicked(
	wxCommandEvent & event
) {
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnVariableSelected(
	wxCommandEvent & event
) {
	std::cout << "VARIABLE SELECTED" << std::endl;

	std::string strValue = event.GetString().ToStdString();

	// Load the data
	m_varActive = m_pncfile->get_var(strValue.c_str());
	m_lVarActiveDims.resize(m_varActive->num_dims(), 0);
	if (m_varActive->get_dim(1)->size() != m_data.GetRows()) {
		std::cout << "Dimension mismatch between variable and data storage" << std::endl;
		return;
	}
	LoadData();

	// Add dimension controls
	m_dimsizer->Clear(true);
	for (long d = 0; d < m_varActive->num_dims(); d++) {
		wxString strDim = wxString(m_varActive->get_dim(d)->name()) + ":";
		m_dimsizer->Add(new wxStaticText(this, -1, strDim), 0, wxEXPAND | wxALL, 4);

		wxButton * wxDimDown = new wxButton(this, ID_DIMDOWN + d, _T("-"), wxDefaultPosition, wxSize(22,22));
		m_vecwxDimIndex[d] = new wxTextCtrl(this, ID_DIMEDIT + d, _T("0"), wxDefaultPosition, wxSize(40, 22), wxTE_CENTRE);
		wxButton * wxDimUp = new wxButton(this, ID_DIMUP + d, _T("+"), wxDefaultPosition, wxSize(22,22));

		m_dimsizer->Add(wxDimDown, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 2);
		m_dimsizer->Add(m_vecwxDimIndex[d], 0, wxEXPAND | wxTOP | wxBOTTOM, 2);
		m_dimsizer->Add(wxDimUp, 0, wxEXPAND | wxRIGHT | wxTOP | wxBOTTOM, 2);

		if (d == 1) {
			wxDimDown->Enable(false);
			m_vecwxDimIndex[d]->Enable(false);
			wxDimUp->Enable(false);

		} else {
			wxDimDown->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnDimButtonClicked, this);
			m_vecwxDimIndex[d]->Bind(wxEVT_TEXT, &wxNcVisFrame::OnDimButtonClicked, this);
			wxDimUp->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnDimButtonClicked, this);
		}
	}
	m_dimsizer->Layout();
	GetSizer()->Fit(this);

	// Set the data range
	float dDataMin = m_data[0];
	float dDataMax = m_data[0];
	for (int i = 1; i < m_data.GetRows(); i++) {
		if (m_data[i] > dDataMax) {
			dDataMax = m_data[i];
		}
		if (m_data[i] < dDataMin) {
			dDataMin = m_data[i];
		}
	}
	m_imagepanel->SetDataRange(dDataMin, dDataMax);

	m_imagepanel->GenerateImageFromImageMap(true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnBoundsChanged(
	wxCommandEvent & event
) {
	std::cout << "BOUNDS CHANGED" << std::endl;

}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnZoomOutClicked(
	wxCommandEvent & event
) {
	m_imagepanel->SetCoordinateRange(0.0, 360.0, -90.0, 90.0, true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnDimButtonClicked(wxCommandEvent & event) {
	std::cout << "DIM BUTTON CLICKED" << std::endl;

	bool fIncrement = false;
	long d = static_cast<long>(event.GetId());
	if ((d >= ID_DIMDOWN) && (d < ID_DIMDOWN + 100)) {
		d -= ID_DIMDOWN;
	} else if ((d >= ID_DIMUP) && (d < ID_DIMUP + 100)) {
		d -= ID_DIMUP;
		fIncrement = true;
	} else {
		_EXCEPTION();
	}

	if (m_varActive == NULL) {
		_EXCEPTION();
	}
	if ((d < 0) || (d >= m_lVarActiveDims.size())) {
		_EXCEPTION();
	}

	long lDimSize = m_varActive->get_dim(d)->size();

	if (fIncrement) {
		if (m_lVarActiveDims[d] == lDimSize-1) {
			m_lVarActiveDims[d] = 0;
		} else {
			m_lVarActiveDims[d]++;
		}
	} else {
		if (m_lVarActiveDims[d] == 0) {
			m_lVarActiveDims[d] = lDimSize-1;
		} else {
			m_lVarActiveDims[d]--;
		}
	}

	char szText[16];
	snprintf(szText, 16, "%li", m_lVarActiveDims[d]);
	m_vecwxDimIndex[d]->ChangeValue(wxString(szText));

	LoadData();

	m_imagepanel->GenerateImageFromImageMap(true);
}

////////////////////////////////////////////////////////////////////////////////


