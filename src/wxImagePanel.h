///////////////////////////////////////////////////////////////////////////////
///
///	\file    wxImagePanel.h
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#ifndef _WXIMAGEPANEL_H_
#define _WXIMAGEPANEL_H_

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/position.h>

#include "CoordTransforms.h"
#include "GridDataSampler.h"
#include "ColorMap.h"
#include "ShpFile.h"
#include "schrift.h"

class wxNcVisFrame;

////////////////////////////////////////////////////////////////////////////////

///	<summary>
///		A wxWidget that manages display of the data.
///	</summary>
class wxImagePanel : public wxPanel {
public:
	///	<summary>
	///		Constructor.
	///	</summary>
	wxImagePanel(
		wxNcVisFrame * parent
	);

	///	<summary>
	///		Get the size of the image panel for a given map size.
	///	</summary>
	wxSize GetPanelSize(
		int nMapWidth = (-1),
		int nMapHeight = (-1)
	) const;

	///	<summary>
	///		Get the position and size of the map panel.
	///	</summary>
	void GetMapPositionSize(
		wxSize & wxs,
		wxPosition & wxp,
		int nImageWidth = (-1),
		int nImageHeight = (-1)
	) const;

	///	<summary>
	///		Reset the size of the panel.
	///	</summary>
	///	<returns>
	///		true if the panel size has changed, false otherwise.
	///	</returns>
	bool ResetPanelSize();

public:
	///	<summary>
	///		Callback for when a paint event is triggered.
	///	</summary>
	void OnPaint(wxPaintEvent & evt);

	///	<summary>
	///		Callback for when a size event is triggered.
	///	</summary>
	void OnSize(wxSizeEvent & evt);

	///	<summary>
	///		Callback for when an idle event is triggered.
	///	</summary>
	void OnIdle(wxIdleEvent & evt);

	///	<summary>
	///		Callback for when the mouse is moved.
	///	</summary>
	void OnMouseMotion(wxMouseEvent & evt);

	///	<summary>
	///		Callback for when the mouse leaves the window.
	///	</summary>
	void OnMouseLeaveWindow(wxMouseEvent & evt);

	///	<summary>
	///		Callback for when the mouse is left-double-clicked.
	///	</summary>
	void OnMouseLeftDoubleClick(wxMouseEvent & evt);

public:
	///	<summary>
	///		Format a label bar label from a value.
	///	</summary>
	void FormatLabelBarLabel(
		double dValue,
		std::string & str
	);

	///	<summary>
	///		Format a tickmark label from a value.
	///	</summary>
	void FormatTickmarkLabel(
		double dValue,
		std::string & str
	);

	///	<summary>
	///		Convert a real coordiante to an image coordinate.
	///	</summary>
	void RealCoordToImageCoord(
		double dX,
		double dY,
		size_t sImageWidth,
		size_t sImageHeight,
		int & iXcoord,
		int & iYcoord
	);

	///	<summary>
	///		Generate the image data from the image map.
	///	</summary>
	template <int NDIM>
	void GenerateImageDataFromImageMap(
		const size_t sImageOffsetX,
		const size_t sImageOffsetY,
		const size_t sImageWidth,
		const size_t sImageHeight,
		const size_t sPanelWidth,
		const size_t sPanelHeight,
		unsigned char * imagedata
	);

	///	<summary>
	///		Generate the image from the image map.
	///	</summary>
	void GenerateImageFromImageMap(
		bool fRedraw = false
	);

	///	<summary>
	///		Toggle invert color map.
	///	</summary>
	void ToggleInvertColorMap(bool fRedraw = false);

	///	<summary>
	///		Toggle invert color map.
	///	</summary>
	bool IsInvertColorMap() {
		return m_colormap.GetInvert();
	}

	///	<summary>
	///		Set the color map.
	///	</summary>
	void SetColorMap(
		const wxString & strColorMap,
		bool fRedraw = false
	);

	///	<summary>
	///		Set the colormap scaling factor.
	///	</summary>
	void SetColorMapScalingFactor(
		float dColorMapScalingFactor,
		bool fRedraw = false
	);

	///	<summary>
	///		Resample data and generate the imagemap.
	///	</summary>
	void ResampleData(
		bool fRedraw = false
	);

	///	<summary>
	///		Resample the coordinate range.
	///	</summary>
	void SetCoordinateRange(
		size_t sMapWidth,
		size_t sMapHeight,
		double dX0,
		double dX1,
		double dY0,
		double dY1,
		bool fRedraw = false
	);

	///	<summary>
	///		Resample the coordinate range.
	///	</summary>
	void SetCoordinateRange(
		double dX0,
		double dX1,
		double dY0,
		double dY1,
		bool fRedraw = false
	);

	///	<summary>
	///		Impose the image size and disable redraw.
	///	</summary>
	void ImposeImageSize(
		size_t sImageWidth,
		size_t sImageHeight
	);

	///	<summary>
	///		Reset the image size to be determined by the panel.
	///	</summary>
	void ResetImageSize();

	///	<summary>
	///		Get the image size.
	///	</summary>
	wxSize GetImageSize();

	///	<summary>
	///		Set the data range.
	///	</summary>
	void SetDataRange(
		float dDataMin,
		float dDataMax,
		bool fRedraw = false
	);

