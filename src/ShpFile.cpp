///////////////////////////////////////////////////////////////////////////////
///
///	\file    ShpFile.cpp
///	\author  Paul Ullrich
///	\version July 9, 2022
///

#include "Exception.h"
#include "Announce.h"
#include "ShpFile.h"
#include "order32.h"

#include <cmath>
#include <iostream>
#include <fstream>

///////////////////////////////////////////////////////////////////////////////

static const int32_t SHPFileCodeRef = 0x0000270a;

static const int32_t SHPVersionRef = 1000;

static const int32_t SHPPointType = 1;
static const int32_t SHPPolylineType = 3;
static const int32_t SHPPolygonType = 5;
static const int32_t SHPMultiPointType = 8;

struct SHPHeader {
	int32_t iFileCode;
	int32_t iUnused[5];
	int32_t iFileLength;
	int32_t iVersion;
	int32_t iShapeType;
};

struct SHPBounds {
	double dXmin;
	double dYmin;
	double dXmax;
	double dYmax;

	double dZmin;
	double dZmax;

	double dMmin;
	double dMmax;
};

struct SHPRecordHeader {
	int32_t iNumber;
	int32_t nLength;
};

struct SHPPolygonHeader {
	double dXmin;
	double dYmin;
	double dXmax;
	double dYmax;
	int32_t nNumParts;
	int32_t nNumPoints;
};

///////////////////////////////////////////////////////////////////////////////

int32_t SwapEndianInt32(const int32_t num) {

	int32_t res;
	const char * pnum = (const char *)(&num);
	char * pres = (char *)(&res);

	pres[0] = pnum[3];
	pres[1] = pnum[2];
	pres[2] = pnum[1];
	pres[3] = pnum[0];

	return res;
}

///////////////////////////////////////////////////////////////////////////////

double SwapEndianDouble(const double num) {

	double res;
  	const char * pnum = (const char *)(&num);
	char * pres = (char *)(&res);

	pres[0] = pnum[7];
	pres[1] = pnum[6];
	pres[2] = pnum[5];
	pres[3] = pnum[4];
	pres[4] = pnum[3];
	pres[5] = pnum[2];
	pres[6] = pnum[1];
	pres[7] = pnum[0];

	return res;
}

///////////////////////////////////////////////////////////////////////////////

