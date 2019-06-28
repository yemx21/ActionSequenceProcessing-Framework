#pragma once
#include "DecisionLake.h"
#include <deque>
#define RANDOMSEAJOBPERCHUNK 1
#define RANDOMSEAVALIDATE 6

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

					template<class Selector>
					class RandomSea
					{
					public:
						RandomSea() :_data(nullptr)
						{
						}

						~RandomSea()
						{
							for (auto* lake : _lakes)
							{
								if (lake) { delete lake; lake = nullptr; }
							}
							_lakes.clear();

							for (auto* selector : _selectors)
							{
								if (selector) { delete selector; selector = nullptr; }
							}
							_selectors.clear();
						}

					public:
						void Train(SampleChunk* data, const std::vector<sizetype>& labels, sizetype maxriver, sizetype maxlevel, sizetype majorfeaturecount, sizetype sidefeaturecount, sizetype minsamplecount, int job)
						{
							_data = data;
							_labels = labels;
							sizetype featureCount = data->FeatureCount;

							sizetype datalen = data->Count;
							std::vector<sizetype> indexs(datalen);
							for (sizetype i = 0; i < datalen; i++) indexs[i] = i;

							/*create feature indexs*/
							std::vector<sizetype> featureindexs = std::vector<sizetype>(featureCount);
							for (sizetype i = 0; i < featureCount; i++)
							{
								featureindexs[i] = i;
							}

							if (maxriver == 1)
							{
								_lakes.resize(1, nullptr); 
								_selectors.resize(1, nullptr);
								flowtobranch(0, data, 1, featureindexs, indexs, labels, maxlevel, majorfeaturecount, minsamplecount);
							}
							else
							{
								_lakes.resize(maxriver, nullptr);
								_selectors.resize(maxriver, nullptr);

								int chunklen = RANDOMSEAJOBPERCHUNK* job;

								size_t lakecount = 0;

								size_t lakechunk = (int)ceil((float)maxriver / chunklen);

								std::deque<datatype> accblock;
								Optional<datatype> lastavgacc;
								int accele = 0;
								bool trainconv = false;
								std::mutex messagemutex;

								for (sizetype chunk = 0; chunk < lakechunk; chunk++)
								{
									int uppern = (chunk + 1) * chunklen;
									if (uppern > maxriver) uppern = maxriver;
									#pragma omp parallel for num_threads(job)
									for (sizetype n = chunk * chunklen; n < uppern; n++)
									{
										std::vector<sizetype> sindexs(datalen);
										for (sizetype i = 0; i < datalen; i++)
										{
											sizetype rng = Random::Generate() % datalen;
											sindexs[i] = indexs[rng];
										}

										if (flowtobranch(n, data, n + 1, featureindexs, sindexs, labels, maxlevel, majorfeaturecount, minsamplecount))
										{
											#pragma omp atomic
											lakecount++;
										}

										{
											std::lock_guard<std::mutex> locker(messagemutex);
											Logger::Get().Report(LogLevel::Info) << lakecount << L" lake finished" << Logger::endl;
										}
									}

									auto tmpret = Predict(data);
									sizetype correct = 0;
									for (sizetype n = 0; n < datalen; n++)
									{
										if (tmpret[n] == data->Label[n]) correct++;
									}

									if (accele < RANDOMSEAVALIDATE)
									{
										accblock.push_back((datatype)((double)correct / datalen));
										accele++;
									}
									
									if (accele == RANDOMSEAVALIDATE)
									{
										datatype avgacc = 0;
										for (int n = 0; n < RANDOMSEAVALIDATE; n++) avgacc += accblock[n];
										avgacc /= RANDOMSEAVALIDATE;

										Logger::Get().Report(LogLevel::Info) << L"chunk acc=" << avgacc * 100.0 << L"%" << Logger::endl;

										if (lastavgacc)
										{
											if (abs(avgacc - *lastavgacc) < 5e-4)
											{
												trainconv = true;
												break;
											}
										}
										lastavgacc = avgacc;
										accblock.pop_front();
										accele--;
									}
								}

								if (trainconv)
								{
									Logger::Get().Report(LogLevel::Info) << L"end up with lakecount=" << lakecount<<  L" lake finished" << Logger::endl;
									_lakes.erase(_lakes.begin() + lakecount, _lakes.end());
									_selectors.erase(_selectors.begin() + lakecount, _selectors.end());
									return;
								}

								#pragma omp parallel for num_threads(job)
								for (sizetype n = lakechunk * chunklen; n < maxriver; n++)
								{
									std::vector<sizetype> sindexs(datalen);
									for (sizetype i = 0; i < datalen; i++)
									{
										sizetype rng = Random::Generate() % datalen;
										sindexs[i] = indexs[rng];
									}

									if (flowtobranch(n, data, n + 1, featureindexs, sindexs, labels, maxlevel, majorfeaturecount, minsamplecount))
									{
										#pragma omp atomic
										lakecount++;
									}

									{
										std::lock_guard<std::mutex> locker(messagemutex);
										Logger::Get().Report(LogLevel::Info) << lakecount << L" lake finished" << Logger::endl;
									}
								}
							}
						}

						std::vector<sizetype> Predict(SampleChunk* data)
						{
							std::vector<sizetype> labret;
							std::vector<sizetype> ret;
							sizetype count = data->Count;
							int featcount = (int)data->FeatureCount;
							std::map<sizetype, size_t> emptyvotes;
							for (sizetype lab : _labels) emptyvotes.insert(std::make_pair(lab, 0ull));

							for (sizetype i = 0; i < count; i++)
							{
								auto feats = data->Features[i];

								std::map<sizetype, size_t> votes = emptyvotes;
								for (DecisionLake* lake : _lakes)
								{
									if (!lake) continue;
									votes[lake->Predict(feats, featcount)]++;
								}

								auto mele = std::max_element(votes.begin(), votes.end(), [](const std::pair<sizetype, size_t>& a, const std::pair<sizetype, size_t>& b) {
									return a.second < b.second;
								});
								ret.push_back(mele->first);
							}
							return ret;
						}

						sizetype Predict(float* feature, int count)
						{
							std::map<sizetype, size_t> emptyvotes;
							for (sizetype lab : _labels) emptyvotes.insert(std::make_pair(lab, 0ull));

							std::map<sizetype, size_t> votes = emptyvotes;
							for (DecisionLake* lake : _lakes)
							{
								if (!lake) continue;
								votes[lake->Predict(feature, count)]++;
							}

							auto mele = std::max_element(votes.begin(), votes.end(), [](const std::pair<sizetype, size_t>& a, const std::pair<sizetype, size_t>& b) {
								return a.second < b.second;
							});

							return mele->first;
						}

						sizetype ComponentPredict(int idx, float* feature, int count)
						{
							return _lakes[idx]->Predict(feature, count);
						}


						sizetype Predict_prob(float* feature, int count, std::vector<double>& clss, std::vector<double>& probs)
						{
							clss.clear();
							probs.clear();

							std::map<sizetype, size_t> emptyvotes;
							for (sizetype lab : _labels) emptyvotes.insert(std::make_pair(lab, 0ull));

							std::map<sizetype, size_t> votes = emptyvotes;

							for (DecisionLake* lake : _lakes)
							{
								if (!lake) continue;
								votes[lake->Predict(feature, count)]++;
							}

							auto mele = std::max_element(votes.begin(), votes.end(), [](const std::pair<sizetype, size_t>& a, const std::pair<sizetype, size_t>& b) {
								return a.second < b.second;
							});

							for (const auto& v : votes)
							{
								clss.push_back(v.first);
								probs.push_back(v.second);
							}

							return mele->first;
						}

						int GetComponentCount() const
						{
							return (int)_lakes.size();
						}

						void Load(const wchar_t* path)
						{
							for (auto lake : _lakes)
							{
								if (lake) { delete lake; lake = nullptr; }
							}
							_lakes.clear();
							_lakes.shrink_to_fit();
							_labels.clear();
							_labels.shrink_to_fit();

							FileStream file;
							if (file.Open(path, FileAccess::Read, FileShare::Read, FileMode::Open))
							{
								int labelcount = 0;
								file.Read((char*)&labelcount, sizeof(int), 0, sizeof(int));
								_labels.resize(labelcount, -1);
								file.Read((char*)_labels.data(), sizeof(sizetype)* labelcount, 0, sizeof(sizetype)* labelcount);

								int lakecount = 0;
								file.Read((char*)&lakecount, sizeof(int), 0, sizeof(int));

								for (int i = 0; i < lakecount; i++)
								{
									_lakes.push_back(DecisionLake::Load(&file));
								}

								file.Close();
							}
						}

						void Save(const wchar_t* path)
						{
							FileStream file;
							if (file.Open(path, FileAccess::Write, FileShare::Write, FileMode::Create))
							{
								int labelcount = (int)_labels.size();
								file.Write((char*)&labelcount, sizeof(int), 0, sizeof(int));
								file.Write((char*)_labels.data(), sizeof(sizetype)* labelcount, 0, sizeof(sizetype)* labelcount);


								int lakecount = (int)_lakes.size();
								file.Write((char*)&lakecount, sizeof(int), 0, sizeof(int));

								for (int i = 0; i < lakecount; i++)
								{
									_lakes[i]->Save(&file);
								}

								file.Close();
							}
						}

						void RemoveLakesExcept(const std::vector<int>& lakeindices)
						{
							if (!_lakes.empty())
							{
								std::vector<DecisionLake*> newlakes;
								for (int i : lakeindices)
								{
									newlakes.push_back(_lakes[i]);
								}
								_lakes = newlakes;

							}

							if (!_selectors.empty())
							{
								std::vector<IFeatureSelector*> newselectors;
								for (int i : lakeindices)
								{
									newselectors.push_back(_selectors[i]);
								}
								_selectors = newselectors;
							}
						}

					private:
						const SampleChunk* _data;
						std::vector<sizetype> _labels;
						std::vector<IFeatureSelector*> _selectors;

					public:
						std::vector<DecisionLake*> _lakes;

					private:
						bool flowtobranch(size_t lakeidx, SampleChunk* data, sizetype id, const std::vector<sizetype>& featureindexs, const std::vector<sizetype>& indexs, const std::vector<sizetype>& labels, sizetype maxlevel, sizetype majorfeaturecount, sizetype minsamplecount)
						{
							auto selector = new Selector();
							auto lake = DecisionLake::Create(id, selector, data, featureindexs, indexs, labels, maxlevel, majorfeaturecount, minsamplecount);
							if (lake)
							{
								_selectors[lakeidx] = selector;
								_lakes[lakeidx] = lake;
								return true;
							}
							delete selector;
							return false;
						}
					};
				}
			}
		}
	}
}
