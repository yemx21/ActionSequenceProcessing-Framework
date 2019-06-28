#include "Clustering.h"
#include <ppl.h>
#include <numeric>
#include <string>
using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::timeseries;
using namespace concurrency;

constexpr float SQRT2 = 1.414213562373095048801688724209698078;

DTWCluster::DTWCache::~DTWCache()
{
	if (Distances) { delete Distances; Distances = nullptr; }
	if (PathCosts) { delete PathCosts; PathCosts = nullptr; }
	if (Step1) { delete Step1; Step1 = nullptr; }
	if (Step2) { delete Step2; Step2 = nullptr; }
}

DTWCluster::DTWCache::DTWCache(int len)
{
	Length = len;
	int upper = Length + 1;

	Distances = new Table<datatype>(upper);
	PathCosts = new Table<datatype>(upper);
	Step1 = new Table<int>(upper);
	Step2 = new Table<int>(upper);
}

void DTWCluster::DTWCache::Reset()
{
	datatype* address_PathCosts = PathCosts->At(Length);
	for (size_t i = 0; i <= Length; i++)
	{
		address_PathCosts[i] = std::numeric_limits<datatype>::infinity();
	}

	for (size_t i = 0; i <= Length; i++)
	{
		PathCosts->At(i, Length) = std::numeric_limits<datatype>::infinity();
	}

	Distances->Clear();
	Step1->Clear();
	Step2->Clear();
}

#pragma optimize("", off)
__declspec(noinline)
void Spin(unsigned int work)
{
	for (unsigned long long i = 0; i < work*work*work; ++i);
}
#pragma optimize("",on)

class SortFunc
{
public:
	explicit SortFunc(unsigned int work = 0)
		:_work(work)
	{
	}

	bool operator()(datatype left, datatype right) const
	{
		Spin(_work);
		return left < right;
	}

private:
	const unsigned int _work;
};

bool RegionNearestAverageDistance(size_t count, int index, const std::vector<std::vector<datatype>>& distmap, int num, datatype& averdist)
{
	averdist = std::numeric_limits<datatype>::quiet_NaN();
	concurrency::concurrent_vector<datatype> sampledists(count - 1);
	const std::vector<datatype>& distmap_selected = distmap[index];
	for (size_t i = 0, n = 0; i < count; i++)
	{
		if (i == index) continue;
		sampledists[n++] = distmap_selected[i];
	}

	concurrency::parallel_sort(sampledists.begin(), sampledists.end(), SortFunc(10));

	int sumcount = 0;
	double sum = 0.0;
	for (size_t i = 0; i < count - 1; i++)
	{
		sum += sampledists[i];
		sumcount++;
		if (sumcount >= num) break;
	}
	if (count == 0) return false;
	averdist = sum / sumcount;
	return true;
}

bool RegionNearestAverageDistance1(size_t count, int index, const std::vector<std::vector<datatype>>& distmap, int num, int sparse, datatype& averdist, int& avercc)
{
	averdist = std::numeric_limits<datatype>::quiet_NaN();
	avercc = 0;
	concurrency::concurrent_vector<datatype> sampledists(count - 1);
	const std::vector<datatype>& distmap_selected = distmap[index];
	for (size_t i = 0, n = 0; i < count; i++)
	{
		if (i == index) continue;
		sampledists[n++] = distmap_selected[i];
	}

	if (sampledists.empty()) return false;

	concurrency::parallel_sort(sampledists.begin(), sampledists.end(), SortFunc(10));

	auto minmax = std::minmax_element(sampledists.begin(), sampledists.end());

	datatype bmin = *minmax.first;
	datatype bmax = *minmax.second;

	datatype cmin = bmin + (bmax - bmin) * 0.012;
	datatype cmax = bmax - (bmax - bmin) * 0.012;

	datatype step = (cmax - cmin) / sparse;

	std::map<int, std::vector<datatype>> hists;
	std::vector<datatype> finalsampledist;
	for (datatype d : sampledists)
	{
		if (d < cmin) continue;
		if (d > cmax) continue;
		int dloc = (d - bmin) / step;
		hists[dloc].push_back(d);
	}

	for (const auto& dp : hists)
	{
		finalsampledist.push_back(dp.second.empty() ? 0.0f : std::accumulate(dp.second.begin(), dp.second.end(), 0.0f) / dp.second.size());
	}

	std::sort(finalsampledist.begin(), finalsampledist.end());

	int fscount = (int)finalsampledist.size();

	int sumcount = 0;
	double sum = 0.0;
	for (size_t i = 0; i < fscount; i++)
	{
		sum += finalsampledist[i];
		sumcount++;
		if (sumcount >= num) break;
	}
	if (sumcount == 0) return false;
	averdist = sum / sumcount;
	avercc = sumcount;
	return true;
}

