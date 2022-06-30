///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisFrame.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#include "wxNcVisFrame.h"

#include <set>

////////////////////////////////////////////////////////////////////////////////

enum {
	ID_Hello = 1,
	ID_COLORMAP = 2,
	ID_DATATRANS = 3,
	ID_BOUNDS = 4,
	ID_ZOOMOUT = 5,
	ID_VARSELECTOR = 100,
	ID_DIMEDIT = 200,
	ID_DIMDOWN = 300,
	ID_DIMUP = 400,
};

////////////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxNcVisFrame, wxFrame)
	EVT_MENU(ID_Hello,   wxNcVisFrame::OnHello)
	EVT_MENU(wxID_EXIT,  wxNcVisFrame::OnExit)
	EVT_MENU(wxID_ABOUT, wxNcVisFrame::OnAbout)
	EVT_BUTTON(ID_COLORMAP, wxNcVisFrame::OnColorMapClicked)
	EVT_BUTTON(ID_DATATRANS, wxNcVisFrame::OnDataTransClicked)
	EVT_TEXT(ID_BOUNDS, wxNcVisFrame::OnBoundsChanged)
	EVT_BUTTON(ID_ZOOMOUT, wxNcVisFrame::OnZoomOutClicked)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

wxNcVisFrame::wxNcVisFrame(
	const wxString & title,
	const wxPoint & pos,
	const wxSize & size,
	const std::string & strNcVisResourceDir,
	const std::vector<std::string> & vecFilenames
) :
	wxFrame(NULL, wxID_ANY, title, pos, size),
	m_strNcVisResourceDir(strNcVisResourceDir),
	m_wxColormapButton(NULL),
	m_wxDataTransButton(NULL),
	m_dimsizer(NULL),
	m_imagepanel(NULL),
	m_varActive(NULL),
	m_sColorMap(0)
{
	m_lDisplayedDims[0] = -1;
	m_lDisplayedDims[1] = -1;

	OpenFiles(vecFilenames);

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
		if (m_mapVarNames[vc].size() == 0) {
			continue;
		}

		char szName[16];
		snprintf(szName, 16, "(%lu) %iD vars", m_mapVarNames[vc].size(), vc);
		m_vecwxVarSelector[vc] = new wxComboBox(this, ID_VARSELECTOR + vc, szName);
		Bind(wxEVT_COMBOBOX, &wxNcVisFrame::OnVariableSelected, this);
		m_vecwxVarSelector[vc]->SetEditable(false);
		for (auto itVar = m_mapVarNames[vc].begin(); itVar != m_mapVarNames[vc].end(); itVar++) {
			m_vecwxVarSelector[vc]->Append(wxString(itVar->first));
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
	SetStatusText( _T("NcVis 2022.06.27") );

	SetSizerAndFit(topsizer);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OpenFiles(
	const std::vector<std::string> & vecFilenames
) {
	_ASSERT(m_vecpncfiles.size() == 0);

	m_vecFilenames = vecFilenames;

	// Enumerate all variables, recording dimension variables
	std::set<std::string> setDimVars;
	for (size_t f = 0; f < vecFilenames.size(); f++) {
		NcFile * pfile = new NcFile(vecFilenames[f].c_str());
		if (!pfile->is_valid()) {
			std::cout << "Error: Unable to open file \"" << vecFilenames[f] << "\"" << std::endl;
			_EXCEPTION();
		}
		m_vecpncfiles.push_back(pfile);

		for (long v = 0; v < pfile->num_vars(); v++) {
			NcVar * var = pfile->get_var(v);
			size_t sVarDims = static_cast<size_t>(var->num_dims());
			if (var->num_dims() >= 10) {
				std::cout << "Error: Only variables of dimension <= 10 supported" << std::endl;
				_EXCEPTION();
			}

			for (long d = 0; d < var->num_dims(); d++) {
				setDimVars.insert(var->get_dim(d)->name());
			}

			auto itVar = m_mapVarNames[sVarDims].find(var->name());
			if (itVar != m_mapVarNames[sVarDims].end()) {
				itVar->second.push_back(f);
			} else {
				std::pair<std::string, std::vector<size_t> > pr;
				pr.first = var->name();
				pr.second.push_back(f);
				m_mapVarNames[sVarDims].insert(pr);
			}
		}
	}

	// Identify longitude and latitude
	auto itLon = m_mapVarNames[1].find("lon");
	if (itLon == m_mapVarNames[1].end()) {
		std::cout << "Error: Variable \"lon\" not found in input files" << std::endl;
		_EXCEPTION();
	}
	auto itLat = m_mapVarNames[1].find("lat");
	if (itLat == m_mapVarNames[1].end()) {
		std::cout << "Error: Variable \"lat\" not found in input files" << std::endl;
		_EXCEPTION();
	}

	// Remove dimension variables
	for (auto it = setDimVars.begin(); it != setDimVars.end(); it++) {
		for (size_t sVarDims = 0; sVarDims < 10; sVarDims++) {
			m_mapVarNames[sVarDims].erase(*it);
		}
	}

	// Get these variables
	NcVar * varLon = m_vecpncfiles[itLon->second[0]]->get_var("lon");
	_ASSERT(varLon != NULL);
	NcVar * varLat = m_vecpncfiles[itLat->second[0]]->get_var("lat");
	_ASSERT(varLat != NULL);

	if (varLon->num_dims() != 1) {
		std::cout << "Error: Only 1D \"lon\" variable supported" << std::endl;
		_EXCEPTION();
	}
	if (varLat->num_dims() != 1) {
		std::cout << "Error: Only 1D \"lat\" variable supported" << std::endl;
		_EXCEPTION();
	}
	if (varLon->get_dim(0)->size() != varLat->get_dim(0)->size()) {
		std::cout << "Error: Variables \"lon\" and \"lat\" must have the same length" << std::endl;
		_EXCEPTION();
	}

	m_strUnstructDimName = varLon->get_dim(0)->name();

	DataArray1D<double> dLon(varLon->get_dim(0)->size());
	DataArray1D<double> dLat(varLat->get_dim(0)->size());

	varLon->get(&(dLon[0]), varLon->get_dim(0)->size());
	varLat->get(&(dLat[0]), varLat->get_dim(0)->size());

	// Initialize the GridDataSampler
	m_gdsqt.Initialize(dLon, dLat);
	//m_gdskd.Initialize(dLon, dLat);

	// Allocate data space
	m_data.Allocate(varLon->get_dim(0)->size());
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::LoadData() {

	_ASSERT(m_lDisplayedDims[0] < m_varActive->num_dims());

	m_varActive->set_cur(&(m_lVarActiveDims[0]));
	if (m_lDisplayedDims[0] == m_varActive->num_dims()-1) {
		std::vector<long> vecSize(m_varActive->num_dims(), 1);
		vecSize[m_lDisplayedDims[0]] = m_varActive->get_dim(m_lDisplayedDims[0])->size();

		m_varActive->get(&(m_data[0]), &(vecSize[0]));

	} else {
		std::vector<long> vecSize(m_varActive->num_dims(), 1);
		std::vector<long> vecStride(m_varActive->num_dims(), 1);
		vecSize[m_lDisplayedDims[0]] = m_varActive->get_dim(m_lDisplayedDims[0])->size();
		for (long d = m_lDisplayedDims[0]+1; d < m_varActive->num_dims(); d++) {
			vecStride[d] = m_varActive->get_dim(d)->size();
		}

		//for (long d = 0; d < m_lVarActiveDims.size(); d++) {
		//	std::cout << m_lVarActiveDims[d] << " " << vecSize[d] << " " << vecStride[d] << std::endl;
		//}

		m_varActive->gets(&(m_data[0]), &(vecSize[0]), &(vecStride[0]));
	}

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

	// Store a map between current dimnames and dimvalues
	if ((m_varActive != NULL) && (m_lVarActiveDims.size() == m_varActive->num_dims())) {
		for (long d = 0; d < m_varActive->num_dims(); d++) {
			auto itDimBookmark = m_mapDimBookmarks.find(m_varActive->get_dim(d)->name());
			if (itDimBookmark == m_mapDimBookmarks.end()) {
				m_mapDimBookmarks.insert(
					std::pair<std::string, long>(
						m_varActive->get_dim(d)->name(),
						m_lVarActiveDims[d]));
			} else {
				itDimBookmark->second = m_lVarActiveDims[d];
			}
		}
	}

	// Load the data
	std::string strValue = event.GetString().ToStdString();

	int vc = static_cast<int>(event.GetId() - ID_VARSELECTOR);
	_ASSERT((vc >= 0) && (vc <= 9));
	auto itVar = m_mapVarNames[vc].find(strValue);
	m_varActive = m_vecpncfiles[itVar->second[0]]->get_var(strValue.c_str());
	m_lVarActiveDims.resize(m_varActive->num_dims());

	for (long d = 0; d < m_varActive->num_dims(); d++) {
		auto itDimCurrent = m_mapDimBookmarks.find(m_varActive->get_dim(d)->name());
		if (itDimCurrent != m_mapDimBookmarks.end()) {
			m_lVarActiveDims[d] = itDimCurrent->second;
		} else {
			m_lVarActiveDims[d] = 0;
		}
		if (m_strUnstructDimName == m_varActive->get_dim(d)->name()) {
			m_lDisplayedDims[0] = d;
		}
	}

	if (m_varActive->get_dim(1)->size() != m_data.GetRows()) {
		std::cout << "Dimension mismatch between variable and data storage" << std::endl;
		return;
	}
	LoadData();

	// Get the height of the control
	wxSize wxsizeButton = m_wxColormapButton->GetSize();
	int nCtrlHeight = wxsizeButton.GetHeight();

	// Add dimension controls
	m_dimsizer->Clear(true);
	for (long d = 0; d < m_varActive->num_dims(); d++) {
		wxString strDim = wxString(m_varActive->get_dim(d)->name()) + ":";
		m_dimsizer->Add(new wxStaticText(this, -1, strDim), 0, wxEXPAND | wxALL, 4);

		char szDimValue[8];
		snprintf(szDimValue, 8, "%li", m_lVarActiveDims[d]);

		wxButton * wxDimDown = new wxButton(this, ID_DIMDOWN + d, _T("-"), wxDefaultPosition, wxSize(26,nCtrlHeight));
		m_vecwxDimIndex[d] = new wxTextCtrl(this, ID_DIMEDIT + d, wxString(szDimValue), wxDefaultPosition, wxSize(40, nCtrlHeight), wxTE_CENTRE);
		wxButton * wxDimUp = new wxButton(this, ID_DIMUP + d, _T("+"), wxDefaultPosition, wxSize(26,nCtrlHeight));

		m_dimsizer->Add(wxDimDown, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, 2);
		m_dimsizer->Add(m_vecwxDimIndex[d], 0, wxEXPAND | wxTOP | wxBOTTOM, 2);
		m_dimsizer->Add(wxDimUp, 0, wxEXPAND | wxRIGHT | wxTOP | wxBOTTOM, 2);

		if ((d == m_lDisplayedDims[0]) || (d == m_lDisplayedDims[1])) {
			wxDimDown->Enable(false);
			m_vecwxDimIndex[d]->SetValue(_T(":"));
			m_vecwxDimIndex[d]->Enable(false);
			wxDimUp->Enable(false);

		} else if (m_varActive->get_dim(d)->size() == 1) {
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
	Layout();
	
	//GetSizer()->Fit(this);

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


