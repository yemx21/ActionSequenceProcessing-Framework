#include "DecisionLake.h"
#include <algorithm>
#include <unordered_map>
#include <iterator>
#include <set>
#include <boost/array.hpp>

using namespace rsdn;
using namespace rsdn::learning;
using namespace rsdn::learning::machines;
using namespace rsdn::learning::machines::supervised;
using namespace rsdn::learning::machines::supervised::details;


DecisionLake::DecisionLake() :level(0)
{

}


DecisionLake::~DecisionLake()
{

}

void DecisionLake::CalculateStatistics(const SampleChunk* data, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, std::vector<std::pair<sizetype, sizetype>>& count)
{
	count.clear();

	if (indexs.empty()) return;

	std::unordered_map<sizetype, sizetype> statistics;

	sizetype labelsize = labels.size();
	for (sizetype i = 0; i < labelsize; i++)
	{
		statistics.insert(std::make_pair(labels[i], 0));
	}

	for (sizetype idx : indexs)
	{
		statistics[data->Label[(size_t)idx]]++;
	}

	for (const auto& s : statistics)
	{
		count.push_back(std::make_pair(s.first, s.second));
	}

	std::sort(count.begin(), count.end(), [](const std::pair<sizetype, sizetype>& a, const std::pair<sizetype, sizetype>& b)
	{
		return a.second > b.second;
	});
}

void DecisionLake::CalculateStatistics1(const SampleChunk* data, const std::vector<sizetype>& indexs1, const std::vector<sizetype>& indexs2, const std::vector<sizetype>& labels, std::vector<std::pair<sizetype, sizetype>>& count)
{
	count.clear();

	std::unordered_map<sizetype, sizetype> statistics;

	sizetype labelsize = labels.size();
	for (sizetype i = 0; i < labelsize; i++)
	{
		statistics.insert(std::make_pair(labels[i], 0));
	}

	for (sizetype idx : indexs1)
	{
		statistics[data->Label[idx]]++;
	}
	for (sizetype idx : indexs2)
	{
		statistics[data->Label[idx]]++;
	}

	for (const auto& s : statistics)
	{
		count.push_back(std::make_pair(s.first, s.second));
	}

	std::sort(count.begin(), count.end(), [](const std::pair<sizetype, sizetype>& a, const std::pair<sizetype, sizetype>& b)
	{
		return a.second > b.second;
	});
}


int featureindexs_random(int i) { return Random::Generate() % i; }