	///	<summary>
	///		Set the status of grid lines.
	///	</summary>
	void SetGridLinesOn(
		bool fGridLinesOn,
		bool fRedraw = false
	);

	///	<summary>
	///		Show override message.
	///	</summary>
	void EnableRedraw(
		bool fEnableRedraw = true
	) {
		m_fEnableRedraw = fEnableRedraw;
	}

public:
	///	<summary>
	///		Get the x coordinate range minimum.
	///	</summary>
	double GetXRangeMin() const {
		return m_dXrange[0];
	}

	///	<summary>
	///		Get the x coordinate range maximum.
	///	</summary>
	double GetXRangeMax() const {
		return m_dXrange[1];
	}

	///	<summary>
	///		Get the x coordinate range minimum.
	///	</summary>
	double GetYRangeMin() const {
		return m_dYrange[0];
	}

	///	<summary>
	///		Get the x coordinate range maximum.
	///	</summary>
	double GetYRangeMax() const {
		return m_dYrange[1];
	}

	///	<summary>
	///		Get the data range minimum.
	///	</summary>
	float GetDataRangeMin() const {
		return m_dDataRange[0];
	}

	///	<summary>
	///		Get the data range maximum.
	///	</summary>
	float GetDataRangeMax() const {
		return m_dDataRange[1];
	}

	///	<summary>
	///		Get the overlay data.
	///	</summary>
	SHPFileData & GetOverlayDataRef() {
		return m_overlaydata;
	}

public:
	///	<summary>
	///		Alignment of text.
	///	</summary>
	enum TextAlignment {
		TextAlignment_Left,
		TextAlignment_Right,
		TextAlignment_Center
	};

	///	<summary>
	///		Calculate the minimum image buffer size for holding the given string.
	///	</summary>
	void CalculateStringMinImageBufferSize(
		SFT & sft,
		const std::string & str,
		int & nMinWidth,
		int & nMinHeight,
		int & nBaseline
	);

	///	<summary>
	///		Draw the specified character at the specified location on the image,
	///		using font information from m_sft.  The coordinate (x,y) indicates
	///		the top-left corner of the character.
	///	</summary>
	template <int NDIM>
	void DrawCharacter(
		SFT & sft,
		unsigned char c,
		int nX,
		int nY,
		int nCanvasWidth,
		int nCanvasHeight,
		unsigned char * imagedata,
		int * pwidth = NULL,
		int * pheight = NULL);

	///	<summary>
	///		Draw the specified string at the specified location on the image.
	///		using font information from m_sft.  The coordinate (x,y) indicates
	///		the top-left corner of the string.
	///	</summary>
	template <int NDIM>
	void DrawString(
		SFT & sft,
		const std::string & str,
		int nX,
		int nY,
		TextAlignment eAlign,
		int nCanvasWidth,
		int nCanvasHeight,
		unsigned char * imagedata,
		int * pwidth = NULL,
		int * pheight = NULL);

	///	<summary>
	///		Export the image to a PNG file.
	///	</summary>
	bool ExportToPNG(
		const wxString & wxstrFilename,
		size_t sImageWidth = static_cast<size_t>(-1),
		size_t sImageHeight = static_cast<size_t>(-1)
	);

	///	<summary>
	///		Repaint the panel now.
	///	</summary>
	void PaintNow();

	///	<summary>
	///		Render the device context.
	///	</summary>
	void Render(wxDC & dc);

private:
	///	<summary>
	///		Pointer to parent frame.
	///	</summary>
	wxNcVisFrame * m_pncvisparent;

	///	<summary>
	///		Colormap.
	///	</summary>
	ColorMap m_colormap;

	///	<summary>
	///		Colormap power scaling factor.
	///	</summary>
	float m_dColorMapScalingFactor;

	///	<summary>
	///		Font information for title bar.
	///	</summary>
	SFT m_sftTitleBar;

	///	<summary>
	///		Font information for label bar.
	///	</summary>
	SFT m_sftLabelBar;

	///	<summary>
	///		Font information for tickmark labels.
	///	</summary>
	SFT m_sftTickmarkLabels;

	///	<summary>
	///		Disable rendering.
	///	</summary>
	bool m_fEnableRedraw;

	///	<summary>
	///		Longitude region displayed in plot.
	///	</summary>
	double m_dXrange[2];

	///	<summary>
	///		Latitude region displayed in plot.
	///	</summary>
	double m_dYrange[2];

	///	<summary>
	///		Data range displayed in plot.
	///	</summary>
	float m_dDataRange[2];

	///	<summary>
	///		Array of sample points in X direction.
	///	</summary>
	std::vector<double> m_dSampleX;

	///	<summary>
	///		Array of sample points in Y direction.
	///	</summary>
	std::vector<double> m_dSampleY;

	///	<summary>
	///		Image map.
	///	</summary>
	std::vector<int> m_imagemap;

	///	<summary>
	///		Overlay information.
	///	</summary>
	SHPFileData m_overlaydata;

	///	<summary>
	///		A flag indicating gridlines should be drawn.
	///	</summary>
	bool m_fGridLinesOn;

	///	<summary>
	///		Image bitmap data.
	///	</summary>
	wxImage m_image;

	///	<summary>
	///		A flag indicating the window has been resized.
	///	</summary>
	bool m_fResize;

	DECLARE_EVENT_TABLE()
};

////////////////////////////////////////////////////////////////////////////////

#endif // _WXIMAGEPANEL_H_

