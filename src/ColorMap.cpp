///////////////////////////////////////////////////////////////////////////////
///
///	\file    ColorMap.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#include "ColorMap.h"
#include <string.h> // memset()
#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <glob.h>
#include <algorithm>

// taken from https://stackoverflow.com/a/8615450
// Written by Trevor Boyd Smith May 10, 2018
std::vector<std::string> glob(const std::string& pattern) {
    using namespace std;

    // glob struct resides on the stack
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));

    // do the glob operation
    int return_value = glob(pattern.c_str(), GLOB_TILDE, NULL, &glob_result);
    if(return_value != 0) {
        globfree(&glob_result);
        stringstream ss;
        ss << "glob() failed with return_value " << return_value << endl;
        throw std::runtime_error(ss.str());
    }

    // collect all the filenames into a std::list<std::string>
    vector<string> filenames;
    for(size_t i = 0; i < glob_result.gl_pathc; ++i) {
        filenames.push_back(string(glob_result.gl_pathv[i]));
    }

    // cleanup
    globfree(&glob_result);

    // done
    return filenames;
}

////////////////////////////////////////////////////////////////////////////////

ColorMapLibrary::ColorMapLibrary() {

	// get the resource directory
	char * szNcVisResourceDir = std::getenv("NCVIS_RESOURCE_DIR");
	if (szNcVisResourceDir == NULL) {
		std::cerr << "WARNING: Please set environment variable \"NCVIS_RESOURCE_DIR\"" << std::endl;
	}

	// get all *.rgb files in the resource directory
	std::stringstream glob_pattern;
	glob_pattern << szNcVisResourceDir << "/*.rgb";
	std::vector<std::string> file_list = glob(glob_pattern.str().c_str());
	for (const auto& path: file_list) {
		// extract the colormap from the path name
		size_t npos = path.find_last_of("/");
		std::string filename = path.substr(npos+1);
		npos = filename.find_last_of(".");
		std::string cmap_name = filename.substr(0, npos);

		if ( cmap_name == DEFAULT_COLORMAP ){
			// add the default colormap to the beginning of the list
			m_vecColorMapNames.insert(m_vecColorMapNames.begin(), cmap_name);
		} else {
			// add the colormap to the list
			m_vecColorMapNames.push_back(cmap_name);
		}

	}

	// register the hand-coded colormaps
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
		// get the resource directory
		char * szNcVisResourceDir = std::getenv("NCVIS_RESOURCE_DIR");
		if (szNcVisResourceDir == NULL) {
			std::cerr << "WARNING: Please set environment variable \"NCVIS_RESOURCE_DIR\"" << std::endl;
		}


		// Attempt to find a colormap file with the corresponding name
		std::stringstream colormap_path;
		colormap_path << szNcVisResourceDir << "/" << strColorMap << ".rgb";
		std::ifstream infile(colormap_path.str());


		if (infile.good()){
			int r,g,b;
			for (int i = 0; i < 256; i++){
				// read the current rgb values
				if (!(infile >> r >> g >> b)){
					_EXCEPTION1("Error parsing colormap file \"%s\"", colormap_path.str().c_str());
				}
				colormap[i][0] = r;
				colormap[i][1] = g;
				colormap[i][2] = b;
			}
		} else {
			// the colormap doesn't exist
			_EXCEPTION1("Invalid colormap \"%s\"", strColorMap.c_str());
		}
		
	}
}

////////////////////////////////////////////////////////////////////////////////

