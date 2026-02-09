///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisFrame.cpp
///	\author  Paul Ullrich
///	\version August 25, 2022
///

#include "wxNcVisFrame.h"
#include <wx/filename.h>
#include <wx/dir.h>

#include "wxNcVisOptionsDialog.h"
#include "wxNcVisExportDialog.h"
#include "STLStringHelper.h"
#include "ShpFile.h"
#include "TimeObj.h"
#include <set>
#include <limits>

////////////////////////////////////////////////////////////////////////////////

static const char * szVersion = "NcVis 2024.01.26";

static const char * szDevInfo = "Supported by the U.S. Department of Energy Office of Science Regional and Global Model Analysis (RGMA) Project Simplifying ESM Analysis Through Standards (SEATS)";

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
	ID_EXPORT = 13,
	ID_COLORMAPINVERT = 14,
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
	EVT_BUTTON(ID_EXPORT, wxNcVisFrame::OnExportClicked)
	EVT_BUTTON(ID_OPTIONS, wxNcVisFrame::OnOptionsClicked)
	EVT_TEXT_ENTER(ID_BOUNDS, wxNcVisFrame::OnBoundsChanged)
	EVT_TEXT_ENTER(ID_RANGEMIN, wxNcVisFrame::OnRangeChanged)
	EVT_TEXT_ENTER(ID_RANGEMAX, wxNcVisFrame::OnRangeChanged)
	EVT_BUTTON(ID_RANGERESETMINMAX, wxNcVisFrame::OnRangeResetMinMax)
	EVT_BUTTON(ID_COLORMAPINVERT, wxNcVisFrame::OnColorMapInvertClicked)
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
	const wxString & wxstrNcVisResourceDir,
	const std::map<wxString, wxString> & mapOptions,
	const std::vector<wxString> & vecFilenames
) :
	wxFrame(NULL, wxID_ANY, title, pos, size),
	m_fVerbose(false),
	m_wxstrNcVisResourceDir(wxstrNcVisResourceDir),
	m_mapOptions(mapOptions),
	m_fRegional(false),
	m_dMaxCellRadius(0.0),
	m_colormaplib(wxstrNcVisResourceDir),
	m_egdsoption(GridDataSamplerOption_QuadTree),
	m_wxDataTransButton(NULL),
	m_panelsizer(NULL),
	m_ctrlsizer(NULL),
	m_rightsizer(NULL),
	m_vardimsizer(NULL),
	m_imagepanel(NULL),
	m_wxNcVisExportDialog(NULL),
	m_wxDimTimer(this,ID_DIMTIMER),
	m_varActive(NULL),
	m_fIsVarActiveUnstructured(false),
	m_lAnimatedDim(-1),
	m_sColorMap(0),
	m_fDataHasMissingValue(false)
{
	std::cout << szVersion << " Paul A. Ullrich" << std::endl;

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

	m_data.resize(1);
	m_data[0] = 0.0;

	if (m_colormaplib.GetColorMapCount() == 0) {
		_EXCEPTIONT("FATAL ERROR: At least one colormap must be specified");
	}

	if (mapOptions.find("-v") != mapOptions.end()) {
		m_fVerbose = true;
	}
	if (mapOptions.find("-r") != mapOptions.end()) {
		m_fRegional = true;
	}

	auto itMCS = mapOptions.find("-mcr");
	if (itMCS != mapOptions.end()) {
		m_dMaxCellRadius = stof(itMCS->second.ToStdString());
		if (m_dMaxCellRadius < 0.0) {
			_EXCEPTIONT("Maximum cell radius (-mcr) must be nonnegative");
		}
	}

	auto itUXC = mapOptions.find("-uxc");
	auto itUYC = mapOptions.find("-uyc");

	if (itUXC != mapOptions.end()) {
		m_strLonVarNameOverride = itUXC->second.ToStdString();
	}
	if (itUYC != mapOptions.end()) {
		m_strLatVarNameOverride = itUYC->second.ToStdString();
	}

	OpenFiles(vecFilenames);

	InitializeWindow();
}

////////////////////////////////////////////////////////////////////////////////