bool ComputeEpsMinPtS(size_t count, const std::vector<std::vector<datatype>>& distmap, int order, double& epsilon, int& minPts, int sparse)
{
	std::vector<datatype> averdists;
	std::vector<int> averccs;
	averdists.resize(order - 2);
	averccs.resize(order - 2);
	for (int o = 3; o <= order; o++)
	{
		concurrency::concurrent_vector<datatype> taverdists;
		taverdists.reserve(count);
		concurrency::concurrent_vector<int> tavercc;
		taverdists.reserve(count);
		#pragma omp parallel for num_threads(Runtime::CpuJobCount())
		for (int i = 0; i < count; i++)
		{
			datatype averdist = std::numeric_limits<datatype>::quiet_NaN();
			int avercc = 0;
			if (RegionNearestAverageDistance1(count, i, distmap, o, sparse, averdist, avercc))
			{
				taverdists.push_back(averdist);
				tavercc.push_back(avercc);
			}
		}
		averdists[o-3] = (taverdists.size() ? std::accumulate(taverdists.begin(), taverdists.end(), 0.0f) / taverdists.size() : 0.0);
		averccs[o-3] = (tavercc.size() ? std::accumulate(tavercc.begin(), tavercc.end(), 0.0f) / tavercc.size() : o);
	}

	std::vector<int> curve_order;
	std::vector<datatype> curve_eps;
	size_t nonzeroeps = 0;
	for (int o = 3, n = 0 , oi = 0; o <= order; o++, n++, oi++)
	{
		datatype eps = averdists[n];
		if (std::isnan(eps)) continue;
		if (eps > 0.0) nonzeroeps++;
		curve_order.push_back(averccs[oi]);
		curve_eps.push_back(eps);
		
	}

	if (nonzeroeps > 1)
	{
		try
		{
			epsilon = std::numeric_limits<datatype>::max();
			minPts = 3;
			for (size_t i = 0; i<curve_eps.size(); i++)
			{
				if (curve_eps[i] < epsilon)
				{
					epsilon = curve_eps[i];
					minPts = curve_order[i];
				}
			}
			return true;
		}
		catch (...)
		{

		}
	}
	else if (nonzeroeps == 1)
	{
		epsilon = curve_eps[0];
		minPts = curve_order[0];
		return true;
	}
	epsilon = 0.1;
	minPts = 3;
	return false;
}

void ExpandCluster(size_t count, int index, const std::vector<std::vector<datatype>>& distmap, std::vector<bool>& isvisited, std::vector<int>& clusterids, std::vector<int>& neighborPts, int clusterId, double epsilon, int minPts)
{
	clusterids[index] = clusterId;
	for (int i = 0; i < neighborPts.size(); i++)
	{
		int in = neighborPts[i];
		if (!isvisited[in])
		{
			isvisited[in] = true;
			std::unordered_set<int> neighborPts2;
			for (size_t n = 0; n < count; n++)
			{
				if (distmap[in][n] <= epsilon)
					neighborPts2.insert(n);
			}
			if (neighborPts2.size() >= minPts)
			{
				for (int n : neighborPts2)
				{
					if (std::find(neighborPts.begin(), neighborPts.end(), n) != neighborPts.end()) continue;
					neighborPts.push_back(n);
				}
			}
		}
		if (clusterids[in] == 0)
			clusterids[in] = clusterId;
	}
}

_inline void Detrend(const datatype* data, int len, std::vector<datatype>& output)
{
	double step = (data[len - 1] - data[0]) / len;
	output.resize(len);
	for (int i = 0; i < len; i++)
	{
		output[i] = data[i] - step * i;
	}
}

size_t IndexOfMax(const std::vector<datatype>& input)
{
	size_t maxIndex = -1;
	datatype maxValue = input[0];

	for (size_t i = 0; i < input.size(); i++)
	{
		if (input[i] >= maxValue)
		{
			maxIndex = i;
			maxValue = input[i];
		}
	}

	return maxIndex;
}

size_t IndexOfMin(const std::vector<datatype>& input)
{
	size_t minIndex = -1;
	datatype minValue = input[0];

	for (size_t i = 0; i < input.size(); i++)
	{
		if (input[i] <= minValue)
		{
			minIndex = i;
			minValue = input[i];
		}
	}

	return minIndex;
}

datatype Median(const std::vector<size_t>& input)
{
	int midIndex = input.size() / 2;

	std::vector<size_t> input_copy = input;
	std::sort(input_copy.begin(), input_copy.end());

	return input.size() % 2 == 0
		? (input_copy[midIndex] + input_copy[midIndex - 1]) / 2
		: input_copy[midIndex];
}

datatype Median(const std::vector<datatype>& input)
{
	int midIndex = input.size() / 2;

	std::vector<datatype> input_copy = input;
	std::sort(input_copy.begin(), input_copy.end());

	return input.size() % 2 == 0
		? (input_copy[midIndex] + input_copy[midIndex - 1]) / 2
		: input_copy[midIndex];
}

datatype DTWCluster::DTWCost(DTWCache* cache, const datatype* data1, const int* labels1, const datatype* data2, const int* labels2, const std::map<int, std::map<int, float>>& labeldistances)
{
	int len = cache->Length;
	cache->Reset();

	for (size_t i = 0; i < len; i++)
	{
		datatype* currentDistances = cache->Distances->At(i);
		auto xVal = data1[i];
		auto xLabel = labels1[i];
		for (size_t j = 0; j < len; j++)
		{
			datatype dist = (xVal - data2[j]) * labeldistances.at(xLabel).at(labels2[j]);
			currentDistances[j] += dist * dist;
		}
	}

	for (int i = 0; i < len; i++)
	{
		datatype* currentDistances = cache->Distances->At(i);
		for (int j = 0; j < len; j++)
			currentDistances[j] = sqrt(currentDistances[j]);
	}

	for (int i = len - 1; i >= 0; i--)
	{
		datatype* currentRowDistances = cache->Distances->At(i);
		datatype* currentRowPathCost = cache->PathCosts->At(i);
		datatype* previousRowPathCost = cache->PathCosts->At(i + 1);

		int* currentRowPredecessorStepX = cache->Step1->At(i);
		int* currentRowPredecessorStepY = cache->Step2->At(i);

		for (int j = len - 1; j >= 0; j--)
		{
			datatype diagonalNeighbourCost = previousRowPathCost[j + 1];
			datatype xNeighbourCost = previousRowPathCost[j];
			datatype yNeighbourCost = currentRowPathCost[j + 1];

			if (std::isinf(diagonalNeighbourCost) && (i == j))
				currentRowPathCost[j] = currentRowDistances[j];
			else
			{
				if (diagonalNeighbourCost <= xNeighbourCost && diagonalNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = diagonalNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 1;
				}
				else if (xNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = xNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 0;
				}
				else
				{
					currentRowPathCost[j] = yNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 0;
					currentRowPredecessorStepY[j] = 1;
				}
			}
		}
	}

	datatype cost = *cache->PathCosts->AddressOf();
	if (std::isnan(cost)) return 0.0;
	return cost / len / SQRT2;
}