DecisionLake::DecisionLake(sizetype lakeid, IFeatureSelector* selector, const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype maxlevel, sizetype majorfeaturecount, sizetype minsamplecount, bool resplit)
{
	level = 0;
	featselector = selector;
	id = lakeid;
	size_t n = indexs.size();
	//std::vector<bool> oobmask(n);
	//std::vector<int> oobidx;
	//std::vector<int> oobperm;
	//std::vector<double> oobres(n, 0.);
	//std::vector<int> oobcount(n, 0);
	//std::vector<int> oobvotes(n*data->LabelCount, 0);

	FlowRiverAtRoot(data, featureindexs, indexs, labels, majorfeaturecount, minsamplecount);

	sizetype lvl = 1;
	size_t i = 0;
	do
	{
		std::vector<WSplit> splits;
		std::vector<WSplitParentInfo> splitpinfos;
		std::vector<WNode> infos;
		std::vector<WLinkage> linkages;

		FlowRiverAtLevel(data, featureindexs, labels, lvl, minsamplecount, majorfeaturecount, splits, splitpinfos, infos);

		size_t scount = splits.size();

		for (i = 0; i < scount; i++)
		{
			WSplit& split = splits[i];
			WNode& info = infos[i];
			info.defaultdir = CalculateBranchIndexs(data, split, split.indexs, info.leftindexs, info.rightindexs, info.missingindexs);

			std::vector<std::pair<sizetype, sizetype>> statistics;

			CalculateValues(data, info.leftindexs, labels, info.leftlabel, info.leftrisk, info.localleftrisk, statistics);
			CalculateValues(data, info.rightindexs, labels, info.rightlabel, info.rightrisk, info.localrightrisk, statistics);
			CalculateValues(data, info.missingindexs, labels, info.missinglabel, info.missingrisk, info.localmissingrisk, statistics);

			switch (info.defaultdir)
			{
			case Direction::Left:
				info.defaultlabel = info.leftlabel;
				break;
			case Direction::Right:
				info.defaultlabel = info.rightlabel;
				break;
			case Direction::Unknown:
			default:
				info.defaultlabel = info.missinglabel;
				break;
			}
		}

		std::vector<WSplit> linksplits;

		if (resplit) FindLinkages(data, featureindexs, labels, minsamplecount, majorfeaturecount, splits, infos, linkages, linksplits);

		scount = splits.size();

		_nodelvls.push_back((sizetype)_nodes.size());
		sizetype nodestidx = _nodes.size();

		size_t lscount = linksplits.size();

		for (i = 0; i < lscount; i++)
		{
			if (linkages[i].dir0 == Direction::Left)
			{
				infos[linkages[i].localidx0].leftlink = nodestidx + scount + i;
			}
			else if (linkages[i].dir0 == Direction::Right)
			{
				infos[linkages[i].localidx0].rightlink = nodestidx + scount + i;
			}

			if (linkages[i].dir1 == Direction::Left)
			{
				infos[linkages[i].localidx1].leftlink = nodestidx + scount + i;
			}
			else if (linkages[i].dir1 == Direction::Right)
			{
				infos[linkages[i].localidx1].rightlink = nodestidx + scount + i;
			}
		}

		for (i = 0; i < scount; i++)
		{
			WSplit& split = splits[i];
			WNode& info = infos[i];
			WSplitParentInfo& spinfo = splitpinfos[i];

			split.nodeidx = nodestidx++;

			_splits.emplace_back(split);

			sizetype splitglobalidx = (sizetype)(_splits.size() - 1);

			info.split = splitglobalidx;

			if (spinfo.ParentNodeIndex >= 0)
			{
				switch (spinfo.Dir)
				{
				case Direction::Left:
					_nodes[spinfo.ParentNodeIndex].left = splitglobalidx;
					break;
				case Direction::Right:
					_nodes[spinfo.ParentNodeIndex].right = splitglobalidx;
					break;
				case Direction::Unknown:
				default:
					_nodes[spinfo.ParentNodeIndex].missing = splitglobalidx;
					break;
				}
			}
		}

		std::copy(infos.begin(), infos.end(), std::back_inserter(_nodes));

		for (i = 0; i < lscount; i++)
		{
			WSplit& split = linksplits[i];

			split.nodeidx = nodestidx++;

			_splits.emplace_back(split);

			WNode info{};
			info.defaultdir = CalculateBranchIndexs(data, split, split.indexs, info.leftindexs, info.rightindexs, info.missingindexs);

			sizetype splitglobalidx = (sizetype)(_splits.size() - 1);

			info.split = splitglobalidx;

			std::vector<std::pair<sizetype, sizetype>> statistics;

			CalculateValues(data, info.leftindexs, labels, info.leftlabel, info.leftrisk, info.localleftrisk, statistics);
			CalculateValues(data, info.rightindexs, labels, info.rightlabel, info.rightrisk, info.localrightrisk, statistics);
			CalculateValues(data, info.missingindexs, labels, info.missinglabel, info.missingrisk, info.localmissingrisk, statistics);
			switch (info.defaultdir)
			{
			case Direction::Left:
				info.defaultlabel = info.leftlabel;
				break;
			case Direction::Right:
				info.defaultlabel = info.rightlabel;
				break;
			case Direction::Unknown:
			default:
				info.defaultlabel = info.missinglabel;
				break;
			}

			_nodes.push_back(info);
		}

		_nodelvlcts.push_back((sizetype)infos.size() + lscount);

		if (infos.size() == 0) break;

		lvl++;

	} while (lvl <= maxlevel);

	level = lvl;


	for (auto& n : _nodes)
	{
		n.KeepInfoOnly();
	}

	for (auto& s : _splits)
	{
		s.KeepInfoOnly();
	}
}

