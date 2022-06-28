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
	const DataArray1D<double> & dLon,
	const DataArray1D<double> & dLat
) {
	_ASSERT(dLon.GetRows() == dLat.GetRows());

	AnnounceStartBlock("Generating quadtree from lat/lon arrays");

	long iReportSize = static_cast<long>(dLon.GetRows()) / 100;
	for (long i = 0; i < dLon.GetRows(); i++) {

		double dLonStandard = LonDegToStandardRange(dLon[i]);

		m_quadtree.insert(dLonStandard, dLat[i], i);

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
	const DataArray1D<double> & dLon,
	const DataArray1D<double> & dLat
) {
	AnnounceStartBlock("Generating kdtree from from lat/lon arrays");

	long iReportSize = static_cast<long>(dLon.GetRows()) / 100;
	for (long i = 0; i < static_cast<long>(dLon.GetRows()); i++) {

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