datatype DTWCluster::DTWPath(DTWCache* cache, const datatype* data1, const int* labels1, const datatype* data2, const int* labels2, const std::map<int, std::map<int, float>>& labeldistances, std::vector<std::tuple<int, int>>& paths)
{
	paths.clear();
	int len = cache->Length;
	cache->Reset();

	for (size_t i = 0; i < len; i++)
	{
		datatype* currentDistances = cache->Distances->At(i);
		auto xVal = data1[i];
		auto xLabel = labels1[i];
		for (size_t j = 0; j < len; j++)
		{
			datatype dist = (xVal - data2[j]) * labeldistances.at(xLabel).at(labels2[j]);
			currentDistances[j] += dist * dist;
		}
	}

	for (int i = 0; i < len; i++)
	{
		datatype* currentDistances = cache->Distances->At(i);
		for (int j = 0; j < len; j++)
			currentDistances[j] = sqrtf(currentDistances[j]);
	}

	for (int i = len - 1; i >= 0; i--)
	{
		datatype* currentRowDistances = cache->Distances->At(i);
		datatype* currentRowPathCost = cache->PathCosts->At(i);
		datatype* previousRowPathCost = cache->PathCosts->At(i + 1);

		int* currentRowPredecessorStepX = cache->Step1->At(i);
		int* currentRowPredecessorStepY = cache->Step2->At(i);

		for (int j = len - 1; j >= 0; j--)
		{
			datatype diagonalNeighbourCost = previousRowPathCost[j + 1];
			datatype xNeighbourCost = previousRowPathCost[j];
			datatype yNeighbourCost = currentRowPathCost[j + 1];

			if (std::isinf(diagonalNeighbourCost) && (i == j))
				currentRowPathCost[j] = currentRowDistances[j];
			else
			{
				if (diagonalNeighbourCost <= xNeighbourCost && diagonalNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = diagonalNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 1;
				}
				else if (xNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = xNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 0;
				}
				else
				{
					currentRowPathCost[j] = yNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 0;
					currentRowPredecessorStepY[j] = 1;
				}
			}
		}
	}

	int indexX = 0;
	int indexY = 0;

	paths.push_back(std::make_tuple(indexX, indexY));
	while (indexX < len - 1 || indexY < len - 1)
	{
		int stepX = cache->Step1->At(indexX, indexY);
		int stepY = cache->Step2->At(indexX, indexY);
		indexX += stepX;
		indexY += stepY;
		paths.push_back(std::make_tuple(indexX, indexY));
	}

	datatype cost = *cache->PathCosts->AddressOf();
	if (std::isnan(cost)) return 0.0;
	return cost / len / SQRT2;
}

datatype DTWCluster::DTWPath2(DTWCache* cache, const datatype* data1, const int* labels1, const std::vector<datatype>& sch2, const std::vector<std::map<int, float>>& schlab2, const std::map<int, std::map<int, float>>& labeldistances, float lamda, std::vector<std::tuple<int, int>>& paths)
{
	paths.clear();
	int len = cache->Length;
	cache->Reset();

	if (lamda >= 0.0)
	{
		for (size_t i = 0; i < len; i++)
		{
			datatype* currentDistances = cache->Distances->At(i);
			auto xVal = data1[i];
			auto xLab = labels1[i];
			for (size_t j = 0; j < len; j++)
			{
				if (schlab2[j].empty())
				{
					datatype dist = exp(-(xVal - sch2[j]) / lamda);
					datatype edist = dist * dist;
					currentDistances[j] += edist;
				}
				else
				{
					datatype labfactor = 0.0f;
					for (auto lf : schlab2[j])
					{
						labfactor += labeldistances.at(xLab).at(lf.first) * lf.second;
					}

					datatype dist = exp(-(xVal - sch2[j]) * labfactor / lamda);
					datatype edist = dist * dist;
					currentDistances[j] += edist;
				}
			}
		}
	}
	else
	{
		for (size_t i = 0; i < len; i++)
		{
			datatype* currentDistances = cache->Distances->At(i);
			auto xVal = data1[i];
			for (size_t j = 0; j < len; j++)
			{
				datatype dist = xVal - sch2[j];
				datatype edist = dist * dist;
				currentDistances[j] += edist;
			}
		}
	}

	for (int i = 0; i < len; i++)
	{
		datatype* currentDistances = cache->Distances->At(i);
		for (int j = 0; j < len; j++)
			currentDistances[j] = sqrtf(currentDistances[j]);
	}

	for (int i = len - 1; i >= 0; i--)
	{
		datatype* currentRowDistances = cache->Distances->At(i);
		datatype* currentRowPathCost = cache->PathCosts->At(i);
		datatype* previousRowPathCost = cache->PathCosts->At(i + 1);

		int* currentRowPredecessorStepX = cache->Step1->At(i);
		int* currentRowPredecessorStepY = cache->Step2->At(i);

		for (int j = len - 1; j >= 0; j--)
		{
			datatype diagonalNeighbourCost = previousRowPathCost[j + 1];
			datatype xNeighbourCost = previousRowPathCost[j];
			datatype yNeighbourCost = currentRowPathCost[j + 1];

			if (std::isinf(diagonalNeighbourCost) && (i == j))
				currentRowPathCost[j] = currentRowDistances[j];
			else
			{
				if (diagonalNeighbourCost <= xNeighbourCost && diagonalNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = diagonalNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 1;
				}
				else if (xNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = xNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 0;
				}
				else
				{
					currentRowPathCost[j] = yNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 0;
					currentRowPredecessorStepY[j] = 1;
				}
			}
		}
	}

	int indexX = 0;
	int indexY = 0;

	paths.push_back(std::make_tuple(indexX, indexY));
	while (indexX < len - 1 || indexY < len - 1)
	{
		int stepX = cache->Step1->At(indexX, indexY);
		int stepY = cache->Step2->At(indexX, indexY);
		indexX += stepX;
		indexY += stepY;
		paths.push_back(std::make_tuple(indexX, indexY));
	}

	datatype cost = *cache->PathCosts->AddressOf();
	if (std::isnan(cost)) return 0.0;
	return cost / len / SQRT2;
}

