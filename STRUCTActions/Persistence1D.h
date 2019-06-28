#pragma once
#include "STRUCT_Config.h"
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#define NO_COLOR -1
#define RESIZE_FACTOR 20

namespace rsdn
{
	namespace learning
	{
		namespace timeseries
		{
			namespace details
			{
				struct TIdxAndData
				{
					TIdxAndData() :Idx(-1), data(0) {}

					bool operator<(const TIdxAndData& other) const
					{
						if (data < other.data) return true;
						if (data > other.data) return false;
						return (Idx < other.Idx);
					}

					int Idx;
					datatype data;
				};


				struct TComponent
				{
					int LeftEdgeIndex;
					int RightEdgeIndex;
					int MinIndex;
					datatype MinValue;
					bool Alive;
				};

				struct TPairedExtrema
				{
					int MinIndex;

					int MaxIndex;

					datatype Persistence;

					bool operator<(const TPairedExtrema& other) const
					{
						if (Persistence < other.Persistence) return true;
						if (Persistence > other.Persistence) return false;
						return (MinIndex < other.MinIndex);
					}
				};


				class Persistence1D
				{
				private:
					const datatype* data;
					sizetype datacount;
					std::vector<TIdxAndData> SortedData;
					std::vector<int> Colors;
					std::vector<TComponent> Components;
					std::vector<TPairedExtrema> PairedExtrema;
					unsigned int TotalComponents;
					bool AliveComponentsVerified;
				public:
					Persistence1D()
					{
					}

					~Persistence1D()
					{
					}


					bool RunPersistence(const datatype* input, sizetype count)
					{
						data = input;
						datacount = count;
						Init();

						if (count == 0) return false;

						CreateIndexValueVector();
						Watershed();
						SortPairedExtrema();
						return true;
					}

					bool GetPairedExtrema(std::vector<TPairedExtrema> & pairs, const datatype threshold = 0) const
					{
						pairs.clear();

						if (PairedExtrema.empty() || threshold < 0.0) return false;

						std::vector<TPairedExtrema>::const_iterator lower_bound = FilterByPersistence(threshold);

						if (lower_bound == PairedExtrema.end()) return false;

						pairs = std::vector<TPairedExtrema>(lower_bound, PairedExtrema.end());

						return true;
					}

					bool GetExtremaIndices(std::vector<int> & min, std::vector<int> & max, const datatype threshold = 0) const
					{
						min.clear();
						max.clear();

						if (PairedExtrema.empty() || threshold < 0.0) return false;

						min.reserve(PairedExtrema.size());
						max.reserve(PairedExtrema.size());

						std::vector<TPairedExtrema>::const_iterator lower_bound = FilterByPersistence(threshold);

						for (std::vector<TPairedExtrema>::const_iterator p = lower_bound; p != PairedExtrema.end(); p++)
						{
							min.push_back((*p).MinIndex);
							max.push_back((*p).MaxIndex);
						}
						return true;
					}

					int GetGlobalMinimumIndex() const
					{
						if (Components.empty()) return -1;
						return Components.front().MinIndex;
					}

					datatype GetGlobalMinimumValue() const
					{
						if (Components.empty()) return 0;
						return Components.front().MinValue;
					}
					
					bool VerifyResults()
					{
						bool flag = true;
						std::vector<int> min, max;
						std::vector<int> combinedIndices;

						GetExtremaIndices(min, max);

						int globalMinIdx = GetGlobalMinimumIndex();

						std::sort(min.begin(), min.end());
						std::sort(max.begin(), max.end());
						combinedIndices.reserve(min.size() + max.size());
						std::set_union(min.begin(), min.end(), max.begin(), max.end(), std::inserter(combinedIndices, combinedIndices.begin()));

						if (combinedIndices.size() != (min.size() + max.size()) ||
							std::binary_search(combinedIndices.begin(), combinedIndices.end(), globalMinIdx) == true)
						{
							flag = false;
						}

						if ((globalMinIdx > (int)datacount - 1) || (globalMinIdx < -1)) flag = false;
						if (globalMinIdx == -1 && min.size() != 0) flag = false;

						std::vector<int>::iterator minUniqueEnd = std::unique(min.begin(), min.end());
						std::vector<int>::iterator maxUniqueEnd = std::unique(max.begin(), max.end());

						if (minUniqueEnd != min.end() ||
							maxUniqueEnd != max.end() ||
							(minUniqueEnd - min.begin()) != (maxUniqueEnd - max.begin()))
						{
							flag = false;
						}

						return flag;
					}

				protected:
					void MergeComponents(const int firstIdx, const int secondIdx)
					{
						int survivorIdx, destroyedIdx;
						if (Components[firstIdx].MinValue < Components[secondIdx].MinValue)
						{
							survivorIdx = firstIdx;
							destroyedIdx = secondIdx;
						}
						else if (Components[firstIdx].MinValue > Components[secondIdx].MinValue)
						{
							survivorIdx = secondIdx;
							destroyedIdx = firstIdx;
						}
						else if (firstIdx < secondIdx) 
						{
							survivorIdx = firstIdx;
							destroyedIdx = secondIdx;
						}
						else
						{
							survivorIdx = secondIdx;
							destroyedIdx = firstIdx;
						}

						Components[destroyedIdx].Alive = false;

						Colors[Components[destroyedIdx].RightEdgeIndex] = survivorIdx;
						Colors[Components[destroyedIdx].LeftEdgeIndex] = survivorIdx;

						if (Components[survivorIdx].MinIndex > Components[destroyedIdx].MinIndex) //destroyed index to the left of survivor, update left edge
						{
							Components[survivorIdx].LeftEdgeIndex = Components[destroyedIdx].LeftEdgeIndex;
						}
						else
						{
							Components[survivorIdx].RightEdgeIndex = Components[destroyedIdx].RightEdgeIndex;
						}
					}

