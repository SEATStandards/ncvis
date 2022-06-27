///////////////////////////////////////////////////////////////////////////////
///
///	\file    ColorMap.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#include "ColorMap.h"

////////////////////////////////////////////////////////////////////////////////

ColorMapLibrary::ColorMapLibrary() {
	m_vecColorMapNames.push_back("jet");
	m_vecColorMapNames.push_back("bluered");
	m_vecColorMapNames.push_back("gray");
	m_vecColorMapNames.push_back("INVALID");
}

////////////////////////////////////////////////////////////////////////////////

size_t ColorMapLibrary::GetColorMapCount() {
	return m_vecColorMapNames.size()-1;
}

////////////////////////////////////////////////////////////////////////////////

const std::string & ColorMapLibrary::GetColorMapName(
	size_t ix
) {
	if (ix >= m_vecColorMapNames.size()) {
		return m_vecColorMapNames[m_vecColorMapNames.size()-1];
	} else {
		return m_vecColorMapNames[ix];
	}
}

////////////////////////////////////////////////////////////////////////////////

void ColorMapLibrary::GenerateColorMap(
	const std::string & strColorMap,
	ColorMap & colormap
) {
	colormap.resize(256, std::vector<unsigned char>(3, 0) );

	// "gray" color map
	if (strColorMap == "gray") {
		for (int i = 0; i < 256; i++) {
			colormap[i][0] = i;
			colormap[i][1] = i;
			colormap[i][2] = i;
		}

	// "jet" color map
	} else if (strColorMap == "jet") {
		for (int i = 0; i <= 32; i++) {
			colormap[i][0] = 0;
			colormap[i][1] = 0;
			colormap[i][2] = (i+32)*4-1;
		}
		for (int i = 33; i <= 96; i++) {
			colormap[i][0] = 0;
			colormap[i][1] = (i-32)*4-1;
			colormap[i][2] = 255;
		}
		for (int i = 97; i < 160; i++) {
			colormap[i][0] = (i-96)*4-1;
			colormap[i][1] = 255;
			colormap[i][2] = (160-i)*4-1;
		}
		for (int i = 160; i <= 224; i++) {
			colormap[i][0] = 255;
			colormap[i][1] = (224-i)*4-1;
			colormap[i][2] = 0;
		}
		for (int i = 224; i < 256; i++) {
			colormap[i][0] = (288-i)*4-1;
			colormap[i][1] = 0;
			colormap[i][2] = 0;
		}

	// "bluered" colormap
	} else if (strColorMap == "bluered") {
		for (int i = 0; i < 128; i++) {
			colormap[i][0] = i*2;
			colormap[i][1] = i*2;
			colormap[i][2] = 127+i;
		}
		for (int i = 128; i < 256; i++) {
			colormap[i][0] = 382-i;
			colormap[i][1] = (255-i)*2;
			colormap[i][2] = (255-i)*2;
		}
/*
		for (int i = 0; i < 256; i++) {
			printf("%i %i %i %i\n", i, colormap[i][0], colormap[i][1], colormap[i][2]);
		}
*/
	} else {
		_EXCEPTION1("Invalid colormap \"%s\"", strColorMap.c_str());
	}
}

////////////////////////////////////////////////////////////////////////////////