void DTWCluster::DBScan(DTWCache* cache, const datatype* data, const int* labels, int featnum, int len, int order, int sparse, const std::map<int, std::map<int, float>>& labeldistances, std::unordered_map<int, std::vector<int>>& clusters)
{
	std::vector<std::vector<datatype>> distmap;
	distmap.resize(len);
	for (size_t i = 0; i < len; i++) distmap[i].resize(len, -1.0f);

	for (size_t i = 0; i < len; i++)
	{
		distmap[i][i] = 0.0f;
		const datatype* data1 = data + i* featnum;
		const int* labels1 = labels + i* featnum;
		for (size_t j = i+1; j < len; j++)
		{ 		
			const datatype* data2 = data + j* featnum;
			const int* labels2 = labels + j* featnum;
			datatype cost = DTWCost(cache, data1, labels1, data2, labels2, labeldistances);
			if (distmap[i][j] < 0.0f) distmap[i][j] = cost;
			if (distmap[j][i] < 0.0f) distmap[j][i] = cost;
		}
	}

	double eps = 0.1;
	int minpts = 3;
	if (!ComputeEpsMinPtS(len, distmap, 5, eps, minpts, sparse))
	{
		std::vector<int> sampleindexs0(len);;
		for (size_t i = 0; i< len; i++) sampleindexs0[i] = i;
		clusters.insert(std::make_pair(2, sampleindexs0));
		return;
	}

	std::vector<bool> isvisited;
	std::vector<int> clusterids;
	isvisited.resize(len, false);
	clusterids.resize(len, 0);

	int clusterId = 0;
	for (size_t i = 0; i < len; i++)
	{
		if (isvisited[i]) continue;
		isvisited[i] = true;

		std::vector<int> neighborPts;
		for (size_t n = 0; n < len; n++)
		{
			if (distmap[i][n] <= eps) neighborPts.push_back(n);
		}

		if (neighborPts.size() < minpts)
			clusterids[i] = -1;
		else
		{
			clusterId++;
			ExpandCluster(len, i, distmap, isvisited, clusterids, neighborPts, clusterId, eps, minpts);
		}
	}

	for (int i = 0; i < len; i++)
	{
		int cid = clusterids[i];
		if (cid > 0)
		{
			if (clusters.find(cid) == clusters.end()) clusters.insert(std::make_pair(cid, std::vector<int>{}));
			clusters[cid].push_back(i);
		}
	}

	for (auto cliter = clusters.begin(); cliter != clusters.end();)
	{
		if (cliter->second.size() <= minpts)
			cliter = clusters.erase(cliter);
		else
			cliter++;
	}

	if (clusters.size() == 1)
	{
		if (clusters.begin()->second.size() <= minpts)
		{
			std::vector<int> sampleindexs0(len);
			for (size_t i = 0; i< len; i++) sampleindexs0[i] = i;
			clusters.insert(std::make_pair(2, sampleindexs0));
		}
	}
	else if (clusters.size() == 0)
	{
		std::vector<int> sampleindexs0(len);
		for (size_t i = 0; i< len; i++) sampleindexs0[i] = i;
		clusters.insert(std::make_pair(2, sampleindexs0));
	}
}