DecisionLake* DecisionLake::Create(sizetype lakeid, IFeatureSelector* selector, const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype maxlevel, sizetype majorfeaturecount, sizetype minsamplecount, bool resplit)
{
	try
	{
		sizetype minlvl = maxlevel / 2;
		DecisionLake* lake = new DecisionLake(lakeid, selector, data, featureindexs, indexs, labels, maxlevel, majorfeaturecount, minsamplecount, resplit);
		if (lake->level >= minlvl)
		{
			return lake;
		}
		delete lake;
		lake = nullptr;
	}
	catch (...)
	{

	}
	return false;
}

void DecisionLake::FindLinkages(const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& labels, sizetype minsamplecount, sizetype majorfeaturecount, const std::vector<WSplit>& splits, const std::vector<WNode>& infos, std::vector<WLinkage>& linkages, std::vector<WSplit>& newsplits)
{
	sizetype len = (sizetype)infos.size();
	if (len < 2) return;

	std::unordered_map<sizetype, std::vector<boost::array<sizetype, 2>>> groups;
	sizetype i, j;
	for (i = 0; i < len; i++)
	{
		const auto& node = infos[i];

		bool leftok = node.leftindexs.size() > 0 && node.leftlabel > 0;
		bool rightok = node.rightindexs.size() > 0 && node.rightlabel > 0;

		if (leftok && rightok)
		{
			if (node.localleftrisk < node.localrightrisk)
			{
				groups[node.leftlabel].push_back(boost::array<sizetype, 2>{i, 1});
			}
			else
			{
				groups[node.rightlabel].emplace_back(boost::array<sizetype, 2>{i, 2});
			}
		}
		else if (leftok)
		{
			groups[node.leftlabel].emplace_back(boost::array<sizetype, 2>{i, 1});
		}
		else if (rightok)
		{
			groups[node.rightlabel].emplace_back(boost::array<sizetype, 2>{i, 2});
		}
	}

	bool l0 = true;
	bool l1 = true;

	for (auto iter = groups.begin(); iter != groups.end(); iter++)
	{
		auto& isg = iter->second;
		sizetype isgc = (sizetype)isg.size();
		if (isgc > 1)
		{
			std::vector<std::tuple<sizetype, sizetype, Direction, sizetype, sizetype, Direction, WSplit>> rdsplits;

			for (i = 0; i < isgc; i++)
			{
				for (j = i + 1; j < isgc; j++)
				{
					l0 = isg[i][1] == 1;
					l1 = isg[j][1] == 1;
					float q0 = splits[isg[i][0]].quality;
					float q1 = splits[isg[j][0]].quality;
					float qmin = q0 < q1 ? q0 : q1;

					if (l0 && l1)
					{
						WSplit ws;
						if (ResplitLinkage(data, infos[isg[i][0]].leftindexs, infos[isg[j][0]].leftindexs, featureindexs, labels, ws, majorfeaturecount))
						{
							if (ws.quality > qmin)
							{
								ws.indexs = infos[isg[i][0]].leftindexs;
								std::copy(infos[isg[j][0]].leftindexs.begin(), infos[isg[j][0]].leftindexs.end(), std::back_inserter(ws.indexs));
								rdsplits.push_back(std::make_tuple(i, isg[i][0], Direction::Left, j, isg[j][0], Direction::Left, ws));
							}
						}
					}
					else if (!l0 && !l1)
					{
						WSplit ws;
						if (ResplitLinkage(data, infos[isg[i][0]].rightindexs, infos[isg[j][0]].rightindexs, featureindexs, labels, ws, majorfeaturecount))
						{
							if (ws.quality > qmin)
							{
								ws.indexs = infos[isg[i][0]].rightindexs;
								std::copy(infos[isg[j][0]].rightindexs.begin(), infos[isg[j][0]].rightindexs.end(), std::back_inserter(ws.indexs));
								rdsplits.push_back(std::make_tuple(i, isg[i][0], Direction::Right, j, isg[j][0], Direction::Right, ws));
							}
						}
					}
					else if (l0 && !l1)
					{
						WSplit ws;
						if (ResplitLinkage(data, infos[isg[i][0]].leftindexs, infos[isg[j][0]].rightindexs, featureindexs, labels, ws, majorfeaturecount))
						{
							if (ws.quality > qmin)
							{
								ws.indexs = infos[isg[i][0]].leftindexs;
								std::copy(infos[isg[j][0]].rightindexs.begin(), infos[isg[j][0]].rightindexs.end(), std::back_inserter(ws.indexs));
								rdsplits.push_back(std::make_tuple(i, isg[i][0], Direction::Left, j, isg[j][0], Direction::Right, ws));
							}
						}
					}
					else if (!l0 && l1)
					{
						WSplit ws;
						if (ResplitLinkage(data, infos[isg[i][0]].rightindexs, infos[isg[j][0]].leftindexs, featureindexs, labels, ws, majorfeaturecount))
						{
							if (ws.quality > qmin)
							{
								ws.indexs = infos[isg[i][0]].rightindexs;
								std::copy(infos[isg[j][0]].leftindexs.begin(), infos[isg[j][0]].leftindexs.end(), std::back_inserter(ws.indexs));
								rdsplits.push_back(std::make_tuple(i, isg[i][0], Direction::Right, j, isg[j][0], Direction::Left, ws));
							}
						}
					}
				}
			}

			if (!rdsplits.empty())
			{
				sizetype rdsc = rdsplits.size();

				sizetype rdchanged = -1;
				do
				{
					rdchanged = -1;
					rdsc = rdsplits.size();

					for (i = 0; i < rdsc; i++)
					{
						for (j = i + 1; j < rdsc; j++)
						{
							sizetype ii0 = std::get<0>(rdsplits[i]);
							sizetype ii1 = std::get<3>(rdsplits[i]);
							sizetype jj0 = std::get<0>(rdsplits[j]);
							sizetype jj1 = std::get<3>(rdsplits[j]);

							if (ii0 == jj0 || ii0 == jj1 || ii1 == jj0 || ii1 == jj1)
							{
								if (std::get<6>(rdsplits[i]).quality > std::get<6>(rdsplits[j]).quality)
								{
									rdchanged = j;
								}
								else
								{
									rdchanged = i;
								}
								break;
							}
						}
						if (rdchanged >= 0)
						{
							break;
						}
					}

					if (rdchanged >= 0)
					{
						rdsplits.erase(rdsplits.begin() + rdchanged);
					}
				} while (rdchanged >= 0);

				for (const auto& rds : rdsplits)
				{
					WLinkage wlink;
					wlink.localidx0 = std::get<1>(rds);
					wlink.dir0 = std::get<2>(rds);
					wlink.localidx1 = std::get<4>(rds);
					wlink.dir1 = std::get<5>(rds);
					linkages.push_back(wlink);
					newsplits.push_back(std::get<6>(rds));
				}
			}
		}
	}
}