bool wxNcVisFrame::GetLonLatVariableNameIter(
	VariableNameFileIxMap::const_iterator & itLon,
	VariableNameFileIxMap::const_iterator & itLat
) const {

	if ((m_strLonVarName != "") && (m_strLatVarName != "")) {
		itLon = m_mapVarNames[1].find(m_strLonVarName);
		itLat = m_mapVarNames[1].find(m_strLatVarName);
		if ((itLon == m_mapVarNames[1].end()) || (itLat == m_mapVarNames[1].end())) {
			return false;
		}
		return true;

	} else {
		itLon = m_mapVarNames[1].end();
		itLat = m_mapVarNames[1].end();
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool wxNcVisFrame::GetLonLatDimDataIter(
	DimDataMap::const_iterator & itLon,
	DimDataMap::const_iterator & itLat
) const {

	if ((m_strLonVarName != "") && (m_strLatVarName != "")) {
		itLon = m_mapDimData.find(m_strLonVarName);
		itLat = m_mapDimData.find(m_strLatVarName);
		return true;

	} else {
		itLon = m_mapDimData.end();
		itLat = m_mapDimData.end();
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::InitializeGridDataSampler() {

	NcError error(NcError::silent_nonfatal);

	std::vector<double> dLon;
	std::vector<double> dLat;

	double dFillValue = std::numeric_limits<double>::max();

	// Get the latitude and longitude variables
	if (m_strVarActiveMultidimLon == "") {
		VariableNameFileIxMap::const_iterator itLon;
		VariableNameFileIxMap::const_iterator itLat;
		bool fSuccess = GetLonLatVariableNameIter(itLon, itLat);
		if (!fSuccess) {
			return;
		}

		std::string strLonName(itLon->first);
		std::string strLatName(itLat->first);

		// Check if lat and lon are the same length
		NcVar * varLon = m_vecpncfiles[itLon->second[0]]->get_var(strLonName.c_str());
		_ASSERT(varLon != NULL);
		NcVar * varLat = m_vecpncfiles[itLat->second[0]]->get_var(strLatName.c_str());
		_ASSERT(varLat != NULL);

		if (varLon->get_dim(0)->size() != varLat->get_dim(0)->size()) {
			return;
		}

		// At this point we can assume that the mesh is unstructured
		dLon.resize(varLon->get_dim(0)->size());
		dLat.resize(varLat->get_dim(0)->size());

		varLon->get(&(dLon[0]), varLon->get_dim(0)->size());
		varLat->get(&(dLat[0]), varLat->get_dim(0)->size());

		NcAtt * attFillValue = varLon->get_att("_FillValue");
		if (attFillValue != NULL) {
			dFillValue = attFillValue->as_double(0);
		}

	// Multidimensional latitude and longitude already specified
	} else {
		_ASSERT(m_strVarActiveMultidimLat != "");
		_ASSERT(m_varActive != NULL);

		int nDims = m_varActive->num_dims();
		VariableNameFileIxMap::const_iterator itLon =
			m_mapVarNames[nDims].find(m_strVarActiveMultidimLon);
		VariableNameFileIxMap::const_iterator itLat =
			m_mapVarNames[nDims].find(m_strVarActiveMultidimLat);

		_ASSERT(itLon != m_mapVarNames[nDims].end());
		_ASSERT(itLat != m_mapVarNames[nDims].end());

		NcVar * varLon = m_vecpncfiles[itLon->second[0]]->get_var(m_strVarActiveMultidimLon.c_str());
		_ASSERT(varLon != NULL);
		NcVar * varLat = m_vecpncfiles[itLat->second[0]]->get_var(m_strVarActiveMultidimLat.c_str());
		_ASSERT(varLat != NULL);

		NcAtt * attFillValue = varLon->get_att("_FillValue");
		if (attFillValue != NULL) {
			dFillValue = attFillValue->as_double(0);
		}

		_ASSERT(varLon->num_dims() == m_varActive->num_dims());
		_ASSERT(varLat->num_dims() == m_varActive->num_dims());

		_ASSERT(m_lDisplayedDims[0] >= 0);
		_ASSERT(m_lDisplayedDims[0] < m_varActive->num_dims());

		dLon.resize(varLon->get_dim(m_lDisplayedDims[0])->size());
		dLat.resize(varLat->get_dim(m_lDisplayedDims[0])->size());

		std::vector<long> vecSize(varLon->num_dims(), 1);
		vecSize[m_lDisplayedDims[0]] = varLon->get_dim(m_lDisplayedDims[0])->size();

		varLon->set_cur(&(m_lVarActiveDims[0]));
		varLat->set_cur(&(m_lVarActiveDims[0]));
		if (m_lDisplayedDims[0] == varLon->num_dims()-1) {
			varLon->get(&(dLon[0]), &(vecSize[0]));
			varLat->get(&(dLat[0]), &(vecSize[0]));

		} else {
			std::vector<long> vecStride(varLon->num_dims(), 1);
			for (long d = m_lDisplayedDims[0]+1; d < varLon->num_dims(); d++) {
				vecStride[d] = varLon->get_dim(d)->size();
			}

			varLon->gets(&(dLon[0]), &(vecSize[0]), &(vecStride[0]));
			varLat->gets(&(dLon[0]), &(vecSize[0]), &(vecStride[0]));
		}
	}

	// Initialize the GridDataSampler
	{
		wxStopWatch sw;

		m_dgdsLonBounds[0] = std::numeric_limits<double>::max();
		m_dgdsLonBounds[1] = -std::numeric_limits<double>::max();
		m_dgdsLatBounds[0] = std::numeric_limits<double>::max();
		m_dgdsLatBounds[1] = -std::numeric_limits<double>::max();
		for (size_t i = 0; i < dLon.size(); i++) {
			if ((dLon[i] == dFillValue) || (std::isnan(dLon[i]))) {
				continue;
			}
			if ((dLat[i] == dFillValue) || (std::isnan(dLat[i]))) {
				continue;
			}

			if (dLon[i] < m_dgdsLonBounds[0]) {
				m_dgdsLonBounds[0] = dLon[i];
			}
			if (dLon[i] > m_dgdsLonBounds[1]) {
				m_dgdsLonBounds[1] = dLon[i];
			}
			if (dLat[i] < m_dgdsLatBounds[0]) {
				m_dgdsLatBounds[0] = dLat[i];
			}
			if (dLat[i] > m_dgdsLatBounds[1]) {
				m_dgdsLatBounds[1] = dLat[i];
			}
		}

		if (!m_fRegional) {
			if (fabs(m_dgdsLonBounds[1] - m_dgdsLonBounds[0] - 360.0) < 1.0) {
				if (fabs(m_dgdsLonBounds[0]) < 1.0) {
					m_dgdsLonBounds[0] = 0.0;
				}
				if (fabs(m_dgdsLonBounds[0] + 180.0) < 1.0) {
					m_dgdsLonBounds[0] = -180.0;
				}
				m_dgdsLonBounds[1] = m_dgdsLonBounds[0] + 360.0;
			}
			if ((fabs(m_dgdsLatBounds[0] + 90.0) < 1.0) && (fabs(m_dgdsLatBounds[1] - 90.0) < 1.0)) {
				m_dgdsLatBounds[0] = -90.0;
				m_dgdsLatBounds[1] = 90.0;
			}
			if (fabs(m_dgdsLonBounds[1] - m_dgdsLonBounds[0] - 2.0 * M_PI) < 0.1) {
				if (fabs(m_dgdsLonBounds[0]) < 0.1) {
					m_dgdsLonBounds[0] = 0.0;
				}
				if (fabs(m_dgdsLonBounds[0] + M_PI) < 0.1) {
					m_dgdsLonBounds[0] = - M_PI;
				}
				m_dgdsLonBounds[1] = m_dgdsLonBounds[0] + 2.0 * M_PI;
			}
			if ((fabs(m_dgdsLatBounds[0] + 0.5 * M_PI) < 0.1) && (fabs(m_dgdsLatBounds[1] - 0.5 * M_PI) < 0.1)) {
				m_dgdsLatBounds[0] = - 0.5 * M_PI;
				m_dgdsLatBounds[1] = 0.5 * M_PI;
			}
		}

		if (m_egdsoption == GridDataSamplerOption_QuadTree) {
			if (m_fRegional) {
				//std::cout << m_dgdsLonBounds[0] << " " << m_dgdsLonBounds[1] << " " << m_dgdsLatBounds[0] << " " << m_dgdsLatBounds[1] << std::endl;
				m_gdsqt.SetRegionalBounds(
					m_dgdsLonBounds[0],
					m_dgdsLonBounds[1],
					m_dgdsLatBounds[0],
					m_dgdsLatBounds[1]);
			}

			m_gdsqt.Initialize(dLon, dLat, dFillValue, m_dMaxCellRadius);
		}
		if (m_egdsoption == GridDataSamplerOption_CubedSphereQuadTree) {
			m_gdscsqt.Initialize(dLon, dLat, dFillValue, m_dMaxCellRadius);
		}
		if (m_egdsoption == GridDataSamplerOption_KDTree) {
			m_gdskd.Initialize(dLon, dLat, dFillValue);
		}
		Announce("Initializing the GridDataSampler took %ldms", sw.Time());
	}

	// Allocate data space
	if (m_data.size() != dLon.size()) {
		m_data.resize(dLon.size());
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OpenFiles(
	const std::vector<wxString> & vecFilenames
) {
	_ASSERT(m_vecpncfiles.size() == 0);

	NcError error(NcError::silent_nonfatal);

	m_vecFilenames = vecFilenames;

	// Standard longitude and latitude names
	const std::string strStandardLonName("longitude");
	const std::string strStandardLatName("latitude");

	std::vector<std::string> vecCommonLonVarNames;
	vecCommonLonVarNames.push_back("lon");
	vecCommonLonVarNames.push_back("longitude");
	vecCommonLonVarNames.push_back("lonCell");
	vecCommonLonVarNames.push_back("mesh_node_x");

	std::vector<std::string> vecCommonLatVarNames;
	vecCommonLatVarNames.push_back("lat");
	vecCommonLatVarNames.push_back("latitude");
	vecCommonLatVarNames.push_back("latCell");
	vecCommonLatVarNames.push_back("mesh_node_y");

	// Enumerate all variables, recording dimension variables
	for (size_t f = 0; f < vecFilenames.size(); f++) {
		NcFile * pfile = new NcFile(vecFilenames[f].c_str());
		if (!pfile->is_valid()) {
			std::cout << "ERROR: Unable to open file \"" << vecFilenames[f] << "\"" << std::endl;
			exit(-1);
		}
		m_vecpncfiles.push_back(pfile);

		for (long v = 0; v < pfile->num_vars(); v++) {
			NcVar * var = pfile->get_var(v);
			size_t sVarDims = static_cast<size_t>(var->num_dims());
			if (var->num_dims() >= NcVarMaximumDimensions) {
				std::cout << "ERROR: Only variables of dimension <= " << NcVarMaximumDimensions << " supported" << std::endl;
				exit(-1);
			}

			for (long d = 0; d < var->num_dims(); d++) {
				auto prDimInsert =
					m_mapDimData.insert(
						DimDataMap::value_type(
							var->get_dim(d)->name(),
							DimDataFileIdAndCoordMap()));
			}

			// Check for longitude/latitude attribute
			NcAtt * attStandardName = var->get_att("standard_name");
			NcAtt * attLongName = var->get_att("long_name");

			// Check for override of both lon and lat
			if ((sVarDims == 1) && (m_strLonVarNameOverride != "") && (m_strLatVarNameOverride != "")) {
				bool fIsDimOverrideVar = false;
				if (m_strLonVarNameOverride == var->name()) {
					m_strLonVarName = m_strLonVarNameOverride;
					fIsDimOverrideVar = true;
				}
				if (m_strLatVarNameOverride == var->name()) {
					m_strLatVarName = m_strLatVarNameOverride;
					fIsDimOverrideVar = true;
				}
				if (fIsDimOverrideVar) {
					if (m_strDefaultUnstructDimName == "") {
						m_strDefaultUnstructDimName = var->get_dim(0)->name();
					} else if (m_strDefaultUnstructDimName != var->get_dim(0)->name()) {
						_EXCEPTIONT("When using -uxc and -uyc, both variables must have same dimensions");
					}
				}

			// Check if this variable is longitude or latitude
			} else if (sVarDims == 1) {

				if (m_strLonVarNameOverride == var->name()) {
					m_strLonVarName = m_strLonVarNameOverride;
				}
				if (m_strLonVarName == "") {
					for (int i = 0; i < vecCommonLonVarNames.size(); i++) {
						if (vecCommonLonVarNames[i] == var->name()) {
							m_strLonVarName = var->name();
							break;
						}
					}
				}
				if (m_strLonVarName == "") {
					if (attStandardName != NULL) {
						if (strStandardLonName == attStandardName->as_string(0)) {
							m_strLonVarName = var->name();
						}
					} else {
						if (attLongName != NULL) {
							if (strStandardLonName == attLongName->as_string(0)) {
								m_strLonVarName = var->name();
							}
						}
					}
				}
				if (m_strLatVarNameOverride == var->name()) {
					m_strLatVarName = m_strLatVarNameOverride;
				}
				if (m_strLatVarName == "") {
					for (int i = 0; i < vecCommonLatVarNames.size(); i++) {
						if (vecCommonLatVarNames[i] == var->name()) {
							m_strLatVarName = var->name();
							break;
						}
					}
				}
				if (m_strLatVarName == "") {
					if (attStandardName != NULL) {
						if (strStandardLatName == attStandardName->as_string(0)) {
							m_strLatVarName = var->name();
						}
					} else {
						if (attLongName != NULL) {
							if (strStandardLatName == attLongName->as_string(0)) {
								m_strLatVarName = var->name();
							}
						}
					}
				}

				if ((m_strLonVarName == var->name()) || (m_strLatVarName == var->name())) {
					if (m_strDefaultUnstructDimName == "") {
						m_strDefaultUnstructDimName = var->get_dim(0)->name();
					} else if (m_strDefaultUnstructDimName != var->get_dim(0)->name()) {
						m_strDefaultUnstructDimName = "-";
					}
				}

			// Check for multidimensional longitudes/latitudes
			} else {
				bool fMultidimLon = false;
				bool fMultidimLat = false;
				if (m_strLonVarNameOverride == var->name()) {
					fMultidimLon = true;
				}
				if (m_strLatVarNameOverride == var->name()) {
					fMultidimLat = true;
				}

				if (attStandardName != NULL) {
					if (strStandardLonName == attStandardName->as_string(0)) {
						fMultidimLon = true;
					}
					if (strStandardLatName == attStandardName->as_string(0)) {
						fMultidimLat = true;
					}
				}
				if (attLongName != NULL) {
					if (strStandardLonName == attLongName->as_string(0)) {
						fMultidimLon = true;
					}
					if (strStandardLatName == attLongName->as_string(0)) {
						fMultidimLat = true;
					}
				}
				std::string strDims;
				if ((fMultidimLon) || (fMultidimLat)) {
					for (int d = 0; d < var->num_dims(); d++) {
						strDims += var->get_dim(d)->name();
						if (d != var->num_dims()-1) {
							strDims += ", ";
						}
					}
				}
				if (fMultidimLon) {
					std::cout << "Multidim lon: (" << strDims << ") " << var->name() << std::endl;
					m_mapMultidimLonVars.insert(
						std::pair<std::string, std::string>(
							strDims, var->name()));
				}
				if (fMultidimLat) {
					std::cout << "Multidim lat: (" << strDims << ") " << var->name() << std::endl;
					m_mapMultidimLatVars.insert(
						std::pair<std::string, std::string>(
							strDims, var->name()));
				}
			}

			// Insert variable into map
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
					std::cout << "WARNING: NetCDF fileset contains a dimension variable \"" << itVarDim->first
						<< "\" which has dimension different than 1" << std::endl;
					continue;
				}

				if (m_fVerbose) {
					Announce("Dimension variable \"%s\" in file %lu (%li values)",
						itVarDim->first.c_str(), f, varDim->get_dim(0)->size());
				}
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
						std::cout << "WARNING: NetCDF fileset contains a dimension variable \""
							<< itVarDim->first << "\" that is non-monotone" << std::endl;
					}
				}
			}
		}
	}

	// Remove dimension variables from the variable name map
	for (auto it = m_mapDimData.begin(); it != m_mapDimData.end(); it++) {
		m_mapVarNames[1].erase(it->first);
	}

	// Assuming a default unstructured dim name has been identified, set it as the unstructured dim
	if ((m_strLonVarName == "") || (m_strLatVarName == "")) {
		m_strDefaultUnstructDimName = "-";
	}
	if ((m_strDefaultUnstructDimName == "-") || (m_strDefaultUnstructDimName == "")) {
		return;
	} else {
		m_strUnstructDimName = m_strDefaultUnstructDimName;
	}

	// Check if lon and lat are dimension variables; if they are then they
	// should not be coordinates on the unstructured mesh.
	DimDataMap::const_iterator itLonDimVar;
	DimDataMap::const_iterator itLatDimVar;

	GetLonLatDimDataIter(itLonDimVar, itLatDimVar);

	if ((itLonDimVar == m_mapDimData.end()) && (itLatDimVar != m_mapDimData.end())) {
		std::cout << "ERROR: In input file \"" << itLatDimVar->first << "\" is a dimension variable but \"lon\" is not" << std::endl;
		exit(-1);
	}
	if ((itLonDimVar != m_mapDimData.end()) && (itLatDimVar == m_mapDimData.end())) {
		std::cout << "ERROR: In input file \"" << itLonDimVar->first << "\" is a dimension variable but \"lat\" is not" << std::endl;
		exit(-1);
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

	// Get the list of shapefiles in the resource dir
	{
		wxDir dirResources(m_wxstrNcVisResourceDir);
		if (!dirResources.IsOpened()) {
			std::cout << "ERROR: Cannot open resource directory \"" << m_wxstrNcVisResourceDir << "\". Resources will not be populated." << std::endl;
			exit(-1);
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
	m_rightsizer->SetMinSize(660,220);

	m_ctrlsizer->Add(menusizer, 0);
	m_ctrlsizer->Add(m_rightsizer, 0, wxEXPAND);

	// Data transform button (also reference widget height)
	m_wxDataTransButton = new wxButton(this, ID_DATATRANS, _T("Linear"));

	// Color map combobox and invert button
	wxBoxSizer * wxColorMapSizer = new wxBoxSizer(wxHORIZONTAL);

	// Color map invert button
	m_wxInvertColorMapButton = new wxButton(this, ID_COLORMAPINVERT, wxString::Format("%lc",(0x25B2)), wxDefaultPosition, wxSize(20,m_wxDataTransButton->GetSize().GetHeight()));
	wxColorMapSizer->Add(m_wxInvertColorMapButton);

	// Color map combobox
	wxComboBox * wxColorMapCombo = new wxComboBox(this, ID_COLORMAP, _T(""), wxDefaultPosition, wxSize(120,m_wxDataTransButton->GetSize().GetHeight()));
	for (size_t c = 0; c < m_colormaplib.GetColorMapCount(); c++) {
		wxColorMapCombo->Append(wxString(m_colormaplib.GetColorMapName(c)));
	}
	wxColorMapCombo->SetSelection(0);
	wxColorMapCombo->SetEditable(false);
	wxColorMapSizer->Add(wxColorMapCombo);
/*
	// Grid lines combobox
	wxComboBox * wxGridLinesCombo = new wxComboBox(this, ID_GRIDLINES, _T(""), wxDefaultPosition, wxSize(140,m_wxDataTransButton->GetSize().GetHeight()));
	wxGridLinesCombo->Append(_T("Grid Off"));
	wxGridLinesCombo->Append(_T("Grid On"));
	wxGridLinesCombo->SetSelection(0);
	wxGridLinesCombo->SetEditable(false);
*/
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

	// Options button
	wxButton * wxOptionsButton = new wxButton(this, ID_OPTIONS, _T("Options"));

	// Export button
	m_wxExportButton = new wxButton(this, ID_EXPORT, _T("Export..."));
	m_wxExportButton->Enable(false);

	// Add controls to the manusizer
	menusizer->Add(wxColorMapSizer, 0, wxEXPAND | wxALL, 2);
	menusizer->Add(m_wxDataTransButton, 0, wxEXPAND | wxALL, 2);
	//menusizer->Add(wxGridLinesCombo, 0, wxEXPAND | wxALL, 2);
	menusizer->Add(wxOverlaysCombo, 0, wxEXPAND | wxALL, 2);
	menusizer->Add(wxSamplerCombo, 0, wxEXPAND | wxALL, 2);
	menusizer->Add(wxOptionsButton, 0, wxEXPAND | wxALL, 2);
	menusizer->Add(m_wxExportButton, 0, wxEXPAND | wxALL, 2);

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

	// Custom variable selector
	//wxBoxSizer *customvarsizer = new wxBoxSizer(wxHORIZONTAL);

	//customvarsizer->Add(new wxStaticText(this, (-1), _T("Custom: "), wxDefaultPosition, wxSize(20,m_wxDataTransButton->GetSize().GetHeight()+4), wxST_ELLIPSIZE_END | wxALIGN_CENTRE_HORIZONTAL | wxALIGN_CENTER_VERTICAL), 1, wxEXPAND | wxALL, 4);
	//customvarsizer->Add(new wxTextCtrl(this, (-1), _T(""), wxDefaultPosition, wxSize(240,m_wxDataTransButton->GetSize().GetHeight()+4), wxTE_PROCESS_ENTER), 2, wxEXPAND | wxALL, 4);


	// Dimensions
	m_vardimsizer = new wxFlexGridSizer(NcVarMaximumDimensions+1, 4, 0, 0);

	// Image panel
	m_imagepanel = new wxImagePanel(this);

	m_imagepanel->SetColorMap(m_colormaplib.GetColorMapName(0));

	m_rightsizer->Add(varsizer, 0, wxALIGN_CENTER, 0);
	//m_rightsizer->Add(customvarsizer, 0, wxALIGN_CENTER, 0);
	m_rightsizer->Add(m_vardimsizer, 0, wxALIGN_CENTER, 0);

	m_panelsizer->Add(m_imagepanel, 1, wxALIGN_TOP | wxALIGN_CENTER | wxSHAPED);
	m_panelsizer->Add(m_ctrlsizer, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER);

	CreateStatusBar();

	// Status bar
	SetStatusMessage(_T(""), true);

	SetSizerAndFit(m_panelsizer);

	// Set selection
	size_t sTotalVariables = 0;
	for (int vc = 0; vc < NcVarMaximumDimensions; vc++) {
		sTotalVariables += m_mapVarNames[vc].size();
	}

	if (sTotalVariables == 1) {
		for (int vc = 0; vc < NcVarMaximumDimensions; vc++) {
			if (m_mapVarNames[vc].size() == 1) {
				m_vecwxVarSelector[vc]->SetSelection(0);

				wxCommandEvent event(wxEVT_NULL, ID_VARSELECTOR + vc);
				event.SetString(wxString(m_mapVarNames[vc].begin()->first));

				OnVariableSelected(event);
				break;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::LoadData() {
	if (m_fVerbose) {
		std::cout << "LOAD DATA" << std::endl;
	}

	// Assume data is not unstructured
	m_fIsVarActiveUnstructured = false;

	// 0D data
	if (m_varActive->num_dims() == 0) {
		m_data.resize(1);
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

		if (m_data.size() != vecSize[m_lDisplayedDims[0]]) {
			m_data.resize(vecSize[m_lDisplayedDims[0]]);
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

		if (m_data.size() != vecSize[m_lDisplayedDims[0]] * vecSize[m_lDisplayedDims[1]]) {
			m_data.resize(vecSize[m_lDisplayedDims[0]] * vecSize[m_lDisplayedDims[1]]);
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
	const std::vector<double> & dSample,
	long lDim,
	std::vector<int> & veccoordmap
) {
	_ASSERT(lDim < m_varActive->num_dims());

	veccoordmap.resize(dSample.size(), 0);

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
		for (size_t s = 0; s < dSample.size(); s++) {
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
		for (size_t s = 0; s < dSample.size(); s++) {
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

void wxNcVisFrame::SampleData(
	const std::vector<double> & dSampleX,
	const std::vector<double> & dSampleY,
	std::vector<int> & imagemap
) {
	if (m_fVerbose) {
		std::cout << "SAMPLE DATA " << dSampleX.size() << " " << dSampleY.size() << std::endl;
	}

	_ASSERT(imagemap.size() >= dSampleX.size() * dSampleY.size());
	_ASSERT(m_data.size() > 0);

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
		for (size_t s = 0; s < imagemap.size(); s++) {
			imagemap[s] = 0;
		}

	// One displayed variable along X axis
	} else if (m_lDisplayedDims[1] == (-1)) {
		_ASSERT((m_lDisplayedDims[0] >= 0) && (m_lDisplayedDims[0] < m_varActive->num_dims()));

		std::vector<int> veccoordmapX(dSampleX.size(), 0);

		MapSampleCoords1DFromActiveVar(dSampleX, m_lDisplayedDims[0], veccoordmapX);

		// Assemble the image map
		size_t s = 0;
		for (size_t j = 0; j < dSampleY.size(); j++) {
		for (size_t i = 0; i < dSampleX.size(); i++) {
			imagemap[s] = veccoordmapX[i];
			s++;
		}
		}

	// Two displayed variables
	} else {
		_ASSERT((m_lDisplayedDims[0] >= 0) && (m_lDisplayedDims[0] < m_varActive->num_dims()));
		_ASSERT((m_lDisplayedDims[1] >= 0) && (m_lDisplayedDims[1] < m_varActive->num_dims()));

		std::vector<int> veccoordmapX(dSampleX.size(), 0);
		std::vector<int> veccoordmapY(dSampleY.size(), 0);

		MapSampleCoords1DFromActiveVar(dSampleY, m_lDisplayedDims[0], veccoordmapY);
		MapSampleCoords1DFromActiveVar(dSampleX, m_lDisplayedDims[1], veccoordmapX);

		size_t sDimYSize = m_varActive->get_dim(m_lDisplayedDims[0])->size();
		size_t sDimXSize = m_varActive->get_dim(m_lDisplayedDims[1])->size();

		// Assemble the image map
		size_t s = 0;
		if (m_lDisplayedDims[0] < m_lDisplayedDims[1]) {
			for (size_t j = 0; j < dSampleY.size(); j++) {
			for (size_t i = 0; i < dSampleX.size(); i++) {
				imagemap[s] = veccoordmapY[j] * sDimXSize + veccoordmapX[i];
				s++;
			}
			}
		} else {
			for (size_t j = 0; j < dSampleY.size(); j++) {
			for (size_t i = 0; i < dSampleX.size(); i++) {
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
	if (m_fVerbose) {
		std::cout << "RESET BOUNDS" << std::endl;
	}

	m_fDisplayedDimPeriodic[0] = false;
	m_fDisplayedDimPeriodic[1] = false;

	m_dDisplayedDimBounds[0][0] = -90.0;
	m_dDisplayedDimBounds[0][1] = 90.0;
	m_dDisplayedDimBounds[1][0] = 0.0;
	m_dDisplayedDimBounds[1][1] = 360.0;

	double dXmin[2] = {m_dDisplayedDimBounds[0][0], m_dDisplayedDimBounds[1][0]};
	double dXmax[2] = {m_dDisplayedDimBounds[0][1], m_dDisplayedDimBounds[1][1]};

	if (m_varActive == NULL) {
		m_imagepanel->SetCoordinateRange(dXmin[1], dXmax[1], dXmin[0], dXmax[0], fRedraw);
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

			m_dDisplayedDimBounds[0][0] = m_dgdsLatBounds[0];
			m_dDisplayedDimBounds[0][1] = m_dgdsLatBounds[1];
			m_dDisplayedDimBounds[1][0] = m_dgdsLonBounds[0];
			m_dDisplayedDimBounds[1][1] = m_dgdsLonBounds[1];

			m_fDisplayedDimPeriodic[1] = true;

			m_imagepanel->SetCoordinateRange(
				m_dDisplayedDimBounds[1][0],
				m_dDisplayedDimBounds[1][1],
				m_dDisplayedDimBounds[0][0],
				m_dDisplayedDimBounds[0][1],
				fRedraw);
			return;
		}
	}

	// Determine bounds for all displayed dimensions
	for (size_t d = 0; d < 2; d++) {
		if (m_lDisplayedDims[d] == (-1)) {
			continue;
		}

		std::string strDimName(m_varActive->get_dim(m_lDisplayedDims[d])->name());

		auto itDim = m_mapDimData.find(strDimName);
		if (itDim != m_mapDimData.end()) {
			if (itDim->second.size() == 0) {
				m_dDisplayedDimBounds[d][0] = -0.5;
				m_dDisplayedDimBounds[d][1] =
					static_cast<double>(m_varActive->get_dim(m_lDisplayedDims[d])->size()-1) + 0.5;
				continue;
			}

			auto itDimCoord = itDim->second.begin();
			const std::vector<double> & coord = itDimCoord->second;
			int nc = coord.size();
			if (coord.size() == 1) {
				m_dDisplayedDimBounds[d][0] = coord[0] - 0.5;
				m_dDisplayedDimBounds[d][1] = coord[0] + 0.5;
			} else if (coord[1] > coord[0]) {
				m_dDisplayedDimBounds[d][0] = coord[0] - 0.5 * (coord[1] - coord[0]);
				m_dDisplayedDimBounds[d][1] = coord[nc-1] + 0.5 * (coord[nc-1] - coord[nc-2]);
			} else {
				m_dDisplayedDimBounds[d][0] = coord[nc-1] + 0.5 * (coord[nc-1] - coord[nc-2]);
				m_dDisplayedDimBounds[d][1] = coord[0] - 0.5 * (coord[1] - coord[0]);
			}

			// Special cases (latitude in degrees)
			if ((strDimName.find("lat") != std::string::npos) ||
			    (strDimName.find("Lat") != std::string::npos) ||
			    (strDimName.find("LAT") != std::string::npos)
			) {
				if (m_dDisplayedDimBounds[d][0] < -90.0) {
					m_dDisplayedDimBounds[d][0] = -90.0;
				}
				if (m_dDisplayedDimBounds[d][1] < -90.0) {
					m_dDisplayedDimBounds[d][1] = -90.0;
				}
				if (m_dDisplayedDimBounds[d][0] > 90.0) {
					m_dDisplayedDimBounds[d][0] = 90.0;
				}
				if (m_dDisplayedDimBounds[d][1] > 90.0) {
					m_dDisplayedDimBounds[d][1] = 90.0;
				}
			}

			// Special cases (longitude in degrees)
			if ((strDimName.find("lon") != std::string::npos) ||
			    (strDimName.find("Lon") != std::string::npos) ||
			    (strDimName.find("LON") != std::string::npos)
			) {
				//TODO: Implement periodic longitudes for 2D variables
				//m_fDisplayedDimPeriodic[d] = true;
				if (nc != 1) {
					double dXleft = 1.5 * coord[0] - 0.5 * coord[1];
					double dXright = 1.5 * coord[nc-1] - 0.5 * coord[nc-2];
					if (fabs(dXright - dXleft - 360.0) < 1.0e-5) {
						if (coord[1] > coord[0]) {
							m_dDisplayedDimBounds[d][0] = coord[0];
							m_dDisplayedDimBounds[d][1] = coord[0] + 360.0;
						} else {
							m_dDisplayedDimBounds[d][0] = coord[nc-1];
							m_dDisplayedDimBounds[d][1] = coord[nc-1] + 360.0;
						}
					}
				}
			}

		} else {
			m_dDisplayedDimBounds[d][0] = -0.5;
			m_dDisplayedDimBounds[d][1] = static_cast<double>(m_varActive->get_dim(m_lDisplayedDims[d])->size()-1) + 0.5;
		}
	}

	// Set coordinate range for specified dimensions
	size_t dmin, dmax;
	if (iDim == (-1)) {
		dmin = 0;
		dmax = 2;

		dXmin[0] = m_dDisplayedDimBounds[0][0];
		dXmax[0] = m_dDisplayedDimBounds[0][1];

		dXmin[1] = m_dDisplayedDimBounds[1][0];
		dXmax[1] = m_dDisplayedDimBounds[1][1];

	} else if (iDim == 0) {
		dmin = static_cast<size_t>(0);
		dmax = static_cast<size_t>(1);

		dXmin[0] = m_dDisplayedDimBounds[0][0];
		dXmax[0] = m_dDisplayedDimBounds[0][1];

		dXmin[1] = m_imagepanel->GetYRangeMin();
		dXmax[1] = m_imagepanel->GetYRangeMax();

	} else if (iDim == 1) {
		dmin = static_cast<size_t>(1);
		dmax = static_cast<size_t>(2);

		dXmin[0] = m_imagepanel->GetXRangeMin();
		dXmax[0] = m_imagepanel->GetXRangeMax();

		dXmin[1] = m_dDisplayedDimBounds[1][0];
		dXmax[1] = m_dDisplayedDimBounds[1][1];

	} else {
		_EXCEPTION();
	}

	if (m_varActive->num_dims() == 1) {
		m_fDisplayedDimPeriodic[1] = true;
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
	if (m_data.size() == 0) {
		return;
	}

	float dDataMin = 0.0;
	float dDataMax = 0.0;

	int i;

	if ((!m_fDataHasMissingValue) || (std::isnan(m_dMissingValueFloat))) {
		for (i = 0; i < m_data.size(); i++) {
			if (!std::isnan(m_data[i])) {
				break;
			}
		}
		if (i != m_data.size()) {
			dDataMin = m_data[i];
			dDataMax = m_data[i];
		}
		for (i++; i < m_data.size(); i++) {
			if (std::isnan(m_data[i])) {
				continue;
			}
			if (m_data[i] < dDataMin) {
				dDataMin = m_data[i];
			}
			if (m_data[i] > dDataMax) {
				dDataMax = m_data[i];
			}
		}

	} else {
		for (i = 0; i < m_data.size(); i++) {
			if ((m_data[i] != m_dMissingValueFloat) && (!std::isnan(m_data[i]))) {
				break;
			}
		}
		if (i != m_data.size()) {
			dDataMin = m_data[i];
			dDataMax = m_data[i];
		}
		for (i++; i < m_data.size(); i++) {
			if ((m_data[i] == m_dMissingValueFloat) || (std::isnan(m_data[i]))) {
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

long wxNcVisFrame::GetDisplayedDimensionValue(
	long lDim
) {
	_ASSERT(m_varActive != NULL);
	_ASSERT((lDim >= 0) && (lDim < m_varActive->num_dims()));
	wxString wxstrValue = m_vecwxDimIndex[lDim]->GetValue();

	return std::stol(wxstrValue.ToStdString());
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::SetStatusMessage(
	const wxString & strMessage,
	bool fIncludeVersion
) {
	if (fIncludeVersion) {
		wxString strMessageBak(szVersion);
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
	wxMessageBox(szDevInfo, "NetCDF Visualizer (NcVis)", wxOK | wxICON_INFORMATION);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnClose(
	wxCloseEvent & event
) {
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
	if (m_fVerbose) {
		std::cout << "OPTIONS DIALOG" << std::endl;
	}

	// Open options dialog
	m_wxNcVisOptionsDialog =
		new wxNcVisOptionsDialog(
			this,
			_T("NcVis Options"),
			wxPoint(60, 60),
			wxSize(500, 400),
			m_plotopts);

	m_wxNcVisOptionsDialog->ShowModal();

	// Check if Ok was clicked -- if so check if plot options have changed
	if (m_wxNcVisOptionsDialog->IsOkClicked()) {
		if (m_plotopts != m_wxNcVisOptionsDialog->GetPlotOptions()) {
			m_plotopts = m_wxNcVisOptionsDialog->GetPlotOptions();

			// Update the size of the image panel and size ratio
			if (m_imagepanel->ResetPanelSize()) {
				m_imagepanel->GenerateImageFromImageMap(true);
			}
			m_imagepanel->GetContainingSizer()->GetItem((size_t)(0))->SetRatio(
				m_imagepanel->GetSize().GetWidth(),
				m_imagepanel->GetSize().GetHeight());

			// Resize window if needed
			if (m_panelsizer->GetMinSize().GetHeight() != m_panelsizer->GetSize().GetHeight()) {
				SetSizerAndFit(m_panelsizer);
			}

			// Layout widgets
			m_vardimsizer->Layout();
			m_rightsizer->Layout();
			m_ctrlsizer->Layout();
			m_panelsizer->Layout();
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnExportClicked(
	wxCommandEvent & event
) {
	if (m_fVerbose) {
		std::cout << "EXPORT DIALOG" << std::endl;
	}

	if (m_varActive == NULL) {
		return;
	}

	std::vector< wxString > vecDimNames;
	std::vector< std::pair<long,long> > vecDimBounds;
	for (long d = 0; d < m_varActive->num_dims(); d++) {
		if ((d == m_lDisplayedDims[0]) || (d == m_lDisplayedDims[1])) {
			continue;
		}
		vecDimNames.push_back(wxString(m_varActive->get_dim(d)->name()));
		vecDimBounds.push_back(std::pair<long,long>(0,m_varActive->get_dim(d)->size()-1));
	}

	// Initialize export dialog
	m_wxNcVisExportDialog =
		new wxNcVisExportDialog(
			this,
			_T("NcVis Export"),
			wxPoint(60, 60),
			wxSize(500, 400),
			vecDimNames,
			vecDimBounds,
			m_imagepanel->GetImageSize().GetWidth(),
			m_imagepanel->GetImageSize().GetHeight());

	m_wxNcVisExportDialog->ShowModal();

	wxNcVisExportDialog::ExportCommand eExportCommand = m_wxNcVisExportDialog->GetExportCommand();

	bool fSuccess = true;

	// Image size for export
	size_t sImageWidth = m_wxNcVisExportDialog->GetExportWidth();
	size_t sImageHeight = m_wxNcVisExportDialog->GetExportHeight();

	// Cancel
	if (eExportCommand == wxNcVisExportDialog::ExportCommand_Cancel) {
		return;

	// Export one frame
	} else if (eExportCommand == wxNcVisExportDialog::ExportCommand_OneFrame) {
		std::cout << "Exporting frame to " << m_wxNcVisExportDialog->GetExportFilename() << std::endl;

		SetStatusMessage(_T(" Rendering Frame 1/1"), true);

		m_imagepanel->ImposeImageSize(sImageWidth, sImageHeight);

		fSuccess =
			m_imagepanel->ExportToPNG(
				m_wxNcVisExportDialog->GetExportFilename(),
				sImageWidth,
				sImageHeight);

		m_imagepanel->ResetImageSize();

		SetStatusMessage(_T(""), true);

	// Export multiple frames
	} else if (eExportCommand == wxNcVisExportDialog::ExportCommand_MultipleFrames) {

		wxString wxstrExportFilePath = m_wxNcVisExportDialog->GetExportFilePath();
		wxString wxstrExportFilePattern = m_wxNcVisExportDialog->GetExportFilePattern();

		wxString wxstrExportDimName = m_wxNcVisExportDialog->GetExportDimName();
		long lExportDimBegin = m_wxNcVisExportDialog->GetExportDimBegin();
		long lExportDimEnd = m_wxNcVisExportDialog->GetExportDimEnd();

		long lActiveDim = (-1);
		for (long d = 0; d < m_varActive->num_dims(); d++) {
			if (wxstrExportDimName == m_varActive->get_dim(d)->name()) {
				lActiveDim = d;
				break;
			}
		}
		_ASSERT(lActiveDim != (-1));

		m_imagepanel->ImposeImageSize(sImageWidth, sImageHeight);

		long lDisplayedValueBackup = GetDisplayedDimensionValue(lActiveDim);

		for (long i = lExportDimBegin; i <= lExportDimEnd; i++) {
			long ix = i - lExportDimBegin;
			SetStatusMessage(wxString::Format(" Rendering Frame %li/%li", ix, lExportDimEnd - lExportDimBegin + 1), true);

			m_lVarActiveDims[lActiveDim] = i;
			SetDisplayedDimensionValue(lActiveDim, i);
			LoadData();

			wxFileName wxfile(wxstrExportFilePath, wxString::Format(wxstrExportFilePattern, static_cast<int>(ix)));

			std::cout << "Exporting frame to " << wxfile.GetFullPath() << std::endl;

			fSuccess =
				m_imagepanel->ExportToPNG(
					wxfile.GetFullPath(),
					sImageWidth,
					sImageHeight);

			if (!fSuccess) {
				break;
			}
		}

		m_lVarActiveDims[lActiveDim] = lDisplayedValueBackup;
		SetDisplayedDimensionValue(lActiveDim, lDisplayedValueBackup);
		LoadData();

		m_imagepanel->ResetImageSize();

		SetStatusMessage(_T(""), true);
	}

	if (!fSuccess) {
		wxMessageDialog wxResultDialog(this, _T("Export failed"), _T("Export to PNG"), wxOK | wxCENTRE | wxICON_EXCLAMATION);
		wxResultDialog.ShowModal();
	}

	//std::cout << m_wxNcVisExportDialog->m_nExportWidth << std::endl;
	//m_wxNcVisExportDialog->SetFocus();
	//m_wxNcVisExportDialog->Destroy();
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
	if (m_fVerbose) {
		std::cout << "VARIABLE SELECTED" << std::endl;
	}

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

	// Check for multidimensional longitudes/latitudes
	bool fReinitializeGridDataSampler = false;
	{
		std::string strDims;
		for (int d = 0; d < m_varActive->num_dims(); d++) {
			strDims += m_varActive->get_dim(d)->name();
			if (d != m_varActive->num_dims()-1) {
				strDims += ", ";
			}
		}

		auto itMultidimLon = m_mapMultidimLonVars.find(strDims);
		auto itMultidimLat = m_mapMultidimLatVars.find(strDims);

		if ((itMultidimLon != m_mapMultidimLonVars.end()) && (itMultidimLat != m_mapMultidimLatVars.end())) {
			_ASSERT(m_varActive->num_dims() > 0);
			int nMaxDim = 0;
			int nMaxDimSize = m_varActive->get_dim(0)->size();
			for (int d = 1; d < m_varActive->num_dims(); d++) {
				if (m_varActive->get_dim(d)->size() > nMaxDimSize) {
					nMaxDimSize = m_varActive->get_dim(d)->size();
					nMaxDim = d;
				}
			}
			m_strUnstructDimName = m_varActive->get_dim(nMaxDim)->name();

			if ((m_strVarActiveMultidimLon != itMultidimLon->second) ||
			    (m_strVarActiveMultidimLat != itMultidimLat->second)
			) {
				fReinitializeGridDataSampler = true;
				m_strVarActiveMultidimLon = itMultidimLon->second;
				m_strVarActiveMultidimLat = itMultidimLat->second;
			}

			Announce("Multidimensional lon/lat found: %s %s", itMultidimLon->second.c_str(), itMultidimLat->second.c_str());
			Announce("Assumed unstructured dim: %s", m_strUnstructDimName.c_str());

		} else {
			if (m_strVarActiveMultidimLon != "") {
				m_strVarActiveMultidimLon = "";
				m_strVarActiveMultidimLat = "";
				m_strUnstructDimName = m_strDefaultUnstructDimName;
				fReinitializeGridDataSampler = true;
			}
		}
	}

	// Generate title
	{
		NcError error(NcError::silent_nonfatal);
		NcAtt * attLongName = m_varActive->get_att("long_name");
		if (attLongName != NULL) {
			m_strVarActiveTitle = std::string("[") + m_varActive->name() + std::string("] ") + attLongName->as_string(0);
		} else {
			m_strVarActiveTitle = m_varActive->name();
		}

		if (m_strVarActiveTitle.length() > 60) {
			m_strVarActiveTitle = m_strVarActiveTitle.substr(0,60);
			m_strVarActiveTitle += "...";
		}

		NcAtt * attUnits = m_varActive->get_att("units");
		if (attUnits != NULL) {
			m_strVarActiveUnits = attUnits->as_string(0);
		} else {
			m_strVarActiveUnits = "";
		}

		if (m_strVarActiveUnits.length() > 20) {
			m_strVarActiveUnits = m_strVarActiveUnits.substr(0,20);
			m_strVarActiveUnits += "...";
		}

	}

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
				m_fIsVarActiveUnstructured = true;
			}
		}

		if (fReinitializeGridDataSampler) {
			InitializeGridDataSampler();
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

	// Activate the export button
	m_wxExportButton->Enable(true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnBoundsChanged(
	wxCommandEvent & event
) {
	if (m_fVerbose) {
		std::cout << "BOUNDS CHANGED" << std::endl;
	}

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
	if (m_fVerbose) {
		std::cout << "RANGE CHANGED" << std::endl;
	}

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
	if (m_fVerbose) {
		std::cout << "TIMER" << std::endl;
	}

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
	if (m_fVerbose) {
		std::cout << "DIM BUTTON CLICKED" << std::endl;
	}

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
	if (m_fVerbose) {
		std::cout << "AXES BUTTON CLICKED" << std::endl;
	}

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

void wxNcVisFrame::OnColorMapInvertClicked(wxCommandEvent & event) {
	if (m_fVerbose) {
		std::cout << "COLORMAP INVERT" << std::endl;
	}

	if (m_imagepanel == NULL) {
		return;
	}

	m_imagepanel->ToggleInvertColorMap(true);

	if (m_imagepanel->IsInvertColorMap()) {
		m_wxInvertColorMapButton->SetLabel(wxString::Format("%lc",(0x25BC))); // Down
	} else {
		m_wxInvertColorMapButton->SetLabel(wxString::Format("%lc",(0x25B2))); // Up
	}
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnColorMapCombo(wxCommandEvent & event) {
	if (m_fVerbose) {
		std::cout << "COLORMAP COMBO" << std::endl;
	}

	if (m_imagepanel == NULL) {
		return;
	}

	std::string strValue = event.GetString().ToStdString();

	m_imagepanel->SetColorMap(strValue, true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnGridLinesCombo(wxCommandEvent & event) {
	if (m_fVerbose) {
		std::cout << "GRID COMBO" << std::endl;
	}

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
	if (m_fVerbose) {
		std::cout << "OVERLAYS COMBO" << std::endl;
	}

	if (m_imagepanel == NULL) {
		return;
	}

	SHPFileData & overlaydata = m_imagepanel->GetOverlayDataRef();

	std::string strValue = event.GetString().ToStdString();

	if (strValue == "Overlays Off") {
		overlaydata.clear();
	} else {
		wxFileName wxfileOverlayPath(m_wxstrNcVisResourceDir, strValue);

		ReadShpFile(wxfileOverlayPath.GetFullPath().ToStdString(), overlaydata, false);
	}

	m_imagepanel->GenerateImageFromImageMap(true);
}

////////////////////////////////////////////////////////////////////////////////

void wxNcVisFrame::OnSamplerCombo(wxCommandEvent & event) {
	if (m_fVerbose) {
		std::cout << "SAMPLER COMBO" << std::endl;
	}

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