void DTWCluster::DBA(DTWCache* cache, const datatype* data, const int* labels, int featnum, const std::vector<int>& indexs, const std::map<int, std::map<int, float>>& labeldistances, float lamda, size_t maxiterations, BufferPtr outdata, std::shared_ptr<GenericBuffer<int>> outlabels)
{
	if (indexs.size() == 1)
	{
		outdata->Reshape(featnum, 1);
		memcpy(outdata->GetCpu(), data, sizeof(datatype) * featnum);
		outlabels->Reshape(featnum, 1);
		memcpy(outlabels->GetCpu(), labels, sizeof(int) * featnum);
		return;
	}

	std::vector<size_t> minindexes;
	std::vector<size_t> maxindexes;
	std::vector<datatype> tempsample;

	for (int idx : indexs)
	{
		Detrend(data + idx * featnum, featnum, tempsample);
		minindexes.push_back(IndexOfMin(tempsample));
		maxindexes.push_back(IndexOfMax(tempsample));
	}

	datatype medianMaxIndex = Median(maxindexes);
	datatype medianMinIndex = Median(minindexes);

	std::vector<datatype> distances;
	for (size_t i = 0; i < minindexes.size(); i++)
	{
		distances.push_back(pow(maxindexes[i] - medianMaxIndex, 2.0) + pow(minindexes[i] - medianMinIndex, 2.0));
	}
	size_t selectedindex = IndexOfMin(distances);

	std::vector<datatype> average;
	average.resize(featnum);
	memcpy(average.data(), data + indexs[selectedindex] * featnum, sizeof(datatype) * featnum);
	std::vector<std::map<int, float>> averagelabels(featnum);
	const int* averagelabels0 = labels + indexs[selectedindex] * featnum;
	for (size_t i = 0; i < featnum; i++) averagelabels[i].insert(std::make_pair(averagelabels0[i], 1.0f));

	std::vector<std::vector<datatype>> points(featnum);
	std::vector<std::vector<int>> paths(featnum);

	std::vector<std::tuple<int, int, datatype>> ref_paths;

	double prevTotalDist = -1;
	double totalDist = -2;
	int count = 0;

	while (totalDist != prevTotalDist && count < maxiterations)
	{
		prevTotalDist = totalDist;

		for (size_t i = 0; i < featnum; i++)
		{
			points[i].clear();
			paths[i].clear();
		}

		std::vector<std::tuple<int, int>> tmp_paths;
		for (int idx : indexs)
		{
			const datatype* sc = data + idx * featnum;
			const int* sl = labels + idx * featnum;
			const datatype* svals = sc;
			const int* slabs = sl;
			datatype ncost = DTWPath2(cache, sc, sl, average, averagelabels, labeldistances, lamda, tmp_paths);

			for (auto it : tmp_paths)
			{
				points[std::get<1>(it)].push_back(svals[std::get<0>(it)]);
				paths[std::get<1>(it)].push_back(slabs[std::get<0>(it)]);
				ref_paths.push_back(std::make_tuple(std::get<1>(it), sl[std::get<0>(it)], ncost));
			}
		}

		for (size_t i = 0; i < featnum; i++)
		{
			const std::vector<datatype>& ps = points[i];
			average[i] = std::accumulate(ps.begin(), ps.end(), 0.0f) / ps.size();

			averagelabels[i].clear();
			const std::vector<int>& ls = paths[i];
			float lscount = (float)ls.size();
			for (int ils : ls)
			{
				if (averagelabels[i].find(ils) == averagelabels[i].end())
					averagelabels[i].insert(std::make_pair(ils, 1.0f));
				else
					averagelabels[i][ils] += 1.0f;
			}

			for (auto& xls : averagelabels[i])
			{
				xls.second /= lscount;
			}
		}

		totalDist = 0.0;

		for (int idx : indexs)
		{
			const datatype* svals = data + idx * featnum;
			const int* slabs = labels + idx *featnum;

			for (size_t i = 0; i < featnum; i++)
			{
				auto xval = svals[i];
				auto xlab = slabs[i];
				auto yval = average[i];
				const std::map<int, float> ylabs = averagelabels[i];
				if (averagelabels[i].empty())
				{
					datatype dist = (xval - yval);
					datatype edist = dist * dist;
					totalDist += edist;
				}
				else
				{
					datatype labfactor = 0.0f;
					for (const auto& yl : ylabs)
					{
						labfactor += labeldistances.at(xlab).at(yl.first) * yl.second;
					}

					datatype dist = (xval - yval) * labfactor;
					datatype edist = dist * dist;
					totalDist += edist;
				}


				//totalDist += pow(svals[i] - average[i], 2.0);
			}
		}
		count++;
	}

	outdata->Reshape(featnum, 1);
	memcpy(outdata->GetCpu(), average.data(), sizeof(datatype) * featnum);

	size_t ref_pathcount = ref_paths.size();
	datatype ref_path_mincost = std::numeric_limits<datatype>::max();
	datatype ref_path_maxcost = std::numeric_limits<datatype>::min();
	for (size_t i = 0; i<ref_pathcount; i++)
	{
		datatype pcost = std::get<2>(ref_paths[i]);
		if (pcost < ref_path_mincost) ref_path_mincost = pcost;
		if (pcost > ref_path_maxcost) ref_path_maxcost = pcost;
	}
	datatype ref_path_dcost = ref_path_maxcost - ref_path_mincost;

	std::vector<std::map<int, float>> probabilities;
	probabilities.resize(featnum);
	outlabels->Reshape(featnum, 1);
	auto outlabelscpu = outlabels->GetCpu();
	for (int nn = 0; nn < featnum; nn++)
	{
		double total_ncost = 0.0;
		std::unordered_map<int, std::vector<double>> votes;
		for (auto np : ref_paths)
		{
			int n2 = std::get<1>(np);
			if (std::get<0>(np) == nn && n2 != -1)
			{
				datatype npcost = std::get<2>(np);
				if (votes.find(n2) == votes.end())
				{
					std::vector<double> vote_p;
					datatype ncost = (ref_path_maxcost - npcost) / ref_path_dcost;
					if (ncost < 0.0) ncost = 0.0;
					else if (ncost > 1.0) ncost = 1.0;
					vote_p.push_back(ncost);
					votes.insert(std::make_pair(n2, vote_p));
					total_ncost += ncost;
				}
				else
				{
					datatype ncost = (ref_path_maxcost - npcost) / ref_path_dcost;
					if (ncost < 0.0) ncost = 0.0;
					else if (ncost > 1.0) ncost = 1.0;
					votes[n2].push_back(ncost);
					total_ncost += ncost;
				}
			}
		}

		probabilities[nn].clear();
		for (auto vv : votes)
		{
			if (total_ncost == 0.0)
				probabilities[nn].insert(std::make_pair(vv.first, 0.0));
			else
			{
				float sum = std::accumulate(vv.second.begin(), vv.second.end(), 0.0);
				probabilities[nn].insert(std::make_pair(vv.first, sum));
			}
		}

		float maxprob = 0.0f;
		outlabelscpu[nn] = -1;

		for (const auto& pp : probabilities[nn])
		{
			if (pp.second > maxprob)
			{
				outlabelscpu[nn] = pp.first;
				maxprob = pp.second;
			}
		}
	}
}