bool DecisionLake::ResplitLinkage(const SampleChunk* data, const std::vector<sizetype>& indexs0, const std::vector<sizetype>& indexs1, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& labels, WSplit& goodsplit, sizetype majorfeaturecount)
{
	std::vector<std::pair<sizetype, sizetype>> statistics;

	CalculateStatistics(data, indexs0, indexs1, statistics);

	bool found = false;
	WSplit split;
	goodsplit.quality = 0.;

	std::vector<sizetype> nfeatureindexs = GetActiveFeatures(featureindexs, majorfeaturecount, statistics);

	for (sizetype fi : nfeatureindexs)
	{
		split = FindDraftSplit2(data, fi, indexs0, indexs1, labels);
		if (split.quality > goodsplit.quality)
		{
			goodsplit = split;
			found = true;
		}
	}
	return found;
}

DecisionLake::WSplit DecisionLake::FindDraftSplit2(const SampleChunk* data, sizetype fi, const std::vector<sizetype>& indexs0, const std::vector<sizetype>& indexs1, const std::vector<sizetype>& labels)
{
	const datatype epsilon = std::numeric_limits<datatype>::epsilon() * 2;
	sizetype n0 = (sizetype)indexs0.size();
	sizetype n1 = (sizetype)indexs1.size();
	sizetype n = n0 + n1;
	sizetype m = (sizetype)labels.size();

	GenericBuffer<sizetype> sortedidxbuf{ (int)n };
	sizetype* sorted_idx = sortedidxbuf.GetCpu();

	const sizetype* sidx0 = &indexs0[0];
	const sizetype* sidx1 = &indexs1[0];
	std::map<sizetype, double> lcw;
	std::map<sizetype, double> rcw;

	sizetype i, j, best_i = -1;
	double best_val = 0.0;

	for (i = 0; i < m; i++)
	{
		lcw.insert(std::make_pair(labels[i], 0.0));
		rcw.insert(std::make_pair(labels[i], 0.0));
	}

	auto valuemapper = data->QueryFeatureAt(fi, indexs0, indexs1);
	auto& values = *valuemapper;

	for (i = 0; i < n0; i++)
	{
		sorted_idx[i] = i;
		int si = sidx0[i];
		rcw[data->Label[si]]++;
	}

	for (i = n0, j = 0; i < n, j<n1; i++, j++)
	{
		sorted_idx[i] = i;
		int si = sidx1[j];
		rcw[data->Label[si]]++;
	}

	std::sort(sorted_idx, sorted_idx + n, [&values](const sizetype a, const sizetype b)->bool
	{
		return values[a] < values[b];
	});

	double L = 0, R = 0, lsum2 = 0, rsum2 = 0;
	for (sizetype l : labels)
	{
		double wval = rcw[l];
		R += wval;
		rsum2 += wval*wval;
	}

	for (i = 0; i < n - 1; i++)
	{
		sizetype curr = sorted_idx[i];
		sizetype next = sorted_idx[i + 1];
		sizetype si = curr < n0 ? sidx0[curr] : sidx1[curr - n0];
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

		if (values[curr] + epsilon < values[next])
		{
			double val = (lsum2*R + rsum2*L) / (L*R);

			if (best_val < val)
			{
				best_val = val;
				best_i = i;
			}
		}
	}

	WSplit split;
	if (best_i >= 0)
	{
		split.varIdx = fi;
		split.c = (values[sorted_idx[best_i]] + values[sorted_idx[best_i + 1]])*0.5f;

		split.quality = (float)best_val;
	}


	return split;
}

