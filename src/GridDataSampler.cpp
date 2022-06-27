///////////////////////////////////////////////////////////////////////////////
///
///	\file    GridDataSampler.cpp
///	\author  Paul Ullrich
///	\version June 27, 2022
///

#include "GridDataSampler.h"
#include "CoordTransforms.h"

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingQuadTree::Initialize(
	const std::string & strInputData
) {
	std::string strLonName("lon");
	std::string strLatName("lat");

	// Load the data file, if provided
	NcFile * pncdata = NULL;
	if (strInputData.length() != 0) {
		pncdata = new NcFile(strInputData.c_str());
		if (!pncdata->is_valid()) {
			_EXCEPTION1("Unable to open data file \"%s\"", strInputData.c_str());
		}
	}

	// Generate quadtree from datafile
	_ASSERT(pncdata != NULL);

	AnnounceStartBlock("Generating quadtree from datafile");

	NcVar * varLon = pncdata->get_var(strLonName.c_str());
	if (varLon == NULL) {
		_EXCEPTION2("Unable to read longitude variable \"%s\" from data file \"%s\"",
			strLonName.c_str(), strInputData.c_str());
	}
	if (varLon->num_dims() != 1) {
		_EXCEPTION2("Longitude variable \"%s\" in data file \"%s\" must have dimension 1",
			strLonName.c_str(), strInputData.c_str());
	}

	NcVar * varLat = pncdata->get_var(strLatName.c_str());
	if (varLon == NULL) {
		_EXCEPTION2("Unable to read latitude variable \"%s\" from data file \"%s\"",
			strLatName.c_str(), strInputData.c_str());
	}
	if (varLat->num_dims() != 1) {
		_EXCEPTION2("Latitude variable \"%s\" in data file \"%s\" must have dimension 1",
			strLatName.c_str(), strInputData.c_str());
	}

	if (varLon->get_dim(0)->size() != varLat->get_dim(0)->size()) {
		_EXCEPTION4("Longitude variable \"%s\" (size %li) and latitude variable \"%s\" (size %li) must have same size",
			varLon->name(), varLon->get_dim(0)->size(),
			varLat->name(), varLat->get_dim(0)->size());
	}

	DataArray1D<double> dLon(varLon->get_dim(0)->size());
	DataArray1D<double> dLat(varLat->get_dim(0)->size());

	varLon->get(&(dLon[0]), varLon->get_dim(0)->size());
	varLat->get(&(dLat[0]), varLat->get_dim(0)->size());

	long iReportSize = varLon->get_dim(0)->size() / 100;
	for (long i = 0; i < varLon->get_dim(0)->size(); i++) {

		// TODO: Convert to standard range
		if (dLon[i] < 0.0) {
			dLon[i] += 360.0;
		}
		if (dLon[i] >= 360.0) {
			dLon[i] -= 360.0;
		}
		if (dLat[i] <= -90.0) {
			dLat[i] += 1.0e-7;
		}
		if (dLat[i] >= 90.0) {
			dLat[i] -= 1.0e-7;
		}

		m_quadtree.insert(dLon[i], dLat[i], i);

		if ((i+1) % iReportSize == 0) {
			Announce("%li%% complete", i / iReportSize);
		}
	}

	AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingQuadTree::Sample(
	const DataArray1D<double> & dSampleLon,
	const DataArray1D<double> & dSampleLat,
	DataArray1D<int> & dImageMap
) const {
	//long lTotal = 0;
	//long lReportSize = static_cast<long>(dSampleLat.GetRows()) * static_cast<long>(dSampleLon.GetRows()) / 100;
	//AnnounceStartBlock("Querying data points within the quadtree");

	dImageMap.Allocate(dSampleLon.GetRows() * dSampleLat.GetRows());

	int s = 0;
	for (int j = 0; j < dSampleLat.GetRows(); j++) {
	for (int i = 0; i < dSampleLon.GetRows(); i++) {
		size_t sI = m_quadtree.find_inexact(dSampleLon[i], dSampleLat[j]);

		if (sI == static_cast<size_t>(-1)) {
			dImageMap[s] = 0;//_EXCEPTION();
		} else {
			dImageMap[s] = sI;
		}

		//lTotal++;
		//if (lTotal % lReportSize == 0) {
		//	Announce("%li%% complete", lTotal / lReportSize);
		//}

		s++;
	}
	}
	//AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingKDTree::Initialize(
	const std::string & strInputData
) {
	std::string strLonName("lon");
	std::string strLatName("lat");

	// Create a kdtree
	m_kdtree = kd_create(3);
	if (m_kdtree == NULL) {
		_EXCEPTIONT("kd_create(3) failed");
	}

	// Load the data file, if provided
	NcFile * pncdata = NULL;
	if (strInputData.length() != 0) {
		pncdata = new NcFile(strInputData.c_str());
		if (!pncdata->is_valid()) {
			_EXCEPTION1("Unable to open data file \"%s\"", strInputData.c_str());
		}
	}

	// Generate quadtree from datafile
	_ASSERT(pncdata != NULL);

	AnnounceStartBlock("Generating kdtree from datafile");

	NcVar * varLon = pncdata->get_var(strLonName.c_str());
	if (varLon == NULL) {
		_EXCEPTION2("Unable to read longitude variable \"%s\" from data file \"%s\"",
			strLonName.c_str(), strInputData.c_str());
	}
	if (varLon->num_dims() != 1) {
		_EXCEPTION2("Longitude variable \"%s\" in data file \"%s\" must have dimension 1",
			strLonName.c_str(), strInputData.c_str());
	}

	NcVar * varLat = pncdata->get_var(strLatName.c_str());
	if (varLon == NULL) {
		_EXCEPTION2("Unable to read latitude variable \"%s\" from data file \"%s\"",
			strLatName.c_str(), strInputData.c_str());
	}
	if (varLat->num_dims() != 1) {
		_EXCEPTION2("Latitude variable \"%s\" in data file \"%s\" must have dimension 1",
			strLatName.c_str(), strInputData.c_str());
	}

	if (varLon->get_dim(0)->size() != varLat->get_dim(0)->size()) {
		_EXCEPTION4("Longitude variable \"%s\" (size %li) and latitude variable \"%s\" (size %li) must have same size",
			varLon->name(), varLon->get_dim(0)->size(),
			varLat->name(), varLat->get_dim(0)->size());
	}

	DataArray1D<double> dLon(varLon->get_dim(0)->size());
	DataArray1D<double> dLat(varLat->get_dim(0)->size());

	varLon->get(&(dLon[0]), varLon->get_dim(0)->size());
	varLat->get(&(dLat[0]), varLat->get_dim(0)->size());

	long iReportSize = varLon->get_dim(0)->size() / 100;
	for (long i = 0; i < varLon->get_dim(0)->size(); i++) {

		double dX;
		double dY;
		double dZ;

		RLLtoXYZ_Deg(dLon[i], dLat[i], dX, dY, dZ);

		kd_insert3(m_kdtree, dX, dY, dZ, (void*)(&m_iRef + i));

		if ((i+1) % iReportSize == 0) {
			Announce("%li%% complete", i / iReportSize);
		}
	}

	AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

void GridDataSamplerUsingKDTree::Sample(
	const DataArray1D<double> & dSampleLon,
	const DataArray1D<double> & dSampleLat,
	DataArray1D<int> & dImageMap
) const {

	//long lTotal = 0;
	//long lReportSize = static_cast<long>(dSampleLat.GetRows()) * static_cast<long>(dSampleLon.GetRows()) / 100;
	//AnnounceStartBlock("Querying data points within the kdtree");

	dImageMap.Allocate(dSampleLon.GetRows() * dSampleLat.GetRows());

	int s = 0;
	for (int j = 0; j < dSampleLat.GetRows(); j++) {
	for (int i = 0; i < dSampleLon.GetRows(); i++) {

		double dX;
		double dY;
		double dZ;

		RLLtoXYZ_Deg(dSampleLon[i], dSampleLat[j], dX, dY, dZ);

		kdres * kdres = kd_nearest3(m_kdtree, dX, dY, dZ);
		if (kdres == NULL) {
			_EXCEPTIONT("Unexpected query failure in kdtree");
		}

		dImageMap[s] = static_cast<size_t>(reinterpret_cast<int*>(kd_res_item_data(kdres)) - &m_iRef);

		//lTotal++;
		//if (lTotal % lReportSize == 0) {
		//	Announce("%li%% complete", lTotal / lReportSize);
		//}

		s++;
	}
	}
	//AnnounceEndBlock("Done");
}

///////////////////////////////////////////////////////////////////////////////

