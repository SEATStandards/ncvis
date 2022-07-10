///////////////////////////////////////////////////////////////////////////////
///
///	\file    ShpFile.h
///	\author  Paul Ullrich
///	\version July 9, 2022
///

#ifndef _SHPFILE_H_
#define _SHPFILE_H_

///////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <utility>

///////////////////////////////////////////////////////////////////////////////

class SHPFileData {
public:
	///	<summary>
	///		Clear the data structure.
	///	</summary>
	void clear() {
		type = 0;
		faces.clear();
		coords.clear();
	}

public:
	///	<summary>
	///		Shape type.
	///	</summary>
	int32_t type;

	///	<summary>
	///		Faces.
	///	</summary>
	std::vector< std::vector<int> > faces;

	///	<summary>
	///		Vertices.
	///	</summary>
	std::vector< std::pair<double, double> > coords;
};

///////////////////////////////////////////////////////////////////////////////

void ReadShpFile(
	const std::string & strInputFile,
	SHPFileData & shpdata,
	bool fVerbose = false
);

///////////////////////////////////////////////////////////////////////////////

#endif // _SHPFILE_H_