void ReadShpFile(
	const std::string & strInputFile,
	SHPFileData & shpdata,
	bool fVerbose
) {
	// Units for each dimension
	const std::string strXYUnits = "lonlat";

	// Convexify the shpdata
	bool fConvexify = false;

	// First polygon
	int iPolyFirst = (-1);

	// Last polygon
	int iPolyLast = (-1);

	// Coarsen
	int nCoarsen = 1;

	// Check file names
	if (strInputFile == "") {
		_EXCEPTIONT("No input file specified");
	}
	if (nCoarsen < 1) {
		_EXCEPTIONT("--coarsen must be greater than or equal to 1");
	}

	// Clear the shpdata
	shpdata.faces.clear();
	shpdata.coords.clear();

	// Load shapefile
	if (fVerbose) AnnounceStartBlock("Loading SHP file \"%s\"", strInputFile.c_str());
	std::ifstream shpfile(
		strInputFile.c_str(), std::ios::in | std::ios::binary);
	if (!shpfile.is_open()) {
		_EXCEPTION1("Unable to open SHP file \"%s\"", strInputFile.c_str());
	}

	SHPHeader shphead;
	shpfile.read((char*)(&shphead), sizeof(SHPHeader));

	if (O32_HOST_ORDER == O32_LITTLE_ENDIAN) {
		shphead.iFileCode = SwapEndianInt32(shphead.iFileCode);
		shphead.iFileLength = SwapEndianInt32(shphead.iFileLength);
	} else if (O32_HOST_ORDER == O32_BIG_ENDIAN) {
		shphead.iVersion = SwapEndianInt32(shphead.iVersion);
		shphead.iShapeType = SwapEndianInt32(shphead.iShapeType);
	} else {
		_EXCEPTIONT("Invalid system Endian");
	}

	if (shphead.iFileCode != SHPFileCodeRef) {
		_EXCEPTIONT("Input file does not appear to be a ESRI Shapefile: "
			"File code mismatch");
	}
	if (shphead.iVersion != SHPVersionRef) {
		_EXCEPTIONT("Input file error: Version mismatch");
	}
	if ((shphead.iShapeType != SHPPolygonType) && (shphead.iShapeType != SHPPolylineType)) {
		_EXCEPTION1("Input file error: Polygon or Polyline type expected, found (%i)", shphead.iShapeType);
	}

	shpdata.type = shphead.iShapeType;

	SHPBounds shpbounds;
	shpfile.read((char*)(&shpbounds), sizeof(SHPBounds));

	if (O32_HOST_ORDER == O32_BIG_ENDIAN) {
		shpbounds.dXmin = SwapEndianDouble(shpbounds.dXmin);
		shpbounds.dYmin = SwapEndianDouble(shpbounds.dYmin);
		shpbounds.dXmax = SwapEndianDouble(shpbounds.dXmax);
		shpbounds.dYmax = SwapEndianDouble(shpbounds.dXmax);

		shpbounds.dZmin = SwapEndianDouble(shpbounds.dZmin);
		shpbounds.dZmax = SwapEndianDouble(shpbounds.dZmax);

		shpbounds.dMmin = SwapEndianDouble(shpbounds.dMmin);
		shpbounds.dMmax = SwapEndianDouble(shpbounds.dMmax);
	}

	// Current position (in 16-bit words)
	int32_t iCurrentPosition = 50;

	int32_t iPolygonIx = 1;

	// Load records
	while (iCurrentPosition < shphead.iFileLength) {

		// Read the record header
		SHPRecordHeader shprechead;
		shpfile.read((char*)(&shprechead), sizeof(SHPRecordHeader));
		if (shpfile.eof()) {
			break;
		}

		if (O32_HOST_ORDER == O32_LITTLE_ENDIAN) {
			shprechead.iNumber = SwapEndianInt32(shprechead.iNumber);
			shprechead.nLength = SwapEndianInt32(shprechead.nLength);
		}

		iPolygonIx++;

		if (fVerbose) AnnounceStartBlock("Poly %i", shprechead.iNumber);

		iCurrentPosition += shprechead.nLength;

		// Read the shape type
		int32_t iShapeType;
		shpfile.read((char*)(&iShapeType), sizeof(int32_t));
		if (shpfile.eof()) {
			break;
		}

		if (O32_HOST_ORDER == O32_BIG_ENDIAN) {
			iShapeType = SwapEndianInt32(iShapeType);
		}
		if ((iShapeType != SHPPolygonType) && (iShapeType != SHPPolylineType)) {
			_EXCEPTIONT("Input file error: Record Polygon or Polylinetype expected");
		}

		// Read the polygon header
		SHPPolygonHeader shppolyhead;
		shpfile.read((char*)(&shppolyhead), sizeof(SHPPolygonHeader));
		if (shpfile.eof()) {
			break;
		}

		if (O32_HOST_ORDER == O32_BIG_ENDIAN) {
			shppolyhead.dXmin = SwapEndianDouble(shppolyhead.dXmin);
			shppolyhead.dYmin = SwapEndianDouble(shppolyhead.dYmin);
			shppolyhead.dXmax = SwapEndianDouble(shppolyhead.dXmax);
			shppolyhead.dYmax = SwapEndianDouble(shppolyhead.dYmax);
			shppolyhead.nNumParts = SwapEndianInt32(shppolyhead.nNumParts);
			shppolyhead.nNumPoints = SwapEndianInt32(shppolyhead.nNumPoints);
		}

		// Sanity check
		if (shppolyhead.nNumParts > 0x1000000) {
			_EXCEPTION1("Polygon NumParts exceeds sanity bound (%i)",
				shppolyhead.nNumParts);
		}
		if (shppolyhead.nNumPoints > 0x1000000) {
			_EXCEPTION1("Polygon NumPoints exceeds sanity bound (%i)",
				shppolyhead.nNumPoints);
		}
		if (fVerbose) {
			Announce("containing %i part(s) with %i points",
				shppolyhead.nNumParts,
				shppolyhead.nNumPoints);
			Announce("Xmin: %3.5f", shppolyhead.dXmin);
			Announce("Ymin: %3.5f", shppolyhead.dYmin);
			Announce("Xmax: %3.5f", shppolyhead.dXmax);
			Announce("Ymax: %3.5f", shppolyhead.dYmax);
		}

		// Number of parts in this shape
		std::vector<int32_t> iPartBeginIx(shppolyhead.nNumParts+1);
		shpfile.read((char*)&(iPartBeginIx[0]),
			shppolyhead.nNumParts * sizeof(int32_t));
		if (shpfile.eof()) {
			break;
		}
		iPartBeginIx[shppolyhead.nNumParts] = shppolyhead.nNumPoints;

		std::vector<double> dPoints(shppolyhead.nNumPoints * 2);
		shpfile.read((char*)&(dPoints[0]),
			shppolyhead.nNumPoints * 2 * sizeof(double));
		if (shpfile.eof()) {
			break;
		}

		if (O32_HOST_ORDER == O32_BIG_ENDIAN) {
			for (int i = 0; i < shppolyhead.nNumParts; i++) {
				iPartBeginIx[i] = SwapEndianInt32(iPartBeginIx[i]);
			}
			for (int i = 0; i < shppolyhead.nNumPoints * 2; i++) {
				dPoints[i] = SwapEndianDouble(dPoints[i]);
			}
		}

		if ((iPolyFirst != (-1)) && (shprechead.iNumber < iPolyFirst)) {
			AnnounceEndBlock("Done");
			continue;
		}

		if ((iPolyLast != (-1)) && (shprechead.iNumber > iPolyLast)) {
			AnnounceEndBlock("Done");
			continue;
		}

		// Load SHP data
		if (strXYUnits == "lonlat") {
			for (size_t iPart = 0; iPart < iPartBeginIx.size()-1; iPart++) {

				size_t nFaces = shpdata.faces.size();
				size_t nCoords = shpdata.coords.size();

				int nShpCoords = (iPartBeginIx[iPart+1] - iPartBeginIx[iPart]) / nCoarsen;
	
				shpdata.faces.resize(nFaces+1);
				shpdata.coords.resize(nCoords + nShpCoords);

				shpdata.faces[nFaces].resize(nShpCoords);

				for (size_t i = 0; i < nShpCoords; i++) {
					int ix = iPartBeginIx[iPart] + i * nCoarsen;

					shpdata.coords[nCoords+i].first = dPoints[2*ix];
					shpdata.coords[nCoords+i].second = dPoints[2*ix+1];

					// Note that shapefile polygons are specified in clockwise order,
					// whereas Exodus files request polygons to be specified in
					// counter-clockwise order.  Hence we need to reorient these Faces.
					shpdata.faces[nFaces][nShpCoords - i - 1] = nCoords+i;
				}
			}

		} else {
			_EXCEPTION1("Invalid units \"%s\"", strXYUnits.c_str());
		}

		if (fVerbose) AnnounceEndBlock("Done");
	}

	if (fVerbose) AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////



