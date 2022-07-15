///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisFrame.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#include "wxNcVisFrame.h"
#include <wx/filename.h>
#include <wx/dir.h>

#include "wxNcVisOptsDialog.h"
#include "STLStringHelper.h"
#include "ShpFile.h"
#include "TimeObj.h"
#include <set>

////////////////////////////////////////////////////////////////////////////////

enum {
	ID_COLORMAP = 2,
	ID_DATATRANS = 3,
	ID_BOUNDS = 4,
	ID_RANGEMIN = 6,
	ID_RANGEMAX = 7,
	ID_RANGERESETMINMAX = 8,
	ID_OPTIONS = 9,
	ID_GRIDLINES = 10,
	ID_OVERLAYS = 11,
	ID_SAMPLER = 12,
	ID_VARSELECTOR = 100,
	ID_DIMEDIT = 200,
	ID_DIMDOWN = 300,
	ID_DIMUP = 400,
	ID_DIMRESET = 500,
	ID_DIMPLAY = 600,
	ID_DIMVALUE = 700,
	ID_AXESX = 1000,
	ID_AXESY = 1100,
	ID_AXESXY = 1200,
	ID_DIMTIMER = 10000
};

////////////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(wxNcVisFrame, wxFrame)
	EVT_CLOSE(wxNcVisFrame::OnClose)
	EVT_MENU(wxID_EXIT, wxNcVisFrame::OnExit)
	EVT_MENU(wxID_ABOUT, wxNcVisFrame::OnAbout)
	EVT_BUTTON(ID_DATATRANS, wxNcVisFrame::OnDataTransClicked)
	EVT_BUTTON(ID_OPTIONS, wxNcVisFrame::OnOptionsClicked)
	EVT_TEXT_ENTER(ID_BOUNDS, wxNcVisFrame::OnBoundsChanged)
	EVT_TEXT_ENTER(ID_RANGEMIN, wxNcVisFrame::OnRangeChanged)
	EVT_TEXT_ENTER(ID_RANGEMAX, wxNcVisFrame::OnRangeChanged)
	EVT_BUTTON(ID_RANGERESETMINMAX, wxNcVisFrame::OnRangeResetMinMax)
	EVT_COMBOBOX(ID_COLORMAP, wxNcVisFrame::OnColorMapCombo)
	EVT_COMBOBOX(ID_GRIDLINES, wxNcVisFrame::OnGridLinesCombo)
	EVT_COMBOBOX(ID_OVERLAYS, wxNcVisFrame::OnOverlaysCombo)
	EVT_COMBOBOX(ID_SAMPLER, wxNcVisFrame::OnSamplerCombo)
	EVT_TIMER(ID_DIMTIMER, wxNcVisFrame::OnDimTimer)
wxEND_EVENT_TABLE()

////////////////////////////////////////////////////////////////////////////////

