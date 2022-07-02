///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisFrame.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#include "wxNcVisFrame.h"

#include "STLStringHelper.h"
#include <set>

////////////////////////////////////////////////////////////////////////////////

enum {
	ID_Hello = 1,
	ID_COLORMAP = 2,
	ID_DATATRANS = 3,
	ID_BOUNDS = 4,
	ID_ZOOMOUT = 5,
	ID_RANGEMIN = 6,
	ID_RANGEMAX = 7,
	ID_RANGERESETMINMAX = 8,
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
	EVT_TEXT_ENTER(ID_BOUNDS, wxNcVisFrame::OnBoundsChanged)
	EVT_TEXT_ENTER(ID_RANGEMIN, wxNcVisFrame::OnRangeChanged)
	EVT_TEXT_ENTER(ID_RANGEMAX, wxNcVisFrame::OnRangeChanged)
	EVT_BUTTON(ID_RANGERESETMINMAX, wxNcVisFrame::OnRangeResetMinMax)
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
	m_fIsVarActiveUnstructured(false),
	m_sColorMap(0)
{
	m_lDisplayedDims[0] = (-1);
	m_lDisplayedDims[1] = (-1);

	m_data.Allocate(1);
	m_data[0] = 0.0;

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

		m_vecwxVarSelector[vc] =
			new wxComboBox(this, ID_VARSELECTOR + vc,
				wxString::Format("(%lu) %iD vars", m_mapVarNames[vc].size(), vc));

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

	// Displayed coordinate bounds
	wxBoxSizer *boundssizer = new wxBoxSizer(wxHORIZONTAL);

	for (int i = 0; i < 4; i++) {
		m_vecwxImageBounds[i] = new wxTextCtrl(this, ID_BOUNDS, "", wxDefaultPosition, wxSize(100, 22), wxTE_CENTRE | wxTE_PROCESS_ENTER);
	}
	boundssizer->Add(new wxStaticText(this, -1, "X:"), 0, wxEXPAND | wxALL, 4);
	boundssizer->Add(m_vecwxImageBounds[0], 0, wxEXPAND | wxALL, 2);
	boundssizer->Add(m_vecwxImageBounds[1], 0, wxEXPAND | wxALL, 2);
	boundssizer->Add(new wxStaticText(this, -1, "Y:"), 0, wxEXPAND | wxALL, 4);
	boundssizer->Add(m_vecwxImageBounds[2], 0, wxEXPAND | wxALL, 2);
	boundssizer->Add(m_vecwxImageBounds[3], 0, wxEXPAND | wxALL, 2);
	boundssizer->Add(new wxButton(this, ID_ZOOMOUT, _T("Zoom Out")), 0, wxEXPAND | wxALL, 2);

	// Displayed color range
	wxBoxSizer *rangesizer = new wxBoxSizer(wxHORIZONTAL);

	m_vecwxRange[0] = new wxTextCtrl(this, ID_RANGEMIN, "", wxDefaultPosition, wxSize(100, 22), wxTE_CENTRE | wxTE_PROCESS_ENTER);
	m_vecwxRange[1] = new wxTextCtrl(this, ID_RANGEMAX, "", wxDefaultPosition, wxSize(100, 22), wxTE_CENTRE | wxTE_PROCESS_ENTER);
	rangesizer->Add(new wxStaticText(this, -1, "Min:"), 0, wxEXPAND | wxALL, 4);
	rangesizer->Add(m_vecwxRange[0], 0, wxEXPAND | wxALL, 2);
	rangesizer->Add(new wxStaticText(this, -1, "Max:"), 0, wxEXPAND | wxALL, 4);
	rangesizer->Add(m_vecwxRange[1], 0, wxEXPAND | wxALL, 2);
	rangesizer->Add(new wxButton(this, ID_RANGERESETMINMAX, _T("Reset Min/Max")), 0, wxEXPAND | wxALL, 2);
	m_vecwxRange[0]->Enable(false);
	m_vecwxRange[1]->Enable(false);

	// Image panel
	m_imagepanel = new wxImagePanel(this);

	//topsizer->Add(infobox, 0, wxALIGN_CENTER, 0);
	topsizer->Add(ctrlsizer, 0, wxALIGN_CENTER, 0);
	topsizer->Add(varsizer, 0, wxALIGN_CENTER, 0);
	topsizer->Add(m_dimsizer, 0, wxALIGN_CENTER, 0);
	topsizer->Add(boundssizer, 0, wxALIGN_CENTER, 0);
	topsizer->Add(rangesizer, 0, wxALIGN_CENTER, 0);
	topsizer->Add(m_imagepanel, 1, wxALIGN_CENTER | wxSHAPED);

	CreateStatusBar();

	// Status bar
	SetStatusMessage(_T(""), true);

	SetSizerAndFit(topsizer);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OpenFiles(
	const std::vector<std::string> & vecFilenames
) {
	_ASSERT(m_vecpncfiles.size() == 0);

	m_vecFilenames = vecFilenames;

	// Enumerate all variables, recording dimension variables
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
				auto prDimInsert =
					m_mapDimData.insert(
						DimDataMap::value_type(
							var->get_dim(d)->name(),
							DimDataFileIdAndCoordMap()));
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

		// Load dimension data into persistent storage
		for (auto itVarDim = m_mapDimData.begin(); itVarDim != m_mapDimData.end(); itVarDim++) {
			NcVar * varDim = pfile->get_var(itVarDim->first.c_str());
			if (varDim != NULL) {
				if (varDim->num_dims() != 1) {
					std::cout << "ERROR: NetCDF fileset contains a dimension variable \"" << itVarDim->first
						<< "\" which has dimension different than 1" << std::endl;
					_EXCEPTION();
				}

				Announce("Dimension variable \"%s\" in file %lu (%li values)",
					itVarDim->first.c_str(), f, varDim->get_dim(0)->size());
				auto prDimDataInfo =
					itVarDim->second.insert(
						DimDataFileIdAndCoordMap::value_type(
							f, std::vector<double>()));

				std::vector<double> & dDimData = prDimDataInfo.first->second;

				prDimDataInfo.first->second.resize(varDim->get_dim(0)->size());
				varDim->get(&(dDimData[0]), varDim->get_dim(0)->size());

				// Verify dimension data is monotone
				if (dDimData.size() > 1) {
					bool fMonotone = true;
					bool fIncreasing = (dDimData[1] > dDimData[0]);
					if (dDimData[1] == dDimData[0]) {
						fMonotone = false;
					}
					if (fMonotone) {
						if (fIncreasing) {
							for (size_t i = 2; i < dDimData.size(); i++) {
								if (dDimData[i] <= dDimData[i-1]) {
									fMonotone = false;
									break;
								}
							}

						} else {
							for (size_t i = 2; i < dDimData.size(); i++) {
								if (dDimData[i] >= dDimData[i-1]) {
									fMonotone = false;
									break;
								}
							}
						}
					}
					if (!fMonotone) {
						std::cout << "ERROR: NetCDF fileset contains a dimension variable \""
							<< itVarDim->first << "\" that is non-monotone" << std::endl;
						_EXCEPTION();
					}
				}
			}
		}
	}

	// Remove dimension variables from the variable name map
	for (auto it = m_mapDimData.begin(); it != m_mapDimData.end(); it++) {
		m_mapVarNames[1].erase(it->first);
	}

	// Check if lon and lat are dimension variables; if they are then they
	// should not be coordinates on the unstructured mesh.
	auto itLonDimVar = m_mapDimData.find("lon");
	auto itLatDimVar = m_mapDimData.find("lat");

	if ((itLonDimVar == m_mapDimData.end()) && (itLatDimVar != m_mapDimData.end())) {
		std::cout << "ERROR: In input file \"lat\" is a dimension variable but \"lon\" is not" << std::endl;
		_EXCEPTION();
	}
	if ((itLonDimVar != m_mapDimData.end()) && (itLatDimVar == m_mapDimData.end())) {
		std::cout << "ERROR: In input file \"lat\" is a dimension variable but \"lon\" is not" << std::endl;
		_EXCEPTION();
	}
	if ((itLonDimVar != m_mapDimData.end()) && (itLatDimVar != m_mapDimData.end())) {
		return;
	}

	// Identify longitude and latitude to determine if unstructured grid is needed
	auto itLon = m_mapVarNames[1].find("lon");
	if (itLon == m_mapVarNames[1].end()) {
		return;
	}
	auto itLat = m_mapVarNames[1].find("lat");
	if (itLat == m_mapVarNames[1].end()) {
		return;
	}

	// Check if lat and lon are the same length
	NcVar * varLon = m_vecpncfiles[itLon->second[0]]->get_var("lon");
	_ASSERT(varLon != NULL);
	NcVar * varLat = m_vecpncfiles[itLat->second[0]]->get_var("lat");
	_ASSERT(varLat != NULL);

	if (varLon->get_dim(0)->size() != varLat->get_dim(0)->size()) {
		return;
	}

	// At this point we can assume that the mesh is unstructured
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

	// Assume data is not unstructured
	m_fIsVarActiveUnstructured = false;

	// 0D data
	if (m_varActive->num_dims() == 0) {
		m_data.Allocate(1);
		m_varActive->get(&(m_data[0]), 1);
		return;
	}

	// 1D data (including unstructured grid data)
	if (m_lDisplayedDims[1] == (-1)) {
		_ASSERT(m_lDisplayedDims[0] < m_varActive->num_dims());
		_ASSERT(m_varActive->num_dims() == m_lVarActiveDims.size());

		if (m_strUnstructDimName == m_varActive->get_dim(m_lDisplayedDims[0])->name()) {
			m_fIsVarActiveUnstructured = true;
		}

		// Reallocate space, if necessary
		std::vector<long> vecSize(m_varActive->num_dims(), 1);
		vecSize[m_lDisplayedDims[0]] = m_varActive->get_dim(m_lDisplayedDims[0])->size();

		if (m_data.GetRows() != vecSize[m_lDisplayedDims[0]]) {
			m_data.Allocate(vecSize[m_lDisplayedDims[0]]);
		}

		// Load data
		m_varActive->set_cur(&(m_lVarActiveDims[0]));
		if (m_lDisplayedDims[0] == m_varActive->num_dims()-1) {
			m_varActive->get(&(m_data[0]), &(vecSize[0]));

		} else {
			std::vector<long> vecStride(m_varActive->num_dims(), 1);
			for (long d = m_lDisplayedDims[0]+1; d < m_varActive->num_dims(); d++) {
				vecStride[d] = m_varActive->get_dim(d)->size();
			}

			m_varActive->gets(&(m_data[0]), &(vecSize[0]), &(vecStride[0]));
		}

	// 2D data
	} else {
		_ASSERT(m_lDisplayedDims[0] != m_lDisplayedDims[1]);
		_ASSERT(m_lDisplayedDims[0] < m_varActive->num_dims());
		_ASSERT(m_lDisplayedDims[1] < m_varActive->num_dims());
		_ASSERT(m_varActive->num_dims() == m_lVarActiveDims.size());

		// Reallocate space, if necessary
		std::vector<long> vecSize(m_varActive->num_dims(), 1);
		vecSize[m_lDisplayedDims[0]] = m_varActive->get_dim(m_lDisplayedDims[0])->size();
		vecSize[m_lDisplayedDims[1]] = m_varActive->get_dim(m_lDisplayedDims[1])->size();

		if (m_data.GetRows() != vecSize[m_lDisplayedDims[0]] * vecSize[m_lDisplayedDims[1]]) {
			m_data.Allocate(vecSize[m_lDisplayedDims[0]] * vecSize[m_lDisplayedDims[1]]);
		}

		// Load data
		m_varActive->set_cur(&(m_lVarActiveDims[0]));
		if ((m_lDisplayedDims[0] == m_varActive->num_dims()-2) &&
		    (m_lDisplayedDims[1] == m_varActive->num_dims()-1)
		) {
			m_varActive->get(&(m_data[0]), &(vecSize[0]));

		} else {
			std::vector<long> vecStride(m_varActive->num_dims(), 1);
			for (long d = m_lDisplayedDims[0]+1; d < m_varActive->num_dims(); d++) {
				vecStride[d] = m_varActive->get_dim(d)->size();
			}

			m_varActive->gets(&(m_data[0]), &(vecSize[0]), &(vecStride[0]));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::MapSampleCoords1DFromActiveVar(
	const DataArray1D<double> & dSample,
	long lDim,
	std::vector<int> & veccoordmap
) {
	_ASSERT(lDim < m_varActive->num_dims());

	veccoordmap.resize(dSample.GetRows(), 0);

	std::string strDim = m_varActive->get_dim(lDim)->name();

	// Load in coordinate arrays, substituting integer arrays if not present
	// Note that dimension 0 corresponds to Y and dimension 1 to X
	std::vector<double> * pvecDimValues;
	std::vector<double> vecDimValues_temp;

	auto itDim = m_mapDimData.find(strDim);
	if ((itDim != m_mapDimData.end()) && (itDim->second.size() != 0)) {
		auto itDimData = itDim->second.begin();
		pvecDimValues = &(itDimData->second);
	} else {
		vecDimValues_temp.resize(m_varActive->get_dim(lDim)->size());
		for (size_t i = 0; i < m_varActive->get_dim(lDim)->size(); i++) {
			vecDimValues_temp[i] = static_cast<double>(i);
		}
		pvecDimValues = &vecDimValues_temp;
	}

	std::cout << dSample.GetRows() << " " << pvecDimValues->size()-1 << std::endl;
	// Determine which data coordinates correspond to the sample coordinates
	for (size_t s = 0; s < dSample.GetRows(); s++) {
		for (size_t t = 1; t < pvecDimValues->size()-1; t++) {
			double dLeft = 0.5 * ((*pvecDimValues)[t-1] + (*pvecDimValues)[t]);
			double dRight = 0.5 * ((*pvecDimValues)[t] + (*pvecDimValues)[t+1]);
			if ((t == 1) && (dSample[s] < dLeft)) {
				veccoordmap[s] = 0;
				break;
			}
			if ((t == pvecDimValues->size()-2) && (dSample[s] > dRight)) {
				veccoordmap[s] = pvecDimValues->size()-1;
				break;
			}
			if ((dSample[s] >= dLeft) && (dSample[s] <= dRight)) {
				veccoordmap[s] = t;
				break;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

// TODO: Sometimes you need to resample when variable is changed
// TODO: Change from DataArray1D to a vector
void wxNcVisFrame::SampleData(
	const DataArray1D<double> & dSampleX,
	const DataArray1D<double> & dSampleY,
	DataArray1D<int> & imagemap
) {
	_ASSERT(imagemap.GetRows() >= dSampleX.GetRows() * dSampleY.GetRows());
	_ASSERT(m_data.GetRows() > 0);

	// Active variable is an unstructured variable; use sampling
	if (m_fIsVarActiveUnstructured) {
		m_gdsqt.Sample(dSampleX, dSampleY, imagemap);

	// No displayed variables
	} else if ((m_lDisplayedDims[0] == (-1)) && (m_lDisplayedDims[1] == (-1))) {
		for (size_t s = 0; s < imagemap.GetRows(); s++) {
			imagemap[s] = 0;
		}

	// One displayed variable
	} else if (m_lDisplayedDims[1] == (-1)) {
		_ASSERT((m_lDisplayedDims[0] >= 0) && (m_lDisplayedDims[0] < m_varActive->num_dims()));

		std::vector<int> veccoordmapX(dSampleX.GetRows(), 0);

		MapSampleCoords1DFromActiveVar(dSampleX, m_lDisplayedDims[0], veccoordmapX);

		// Assemble the image map
		size_t s = 0;
		for (size_t j = 0; j < dSampleY.GetRows(); j++) {
		for (size_t i = 0; i < dSampleX.GetRows(); i++) {
			imagemap[s] = veccoordmapX[i];
			s++;
		}
		}

	// Two displayed variables
	} else {
		_ASSERT((m_lDisplayedDims[0] >= 0) && (m_lDisplayedDims[0] < m_varActive->num_dims()));
		_ASSERT((m_lDisplayedDims[1] >= 0) && (m_lDisplayedDims[1] < m_varActive->num_dims()));

		std::vector<int> veccoordmapX(dSampleX.GetRows(), 0);
		std::vector<int> veccoordmapY(dSampleY.GetRows(), 0);

		MapSampleCoords1DFromActiveVar(dSampleY, m_lDisplayedDims[0], veccoordmapY);
		MapSampleCoords1DFromActiveVar(dSampleX, m_lDisplayedDims[1], veccoordmapX);

		size_t sDimXSize = m_varActive->get_dim(m_lDisplayedDims[1])->size();

		// Assemble the image map
		size_t s = 0;
		for (size_t j = 0; j < dSampleY.GetRows(); j++) {
		for (size_t i = 0; i < dSampleX.GetRows(); i++) {
			imagemap[s] = veccoordmapY[j] * sDimXSize + veccoordmapX[i];
			s++;
		}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::SetDisplayedBounds(
	double dX0,
	double dX1,
	double dY0,
	double dY1
) {
	m_vecwxImageBounds[0]->ChangeValue(wxString::Format(wxT("%.7g"), dX0));
	m_vecwxImageBounds[1]->ChangeValue(wxString::Format(wxT("%.7g"), dX1));
	m_vecwxImageBounds[2]->ChangeValue(wxString::Format(wxT("%.7g"), dY0));
	m_vecwxImageBounds[3]->ChangeValue(wxString::Format(wxT("%.7g"), dY1));
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::SetDisplayedDataRange(
	float dDataMin,
	float dDataMax
) {
	m_vecwxRange[0]->ChangeValue(wxString::Format(wxT("%.7g"), dDataMin));
	m_vecwxRange[1]->ChangeValue(wxString::Format(wxT("%.7g"), dDataMax));
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::SetDataRangeByMinMax() {
	if (m_data.GetRows() == 0) {
		return;
	}

	float dDataMin = m_data[0];
	float dDataMax = m_data[0];
	for (int i = 1; i < m_data.GetRows(); i++) {
		if (m_data[i] < dDataMin) {
			dDataMin = m_data[i];
		}
		if (m_data[i] > dDataMax) {
			dDataMax = m_data[i];
		}
	}

	m_imagepanel->SetDataRange(dDataMin, dDataMax, true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::SetStatusMessage(
	const wxString & strMessage,
	bool fIncludeVersion
) {
	if (fIncludeVersion) {
		wxString strMessageBak = _T("NcVis 2022.07.01");
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
	wxLogMessage("Hello world!");
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
	wxMessageBox( "Developed by Paul A. Ullrich\n\nFunding for the development of ncvis is provided by the United States Department of Energy Office of Science under the Regional and Global Model Analysis project \"SEATS: Simplifying ESM Analysis Through Standards.\"",
				  "NetCDF Visualizer (ncvis)", wxOK | wxICON_INFORMATION );
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

	// Change the active variable
	std::string strValue = event.GetString().ToStdString();

	int vc = static_cast<int>(event.GetId() - ID_VARSELECTOR);
	_ASSERT((vc >= 0) && (vc <= 9));
	auto itVar = m_mapVarNames[vc].find(strValue);
	m_varActive = m_vecpncfiles[itVar->second[0]]->get_var(strValue.c_str());
	_ASSERT(m_varActive != NULL);

	m_lVarActiveDims.resize(m_varActive->num_dims());

	// Initialize displayed dimension(s) and active dimensions
	m_lDisplayedDims[0] = (-1);
	m_lDisplayedDims[1] = (-1);
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

	if (m_lDisplayedDims[0] == (-1)) {
		if (m_varActive->num_dims() == 0) {
			m_lDisplayedDims[1] = (-1);
		} else if (m_varActive->num_dims() == 1) {
			m_lDisplayedDims[0] = 0;
		} else {
			m_lDisplayedDims[0] = m_varActive->num_dims()-2;
			m_lDisplayedDims[1] = m_varActive->num_dims()-1;
		}
	}

	// Load the data
	LoadData();

	// Revert all other combo boxes
	for (int vc = 0; vc < 10; vc++) {
		if ((m_vecwxVarSelector[vc] != NULL) && (vc != m_varActive->num_dims())) {
			m_vecwxVarSelector[vc]->ChangeValue(
				wxString::Format("(%lu) %iD vars", m_mapVarNames[vc].size(), vc));
		}
	}

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
			if (m_strUnstructDimName == m_varActive->get_dim(d)->name()) {
				m_vecwxDimIndex[d]->SetValue(_T("XY"));
			} else if (d == m_lDisplayedDims[0]) {
				m_vecwxDimIndex[d]->SetValue(_T("Y"));
			} else {
				m_vecwxDimIndex[d]->SetValue(_T("X"));
			}
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

	// TODO: This code leads to two PAINT messages being sent
	m_dimsizer->Layout();
	Layout();
	
	// Set the data range
	m_vecwxRange[0]->Enable(true);
	m_vecwxRange[1]->Enable(true);

	SetDataRangeByMinMax();

	//m_imagepanel->GenerateImageFromImageMap(true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnBoundsChanged(
	wxCommandEvent & event
) {
	std::cout << "BOUNDS CHANGED" << std::endl;

	std::string strX0 = m_vecwxImageBounds[0]->GetValue().ToStdString();
	std::string strX1 = m_vecwxImageBounds[1]->GetValue().ToStdString();
	std::string strY0 = m_vecwxImageBounds[2]->GetValue().ToStdString();
	std::string strY1 = m_vecwxImageBounds[3]->GetValue().ToStdString();

	if (!STLStringHelper::IsFloat(strX0) ||
	    !STLStringHelper::IsFloat(strX1) ||
	    !STLStringHelper::IsFloat(strY0) ||
	    !STLStringHelper::IsFloat(strY1)
	) {
		SetDisplayedBounds(
			m_imagepanel->GetXRangeMin(),
			m_imagepanel->GetXRangeMax(),
			m_imagepanel->GetYRangeMin(),
			m_imagepanel->GetYRangeMax());
		return;
	}

	double dX0 = stof(strX0);
	double dX1 = stof(strX1);
	double dY0 = stof(strY0);
	double dY1 = stof(strY1);

	m_imagepanel->SetCoordinateRange(dX0, dX1, dY0, dY1, true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnRangeChanged(
	wxCommandEvent & event
) {
	std::cout << "RANGE CHANGED" << std::endl;

	std::string strMin = m_vecwxRange[0]->GetValue().ToStdString();
	std::string strMax = m_vecwxRange[1]->GetValue().ToStdString();

	if (!STLStringHelper::IsFloat(strMin) || !STLStringHelper::IsFloat(strMax)) {
		SetDisplayedDataRange(
			m_imagepanel->GetDataRangeMin(),
			m_imagepanel->GetDataRangeMax());
		return;
	}

	float dRangeMin = stof(strMin);
	float dRangeMax = stof(strMax);

	if (dRangeMin > dRangeMax) {
		dRangeMin = dRangeMax;
	}

	m_imagepanel->SetDataRange(dRangeMin, dRangeMax, true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnRangeResetMinMax(
	wxCommandEvent & event
) {
	SetDataRangeByMinMax();
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