void DTWCluster::ClusteringLP(const datatype* data, const int* labels, int featnum, int num, float lamda, const int* reflabels, int reflabelnum, float labeldistancebaseterm, int sparse, BufferPtr outdata, std::shared_ptr<GenericBuffer<int>> outlabels)
{
	std::map<int, std::map<int, float>> labeldists;
	for (int i = 0; i < reflabelnum; i++)
	{
		for (int j = 0; j < reflabelnum; j++)
		{
			int labdif = abs(reflabels[i] - reflabels[j]);
			int labcut = (int)floor(reflabelnum / 2);
			int labfinal = labdif > labcut ? reflabelnum - labdif : labdif;	
			labeldists[reflabels[i]][reflabels[j]] = pow(labeldistancebaseterm, labfinal);
		}
	}

	std::vector<BufferPtr> outdatabufs{};
	std::vector<std::shared_ptr<GenericBuffer<int>>> outlabelbufs{};

	if (num > 1)
	{
		DTWCache* dtwcache = new DTWCache(featnum);
		std::unordered_map<int, std::vector<int>> clusters;
		DTWCluster::DBScan(dtwcache, data, labels, featnum, num, 5, sparse, labeldists, clusters);

		for (auto hs : clusters)
		{
			BufferPtr todbuf = std::make_shared<Buffer>();
			std::shared_ptr<GenericBuffer<int>> tolbuf = std::make_shared<GenericBuffer<int>>();

			DBA(dtwcache, data, labels, featnum, hs.second, labeldists, lamda, 100, todbuf, tolbuf);

			outdatabufs.push_back(todbuf);
			outlabelbufs.push_back(tolbuf);
		}

		delete dtwcache;
		dtwcache = nullptr;
	}
	else
	{
		BufferPtr todbuf = std::make_shared<Buffer>(featnum);
		std::shared_ptr<GenericBuffer<int>> tolbuf = std::make_shared<GenericBuffer<int>>(featnum);

		memcpy(todbuf->GetCpu(), data, sizeof(datatype) * featnum);
		memcpy(tolbuf->GetCpu(), labels, sizeof(int) * featnum);

		outdatabufs.push_back(todbuf);
		outlabelbufs.push_back(tolbuf);
	}

	int clusternum = (int)outdatabufs.size();
	outdata->Reshape(featnum, clusternum);
	outlabels->Reshape(featnum, clusternum);

	datatype* outdatacpu = outdata->GetCpu();
	int* outlabelcpu = outlabels->GetCpu();

	for (int i = 0; i < clusternum; i++)
	{
		memcpy(outdatacpu, outdatabufs[i]->GetCpu(), sizeof(datatype)* featnum);
		memcpy(outlabelcpu, outlabelbufs[i]->GetCpu(), sizeof(int)* featnum);
		outdatacpu += featnum;
		outlabelcpu += featnum;
	}
}

datatype DTWCluster::DTWCost0(DTWCache* cache, const datatype* data1, const datatype* data2)
{
	int len = cache->Length;
	cache->Reset();

	for (int i = 0; i < len; i++)
	{
		datatype* currentDistances = cache->Distances->At(i);
		auto xVal = data1[i];
		for (size_t j = 0; j < len; j++)
		{
			currentDistances[j] = abs(xVal - data2[j]);
		}
	}

	for (int i = len - 1; i >= 0; i--)
	{
		datatype* currentRowDistances = cache->Distances->At(i);
		datatype* currentRowPathCost = cache->PathCosts->At(i);
		datatype* previousRowPathCost = cache->PathCosts->At(i + 1);

		int* currentRowPredecessorStepX = cache->Step1->At(i);
		int* currentRowPredecessorStepY = cache->Step2->At(i);

		for (int j = len - 1; j >= 0; j--)
		{
			datatype diagonalNeighbourCost = previousRowPathCost[j + 1];
			datatype xNeighbourCost = previousRowPathCost[j];
			datatype yNeighbourCost = currentRowPathCost[j + 1];

			if (std::isinf(diagonalNeighbourCost) && (i == j))
				currentRowPathCost[j] = currentRowDistances[j];
			else
			{
				if (diagonalNeighbourCost <= xNeighbourCost && diagonalNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = diagonalNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 1;
				}
				else if (xNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = xNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 0;
				}
				else
				{
					currentRowPathCost[j] = yNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 0;
					currentRowPredecessorStepY[j] = 1;
				}
			}
		}
	}

	datatype cost = *cache->PathCosts->AddressOf();
	if (std::isnan(cost)) return 0.0;
	return cost / len / SQRT2;
}

datatype DTWCluster::DTWPath0(DTWCache* cache, const datatype* data1, const std::vector<datatype>& sch2, float lamda, std::vector<std::tuple<int, int>>& paths)
{
	paths.clear();
	int len = cache->Length;
	cache->Reset();

	if (lamda >= 0.0)
	{
		for (size_t i = 0; i < len; i++)
		{
			datatype* currentDistances = cache->Distances->At(i);
			auto xVal = data1[i];
			for (size_t j = 0; j < len; j++)
			{
				datatype dist = exp(-(xVal - sch2[j]) / lamda);
				datatype edist = dist * dist;
				currentDistances[j] += edist;
			}
		}
	}
	else
	{
		for (size_t i = 0; i < len; i++)
		{
			datatype* currentDistances = cache->Distances->At(i);
			auto xVal = data1[i];
			for (size_t j = 0; j < len; j++)
			{
				datatype dist = xVal - sch2[j];
				datatype edist = dist * dist;
				currentDistances[j] += edist;
			}
		}
	}

	for (int i = 0; i < len; i++)
	{
		datatype* currentDistances = cache->Distances->At(i);
		for (int j = 0; j < len; j++)
			currentDistances[j] = sqrtf(currentDistances[j]);
	}

	for (int i = len - 1; i >= 0; i--)
	{
		datatype* currentRowDistances = cache->Distances->At(i);
		datatype* currentRowPathCost = cache->PathCosts->At(i);
		datatype* previousRowPathCost = cache->PathCosts->At(i + 1);

		int* currentRowPredecessorStepX = cache->Step1->At(i);
		int* currentRowPredecessorStepY = cache->Step2->At(i);

		for (int j = len - 1; j >= 0; j--)
		{
			datatype diagonalNeighbourCost = previousRowPathCost[j + 1];
			datatype xNeighbourCost = previousRowPathCost[j];
			datatype yNeighbourCost = currentRowPathCost[j + 1];

			if (std::isinf(diagonalNeighbourCost) && (i == j))
				currentRowPathCost[j] = currentRowDistances[j];
			else
			{
				if (diagonalNeighbourCost <= xNeighbourCost && diagonalNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = diagonalNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 1;
				}
				else if (xNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = xNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 0;
				}
				else
				{
					currentRowPathCost[j] = yNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 0;
					currentRowPredecessorStepY[j] = 1;
				}
			}
		}
	}

	int indexX = 0;
	int indexY = 0;

	paths.push_back(std::make_tuple(indexX, indexY));
	while (indexX < len - 1 || indexY < len - 1)
	{
		int stepX = cache->Step1->At(indexX, indexY);
		int stepY = cache->Step2->At(indexX, indexY);
		indexX += stepX;
		indexY += stepY;
		paths.push_back(std::make_tuple(indexX, indexY));
	}

	datatype cost = *cache->PathCosts->AddressOf();
	if (std::isnan(cost)) return 0.0;
	return cost / len / SQRT2;
}