					void CreatePairedExtrema(const int firstIdx, const int secondIdx)
					{
						TPairedExtrema pair;

						if (data[firstIdx] > data[secondIdx])
						{
							pair.MaxIndex = firstIdx;
							pair.MinIndex = secondIdx;
						}
						else if (data[secondIdx] > data[firstIdx])
						{
							pair.MaxIndex = secondIdx;
							pair.MinIndex = firstIdx;
						}
						else if (firstIdx < secondIdx)
						{
							pair.MinIndex = firstIdx;
							pair.MaxIndex = secondIdx;
						}
						else
						{
							pair.MinIndex = secondIdx;
							pair.MaxIndex = firstIdx;
						}

						pair.Persistence = data[pair.MaxIndex] - data[pair.MinIndex];

						if (PairedExtrema.capacity() == PairedExtrema.size())
						{
							PairedExtrema.reserve(PairedExtrema.size() * 2 + 1);
						}

						PairedExtrema.push_back(pair);
					}

					void CreateComponent(const int minIdx)
					{
						TComponent comp;
						comp.Alive = true;
						comp.LeftEdgeIndex = minIdx;
						comp.RightEdgeIndex = minIdx;
						comp.MinIndex = minIdx;
						comp.MinValue = data[minIdx];

						if (Components.capacity() <= TotalComponents)
						{
							Components.reserve(2 * TotalComponents + 1);
						}

						Components.push_back(comp);
						Colors[minIdx] = TotalComponents;
						TotalComponents++;
					}

					void ExtendComponent(const int componentIdx, const int dataIdx)
					{						
						if (dataIdx + 1 == Components[componentIdx].LeftEdgeIndex)
						{
							Components[componentIdx].LeftEdgeIndex = dataIdx;
						}
						else if (dataIdx - 1 == Components[componentIdx].RightEdgeIndex)
						{
							Components[componentIdx].RightEdgeIndex = dataIdx;
						}
						Colors[dataIdx] = componentIdx;
					}

					void Init()
					{
						SortedData.clear();
						SortedData.reserve(datacount);

						Colors.clear();
						Colors.resize(datacount);
						std::fill(Colors.begin(), Colors.end(), NO_COLOR);

						int vectorSize = (int)(datacount / RESIZE_FACTOR) + 1;

						Components.clear();
						Components.reserve(vectorSize);

						PairedExtrema.clear();
						PairedExtrema.reserve(vectorSize);

						TotalComponents = 0;
						AliveComponentsVerified = false;
					}


					void CreateIndexValueVector()
					{
						if (datacount == 0) return;

						for (std::vector<datatype>::size_type i = 0; i != datacount; i++)
						{
							TIdxAndData dataidxpair;

							dataidxpair.data = data[i];
							dataidxpair.Idx = (int)i;

							SortedData.push_back(dataidxpair);
						}

						std::sort(SortedData.begin(), SortedData.end());
					}


					void Watershed()
					{
						if (SortedData.size() == 1)
						{
							CreateComponent(0);
							return;
						}

						for (std::vector<TIdxAndData>::iterator p = SortedData.begin(); p != SortedData.end(); p++)
						{
							int i = (*p).Idx;
							if (i == 0)
							{
								if (Colors[i + 1] == NO_COLOR)
								{
									CreateComponent(i);
								}
								else
								{
									ExtendComponent(Colors[i + 1], i);
								}

								continue;
							}
							else if (i == Colors.size() - 1)
							{
								if (Colors[i - 1] == NO_COLOR)
								{
									CreateComponent(i);
								}
								else
								{
									ExtendComponent(Colors[i - 1], i);
								}
								continue;
							}

							if (Colors[i - 1] == NO_COLOR && Colors[i + 1] == NO_COLOR) //local minimum - create new component
							{
								CreateComponent(i);
							}
							else if (Colors[i - 1] != NO_COLOR && Colors[i + 1] == NO_COLOR)
							{
								ExtendComponent(Colors[i - 1], i);
							}
							else if (Colors[i - 1] == NO_COLOR && Colors[i + 1] != NO_COLOR) 
							{
								ExtendComponent(Colors[i + 1], i);
							}
							else if (Colors[i - 1] != NO_COLOR && Colors[i + 1] != NO_COLOR)
							{
								int leftComp, rightComp;

								leftComp = Colors[i - 1];
								rightComp = Colors[i + 1];

								if (Components[rightComp].MinValue < Components[leftComp].MinValue)
								{
									CreatePairedExtrema(Components[leftComp].MinIndex, i);
								}
								else
								{
									CreatePairedExtrema(Components[rightComp].MinIndex, i);
								}

								MergeComponents(leftComp, rightComp);
								Colors[i] = Colors[i - 1];
							}
						}
					}

					void SortPairedExtrema()
					{
						std::sort(PairedExtrema.begin(), PairedExtrema.end());
					}

					std::vector<TPairedExtrema>::const_iterator FilterByPersistence(const datatype threshold = 0) const
					{
						if (threshold == 0 || threshold < 0) return PairedExtrema.begin();

						TPairedExtrema searchPair;
						searchPair.Persistence = threshold;
						searchPair.MaxIndex = 0;
						searchPair.MinIndex = 0;
						return(lower_bound(PairedExtrema.begin(), PairedExtrema.end(), searchPair));
					}
				};
			}
		}
	}
}