std::vector<sizetype> DecisionLake::GetActiveFeatures(const std::vector<sizetype>& featureindexs, sizetype majorfeaturecount, const std::vector<std::pair<sizetype, sizetype>>& statistics)
{
	std::vector<sizetype> ret;
	if (featselector)
		featselector->OnSelecting(featureindexs, statistics, ret);
	else
		ret = featureindexs;
	std::random_shuffle(ret.begin(), ret.end(), featureindexs_random);

	if (ret.size() > majorfeaturecount)
	{
		ret.erase(ret.begin() + majorfeaturecount, ret.end());
	}

	return ret;
}

void DecisionLake::FlowRiverAtLevel(const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& labels, sizetype level, sizetype minsamplecount, sizetype majorfeaturecount, std::vector<WSplit>& splits, std::vector<WSplitParentInfo>& splitpinfos, std::vector<WNode>& ninfos)
{
	sizetype plvl = level - 1;

	for (sizetype n = _nodelvls[plvl]; n< _nodelvls[plvl] + _nodelvlcts[plvl]; n++)
	{
		const WNode& node = _nodes[n];
		if (node.leftlink == -1) FlowRiverFromIndexs(data, featureindexs, node.leftindexs, labels, minsamplecount, majorfeaturecount, ninfos, splits, splitpinfos, n, Direction::Left);
		if (node.rightlink == -1) FlowRiverFromIndexs(data, featureindexs, node.rightindexs, labels, minsamplecount, majorfeaturecount, ninfos, splits, splitpinfos, n, Direction::Right);
		//FlowRiverFromIndexs(data, featureindexs, node.missingindexs, labels, minsamplecount, majorfeaturecount, ninfos, splits, splitpinfos, n, Direction::Unknown);
	}
}

