///////////////////////////////////////////////////////////////////////////////
///
///	\file    GriddedData.h
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#ifndef _GRIDDEDDATA_H_
#define _GRIDDEDDATA_H_

////////////////////////////////////////////////////////////////////////////////

class GriddedData {
public:
	///	<summary>
	///		Longitude coordinates of points.
	///	</summary>
	DataArray1D<double> m_dLon;

	///	<summary>
	///		Latitude coordinates of points.
	///	</summary>
	DataArray1D<double> m_dLat;

	///	<summary>
	///		Latitude coordinates of points.
	///	</summary>
	DataArray1D<double> m_dLat;
};

////////////////////////////////////////////////////////////////////////////////

#endif
