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

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A class that manages the NcVis app frame.
///	</summary>
class wxNcVisFrame : public wxFrame {

public:
	///	<summary>
	///		Constructor.
	///	</summary>
	wxNcVisFrame(
		const wxString & title,
		const wxPoint & pos,
		const wxSize & size);

	///	<summary>
	///		Constructor.
	///	</summary>
	wxNcVisFrame(
		const wxString & title,
		const wxPoint & pos,
		const wxSize & size,
		const wxString & strFilename);

	///	<summary>
	///		Initialize the wxNcVisFrame.
	///	</summary>
	void InitializeWindow();

	///	<summary>
	///		Open the specified file.
	///	</summary>
	void OpenFile(const wxString & strFilename);

	///	<summary>
	///		Load data from the active variable.
	///	</summary>
	void LoadData();

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
	///		Callback triggered when the color map button is pressed.
	///	</summary>
	void OnColorMapClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the data transform button is pressed.
	///	</summary>
	void OnDataTransClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when a new variable has been selected for rendering.
	///	</summary>
	void OnVariableSelected(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the bounds of the domain have been edited.
	///	</summary>
	void OnBoundsChanged(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when the zoom out button is pressed.
	///	</summary>
	void OnZoomOutClicked(wxCommandEvent & event);

	///	<summary>
	///		Callback triggered when a dimension button is pressed.
	///	</summary>
	void OnDimButtonClicked(wxCommandEvent & event);

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
	wxComboBox * m_vecwxVarSelector[10];

	///	<summary>
	///		Text controls for indicating displayed bounds.
	///	</summary>
	wxTextCtrl * m_vecwxImageBounds[4];

	///	<summary>
	///		Sizer containing variable dimensions.
	///	</summary>
	wxBoxSizer * m_dimsizer;

	///	<summary>
	///		Text controls for indicating dimension indices.
	///	</summary>
	wxTextCtrl * m_vecwxDimIndex[10];

	///	<summary>
	///		Image panel used to display data.
	///	</summary>
	wxImagePanel * m_imagepanel;

private:
	///	<summary>
	///		Filename being displayed.
	///	</summary>
	wxString m_strFilename;

	///	<summary>
	///		NetCDF file being worked on.
	///	</summary>
	NcFile * m_pncfile;

	///	<summary>
	///		NetCDF variable currently loaded.
	///	</summary>
	NcVar * m_varActive;

	///	<summary>
	///		Current auxiliary dimension indices for loaded variable.
	///	</summary>
	std::vector<long> m_lVarActiveDims;

	///	<summary>
	///		List of variables in the file.
	///	</summary>
	std::vector<std::string> m_vecVarNames[10];

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