void DecisionLake::FlowRiverFromIndexs(const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype minsamplecount, sizetype majorfeaturecount, std::vector<WNode>& ninfos, std::vector<WSplit>& splits, std::vector<WSplitParentInfo>& npinfos, sizetype pnodeidx, Direction dir)
{
	sizetype n = (sizetype)indexs.size();
	bool cansplit = true;
	WNode info;
	info.parent = pnodeidx;
	std::vector<std::pair<sizetype, sizetype>> statistics;
	CalculateValues(data, indexs, labels, info.label, info.risk, info.localrisk, statistics);

	if (n <= minsamplecount)
		cansplit = false;
	else
	{
		sizetype firstlabel = data->Label[indexs[0]];
		bool allsame = true;
		for (sizetype idx : indexs)
		{
			if (data->Label[idx] != firstlabel)
			{
				allsame = false;
				break;
			}
		}
		cansplit = !allsame;
	}

	WSplit draftsplit;
	WSplitParentInfo draftsplitpinfo;
	bool hasdraftsplit = false;
	if (cansplit)
	{
		hasdraftsplit = FindDraftGoodSplit(data, indexs, featureindexs, labels, draftsplit, majorfeaturecount, statistics);
	}

	if (hasdraftsplit)
	{
		draftsplit.indexs = indexs;
		draftsplit.nodeidx = pnodeidx;
		draftsplitpinfo.ParentNodeIndex = pnodeidx;
		draftsplitpinfo.Dir = dir;
		ninfos.push_back(info);
		splits.push_back(draftsplit);
		npinfos.push_back(draftsplitpinfo);
	}
}

void DecisionLake::FlowRiverAtRoot(const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype majorfeaturecount, sizetype minsamplecount)
{
	WNode root;
	root.id = 0;
	root.parent = -1;
	root.level = 0;
	root.samplecount = indexs.size();

	sizetype n = (sizetype)indexs.size();
	bool cansplit = true;
	std::vector<std::pair<sizetype, sizetype>> statistics;
	CalculateValues(data, indexs, labels, root.label, root.risk, root.localrisk, statistics);

	if (n <= minsamplecount)
		cansplit = false;
	else
	{
		sizetype firstlabel = data->Label[indexs[0]];
		bool allsame = true;
		for (sizetype idx : indexs)
		{
			if (data->Label[idx] != firstlabel)
			{
				allsame = false;
				break;
			}
		}
		cansplit = !allsame;
	}

	WSplit draftsplit;
	bool hasdraftsplit = false;
	if (cansplit)
	{
		hasdraftsplit = FindDraftGoodSplit(data, indexs, featureindexs, labels, draftsplit, majorfeaturecount, statistics);
	}

	if (hasdraftsplit)
	{
		draftsplit.nodeidx = 0;
		root.defaultdir = CalculateBranchIndexs(data, draftsplit, indexs, root.leftindexs, root.rightindexs, root.missingindexs);

		CalculateValues(data, root.leftindexs, labels, root.leftlabel, root.leftrisk, root.localleftrisk, statistics);
		CalculateValues(data, root.rightindexs, labels, root.rightlabel, root.rightrisk, root.localrightrisk, statistics);
		CalculateValues(data, root.missingindexs, labels, root.missinglabel, root.missingrisk, root.localmissingrisk, statistics);

		switch (root.defaultdir)
		{
		case Direction::Left:
			root.defaultlabel = root.leftlabel;
			break;
		case Direction::Right:
			root.defaultlabel = root.rightlabel;
			break;
		case Direction::Unknown:
		default:
			root.defaultlabel = root.missinglabel;
			break;
		}
	}

	_splits.push_back(draftsplit);
	root.split = (sizetype)(_splits.size() - 1);

	_nodes.emplace_back(root);
	_nodelvls.push_back(0);
	_nodelvlcts.push_back(1);
}

void DecisionLake::CalculateValues(const SampleChunk* data, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype& label, sizetype& risk, float& localrisk, std::vector<std::pair<sizetype, sizetype>>& statistics)
{
	sizetype n = (sizetype)indexs.size();

	CalculateStatistics(data, indexs, labels, statistics);

	if (statistics.empty())
	{
		label = -1;
		risk = n;
		localrisk = 1.0;
	}
	else
	{
		label = statistics[0].first;
		risk = n - statistics[0].second;
		localrisk = (float)risk / n;
	}
}

