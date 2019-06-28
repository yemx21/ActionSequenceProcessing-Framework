#include "DecisionUnitL1.h"
#include <deque>
#include <algorithm>
#include <numeric>
#include <ostream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <Windows.h>
using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::machines;
using namespace rsdn::learning::machines::supervised;

static bool IsValidAddress(const void* lp, UINT_PTR nBytes, bool readWrite = true)
{
	return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
		(!readWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}

void DecisionUnitL1::Train(const SampleChunk* data, int splitoffset, const std::vector<sizetype>& indices, const std::vector<sizetype>& selindices, std::vector<datatype>& factors, datatype& c, datatype& qual, int batchstablity, datatype lr, int maxiter, datatype conv, datatype rr)
{
	auto count = indices.size();
	std::vector<int> suffle;
	suffle.resize(count);
	for (int i = 0; i < count; i++) suffle[i] = i;

	double mu = 0.0;
	double norm = 1.0;
	qual = 0.0;
	//datatype oc = 0.0;
	c = 0.5;
	int iter = 0;
	auto selsize = selindices.size();
	std::vector<double> l1;
	factors.resize(selsize, 1.0);
	l1.resize(selsize, 0.0);

	std::deque<datatype> sparsities;
	double lastsp = 1.0;
	double spconv = 0.0005;// 0.5 / selsize;

	std::deque<datatype> qualities;
	double lastql = 1.0;


	auto uniquelabels = data->UniqueLabels;
	auto labelcount = uniquelabels.size();

	GenericBuffer<sizetype> sortedidxbuf{(int)count};
	sizetype* sorted_idx = sortedidxbuf.GetCpu();
	const sizetype* sidx = &indices[0];

	while (norm > conv)
	{
		std::vector<datatype> oldfactors = factors;

		std::random_shuffle(suffle.begin(), suffle.end(), [](int mx) -> int
		{
			return Random::Generate(true) % mx;
		});

		std::vector<double> values;
		values.resize(count, 0.0);

		std::map<sizetype, double> lcw;
		std::map<sizetype, double> rcw;

		sizetype i, best_i = -1;
		double best_val = 0.0;

		for (i = 0; i < count; i++)
		{
			auto sam1 = data->Features[indices[i]];
			auto sam2 = sam1 + splitoffset;

			for (int j = 0; j < selsize; j++)
			{
				values[i] += (sam1[selindices[j]] - sam2[selindices[j]])* factors[j];
			}
			values[i] = 1.0 / (1.0 + exp(-values[i]));
		}

		for (i = 0; i < labelcount; i++)
		{
			lcw.insert(std::make_pair(uniquelabels[i], 0.0));
			rcw.insert(std::make_pair(uniquelabels[i], 0.0));
		}

		for (i = 0; i < count; i++)
		{
			sorted_idx[i] = i;
			int si = sidx[i];
			rcw[data->Label[si]]++;
		}

		std::sort(sorted_idx, sorted_idx + count, [&values, &indices](const int a, const int b)->bool
		{
			return values[a] < values[b];
		});

		double L = 0, R = 0, lsum2 = 0, rsum2 = 0;
		for (sizetype l : uniquelabels)
		{
			double wval = rcw[l];
			R += wval;
			rsum2 += wval*wval;
		}

		for (i = 0; i < count - 1; i++)
		{
			sizetype curr = sorted_idx[i];
			sizetype next = sorted_idx[i + 1];
			sizetype si = sidx[curr];
			L++;
			R--;
			sizetype idx = data->Label[si];
			double lv = lcw[idx], rv = rcw[idx];
			lsum2 += 2 * lv + 1;
			rsum2 -= 2 * rv - 1;
			lcw[idx] = lv + 1;
			rcw[idx] = rv - 1;

			if (isnan(values[curr]) || isnan(values[next]))
			{
				continue;
			}

			if (values[curr] + 1e-4 < values[next])
			{
				double val = (lsum2*R + rsum2*L) / (L*R);
				if (best_val < val)
				{
					best_val = val;
					best_i = i;
				}
			}
		}

		if (best_i >= 0)
		{
			c = (values[sorted_idx[best_i]] + values[sorted_idx[best_i + 1]])*0.5f;
			lastql = (float)best_val;
		}

		std::map<sizetype, sizetype> lefthist;
		std::map<sizetype, sizetype> righthist;

		for (sizetype l : uniquelabels)
		{
			lefthist[l] = 0;
			righthist[l] = 0;
		}

		for (i = 0; i < count; i++)
		{
			if (values[i] > c) 
				righthist[data->Label[i]]++;
			else 
				lefthist[data->Label[i]]++;
		}

		sizetype positivelabel = -1;

		while (!lefthist.empty() && !righthist.empty())
		{
			auto lefthist_major = std::max_element(lefthist.begin(), lefthist.end(), [](const std::pair<sizetype, sizetype>& a, const std::pair<sizetype, sizetype>& b) {
				return a.second < b.second;
			});

			auto righthist_major = std::max_element(righthist.begin(), righthist.end(), [](const std::pair<sizetype, sizetype>& a, const std::pair<sizetype, sizetype>& b) {
				return a.second < b.second;
			});

			if (lefthist_major->first == righthist_major->second)
			{
				if (lefthist_major->second > righthist_major->second)
				{
					lefthist.erase(lefthist_major->first);
				}
			}
			else
			{
				positivelabel = lefthist_major->first;
				break;
			}
		};

		for (i = 0; i < count; i++)
		{
			auto sam1 = data->Features[suffle[i]];
			auto sam2 = sam1 + splitoffset;
			mu += (rr * lr);
			
			double sval = 0.0;
			for (int j = 0; j < selsize; j++)
			{
				sval += (sam1[selindices[j]] - sam2[selindices[j]])* factors[j];
			}
			sval = 1.0 / (1.0 + exp(-sval));

			double actual = data->Label[i] == positivelabel ? 1.0 : 0.0;

			for (int j = 0; j < selsize; j++)
			{
				factors[j] += lr * (actual - sval) * (sam1[selindices[j]] - sam2[selindices[j]]);

				auto z = factors[j];
				if (factors[j] > 0.0)
					factors[j] = RSDN_MAX(0, factors[j] - mu - l1[j]);
				else if (factors[j] < 0.0)
					factors[j] = RSDN_MIN(0, factors[j] - mu + l1[j]);

				if (factors[j] > 1.0) factors[j] = 0.0;
				if (factors[j] < -1.0) factors[j] = 0.0;

				l1[j] += (factors[j] - z);
			}
		}

		unsigned int sparsity = 0;
		for (int i = 0; i < selsize; i++)
		{
			if (abs(factors[i]) > 1e-6) sparsity++;
		}
		sparsities.push_back((datatype)sparsity / selsize);

		if (sparsities.size() > batchstablity)
		{
			sparsities.pop_front();
			double sp = std::accumulate(sparsities.begin(), sparsities.end(), 0.0) / (sparsities.size());

			//printf("sp: %f\n", (float)sp);

			if (abs(sp - lastsp) < spconv)
			{
				qual = lastql;
				break;
			}
			lastsp = sp;
		}

		//printf("qual: %f, c: %f\n", (float)best_val, (float)c);

		//if (iter % 50 == 0)
		//{
		//	printf("norm: %f\n", (float)norm);
		//}

		if (++iter > maxiter)
		{
			qual = lastql;
			break;
		}
	}
}