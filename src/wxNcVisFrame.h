///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxNcVisFrame.h
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#ifndef _WXNCVISFRAME_H_
#define _WXNCVISFRAME_H_

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
	#include <wx/wx.h>
#endif

#include "wxImagePanel.h"
#include "ColorMap.h"
#include "GridDataSampler.h"

#include <map>
#include <vector>

////////////////////////////////////////////////////////////////////////////////

class wxNcVisOptsDialog;
class wxGridBagSizer;

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class that manages the NcVis app frame.
///	</summary>
class wxNcVisFrame : public wxFrame {

public:
	///	<summary>
	///		Maximum number of dimensions per variable.
	///	</summary>
	static const int NcVarMaximumDimensions = 5;

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	wxNcVisFrame(
		const wxString & title,
		const wxPoint & pos,
		const wxSize & size,
		const std::string & strNcVisResourceDir,
		const std::vector<std::string> & vecFilenames
	);

	///	<summary>
	///		Initialize the wxNcVisFrame.
	///	</summary>
	void InitializeWindow();

	///	<summary>
	///		Open the specified file.
	///	</summary>
	void OpenFiles(const std::vector<std::string> & strFilenames);

	///	<summary>
	///		Get the NcVis resource directory.
	///	</summary>
	const std::string & GetResourceDir() const {
		return m_strNcVisResourceDir;
	}

	///	<summary>
	///		Load data from the active variable.
	///	</summary>
	void LoadData();

	///	<summary>
	///		Get a pointer to the data.
	///	</summary>
	const DataArray1D<float> & GetData() const {
		return m_data;
	}

	///	<summary>
	///		Map an array of sample coordinates in 1D to indices in
	///		the dimension variable of the active variable.
	///	</summary>
	void MapSampleCoords1DFromActiveVar(
		const DataArray1D<double> & dSample,
		long lDim,
		std::vector<int> & veccoordmap
	);

	///	<summary>
	///		Constrain sample coordinates (e.g., if periodicity is present).
	///	</summary>
	void ConstrainSampleCoordinates(
		DataArray1D<double> & dSampleX,
		DataArray1D<double> & dSampleY
	);

	///	<summary>
	///		Sample the data.
	///	</summary>
	void SampleData(
		const DataArray1D<double> & dSampleX,
		const DataArray1D<double> & dSampleY,
		DataArray1D<int> & imagemap
	);

	///	<summary>
	///		Set the bounds displayed.
	///	</summary>
	void SetDisplayedBounds(
		double dX0,
		double dX1,
		double dY0,
		double dY1
	);

	///	<summary>
	///		Update the data range displayed in the controls.
	///	</summary>
	void SetDisplayedDataRange(
		float dDataMin,
		float dDataMax
	);

	///	<summary>
	///		Set the data range using the min/max of visible data.
	///	</summary>
	void SetDataRangeByMinMax(
		bool fRedraw = false
	);