bool DecisionLake::FindDraftGoodSplit(const SampleChunk* data, const std::vector<sizetype>& indexs, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& labels, DecisionLake::WSplit& goodsplit, sizetype majorfeaturecount, const std::vector<std::pair<sizetype, sizetype>>& statistics)
{
	bool found = false;
	WSplit split;
	goodsplit.quality = 0.;

	std::vector<sizetype> nfeatureindexs = GetActiveFeatures(featureindexs, majorfeaturecount, statistics);

	for (sizetype fi : nfeatureindexs)
	{
		split = FindDraftSplit(data, fi, indexs, labels);
		if (split.quality > goodsplit.quality)
		{
			goodsplit = split;
			found = true;
		}
	}
	return found;
}

DecisionLake::WSplit DecisionLake::FindDraftSplit(const SampleChunk* data, sizetype fi, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels)
{
	const datatype epsilon = std::numeric_limits<datatype>::epsilon() * 2;
	sizetype n = (sizetype)indexs.size();
	sizetype m = (sizetype)labels.size();

	GenericBuffer<sizetype> sortedidxbuf{ (int)n };
	sizetype* sorted_idx = sortedidxbuf.GetCpu();
	const sizetype* sidx = &indexs[0];
	std::map<sizetype, double> lcw;
	std::map<sizetype, double> rcw;

	auto valuemapper = data->QueryFeatureAt(fi, indexs);
	auto& values = *valuemapper;

	sizetype i, best_i = -1;
	double best_val = 0.0;

	for (i = 0; i < m; i++)
	{
		lcw.insert(std::make_pair(labels[i], 0.0));
		rcw.insert(std::make_pair(labels[i], 0.0));
	}

	for (i = 0; i < n; i++)
	{
		sorted_idx[i] = i;
		int si = sidx[i];
		rcw[data->Label[si]]++;
	}

	std::sort(sorted_idx, sorted_idx + n, [&values, &indexs](const int a, const int b)->bool
	{
		return values[a] < values[b];
	});

	double L = 0, R = 0, lsum2 = 0, rsum2 = 0;
	for (sizetype l : labels)
	{
		double wval = rcw[l];
		R += wval;
		rsum2 += wval*wval;
	}

	for (i = 0; i < n - 1; i++)
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

		if (values[curr] + epsilon < values[next])
		{
			double val = (lsum2*R + rsum2*L) / (L*R);

			if (best_val < val)
			{
				best_val = val;
				best_i = i;
			}
		}
	}

	WSplit split;
	if (best_i >= 0)
	{
		split.varIdx = fi;
		split.c = (values[sorted_idx[best_i]] + values[sorted_idx[best_i + 1]])*0.5f;

		split.quality = (float)best_val;
	}

	return split;
}

DecisionLake::Direction DecisionLake::CalculateBranchIndexs(const SampleChunk* data, const WSplit& split, const std::vector<sizetype>& indexs, std::vector<sizetype>& left, std::vector<sizetype>& right, std::vector<sizetype>& missing)
{
	sizetype i, si, n = (sizetype)indexs.size(), vi = split.varIdx;
	left.reserve(n);
	right.reserve(n);
	left.clear();
	right.clear();

	auto valuemapper = data->QueryFeatureAt(vi, indexs);
	auto& values = *valuemapper;
	float c = split.c;
	double wleft = 0, wright = 0;

	for (i = 0; i < n; i++)
	{
		si = indexs[i];

		if (isnan(values[i]))
		{
			missing.push_back(si);
		}
		else if (values[i] <= c)
		{
			left.push_back(si);
			wleft++;
		}
		else
		{
			right.push_back(si);
			wright++;
		}
	}

	return wleft > wright ? Direction::Left : Direction::Right;
}