void DTWCluster::DBScan(DTWCache* cache, const datatype* data, const std::vector<int>& indices, int featnum, int len, int order, int sparse, std::unordered_map<int, std::vector<int>>& clusters)
{
	int sellen = (int)indices.size();

	std::vector<std::vector<datatype>> distmap;
	distmap.resize(sellen);
	for (size_t i = 0; i < sellen; i++) distmap[i].resize(sellen, -1.0f);

	#pragma omp parallel for num_threads(Runtime::CpuJobCount())
	for (int i = 0; i < sellen; i++)
	{
		distmap[i][i] = 0.0f;
		const datatype* data1 = data + indices[i]* featnum;
		for (size_t j = i+1; j < sellen; j++)
		{
			const datatype* data2 = data + indices[j]* featnum;
			datatype cost = DTWCost0(cache, data1, data2);
			if (distmap[i][j] < 0.0f) distmap[i][j] = cost;
			if (distmap[j][i] < 0.0f) distmap[j][i] = cost;
		}
	}

	double eps = 0.1;
	int minpts = 3;
	if (!ComputeEpsMinPtS(sellen, distmap, order, eps, minpts, sparse))
	{
		std::vector<int> sampleindexs0(sellen);;
		for (size_t i = 0; i< sellen; i++) sampleindexs0[i] = indices[i];
		clusters.insert(std::make_pair(2, sampleindexs0));
		return;
	}
	std::vector<bool> isvisited;
	std::vector<int> clusterids;
	isvisited.resize(sellen, false);
	clusterids.resize(sellen, 0);

	int clusterId = 0;
	for (size_t i = 0; i < sellen; i++)
	{
		if (isvisited[i]) continue;
		isvisited[i] = true;

		std::vector<int> neighborPts;
		for (size_t n = 0; n < sellen; n++)
		{
			if (distmap[i][n] <= eps) neighborPts.push_back(n);
		}

		if (neighborPts.size() < minpts)
			clusterids[i] = -1;
		else
		{
			clusterId++;
			ExpandCluster(sellen, i, distmap, isvisited, clusterids, neighborPts, clusterId, eps, minpts);
		}
	}

	for (int i = 0; i < sellen; i++)
	{
		int cid = clusterids[i];
		if (cid > 0)
		{
			if (clusters.find(cid) == clusters.end()) clusters.insert(std::make_pair(cid, std::vector<int>{}));
			clusters[cid].push_back(indices[i]);
		}
	}

	for (auto cliter = clusters.begin(); cliter != clusters.end();)
	{
		if (cliter->second.size() <= minpts)
			cliter = clusters.erase(cliter);
		else
			cliter++;
	}

	if (clusters.size() == 1)
	{
		if (clusters.begin()->second.size() <= minpts)
		{
			std::vector<int> sampleindexs0(sellen);
			for (size_t i = 0; i< sellen; i++) sampleindexs0[i] = indices[i];
			clusters.insert(std::make_pair(2, sampleindexs0));
		}
	}
	else if (clusters.size() == 0)
	{
		std::vector<int> sampleindexs0(sellen);
		for (size_t i = 0; i< sellen; i++) sampleindexs0[i] = indices[i];
		clusters.insert(std::make_pair(2, sampleindexs0));
	}
}

datatype DTWCluster::DTWPath0(DTWCache* cache, const datatype* data1, const std::vector<datatype>& sch2, std::vector<std::tuple<int, int>>& paths)
{
	paths.clear();
	int len = cache->Length;
	cache->Reset();
	
	for (size_t i = 0; i < len; i++)
	{
		datatype* currentDistances = cache->Distances->At(i);
		auto xVal = data1[i];
		for (size_t j = 0; j < len; j++)
		{
			currentDistances[j] += abs(xVal - sch2[j]);
		}
	}

	for (int i = len - 1; i >= 0; i--)
	{
		datatype* currentRowDistances = cache->Distances->At(i);
		datatype* currentRowPathCost = cache->PathCosts->At(i);
		datatype* previousRowPathCost = cache->PathCosts->At(i + 1);

		int* currentRowPredecessorStepX = cache->Step1->At(i);
		int* currentRowPredecessorStepY = cache->Step2->At(i);

		for (int j = len - 1; j >= 0; j--)
		{
			datatype diagonalNeighbourCost = previousRowPathCost[j + 1];
			datatype xNeighbourCost = previousRowPathCost[j];
			datatype yNeighbourCost = currentRowPathCost[j + 1];

			if (std::isinf(diagonalNeighbourCost) && (i == j))
				currentRowPathCost[j] = currentRowDistances[j];
			else
			{
				if (diagonalNeighbourCost <= xNeighbourCost && diagonalNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = diagonalNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 1;
				}
				else if (xNeighbourCost <= yNeighbourCost)
				{
					currentRowPathCost[j] = xNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 1;
					currentRowPredecessorStepY[j] = 0;
				}
				else
				{
					currentRowPathCost[j] = yNeighbourCost + currentRowDistances[j];
					currentRowPredecessorStepX[j] = 0;
					currentRowPredecessorStepY[j] = 1;
				}
			}
		}
	}

	int indexX = 0;
	int indexY = 0;

	paths.push_back(std::make_tuple(indexX, indexY));
	while (indexX < len - 1 || indexY < len - 1)
	{
		int stepX = cache->Step1->At(indexX, indexY);
		int stepY = cache->Step2->At(indexX, indexY);
		indexX += stepX;
		indexY += stepY;
		paths.push_back(std::make_tuple(indexX, indexY));
	}

	datatype cost = *cache->PathCosts->AddressOf();
	if (std::isnan(cost)) return 0.0;
	return cost / len / SQRT2;
}

