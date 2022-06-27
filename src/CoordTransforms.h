///////////////////////////////////////////////////////////////////////////////
///
///	\file    CoordTransforms.h
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#ifndef _COORDTRANSFORMS_H_
#define _COORDTRANSFORMS_H_

#include <cmath>

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Convert radians to degrees.
///	</summary>
inline double RadToDeg(
	double dRad
) {
	return (dRad * 180.0 / M_PI);
}

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Convert degrees to radians.
///	</summary>
inline double DegToRad(
	double dDeg
) {
	return (dDeg * M_PI / 180.0);
}

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Translate a longitude value to the range [0,360)
///	</summary>
inline double LonDegToStandardRange(
	double dLonDeg
) {
	dLonDeg = (dLonDeg - 360.0 * floor(dLonDeg / 360.0));
	if ((dLonDeg < 0.0) || (dLonDeg >= 360.0)) {
		return 0.0;
	}
	return dLonDeg;
}

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Calculate 3D Cartesian coordinates from latitude and longitude,
///		in radians.
///	</summary>
inline void RLLtoXYZ_Rad(
	double dLonRad,
	double dLatRad,
	double & dX,
	double & dY,
	double & dZ
) {
	dX = cos(dLonRad) * cos(dLatRad);
	dY = sin(dLonRad) * cos(dLatRad);
	dZ = sin(dLatRad);
}

///////////////////////////////////////////////////////////////////////////////

///	<summary>
///		Calculate 3D Cartesian coordinates from latitude and longitude,
///		in degrees.
///	</summary>
inline void RLLtoXYZ_Deg(
	double dLonDeg,
	double dLatDeg,
	double & dX,
	double & dY,
	double & dZ
) {
	return RLLtoXYZ_Rad(
		DegToRad(dLonDeg),
		DegToRad(dLatDeg),
		dX, dY, dZ);
}

///////////////////////////////////////////////////////////////////////////////

#endif // _COORDTRANSFORMS_H_