wxNcVisFrame::wxNcVisFrame(
	const wxString & title,
	const wxPoint & pos,
	const wxSize & size,
	const std::string & strNcVisResourceDir,
	const std::map<std::string, std::string> & mapOptions,
	const std::vector<std::string> & vecFilenames
) :
	wxFrame(NULL, wxID_ANY, title, pos, size),
	m_strNcVisResourceDir(strNcVisResourceDir),
	m_mapOptions(mapOptions),
	m_colormaplib(strNcVisResourceDir),
	m_egdsoption(GridDataSamplerOption_QuadTree),
	m_wxDataTransButton(NULL),
	m_panelsizer(NULL),
	m_ctrlsizer(NULL),
	m_rightsizer(NULL),
	m_vardimsizer(NULL),
	m_imagepanel(NULL),
	m_wxNcVisOptsDialog(NULL),
	m_wxDimTimer(this,ID_DIMTIMER),
	m_varActive(NULL),
	m_fIsVarActiveUnstructured(false),
	m_lAnimatedDim(-1),
	m_sColorMap(0),
	m_fDataHasMissingValue(false)
{
	m_lDisplayedDims[0] = (-1);
	m_lDisplayedDims[1] = (-1);

	m_vecwxImageBounds[0] = NULL;
	m_vecwxImageBounds[1] = NULL;
	m_vecwxImageBounds[2] = NULL;
	m_vecwxImageBounds[3] = NULL;

	for (size_t d = 0; d < NcVarMaximumDimensions; d++) {
		m_vecwxDimIndex[d] = NULL;
		m_vecwxPlayButton[d] = NULL;
	}

	m_data.Allocate(1);
	m_data[0] = 0.0;

	if (m_colormaplib.GetColorMapCount() == 0) {
		_EXCEPTIONT("FATAL ERROR: At least one colormap must be specified");
	}

	OpenFiles(vecFilenames);

	InitializeWindow();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::InitializeGridDataSampler() {

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
	{
		wxStopWatch sw;
		if (m_egdsoption == GridDataSamplerOption_QuadTree) {
			m_gdsqt.Initialize(dLon, dLat);
		}
		if (m_egdsoption == GridDataSamplerOption_CubedSphereQuadTree) {
			m_gdscsqt.Initialize(dLon, dLat);
		}
		if (m_egdsoption == GridDataSamplerOption_KDTree) {
			m_gdskd.Initialize(dLon, dLat);
		}
		Announce("Initializing the GridDataSampler took %ldms", sw.Time());
	}

	// Allocate data space
	if (m_data.GetRows() != varLon->get_dim(0)->size()) {
		m_data.Allocate(varLon->get_dim(0)->size());
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OpenFiles(
	const std::vector<std::string> & vecFilenames
) {
	_ASSERT(m_vecpncfiles.size() == 0);

	NcError error(NcError::silent_nonfatal);

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
			if (var->num_dims() >= NcVarMaximumDimensions) {
				std::cout << "Error: Only variables of dimension <= " << NcVarMaximumDimensions << " supported" << std::endl;
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

				NcAtt * attUnits = varDim->get_att("units");
				if (attUnits != NULL) {
					itVarDim->second.m_strUnits = attUnits->as_string(0);
				}

				NcAtt * attCalendar = varDim->get_att("calendar");
				if (attCalendar != NULL) {
					itVarDim->second.m_strCalendar = attCalendar->as_string(0);
				}

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

	// Determine which GridDataSampler was specified on the command line
	auto itGridDataSampler = m_mapOptions.find("-g");
	if (itGridDataSampler != m_mapOptions.end()) {
		if (itGridDataSampler->second == "qt") {
			m_egdsoption = GridDataSamplerOption_QuadTree;
		} else if (itGridDataSampler->second == "csqt") {
			m_egdsoption = GridDataSamplerOption_CubedSphereQuadTree;
		} else if (itGridDataSampler->second == "kd") {
			m_egdsoption = GridDataSamplerOption_KDTree;
		} else {
			_EXCEPTIONT("Invalid value for option -g: Expected [csqt,qt,kd]");
		}

	} else {
		m_egdsoption = GridDataSamplerOption_QuadTree;
	}

	// Initialize the GridDataSampler
	InitializeGridDataSampler();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::InitializeWindow() {
/*
	// Initialize options dialog
	m_wxNcVisOptsDialog =
		new wxNcVisOptsDialog(
			_T("NcVis Options"),
			wxPoint(60, 60),
			wxSize(400, 400),
			m_strNcVisResourceDir);

	m_wxNcVisOptsDialog->Hide();
*/
	// Get the list of shapefiles in the resource dir
	{
		wxDir dirResources(m_strNcVisResourceDir);
		if (!dirResources.IsOpened()) {
			std::cout << "ERROR: Cannot open resource directory \"" << m_strNcVisResourceDir << "\". Resources will not be populated." << std::endl;
		} else {
			wxString wxstrFilename;
			bool cont = dirResources.GetFirst(&wxstrFilename, _T("*.shp"), wxDIR_FILES);
			while (cont) {
				m_vecNcVisResourceShpFiles.push_back(wxstrFilename);
				cont = dirResources.GetNext(&wxstrFilename);
			}
		}
	}

	// Create menu
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(wxID_EXIT);

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, "&File" );
	menuBar->Append( menuHelp, "&Help" );

	SetMenuBar( menuBar );

	// Create a top-level panel to hold all the contents of the frame
    wxPanel * panel = new wxPanel(this, wxID_ANY);

	// Master panel sizer (controls on top, image panel on bottom)
	m_panelsizer = new wxBoxSizer(wxVERTICAL);

	// Variable controls (menu to left, variables to right)
	m_ctrlsizer = new wxBoxSizer(wxHORIZONTAL);

	// Vertical menu bar
	wxBoxSizer * menusizer = new wxBoxSizer(wxVERTICAL);

	// Variable controls
	m_rightsizer = new wxStaticBoxSizer(wxVERTICAL, this);
	m_rightsizer->SetMinSize(640,220);

	m_ctrlsizer->Add(menusizer, 0);
	m_ctrlsizer->Add(m_rightsizer, 0, wxEXPAND);
/*
	// Info box
	wxTextCtrl *infobox = new wxTextCtrl(this, -1,
      wxT("Hi!"), wxDefaultPosition, wxDefaultSize,
      wxTE_MULTILINE | wxTE_RICH , wxDefaultValidator, wxTextCtrlNameStr);
      Maximize();
*/
	// Data transform button (also reference widget height)
	m_wxDataTransButton = new wxButton(this, ID_DATATRANS, _T("Linear"));

	// Color map combobox
	wxComboBox * wxColorMapCombo = new wxComboBox(this, ID_COLORMAP, _T(""), wxDefaultPosition, wxSize(140,m_wxDataTransButton->GetSize().GetHeight()));
	for (size_t c = 0; c < m_colormaplib.GetColorMapCount(); c++) {
		wxColorMapCombo->Append(wxString(m_colormaplib.GetColorMapName(c)));
	}
	wxColorMapCombo->SetSelection(0);
	wxColorMapCombo->SetEditable(false);

	// Grid lines combobox
	wxComboBox * wxGridLinesCombo = new wxComboBox(this, ID_GRIDLINES, _T(""), wxDefaultPosition, wxSize(140,m_wxDataTransButton->GetSize().GetHeight()));
	wxGridLinesCombo->Append(_T("Grid Off"));
	wxGridLinesCombo->Append(_T("Grid On"));
	wxGridLinesCombo->SetSelection(0);
	wxGridLinesCombo->SetEditable(false);

	// Overlap combobox
	wxComboBox * wxOverlaysCombo = new wxComboBox(this, ID_OVERLAYS, _T(""), wxDefaultPosition, wxSize(140,m_wxDataTransButton->GetSize().GetHeight()));
	wxOverlaysCombo->Append(_T("Overlays Off"));
	for (size_t f = 0; f < m_vecNcVisResourceShpFiles.size(); f++) {
		wxOverlaysCombo->Append(m_vecNcVisResourceShpFiles[f]);
	}
	wxOverlaysCombo->SetSelection(0);
	wxOverlaysCombo->SetEditable(false);

	// Sampler combobox
	wxComboBox * wxSamplerCombo = new wxComboBox(this, ID_SAMPLER, _T(""), wxDefaultPosition, wxSize(140,m_wxDataTransButton->GetSize().GetHeight()));
	wxSamplerCombo->Append(_T("QuadTree (fast)"));
	wxSamplerCombo->Append(_T("CS QuadTree"));
	wxSamplerCombo->Append(_T("kd-Tree (best)"));
	wxSamplerCombo->SetSelection((int)m_egdsoption);
	wxSamplerCombo->SetEditable(false);

	// Add controls to the manusizer
	menusizer->Add(wxColorMapCombo, 0, wxEXPAND | wxALL, 2);
	menusizer->Add(m_wxDataTransButton, 0, wxEXPAND | wxALL, 2);
	menusizer->Add(wxGridLinesCombo, 0, wxEXPAND | wxALL, 2);
	menusizer->Add(wxOverlaysCombo, 0, wxEXPAND | wxALL, 2);
	menusizer->Add(wxSamplerCombo, 0, wxEXPAND | wxALL, 2);
	//menusizer->Add(new wxButton(this, ID_OPTIONS, _T("Options")), 0, wxEXPAND | wxALL, 2);
	menusizer->Add(new wxButton(this, -1, _T("Export")), 0, wxEXPAND | wxALL, 2);

	// Variable selector
	wxBoxSizer *varsizer = new wxBoxSizer(wxHORIZONTAL);

	for (int vc = 0; vc < NcVarMaximumDimensions; vc++) {
		m_vecwxVarSelector[vc] = NULL;
	}
	for (int vc = 0; vc < NcVarMaximumDimensions; vc++) {
		if (m_mapVarNames[vc].size() == 0) {
			continue;
		}

		m_vecwxVarSelector[vc] =
			new wxComboBox(this, ID_VARSELECTOR + vc,
				wxString::Format("(%lu) %iD vars", m_mapVarNames[vc].size(), vc),
				wxDefaultPosition, wxSize(120, m_wxDataTransButton->GetSize().GetHeight()));

		m_vecwxVarSelector[vc]->Bind(wxEVT_COMBOBOX, &wxNcVisFrame::OnVariableSelected, this);
		m_vecwxVarSelector[vc]->SetEditable(false);
		for (auto itVar = m_mapVarNames[vc].begin(); itVar != m_mapVarNames[vc].end(); itVar++) {
			m_vecwxVarSelector[vc]->Append(wxString(itVar->first));
		}
		varsizer->Add(m_vecwxVarSelector[vc], 0, wxEXPAND | wxBOTTOM, 8);
	}

	// Dimensions
	m_vardimsizer = new wxFlexGridSizer(NcVarMaximumDimensions+1, 4, 0, 0);

	// Image panel
	m_imagepanel = new wxImagePanel(this);

	m_imagepanel->SetColorMap(m_colormaplib.GetColorMapName(0));

	m_rightsizer->Add(varsizer, 0, wxALIGN_CENTER, 0);
	m_rightsizer->Add(m_vardimsizer, 0, wxALIGN_CENTER, 0);

	m_panelsizer->Add(m_imagepanel, 1, wxALIGN_TOP | wxALIGN_CENTER | wxSHAPED);
	m_panelsizer->Add(m_ctrlsizer, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER);

	CreateStatusBar();

	// Status bar
	SetStatusMessage(_T(""), true);

	SetSizerAndFit(m_panelsizer);
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

		} else if (
			(m_lDisplayedDims[1] == m_varActive->num_dims()-2) &&
		    (m_lDisplayedDims[0] == m_varActive->num_dims()-1)
		) {
			m_varActive->get(&(m_data[0]), &(vecSize[0]));

		} else {
			std::vector<long> vecStride(m_varActive->num_dims(), 1);

			long lDisplayedDimsMin = m_lDisplayedDims[0];
			long lDisplayedDimsMax = m_lDisplayedDims[1];
			if (lDisplayedDimsMin > lDisplayedDimsMax) {
				lDisplayedDimsMin = m_lDisplayedDims[1];
				lDisplayedDimsMax = m_lDisplayedDims[0];
			}

			for (long d = lDisplayedDimsMin+1; d < m_varActive->num_dims(); d++) {
				if (d != lDisplayedDimsMax) {
					vecStride[d] = m_varActive->get_dim(d)->size();
				}
			}
/*
			std::cout << m_lDisplayedDims[0] << " : " << m_lDisplayedDims[1] << std::endl;

			for (long d = 0; d < vecSize.size(); d++) {
				std::cout << m_lVarActiveDims[d] << ",";
			}
			std::cout << std::endl;

			for (long d = 0; d < vecSize.size(); d++) {
				std::cout << vecSize[d] << ",";
			}
			std::cout << std::endl;

			for (long d = 0; d < vecStride.size(); d++) {
				std::cout << vecStride[d] << ",";
			}
			std::cout << std::endl;
*/
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

	// Determine which data coordinates correspond to the sample coordinates
	if (pvecDimValues->size() < 2) {
		return;
	}

	// Monotone increasing coordinate
	if ((*pvecDimValues)[1] > (*pvecDimValues)[0]) {
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

	// Monotone decreasing coordinate
	} else {
		for (size_t s = 0; s < dSample.GetRows(); s++) {
			for (size_t t = 1; t < pvecDimValues->size()-1; t++) {
				double dLeft = 0.5 * ((*pvecDimValues)[t-1] + (*pvecDimValues)[t]);
				double dRight = 0.5 * ((*pvecDimValues)[t] + (*pvecDimValues)[t+1]);
				if ((t == 1) && (dSample[s] > dLeft)) {
					veccoordmap[s] = 0;
					break;
				}
				if ((t == pvecDimValues->size()-2) && (dSample[s] < dRight)) {
					veccoordmap[s] = pvecDimValues->size()-1;
					break;
				}
				if ((dSample[s] <= dLeft) && (dSample[s] >= dRight)) {
					veccoordmap[s] = t;
					break;
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

// TODO: Change from DataArray1D to a vector
void wxNcVisFrame::SampleData(
	const DataArray1D<double> & dSampleX,
	const DataArray1D<double> & dSampleY,
	DataArray1D<int> & imagemap
) {
	std::cout << "SAMPLE DATA" << std::endl;

	_ASSERT(imagemap.GetRows() >= dSampleX.GetRows() * dSampleY.GetRows());
	_ASSERT(m_data.GetRows() > 0);

	// Active variable is an unstructured variable; use sampling
	if (m_fIsVarActiveUnstructured) {
		if (m_egdsoption == GridDataSamplerOption_QuadTree) {
			m_gdsqt.Sample(dSampleX, dSampleY, imagemap);
		} else if (m_egdsoption == GridDataSamplerOption_CubedSphereQuadTree) {
			m_gdscsqt.Sample(dSampleX, dSampleY, imagemap);
		} else if (m_egdsoption == GridDataSamplerOption_KDTree) {
			m_gdskd.Sample(dSampleX, dSampleY, imagemap);
		} else {
			_EXCEPTIONT("No GridDataSampler initialized");
		}

	// No displayed variables
	} else if ((m_lDisplayedDims[0] == (-1)) && (m_lDisplayedDims[1] == (-1))) {
		for (size_t s = 0; s < imagemap.GetRows(); s++) {
			imagemap[s] = 0;
		}

	// One displayed variable along X axis
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

		size_t sDimYSize = m_varActive->get_dim(m_lDisplayedDims[0])->size();
		size_t sDimXSize = m_varActive->get_dim(m_lDisplayedDims[1])->size();

		// Assemble the image map
		size_t s = 0;
		if (m_lDisplayedDims[0] < m_lDisplayedDims[1]) {
			for (size_t j = 0; j < dSampleY.GetRows(); j++) {
			for (size_t i = 0; i < dSampleX.GetRows(); i++) {
				imagemap[s] = veccoordmapY[j] * sDimXSize + veccoordmapX[i];
				s++;
			}
			}
		} else {
			for (size_t j = 0; j < dSampleY.GetRows(); j++) {
			for (size_t i = 0; i < dSampleX.GetRows(); i++) {
				imagemap[s] = veccoordmapX[i] * sDimYSize + veccoordmapY[j];
				s++;
			}
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
	if ((m_vecwxImageBounds[0] == NULL) && (m_vecwxImageBounds[1] == NULL)) {
		if (m_vecwxImageBounds[2] != NULL) {
			m_vecwxImageBounds[2]->ChangeValue(wxString::Format(wxT("%.7g"), dX0));
		}
		if (m_vecwxImageBounds[3] != NULL) {
			m_vecwxImageBounds[3]->ChangeValue(wxString::Format(wxT("%.7g"), dX1));
		}

	} else {
		if (m_vecwxImageBounds[0] != NULL) {
			m_vecwxImageBounds[0]->ChangeValue(wxString::Format(wxT("%.7g"), dX0));
		}
		if (m_vecwxImageBounds[1] != NULL) {
			m_vecwxImageBounds[1]->ChangeValue(wxString::Format(wxT("%.7g"), dX1));
		}
		if (m_vecwxImageBounds[2] != NULL) {
			m_vecwxImageBounds[2]->ChangeValue(wxString::Format(wxT("%.7g"), dY0));
		}
		if (m_vecwxImageBounds[3] != NULL) {
			m_vecwxImageBounds[3]->ChangeValue(wxString::Format(wxT("%.7g"), dY1));
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::ResetBounds(
	int iDim,
	bool fRedraw
) {
	std::cout << "RESET BOUNDS" << std::endl;
	double dXmin[2] = {0.0, -90.0};
	double dXmax[2] = {360.0, 90.0};

	if (m_varActive == NULL) {
		m_imagepanel->SetCoordinateRange(dXmin[0], dXmax[0], dXmin[1], dXmax[1], fRedraw);
		return;
	}
	if (m_varActive->num_dims() == 0) {
		m_imagepanel->SetCoordinateRange(0.0, 1.0, 0.0, 1.0, fRedraw);
		return;
	}

	_ASSERT((m_lDisplayedDims[0] != (-1)) || (m_lDisplayedDims[1] != (-1)));

	if (m_lDisplayedDims[0] != (-1)) {
		if (m_strUnstructDimName == m_varActive->get_dim(m_lDisplayedDims[0])->name()) {
			_ASSERT(m_lDisplayedDims[1] == (-1));
			m_imagepanel->SetCoordinateRange(dXmin[0], dXmax[0], dXmin[1], dXmax[1], fRedraw);
			return;
		}
	}

	size_t dmin, dmax;
	if (iDim == (-1)) {
		dmin = 0;
		dmax = 2;

	} else if (iDim == 0) {
		dmin = static_cast<size_t>(0);
		dmax = static_cast<size_t>(1);

		dXmin[1] = m_imagepanel->GetXRangeMin();
		dXmax[1] = m_imagepanel->GetXRangeMax();

	} else if (iDim == 1) {
		dmin = static_cast<size_t>(1);
		dmax = static_cast<size_t>(2);

		dXmin[0] = m_imagepanel->GetYRangeMin();
		dXmax[0] = m_imagepanel->GetYRangeMax();

	} else {
		_EXCEPTION();
	}

	for (size_t d = dmin; d < dmax; d++) {
		if (m_lDisplayedDims[d] == (-1)) {
			continue;
		}

		std::string strDimName(m_varActive->get_dim(m_lDisplayedDims[d])->name());

		auto itDim = m_mapDimData.find(strDimName);
		if (itDim != m_mapDimData.end()) {
			if (itDim->second.size() == 0) {
				dXmin[d] = -0.5;
				dXmax[d] = static_cast<double>(m_varActive->get_dim(m_lDisplayedDims[d])->size()-1) + 0.5;
				continue;
			}

			auto itDimCoord = itDim->second.begin();
			const std::vector<double> & coord = itDimCoord->second;
			if (coord.size() == 1) {
				dXmin[d] = coord[0];
				dXmax[d] = coord[0];
			} else if (coord[1] > coord[0]) {
				int nc = coord.size();
				dXmin[d] = coord[0] - 0.5 * (coord[1] - coord[0]);
				dXmax[d] = coord[nc-1] + 0.5 * (coord[nc-1] - coord[nc-2]);
			} else {
				int nc = coord.size();
				dXmin[d] = coord[nc-1] + 0.5 * (coord[nc-1] - coord[nc-2]);
				dXmax[d] = coord[0] - 0.5 * (coord[1] - coord[0]);
			}

			// Special cases (latitude)
			if ((strDimName.find("lat") != std::string::npos) ||
			    (strDimName.find("Lat") != std::string::npos) ||
			    (strDimName.find("LAT") != std::string::npos)
			) {
				if (dXmin[d] < -90.0) {
					dXmin[d] = -90.0;
				}
				if (dXmax[d] < -90.0) {
					dXmax[d] = -90.0;
				}
				if (dXmin[d] > 90.0) {
					dXmin[d] = 90.0;
				}
				if (dXmax[d] > 90.0) {
					dXmax[d] = 90.0;
				}
			}

			// Special cases (longitude)
			if ((strDimName.find("lon") != std::string::npos) ||
			    (strDimName.find("Lon") != std::string::npos) ||
			    (strDimName.find("LON") != std::string::npos)
			) {
				if (fabs(dXmax[d] - dXmin[d] - 360.0) < 1.0e-5) {
					if (coord[1] > coord[0]) {
						dXmin[d] = coord[0];
						dXmax[d] = coord[0] + 360.0;
					} else {
						dXmin[d] = coord[coord.size()-1];
						dXmax[d] = coord[coord.size()-1] + 360.0;
					}
				}
			}

		} else {
			dXmin[d] = -0.5;
			dXmax[d] = static_cast<double>(m_varActive->get_dim(m_lDisplayedDims[d])->size()-1) + 0.5;
		}
	}

	if (m_varActive->num_dims() == 1) {
		m_imagepanel->SetCoordinateRange(dXmin[0], dXmax[0], 0.0, 1.0, fRedraw);
	} else {
		m_imagepanel->SetCoordinateRange(dXmin[1], dXmax[1], dXmin[0], dXmax[0], fRedraw);
	}
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

void wxNcVisFrame::SetDataRangeByMinMax(
	bool fRedraw
) {
	if (m_data.GetRows() == 0) {
		return;
	}

	float dDataMin = m_data[0];
	float dDataMax = m_data[0];
	if (!m_fDataHasMissingValue) {
		for (int i = 1; i < m_data.GetRows(); i++) {
			if (m_data[i] < dDataMin) {
				dDataMin = m_data[i];
			}
			if (m_data[i] > dDataMax) {
				dDataMax = m_data[i];
			}
		}
	} else {
		int i;
		for (i = 0; i < m_data.GetRows(); i++) {
			if (m_data[i] != m_dMissingValueFloat) {
				dDataMin = m_data[i];
				dDataMax = m_data[i];
				break;
			}
		}
		for (; i < m_data.GetRows(); i++) {
			if (m_data[i] == m_dMissingValueFloat) {
				continue;
			}
			if (m_data[i] < dDataMin) {
				dDataMin = m_data[i];
			}
			if (m_data[i] > dDataMax) {
				dDataMax = m_data[i];
			}
		}
	}

	m_imagepanel->SetDataRange(dDataMin, dDataMax, fRedraw);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::SetDisplayedDimensionValue(
	long lDim,
	long lValue
) {
	_ASSERT(m_varActive != NULL);
	_ASSERT(m_vecwxDimIndex[lDim] != NULL);

	m_vecwxDimIndex[lDim]->ChangeValue(wxString::Format("%li", lValue));

	if (m_vecwxDimValue[lDim] != NULL) {
		std::string strDimName(m_varActive->get_dim(lDim)->name());
		auto it = m_mapDimData.find(strDimName);
		if (it != m_mapDimData.end()) {
			std::string strDimUnits(it->second.units());
			if (it->second.size() != 0) {
				const std::vector<double> & vecDimValues = it->second[0];
				if (vecDimValues.size() > lValue) {
					if (strDimUnits == "") {
						m_vecwxDimValue[lDim]->ChangeValue(
							wxString::Format("%g", vecDimValues[lValue]));
					} else {
						Time time(Time::CalendarTypeFromString(it->second.calendar()));
						if (time.FromCFCompliantUnitsOffsetDouble(strDimUnits, vecDimValues[lValue])) {
							m_vecwxDimValue[lDim]->ChangeValue(
								time.ToString());
						} else {
							m_vecwxDimValue[lDim]->ChangeValue(
								wxString::Format("%g %s",
									vecDimValues[lValue],
									strDimUnits));
						}
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::SetStatusMessage(
	const wxString & strMessage,
	bool fIncludeVersion
) {
	if (fIncludeVersion) {
		wxString strMessageBak = _T("NcVis 2022.07.15");
		strMessageBak += strMessage;
		SetStatusText( strMessageBak );
	} else {
		SetStatusText( strMessage );
	}
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

void wxNcVisFrame::OnClose(
	wxCloseEvent & event
) {
	if (m_wxNcVisOptsDialog != NULL) {
		m_wxNcVisOptsDialog->Destroy();
	}
	Destroy();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnDataTransClicked(
	wxCommandEvent & event
) {
	if (m_imagepanel == NULL) {
		return;
	}

	wxString wxstrLabel = m_wxDataTransButton->GetLabel();

	if (wxstrLabel == "Linear") {
		m_wxDataTransButton->SetLabel("Low");
		m_imagepanel->SetColorMapScalingFactor(0.25, true);

	} else if (wxstrLabel == "Low") {
		m_wxDataTransButton->SetLabel("High");
		m_imagepanel->SetColorMapScalingFactor(4.0, true);

	} else if (wxstrLabel == "High") {
		m_wxDataTransButton->SetLabel("Linear");
		m_imagepanel->SetColorMapScalingFactor(1.0, true);

	} else {
		_EXCEPTION();
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnOptionsClicked(
	wxCommandEvent & event
) {

	if (m_wxNcVisOptsDialog == NULL) {
		return;
	}

	m_wxNcVisOptsDialog->Show(true);
	m_wxNcVisOptsDialog->SetFocus();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::GenerateDimensionControls() {

	// Get the height of the control
	wxSize wxsizeButton = m_wxDataTransButton->GetSize();
	int nCtrlHeight = wxsizeButton.GetHeight();
	wxSize wxSquareSize(nCtrlHeight+2, nCtrlHeight);

	// Add dimension controls
	m_vecwxImageBounds[0] = NULL;
	m_vecwxImageBounds[1] = NULL;
	m_vecwxImageBounds[2] = NULL;
	m_vecwxImageBounds[3] = NULL;
	m_vecwxRange[0] = NULL;
	m_vecwxRange[1] = NULL;

	m_vardimsizer->Clear(true);

	for (long d = 0; d < m_varActive->num_dims(); d++) {
		wxString strDim = wxString(m_varActive->get_dim(d)->name()) + ":";

		wxBoxSizer * vardimboxsizerxy = new wxBoxSizer(wxHORIZONTAL);
		m_vecwxActiveAxes[d][0] = new wxButton(this, ID_AXESX + d, _T("X"), wxDefaultPosition, wxSquareSize);
		m_vecwxActiveAxes[d][1] = new wxButton(this, ID_AXESY + d, _T("Y"), wxDefaultPosition, wxSquareSize);
		m_vecwxActiveAxes[d][2] = new wxButton(this, ID_AXESXY + d, _T("XY"), wxDefaultPosition, wxSize(2*nCtrlHeight,nCtrlHeight));

		m_vecwxActiveAxes[d][0]->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnAxesButtonClicked, this);
		m_vecwxActiveAxes[d][1]->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnAxesButtonClicked, this);
		m_vecwxActiveAxes[d][2]->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnAxesButtonClicked, this);

		vardimboxsizerxy->Add(m_vecwxActiveAxes[d][0], 0, wxEXPAND | wxALL, 2);
		vardimboxsizerxy->Add(m_vecwxActiveAxes[d][1], 0, wxEXPAND | wxALL, 2);
		vardimboxsizerxy->Add(m_vecwxActiveAxes[d][2], 0, wxEXPAND | wxALL, 2);
		m_vardimsizer->Add(vardimboxsizerxy, 0, wxEXPAND | wxALL, 2);

		m_vardimsizer->Add(new wxStaticText(this, -1, wxString(m_varActive->get_dim(d)->name()), wxDefaultPosition, wxSize(60,nCtrlHeight), wxST_ELLIPSIZE_END | wxALIGN_CENTRE_HORIZONTAL | wxALIGN_CENTER_VERTICAL), 1, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 4);

		if (m_strUnstructDimName != m_varActive->get_dim(d)->name()) {
			m_vecwxActiveAxes[d][2]->Enable(false);
		} else {
			m_vecwxActiveAxes[d][0]->Enable(false);
			m_vecwxActiveAxes[d][1]->Enable(false);
		}
		if ((m_varActive->num_dims() < 3) && (m_fIsVarActiveUnstructured)) {
			m_vecwxActiveAxes[d][0]->Enable(false);
			m_vecwxActiveAxes[d][1]->Enable(false);
		}
		if (m_varActive->num_dims() < 2) {
			m_vecwxActiveAxes[d][0]->Enable(false);
			m_vecwxActiveAxes[d][1]->Enable(false);
		}

		if (d == m_lDisplayedDims[0]) {

			// Dimension is the XY coordinate on the plot (unstructured)
			if (m_strUnstructDimName == m_varActive->get_dim(d)->name()) {
				m_vecwxActiveAxes[d][2]->SetLabelMarkup(_T("<span color=\"red\" weight=\"bold\">XY</span>"));

				wxBoxSizer * vardimboxsizerminmax = new wxBoxSizer(wxHORIZONTAL);
				m_vecwxImageBounds[0] = new wxTextCtrl(this, ID_BOUNDS, _T(""), wxDefaultPosition, wxSize(100, nCtrlHeight), wxTE_CENTRE | wxTE_PROCESS_ENTER);
				m_vecwxImageBounds[1] = new wxTextCtrl(this, ID_BOUNDS, _T(""), wxDefaultPosition, wxSize(100, nCtrlHeight), wxTE_CENTRE | wxTE_PROCESS_ENTER);
				m_vecwxImageBounds[2] = new wxTextCtrl(this, ID_BOUNDS, _T(""), wxDefaultPosition, wxSize(100, nCtrlHeight), wxTE_CENTRE | wxTE_PROCESS_ENTER);
				m_vecwxImageBounds[3] = new wxTextCtrl(this, ID_BOUNDS, _T(""), wxDefaultPosition, wxSize(100, nCtrlHeight), wxTE_CENTRE | wxTE_PROCESS_ENTER);
				vardimboxsizerminmax->Add(m_vecwxImageBounds[0], 1, wxEXPAND | wxALL, 0);
				vardimboxsizerminmax->Add(m_vecwxImageBounds[1], 1, wxEXPAND | wxALL, 0);
				vardimboxsizerminmax->Add(m_vecwxImageBounds[2], 1, wxEXPAND | wxALL, 0);
				vardimboxsizerminmax->Add(m_vecwxImageBounds[3], 1, wxEXPAND | wxALL, 0);

				m_vardimsizer->Add(vardimboxsizerminmax, 0, wxEXPAND | wxALL, 2);

				wxButton * wxDimReset = new wxButton(this, ID_DIMRESET + d, _T("Reset"), wxDefaultPosition, wxSize(3*nCtrlHeight,nCtrlHeight));
				wxDimReset->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnDimButtonClicked, this);
				m_vardimsizer->Add(wxDimReset, 0, wxEXPAND | wxALL, 0);

			// Dimension is the Y coordinate on the plot or variable is 1D
			} else {
				if (m_varActive->num_dims() >= 2) {
					m_vecwxActiveAxes[d][1]->SetLabelMarkup(_T("<span color=\"red\" weight=\"bold\">Y</span>"));
				}

				wxBoxSizer * vardimboxsizerminmax = new wxBoxSizer(wxHORIZONTAL);
				m_vecwxImageBounds[2] = new wxTextCtrl(this, ID_BOUNDS, _T(""), wxDefaultPosition, wxSize(200, nCtrlHeight), wxTE_CENTRE | wxTE_PROCESS_ENTER);
				m_vecwxImageBounds[3] = new wxTextCtrl(this, ID_BOUNDS, _T(""), wxDefaultPosition, wxSize(200, nCtrlHeight), wxTE_CENTRE | wxTE_PROCESS_ENTER);
				vardimboxsizerminmax->Add(m_vecwxImageBounds[2], 1, wxEXPAND | wxALL, 0);
				vardimboxsizerminmax->Add(m_vecwxImageBounds[3], 1, wxEXPAND | wxALL, 0);

				m_vardimsizer->Add(vardimboxsizerminmax, 0, wxEXPAND | wxALL, 2);

				wxButton * wxDimReset = new wxButton(this, ID_DIMRESET + d, _T("Reset"), wxDefaultPosition, wxSize(3*nCtrlHeight,nCtrlHeight));
				wxDimReset->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnDimButtonClicked, this);
				m_vardimsizer->Add(wxDimReset, 0, wxEXPAND | wxALL, 0);
			}

		// Dimension is the X coordinate on the plot
		} else if (d == m_lDisplayedDims[1]) {
			m_vecwxActiveAxes[d][0]->SetLabelMarkup(_T("<span color=\"red\" weight=\"bold\">X</span>"));

			wxBoxSizer * vardimboxsizerminmax = new wxBoxSizer(wxHORIZONTAL);
			m_vecwxImageBounds[0] = new wxTextCtrl(this, ID_BOUNDS, _T(""), wxDefaultPosition, wxSize(200, nCtrlHeight), wxTE_CENTRE | wxTE_PROCESS_ENTER);
			m_vecwxImageBounds[1] = new wxTextCtrl(this, ID_BOUNDS, _T(""), wxDefaultPosition, wxSize(200, nCtrlHeight), wxTE_CENTRE | wxTE_PROCESS_ENTER);
			vardimboxsizerminmax->Add(m_vecwxImageBounds[0], 1, wxEXPAND | wxALL, 0);
			vardimboxsizerminmax->Add(m_vecwxImageBounds[1], 1, wxEXPAND | wxALL, 0);

			m_vardimsizer->Add(vardimboxsizerminmax, 0, wxEXPAND | wxALL, 2);

			wxButton * wxDimReset = new wxButton(this, ID_DIMRESET + d, _T("Reset"), wxDefaultPosition, wxSize(3*nCtrlHeight,nCtrlHeight));
			wxDimReset->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnDimButtonClicked, this);
			m_vardimsizer->Add(wxDimReset, 0, wxEXPAND | wxALL, 0);

		// Dimension is freely specified
		} else {
			wxBoxSizer * vardimboxsizer = new wxBoxSizer(wxHORIZONTAL);
			wxButton * wxDimDown = new wxButton(this, ID_DIMDOWN + d, _T("-"), wxDefaultPosition, wxSquareSize);
			m_vecwxDimIndex[d] = new wxTextCtrl(this, ID_DIMEDIT + d, _T(""), wxDefaultPosition, wxSize(50, nCtrlHeight), wxTE_CENTRE | wxTE_PROCESS_ENTER);
			m_vecwxDimValue[d] = new wxTextCtrl(this, ID_DIMVALUE + d, _T(""), wxDefaultPosition, wxSize(150, nCtrlHeight), wxTE_CENTRE | wxTE_PROCESS_ENTER);
			wxButton * wxDimUp = new wxButton(this, ID_DIMUP + d, _T("+"), wxDefaultPosition, wxSquareSize);
			m_vecwxPlayButton[d] = new wxButton(this, ID_DIMPLAY + d, wxString::Format("%lc",(0x25B6)), wxDefaultPosition, wxSquareSize);

			SetDisplayedDimensionValue(d, m_lVarActiveDims[d]);

			vardimboxsizer->Add(wxDimDown, 0, wxEXPAND | wxRIGHT, 1);
			vardimboxsizer->Add(m_vecwxDimIndex[d], 1, wxEXPAND | wxRIGHT, 0);
			vardimboxsizer->Add(m_vecwxDimValue[d], 3, wxEXPAND | wxRIGHT, 1);
			vardimboxsizer->Add(wxDimUp, 0, wxEXPAND | wxRIGHT, 1);
			vardimboxsizer->Add(m_vecwxPlayButton[d], 0, wxEXPAND | wxALL, 0);

			wxDimDown->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnDimButtonClicked, this);
			m_vecwxDimIndex[d]->Bind(wxEVT_TEXT, &wxNcVisFrame::OnDimButtonClicked, this);
			wxDimUp->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnDimButtonClicked, this);
			m_vecwxPlayButton[d]->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnDimButtonClicked, this);

			m_vardimsizer->Add(vardimboxsizer, 0, wxEXPAND | wxALL, 2);

			wxButton * wxDimReset = new wxButton(this, ID_DIMRESET + d, _T("Reset"), wxDefaultPosition, wxSize(3*nCtrlHeight,nCtrlHeight));
			wxDimReset->Bind(wxEVT_BUTTON, &wxNcVisFrame::OnDimButtonClicked, this);
			m_vardimsizer->Add(wxDimReset, 0, wxEXPAND | wxALL, 0);

			m_vecwxDimValue[d]->Enable(false);
		}
	}

	// Data range controls
	m_vecwxRange[0] = new wxTextCtrl(this, ID_RANGEMIN, _T(""), wxDefaultPosition, wxSize(200,nCtrlHeight+4), wxTE_CENTRE | wxTE_PROCESS_ENTER);
	m_vecwxRange[1] = new wxTextCtrl(this, ID_RANGEMAX, _T(""), wxDefaultPosition, wxSize(200,nCtrlHeight+4), wxTE_CENTRE | wxTE_PROCESS_ENTER);

	wxBoxSizer * varboundsminmax = new wxBoxSizer(wxHORIZONTAL);
	varboundsminmax->Add(m_vecwxRange[0], 1, wxEXPAND | wxALL, 0);
	varboundsminmax->Add(m_vecwxRange[1], 1, wxEXPAND | wxALL, 0);

	m_vardimsizer->Add(new wxStaticText(this, -1, _T("")), 0, wxEXPAND | wxALL, 0);
	m_vardimsizer->Add(new wxStaticText(this, -1, _T("range"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL | wxALIGN_CENTER_VERTICAL), 1, wxALIGN_CENTER_VERTICAL | wxEXPAND | wxALL, 4);
	m_vardimsizer->Add(varboundsminmax, 0, wxEXPAND | wxALL, 2);
	m_vardimsizer->Add(new wxButton(this, ID_RANGERESETMINMAX, _T("Reset"), wxDefaultPosition, wxSize(3*nCtrlHeight,nCtrlHeight)), 0, wxEXPAND | wxALL, 0);

	m_vecwxRange[0]->Enable(true);
	m_vecwxRange[1]->Enable(true);

	SetDataRangeByMinMax(false);

	// Resize window if needed
	if (m_panelsizer->GetMinSize().GetHeight() > m_panelsizer->GetSize().GetHeight()) {
		SetSizerAndFit(m_panelsizer);
	}

	// Layout widgets
	m_vardimsizer->Layout();
	m_rightsizer->Layout();
	m_ctrlsizer->Layout();
	m_panelsizer->Layout();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnVariableSelected(
	wxCommandEvent & event
) {
	std::cout << "VARIABLE SELECTED" << std::endl;

	// Turn off animation if active
	StopAnimation();

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

	// Store current active variable dimensions
	std::string strPreviousDimName[2];
	if (m_varActive != NULL) {
		if (m_lDisplayedDims[0] != (-1)) {
			strPreviousDimName[0] = m_varActive->get_dim(m_lDisplayedDims[0])->name();
		}
		if (m_lDisplayedDims[1] != (-1)) {
			strPreviousDimName[1] = m_varActive->get_dim(m_lDisplayedDims[1])->name();
		}
	}

	// Change the active variable
	std::string strValue = event.GetString().ToStdString();

	int vc = static_cast<int>(event.GetId() - ID_VARSELECTOR);
	_ASSERT((vc >= 0) && (vc < NcVarMaximumDimensions));
	auto itVar = m_mapVarNames[vc].find(strValue);
	m_varActive = m_vecpncfiles[itVar->second[0]]->get_var(strValue.c_str());
	_ASSERT(m_varActive != NULL);

	// Check for missing value
	{
		NcError error(NcError::silent_nonfatal);
		NcAtt * attMissingValue = m_varActive->get_att("_FillValue");
		if (attMissingValue != NULL) {
			m_fDataHasMissingValue = true;
			m_dMissingValueFloat = attMissingValue->as_float(0);
		} else {
			m_fDataHasMissingValue = false;
		}
	}

	m_lVarActiveDims.resize(m_varActive->num_dims());

	// Initialize displayed dimension(s) and active dimensions
	m_lDisplayedDims[0] = (-1);
	m_lDisplayedDims[1] = (-1);

	// First check if previously selected dimensions already exist in data
	if ((strPreviousDimName[0] != "") && (strPreviousDimName[1] != "")) {
		for (long d = 0; d < m_varActive->num_dims(); d++) {
			if (strPreviousDimName[0] == m_varActive->get_dim(d)->name()) {
				m_lDisplayedDims[0] = d;
			}
			if (strPreviousDimName[1] == m_varActive->get_dim(d)->name()) {
				m_lDisplayedDims[1] = d;
			}
		}

	} else if (strPreviousDimName[0] != "") {
		for (long d = 0; d < m_varActive->num_dims(); d++) {
			if (strPreviousDimName[0] == m_varActive->get_dim(d)->name()) {
				m_lDisplayedDims[0] = d;
			}
		}
	}

	// Otherwise select new dimensions by variable type
	if ((m_lDisplayedDims[0] == (-1)) && (m_lDisplayedDims[1] == (-1))) {
		for (long d = 0; d < m_varActive->num_dims(); d++) {
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

		ResetBounds();

	} else if (m_lDisplayedDims[0] == (-1)) {
		_ASSERT(m_varActive->num_dims() >= 1);
		if (m_varActive->num_dims() == 1) {
			m_lDisplayedDims[0] = m_lDisplayedDims[1];
			m_lDisplayedDims[1] = (-1);
		} else {
			for (long d = m_varActive->num_dims()-1; d >= 0; d--) {
				if (d != m_lDisplayedDims[1]) {
					m_lDisplayedDims[0] = d;
					break;
				}
			}
			ResetBounds(0);
		}

	} else if (m_lDisplayedDims[1] == (-1)) {
		_ASSERT(m_varActive->num_dims() >= 1);
		if (m_strUnstructDimName == m_varActive->get_dim(m_lDisplayedDims[0])->name()) {
		} else if (m_varActive->num_dims() != 1) {
			for (long d = m_varActive->num_dims()-1; d >= 0; d--) {
				if (d != m_lDisplayedDims[0]) {
					m_lDisplayedDims[1] = d;
					break;
				}
			}
			ResetBounds(1);
		}
	}

	// Set lVarActiveDims using bookmarked indices
	for (long d = 0; d < m_varActive->num_dims(); d++) {
		if ((d == m_lDisplayedDims[0]) || (d == m_lDisplayedDims[1])) {
			m_lVarActiveDims[d] = 0;
		} else {
			auto itDimCurrent = m_mapDimBookmarks.find(m_varActive->get_dim(d)->name());
			if (itDimCurrent != m_mapDimBookmarks.end()) {
				m_lVarActiveDims[d] = itDimCurrent->second;
			} else {
				m_lVarActiveDims[d] = 0;
			}
		}
	}

	// Load the data
	LoadData();

	// Revert all other combo boxes
	for (int vc = 0; vc < NcVarMaximumDimensions; vc++) {
		if ((m_vecwxVarSelector[vc] != NULL) && (vc != m_varActive->num_dims())) {
			m_vecwxVarSelector[vc]->ChangeValue(
				wxString::Format("(%lu) %iD vars", m_mapVarNames[vc].size(), vc));
		}
	}

	// Generate dimension controls
	GenerateDimensionControls();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnBoundsChanged(
	wxCommandEvent & event
) {
	std::cout << "BOUNDS CHANGED" << std::endl;

	// 1D variable, only one set of bounds available
	if ((m_vecwxImageBounds[0] == NULL) && (m_vecwxImageBounds[1] == NULL)) {
		_ASSERT(m_vecwxImageBounds[2] != NULL);
		_ASSERT(m_vecwxImageBounds[3] != NULL);

		std::string strX0 = m_vecwxImageBounds[2]->GetValue().ToStdString();
		std::string strX1 = m_vecwxImageBounds[3]->GetValue().ToStdString();

		if (!STLStringHelper::IsFloat(strX0) ||
		    !STLStringHelper::IsFloat(strX1)
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

		m_imagepanel->SetCoordinateRange(dX0, dX1, 0.0, 1.0, true);

	// 2D variable, two sets of bounds available
	} else {
		_ASSERT(m_vecwxImageBounds[0] != NULL);
		_ASSERT(m_vecwxImageBounds[1] != NULL);
		_ASSERT(m_vecwxImageBounds[2] != NULL);
		_ASSERT(m_vecwxImageBounds[3] != NULL);

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
	SetDataRangeByMinMax(true);
}

////////////////////////////////////////////////////////////////////////////////

//TODO: Verify that timer is actually able to execute with the desired frequency
void wxNcVisFrame::OnDimTimer(wxTimerEvent & event) {
	std::cout << "TIMER" << std::endl;

	_ASSERT(m_varActive != NULL);

	long lDimSize = m_varActive->get_dim(m_lAnimatedDim)->size();
	if (m_lVarActiveDims[m_lAnimatedDim] == lDimSize-1) {
		m_lVarActiveDims[m_lAnimatedDim] = 0;
	} else {
		m_lVarActiveDims[m_lAnimatedDim]++;
	}

	SetDisplayedDimensionValue(m_lAnimatedDim, m_lVarActiveDims[m_lAnimatedDim]);

	LoadData();

	m_imagepanel->GenerateImageFromImageMap(true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::StartAnimation(long d) {
	StopAnimation();

	_ASSERT((d >= 0) && (d < NcVarMaximumDimensions));
	_ASSERT(m_vecwxPlayButton[d] != NULL);
	m_lAnimatedDim = d;
	m_wxDimTimer.Start(100);
	m_vecwxPlayButton[m_lAnimatedDim]->SetLabelMarkup(wxString::Format("<b>%lc</b>",(wchar_t)(8545)));
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::StopAnimation() {
	if (m_lAnimatedDim != (-1)) {
		if (m_vecwxPlayButton[m_lAnimatedDim] != NULL) {
			m_vecwxPlayButton[m_lAnimatedDim]->SetLabel(wxString::Format("%lc",(wchar_t)(0x25B6)));
			m_wxDimTimer.Stop();
			m_lAnimatedDim = (-1);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnDimButtonClicked(wxCommandEvent & event) {
	std::cout << "DIM BUTTON CLICKED" << std::endl;

	_ASSERT(m_varActive != NULL);

	enum {
		DIMCOMMAND_DECREMENT,
		DIMCOMMAND_INCREMENT,
		DIMCOMMAND_RESET,
		DIMCOMMAND_PLAY
	} eDimCommand;

	bool fResetBounds = false;
	long d = static_cast<long>(event.GetId());

	// Decrement dimension
	if ((d >= ID_DIMDOWN) && (d < ID_DIMDOWN + 100)) {
		eDimCommand = DIMCOMMAND_DECREMENT;
		d -= ID_DIMDOWN;

		long lDimSize = m_varActive->get_dim(d)->size();
		if (m_lVarActiveDims[d] == 0) {
			m_lVarActiveDims[d] = lDimSize-1;
		} else {
			m_lVarActiveDims[d]--;
		}

		SetDisplayedDimensionValue(d, m_lVarActiveDims[d]);

	// Increment dimension
	} else if ((d >= ID_DIMUP) && (d < ID_DIMUP + 100)) {
		eDimCommand = DIMCOMMAND_INCREMENT;
		d -= ID_DIMUP;

		long lDimSize = m_varActive->get_dim(d)->size();
		if (m_lVarActiveDims[d] == lDimSize-1) {
			m_lVarActiveDims[d] = 0;
		} else {
			m_lVarActiveDims[d]++;
		}

		SetDisplayedDimensionValue(d, m_lVarActiveDims[d]);

	// Reset dimension
	} else if ((d >= ID_DIMRESET) && (d < ID_DIMRESET + 100)) {
		eDimCommand = DIMCOMMAND_RESET;
		d -= ID_DIMRESET;

		if ((d == m_lDisplayedDims[0]) || (d == m_lDisplayedDims[1])) {
			ResetBounds();
		} else {
			m_lVarActiveDims[d] = 0;
			SetDisplayedDimensionValue(d, m_lVarActiveDims[d]);
		}

	// Edit dimension
	} else if ((d >= ID_DIMEDIT) && (d < ID_DIMEDIT + 100)) {
		d -= ID_DIMEDIT;

		std::string strDimValue = m_vecwxDimIndex[d]->GetValue().ToStdString();
		if ((strDimValue == "") || (STLStringHelper::IsInteger(strDimValue))) {
			m_lVarActiveDims[d] = std::stoi(strDimValue);
			if (m_lVarActiveDims[d] < 0) {
				m_lVarActiveDims[d] = 0;
			} else if (m_lVarActiveDims[d] >= m_varActive->get_dim(d)->size()) {
				m_lVarActiveDims[d] = m_varActive->get_dim(d)->size()-1;
			}
		}

		SetDisplayedDimensionValue(d, m_lVarActiveDims[d]);

	// Play dimension
	} else if ((d >= ID_DIMPLAY) && (d < ID_DIMPLAY + 100)) {
		d -= ID_DIMPLAY;

		if (d != m_lAnimatedDim) {
			StartAnimation(d);
		} else {
			StopAnimation();
		}

	} else {
		_EXCEPTION();
	}

	if ((d < 0) || (d >= m_lVarActiveDims.size())) {
		_EXCEPTION();
	}

	LoadData();

	m_imagepanel->GenerateImageFromImageMap(true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnAxesButtonClicked(wxCommandEvent & event) {
	std::cout << "AXES BUTTON CLICKED" << std::endl;

	enum {
		AXESCOMMAND_X,
		AXESCOMMAND_Y,
		AXESCOMMAND_XY
	} eAxesCommand;

	// Turn off animation if active
	StopAnimation();

	// Adjust axes
	int iResetDim = (-1);
	long d = static_cast<long>(event.GetId());
	if ((d >= ID_AXESX) && (d < ID_AXESX + 100)) {
		eAxesCommand = AXESCOMMAND_X;
		d -= ID_AXESX;
		if (m_lDisplayedDims[1] == d) {
			return;
		}
		if (m_lDisplayedDims[0] == d) {
			m_lDisplayedDims[0] = m_lDisplayedDims[1];
		} else {
			iResetDim = 1;
		}
		m_lDisplayedDims[1] = d;
		m_lVarActiveDims[d] = 0;

		if (m_lDisplayedDims[0] != (-1)) {
			if (m_strUnstructDimName == m_varActive->get_dim(m_lDisplayedDims[0])->name()) {
				for (long d = m_varActive->num_dims()-1; d >= 0; d--) {
					if ((d != m_lDisplayedDims[1]) && (m_strUnstructDimName != m_varActive->get_dim(d)->name())) {
						m_lDisplayedDims[0] = d;
						break;
					}
				}
				iResetDim = (-1);
			}
		}

	} else if ((d >= ID_AXESY) && (d < ID_AXESY + 100)) {
		eAxesCommand = AXESCOMMAND_Y;
		d -= ID_AXESY;
		if (m_lDisplayedDims[0] == d) {
			return;
		}
		if (m_lDisplayedDims[1] == d) {
			m_lDisplayedDims[1] = m_lDisplayedDims[0];
		} else {
			iResetDim = 0;
		}
		m_lDisplayedDims[0] = d;
		m_lVarActiveDims[d] = 0;

		if (m_lDisplayedDims[1] != (-1)) {
			if (m_strUnstructDimName == m_varActive->get_dim(m_lDisplayedDims[1])->name()) {
				for (long d = m_varActive->num_dims()-1; d >= 0; d--) {
					if ((d != m_lDisplayedDims[0]) && (m_strUnstructDimName != m_varActive->get_dim(d)->name())) {
						m_lDisplayedDims[1] = d;
						break;
					}
				}
				iResetDim = (-1);
			}
		}

	} else if ((d >= ID_AXESXY) && (d < ID_AXESXY + 100)) {
		eAxesCommand = AXESCOMMAND_XY;
		d -= ID_AXESXY;
		if (m_lDisplayedDims[0] == d) {
			return;
		}
		if (m_lDisplayedDims[0] != (-1)) {
			m_lVarActiveDims[m_lDisplayedDims[0]] = 0;
		}
		if (m_lDisplayedDims[1] != (-1)) {
			m_lVarActiveDims[m_lDisplayedDims[1]] = 0;
		}
		m_lDisplayedDims[0] = d;
		m_lDisplayedDims[1] = (-1);

	} else {
		_EXCEPTION();
	}

	// Reset bounds
	ResetBounds(iResetDim);

	// Redraw data
	LoadData();

	GenerateDimensionControls();
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnColorMapCombo(wxCommandEvent & event) {
	std::cout << "COLORMAP COMBO" << std::endl;

	if (m_imagepanel == NULL) {
		return;
	}

	std::string strValue = event.GetString().ToStdString();

	m_imagepanel->SetColorMap(strValue, true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnGridLinesCombo(wxCommandEvent & event) {
	std::cout << "GRID COMBO" << std::endl;

	if (m_imagepanel == NULL) {
		return;
	}

	std::string strValue = event.GetString().ToStdString();

	if (strValue == "Grid Off") {
		m_imagepanel->SetGridLinesOn(false, true);
	} else {
		m_imagepanel->SetGridLinesOn(true, true);
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnOverlaysCombo(wxCommandEvent & event) {
	std::cout << "OVERLAYS COMBO" << std::endl;

	if (m_imagepanel == NULL) {
		return;
	}

	SHPFileData & overlaydata = m_imagepanel->GetOverlayDataRef();

	std::string strValue = event.GetString().ToStdString();

	if (strValue == "Overlays Off") {
		overlaydata.clear();
	} else {
		wxFileName wxfileOverlayPath(m_strNcVisResourceDir, strValue);

		ReadShpFile(wxfileOverlayPath.GetFullPath().ToStdString(), overlaydata, false);
	}

	m_imagepanel->GenerateImageFromImageMap(true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnSamplerCombo(wxCommandEvent & event) {
	std::cout << "SAMPLER COMBO" << std::endl;

	int iSamplerSelection = event.GetSelection();

	_ASSERT((iSamplerSelection >= GridDataSamplerOption_First) && (iSamplerSelection <= GridDataSamplerOption_Last));

	if ((GridDataSamplerOption)(iSamplerSelection) == m_egdsoption) {
		return;
	}

	m_egdsoption = (GridDataSamplerOption)(iSamplerSelection);

	if ((m_egdsoption == GridDataSamplerOption_QuadTree) && (!m_gdsqt.IsInitialized())) {
		InitializeGridDataSampler();
	}
	if ((m_egdsoption == GridDataSamplerOption_CubedSphereQuadTree) && (!m_gdscsqt.IsInitialized())) {
		InitializeGridDataSampler();
	}
	if ((m_egdsoption == GridDataSamplerOption_KDTree) && (!m_gdskd.IsInitialized())) {
		InitializeGridDataSampler();
	}

	m_imagepanel->ResampleData(true);
}

////////////////////////////////////////////////////////////////////////////////