sizetype DecisionLake::Predict(datatype* feats, int featcount)
{
	int nidx = 0, prev = nidx, c = 0;

	for (;;)
	{
		if (nidx < 0)
			break;
		prev = nidx;
		const WNode& node = _nodes[nidx];
		if (node.split < 0)
			break;
		const WSplit& ws = _splits[node.split];
		int vi = ws.varIdx;
		float val = feats[vi];
		if (isnan(val))
		{
			if (node.missing >= 0)
			{
				nidx = _splits[node.missing].nodeidx;
				continue;
			}
			else
			{
				switch (node.defaultdir)
				{
				case Direction::Unknown:
				default:
					return node.defaultlabel;
					break;
				case Direction::Left:
					if (node.left < 0)
						return node.leftlabel;
					else
					{
						nidx = _splits[node.left].nodeidx;
						continue;
					}
					break;
				case Direction::Right:
					if (node.right < 0)
						return node.rightlabel;
					else
					{
						nidx = _splits[node.right].nodeidx;
						continue;
					}
					break;
				}
			}
		}

		if (val <= ws.c)
		{
			if (node.leftlink >= 0)
			{
				nidx = node.leftlink;
			}
			else
			{
				if (node.left < 0)
					return node.leftlabel;
				else
					nidx = _splits[node.left].nodeidx;
			}
		}
		else
		{
			if (node.rightlink >= 0)
			{
				nidx = node.rightlink;
			}
			else
			{
				if (node.right < 0)
					return node.rightlabel;
				else
					nidx = _splits[node.right].nodeidx;
			}
		}
	}

	return _nodes[prev].label;
}

DecisionLake* DecisionLake::Load(FileStream* file)
{
	DecisionLake* lake = new DecisionLake();
	lake->featselector = nullptr;

	file->Read((char*)&lake->id, sizeof(sizetype), 0, sizeof(sizetype));

	sizetype splitcount = 0;
	sizetype nodecount = 0;

	file->Read((char*)&splitcount, sizeof(sizetype), 0, sizeof(sizetype));
	for (sizetype i = 0; i < splitcount; i++)
	{
		WSplit sp{};
		file->Read((char*)&sp.varIdx, sizeof(int), 0, sizeof(int));
		file->Read((char*)&sp.c, sizeof(float), 0, sizeof(float));
		file->Read((char*)&sp.nodeidx, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&sp.quality, sizeof(float), 0, sizeof(float));
		lake->_splits.push_back(sp);
	}

	file->Read((char*)&nodecount, sizeof(sizetype), 0, sizeof(sizetype));
	for (sizetype i = 0; i < nodecount; i++)
	{
		WNode wn{};

		int defdir = 0;

		file->Read((char*)&defdir, sizeof(int), 0, sizeof(int));
		wn.defaultdir = (Direction)defdir;

		file->Read((char*)&wn.defaultlabel, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.left, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.leftlabel, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.leftlink, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.leftrisk, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.missing, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.missinglabel, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.missingrisk, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.right, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.rightlabel, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.rightlink, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.rightrisk, sizeof(sizetype), 0, sizeof(sizetype));
		file->Read((char*)&wn.split, sizeof(sizetype), 0, sizeof(sizetype));

		lake->_nodes.push_back(wn);
	}

	return lake;
}

void DecisionLake::Save(FileStream* file)
{
	file->Write((char*)&id, sizeof(sizetype), 0, sizeof(sizetype));

	sizetype splitcount = (sizetype)_splits.size();
	sizetype nodecount = (sizetype)_nodes.size();

	file->Write((char*)&splitcount, sizeof(sizetype), 0, sizeof(sizetype));
	for (sizetype i = 0; i < splitcount; i++)
	{
		const WSplit& sp = _splits[i];

		file->Write((char*)&sp.varIdx, sizeof(int), 0, sizeof(int));
		file->Write((char*)&sp.c, sizeof(float), 0, sizeof(float));
		file->Write((char*)&sp.nodeidx, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&sp.quality, sizeof(float), 0, sizeof(float));
	}

	file->Write((char*)&nodecount, sizeof(sizetype), 0, sizeof(sizetype));
	for (sizetype i = 0; i < nodecount; i++)
	{
		const WNode& wn = _nodes[i];

		int defdir = (int)wn.defaultdir;

		file->Write((char*)&defdir, sizeof(int), 0, sizeof(int));
		file->Write((char*)&wn.defaultlabel, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.left, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.leftlabel, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.leftlink, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.leftrisk, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.missing, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.missinglabel, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.missingrisk, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.right, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.rightlabel, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.rightlink, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.rightrisk, sizeof(sizetype), 0, sizeof(sizetype));
		file->Write((char*)&wn.split, sizeof(sizetype), 0, sizeof(sizetype));
	}
}
