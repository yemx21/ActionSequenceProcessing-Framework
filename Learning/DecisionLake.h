#pragma once
#include "Learning_Config.h"

namespace rsdn
{
	namespace learning
	{
		namespace machines
		{
			namespace supervised
			{
				namespace details
				{
					class IFeatureSelector
					{
					public:
						virtual void OnSelecting(const std::vector<sizetype>& featureindexs, const std::vector<std::pair<sizetype, sizetype>>& statistics, std::vector<sizetype>& ret) = 0;
					};

					class DecisionLake
					{
					protected:
						DecisionLake();
						sizetype id;
						sizetype level;
						IFeatureSelector* featselector;
					public:
						~DecisionLake();

					public:
						DecisionLake(sizetype lakeid, IFeatureSelector* selector, const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype maxlevel, sizetype majorfeaturecount, sizetype minsamplecount, bool resplit = false);

						static DecisionLake* Create(sizetype lakeid, IFeatureSelector* selector, const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype maxlevel, sizetype majorfeaturecount, sizetype minsamplecount, bool resplit = false);

						sizetype Predict(datatype* feats, int featcount);

					private:
						void CalculateStatistics(const SampleChunk* data, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, std::vector<std::pair<sizetype, sizetype>>& count);
						void CalculateStatistics1(const SampleChunk* data, const std::vector<sizetype>& indexs1, const std::vector<sizetype>& indexs2, const std::vector<sizetype>& labels, std::vector<std::pair<sizetype, sizetype>>& count);

					private:
					public:
						struct WSplit
						{
							WSplit()
							{
								varIdx = next = 0;
								quality = c = 0.f;
							}

							void KeepInfoOnly()
							{
								indexs.clear();
								indexs.shrink_to_fit();
							}

							int varIdx;
							float quality;
							int next;
							sizetype nodeidx;
							float c;

							std::vector<sizetype> indexs;
						};

						enum class Direction
						{
							Unknown,
							Left,
							Right,
						};

						struct WSplitParentInfo
						{
							sizetype ParentNodeIndex;
							Direction Dir;

							WSplitParentInfo()
							{
								Dir = Direction::Unknown;
								ParentNodeIndex = -1;
							}
						};
						struct WNode
						{
							WNode()
							{
								label = samplecount = level = 0;
								parent = left = right = missing = id = split = -1;
								defaultdir = Direction::Unknown;
								leftlink = rightlink = -1;
								risk = 0;
							}

							void KeepInfoOnly()
							{
								leftindexs.clear();
								leftindexs.shrink_to_fit();

								rightindexs.clear();
								rightindexs.shrink_to_fit();

								missingindexs.clear();
								missingindexs.shrink_to_fit();
							}

							sizetype label;
							sizetype parent;
							sizetype split;
							sizetype left;
							sizetype leftlink;
							sizetype right;
							sizetype rightlink;
							sizetype missing;
							Direction defaultdir;
							sizetype defaultlabel;

							sizetype id;
							std::vector<sizetype> friends;

							std::vector<sizetype> leftindexs;
							std::vector<sizetype> rightindexs;
							std::vector<sizetype> missingindexs;

							sizetype samplecount;
							sizetype level;

							sizetype risk;
							float localrisk;

							sizetype leftlabel;
							sizetype rightlabel;
							sizetype missinglabel;

							sizetype leftrisk;
							sizetype rightrisk;
							sizetype missingrisk;

							float localleftrisk;
							float localrightrisk;
							float localmissingrisk;
						};

						struct WLinkage
						{
							sizetype localidx0;
							Direction dir0;
							sizetype localidx1;
							Direction dir1;

							WLinkage()
							{
								localidx0 = localidx1 = -1;
								dir0 = dir1 = Direction::Unknown;
							}
						};

					private:

						std::vector<sizetype> GetActiveFeatures(const std::vector<sizetype>& featureindexs, sizetype majorfeaturecount, const std::vector<std::pair<sizetype, sizetype>>& statistics);

						void FlowRiverAtRoot(const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype majorfeaturecount, sizetype minsamplecount);

						void FlowRiverAtLevel(const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& labels, sizetype level, sizetype minsamplecount, sizetype majorfeaturecount, std::vector<WSplit>& splits, std::vector<WSplitParentInfo>& splitpinfos, std::vector<WNode>& ninfos);

						void CalculateValues(const SampleChunk* data, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype& label, sizetype& risk, float& localrisk, std::vector<std::pair<sizetype, sizetype>>& statistics);

						bool FindDraftGoodSplit(const SampleChunk* data, const std::vector<sizetype>& indexs, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& labels, WSplit& goodsplit, sizetype majorfeaturecount, const std::vector<std::pair<sizetype, sizetype>>& statistics);

						WSplit FindDraftSplit(const SampleChunk* data, sizetype fi, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels);

						Direction CalculateBranchIndexs(const SampleChunk* data, const WSplit& split, const std::vector<sizetype>& indexs, std::vector<sizetype>& left, std::vector<sizetype>& right, std::vector<sizetype>& missing);

						void FlowRiverFromIndexs(const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype minsamplecount, sizetype majorfeaturecount, std::vector<WNode>& ninfos, std::vector<WSplit>& splits, std::vector<WSplitParentInfo>& npinfos, sizetype pnodeidx, Direction dir);

						void FindLinkages(const SampleChunk* data, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& labels, sizetype minsamplecount, sizetype majorfeaturecount, const std::vector<WSplit>& splits, const std::vector<WNode>& infos, std::vector<WLinkage>& linkages, std::vector<WSplit>& newsplits);

						bool ResplitLinkage(const SampleChunk* data, const std::vector<sizetype>& indexs0, const std::vector<sizetype>& indexs1, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& labels, WSplit& goodsplit, sizetype majorfeaturecount);

						WSplit FindDraftSplit2(const SampleChunk* data, sizetype fi, const std::vector<sizetype>& indexs0, const std::vector<sizetype>& indexs1, const std::vector<sizetype>& labels);

					public:
						std::vector<WSplit> _splits;

						std::vector<WNode> _nodes;

						std::vector<sizetype> _nodelvls;
						std::vector<sizetype> _nodelvlcts;

						void Save(FileStream* file);
						static DecisionLake* Load(FileStream* file);

					};

				}
			}
		}
	}
}