	///	<summary>
	///		Set the status message.
	///	</summary>
	void SetStatusMessage(
		const wxString & strMessage,
		bool fIncludeVersion
	);

private:
	void OnHello(wxCommandEvent & event);
	void OnExit(wxCommandEvent & event);
	void OnAbout(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the window is closed.
	///	</summary>
	void OnClose(wxCloseEvent & event);

	///	<summary>
	///		Callback triggered when the color map button is pressed.
	///	</summary>
	void OnColorMapClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the data transform button is pressed.
	///	</summary>
	void OnDataTransClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the options button is pressed.
	///	</summary>
	void OnOptionsClicked(wxCommandEvent & event);

	///	<summary>
	///		Generate dimension controls for a given variable.
	///	</summary>
	void GenerateDimensionControls();

	///	<summary>
	///		Callback triggered when a new variable has been selected for rendering.
	///	</summary>
	void OnVariableSelected(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the bounds of the domain have been edited.
	///	</summary>
	void OnBoundsChanged(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the range has been edited.
	///	</summary>
	void OnRangeChanged(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the range reset min/max button is pressed.
	///	</summary>
	void OnRangeResetMinMax(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the zoom out button is pressed.
	///	</summary>
	void OnZoomOutClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when a dimension button is pressed.
	///	</summary>
	void OnDimButtonClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when an axes button is pressed.
	///	</summary>
	void OnAxesButtonClicked(wxCommandEvent & event);

private:
	///	<summary>
	///		Button for changing color map.
	///	</summary>
	wxButton * m_wxColormapButton;

	///	<summary>
	///		Button for changing data range.
	///	</summary>
	wxButton * m_wxDataTransButton;

	///	<summary>
	///		Combo boxes for choosing the variable.
	///	</summary>
	wxComboBox * m_vecwxVarSelector[NcVarMaximumDimensions];

	///	<summary>
	///		Text controls for indicating displayed bounds.
	///	</summary>
	wxTextCtrl * m_vecwxImageBounds[4];

	///	<summary>
	///		Buttons for indicating active axes.
	///	</summary>
	wxButton * m_vecwxActiveAxes[NcVarMaximumDimensions][3];

	///	<summary>
	///		Sizer for the panel.
	///	</summary>
	wxBoxSizer * m_panelsizer;

	///	<summary>
	///		Sizer containing all controls.
	///	</summary>
	wxBoxSizer * m_ctrlsizer;

	///	<summary>
	///		Sizer to the right of the controls menu.
	///	</summary>
	wxBoxSizer * m_rightsizer;

	///	<summary>
	///		Sizer containing variable dimensions.
	///	</summary>
	wxFlexGridSizer * m_vardimsizer;

	///	<summary>
	///		Text controls for indicating displayed bounds.
	///	</summary>
	wxTextCtrl * m_vecwxRange[2];

	///	<summary>
	///		Text controls for indicating dimension indices.
	///	</summary>
	wxTextCtrl * m_vecwxDimIndex[NcVarMaximumDimensions];

	///	<summary>
	///		Image panel used to display data.
	///	</summary>
	wxImagePanel * m_imagepanel;

	///	<summary>
	///		Options frame.
	///	</summary>
	wxNcVisOptsDialog * m_wxNcVisOptsDialog;

private:
	///	<summary>
	///		Directory containing ncvis resources.
	///	</summary>
	std::string m_strNcVisResourceDir;

	///	<summary>
	///		Filename being displayed.
	///	</summary>
	std::vector<std::string> m_vecFilenames;

	///	<summary>
	///		NetCDF file being worked on.
	///	</summary>
	std::vector<NcFile *> m_vecpncfiles;

	///	<summary>
	///		Dimension data, stored persistently to avoid having to reload.
	///	</summary>
	typedef std::map<size_t, std::vector<double> > DimDataFileIdAndCoordMap;
	typedef std::map<std::string, DimDataFileIdAndCoordMap> DimDataMap;
	DimDataMap m_mapDimData;

	///	<summary>
	///		Unstructured index dimension name.
	///	</summary>
	std::string m_strUnstructDimName;

	///	<summary>
	///		NetCDF variable currently loaded.
	///	</summary>
	NcVar * m_varActive;

	///	<summary>
	///		Bookmarked dimensions.
	///	</summary>
	std::map<std::string, long> m_mapDimBookmarks;

	///	<summary>
	///		Flag indicating currently loaded data is unstructured.
	///	</summary>
	bool m_fIsVarActiveUnstructured;

	///	<summary>
	///		Current auxiliary dimension indices for loaded variable.
	///	</summary>
	std::vector<long> m_lVarActiveDims;

	///	<summary>
	///		Currently displayed variable dimension indices.
	///	</summary>
	long m_lDisplayedDims[2];

	///	<summary>
	///		Map between variables found among files and file index where they can be
	///		found.  Variables are further indexed by number of dimensions.
	///	</summary>
	std::map<std::string, std::vector<size_t> > m_mapVarNames[10];

private:
	///	<summary>
	///		ColorMap index.
	///	</summary>
	size_t m_sColorMap;

	///	<summary>
	///		Class for sampling data on the grid using a quad tree.
	///	</summary>
	GridDataSamplerUsingQuadTree m_gdsqt;

	///	<summary>
	///		Class for sampling data on the grid using a kd tree.
	///	</summary>
	GridDataSamplerUsingKDTree m_gdskd;

	///	<summary>
	///		Data being visualized.
	///	</summary>
	DataArray1D<float> m_data;

	wxDECLARE_EVENT_TABLE();
};

////////////////////////////////////////////////////////////////////////////////


#endif // _WXNCVISFRAME_H_