void DTWCluster::DBA(DTWCache* cache, const datatype* data, int featnum, const std::vector<int>& indexs, size_t maxiterations, BufferPtr outdata)
{
	if (indexs.size() == 1)
	{
		outdata->Reshape(featnum, 1);
		memcpy(outdata->GetCpu(), data, sizeof(datatype) * featnum);
		return;
	}

	std::vector<size_t> minindexes;
	std::vector<size_t> maxindexes;
	std::vector<datatype> tempsample;

	for (int idx : indexs)
	{
		Detrend(data + idx * featnum, featnum, tempsample);
		minindexes.push_back(IndexOfMin(tempsample));
		maxindexes.push_back(IndexOfMax(tempsample));
	}

	datatype medianMaxIndex = Median(maxindexes);
	datatype medianMinIndex = Median(minindexes);

	std::vector<datatype> distances;
	for (size_t i = 0; i < minindexes.size(); i++)
	{
		distances.push_back(pow(maxindexes[i] - medianMaxIndex, 2.0) + pow(minindexes[i] - medianMinIndex, 2.0));
	}
	size_t selectedindex = IndexOfMin(distances);

	std::vector<datatype> average;
	average.resize(featnum);
	memcpy(average.data(), data + indexs[selectedindex] * featnum, sizeof(datatype) * featnum);

	std::vector<std::vector<datatype>> points(featnum);

	double prevTotalDist = -1;
	double totalDist = -2;
	int count = 0;

	while (totalDist != prevTotalDist && count < maxiterations)
	{
		prevTotalDist = totalDist;

		for (size_t i = 0; i < featnum; i++)
		{
			points[i].clear();
		}

		std::vector<std::tuple<int, int>> tmp_paths;
		for (int idx : indexs)
		{
			const datatype* sc = data + idx * featnum;
			const datatype* svals = sc;
			datatype ncost = DTWPath0(cache, sc, average, tmp_paths);

			for (auto it : tmp_paths)
			{
				points[std::get<1>(it)].push_back(svals[std::get<0>(it)]);
			}
		}

		for (size_t i = 0; i < featnum; i++)
		{
			const std::vector<datatype>& ps = points[i];
			average[i] = std::accumulate(ps.begin(), ps.end(), 0.0f) / ps.size();
		}

		totalDist = 0.0;

		for (int idx : indexs)
		{
			const datatype* svals = data + idx * featnum;

			for (size_t i = 0; i < featnum; i++)
			{
				auto xval = svals[i];
				auto yval = average[i];
				datatype dist = (xval - yval);
				datatype edist = dist * dist;
				totalDist += edist;
			}
		}
		count++;
	}

	outdata->Reshape(featnum, 1);
	memcpy(outdata->GetCpu(), average.data(), sizeof(datatype) * featnum);
}

void DTWCluster::ClusteringS(const datatype* data, const int* labels, int featnum, int num, int sparse, BufferPtr outdata, std::shared_ptr<GenericBuffer<int>> outlabels, int dbscan_order)
{
	int dbscn_ord = 5;
	if (dbscan_order != -1) dbscn_ord = dbscan_order;
	if (dbscn_ord < 3) dbscn_ord = 3;
	std::vector<BufferPtr> outdatabufs{};
	std::vector<int> outlabelbufs{};
	if (num > 1)
	{
		DTWCache* dtwcache = new DTWCache(featnum);
		std::map<int, std::vector<int>> clsdatamap;
		for (int i = 0; i < num; i++)
		{
			if (labels[i] < 0) continue;
			auto idata = data + i* featnum;
			bool hasnan = false;
			for (int j = 0; j < featnum; j++)
			{
				if (isnan(idata[j]))
				{
					hasnan = true;
					break;
				}
			}
			if (hasnan) continue;
			clsdatamap[labels[i]].push_back(i);
		}

		int clm = 0;
		std::wcout << L"Clustering.";
		for (const auto& clsmap : clsdatamap)
		{
			std::unordered_map<int, std::vector<int>> clusters;
			DTWCluster::DBScan(dtwcache, data, clsmap.second, featnum, num, dbscn_ord, sparse, clusters);
			std::wcout << L".";
			for (auto hs : clusters)
			{
				BufferPtr todbuf = std::make_shared<Buffer>();
				DBA(dtwcache, data, featnum, hs.second, 100, todbuf);

				outdatabufs.push_back(todbuf);
				outlabelbufs.push_back(clsmap.first);
			}
		}
		std::wcout<<std::endl;

		delete dtwcache;
		dtwcache = nullptr;
	}
	else
	{
		BufferPtr todbuf = std::make_shared<Buffer>(featnum);
		std::shared_ptr<GenericBuffer<int>> tolbuf = std::make_shared<GenericBuffer<int>>(featnum);

		memcpy(todbuf->GetCpu(), data, sizeof(datatype) * featnum);

		outdatabufs.push_back(todbuf);
		outlabelbufs.push_back(labels[0]);
	}

	int clusternum = (int)outdatabufs.size();
	outdata->Reshape(clusternum, featnum);
	outlabels->Reshape(clusternum);

	datatype* outdatacpu = outdata->GetCpu();
	memcpy(outlabels->GetCpu(), outlabelbufs.data(), sizeof(int) * clusternum);
	for (int i = 0; i < clusternum; i++)
	{
		memcpy(outdatacpu, outdatabufs[i]->GetCpu(), sizeof(datatype)* featnum);
		outdatacpu += featnum;
	}
}


