#include "ActionNormalsDataConversionLayer.h"
#include "STRUCTDataLayer.h"
#include "ActionDataPacket.h"
#include "ActionRepresentationDataPacket.h"
#include "Spline.h"
using namespace rsdn::learning::timeseries;

ActionNormalsDataConversionLayer::ActionNormalsDataConversionLayer() 
{
	
}

ActionNormalsDataConversionLayer::~ActionNormalsDataConversionLayer()
{

}

std::unique_ptr<std::future<ResultPtr>> ActionNormalsDataConversionLayer::RunAsync(std::atomic<bool>& cancel)
{
	return std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, [&cancel, &param = params]()
	{
		ResultPtr ret = std::make_shared<Result>(false);
		try
		{
			std::wstring samplesinput;
			try
			{
				samplesinput = param->GetItem<std::wstring>(L"samplesinput");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: samplesinput";
				return ret;
			}

			std::wstring pairsinput;
			try
			{
				pairsinput = param->GetItem<std::wstring>(L"pairsinput");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: pairsinput";
				return ret;
			}

			std::wstring samplesoutputdir;
			try
			{
				samplesoutputdir = param->GetItem<std::wstring>(L"samplesoutputdir");
			}
			catch (...)
			{
				ret->Message = L"invalid parameter: samplesoutputdir";
				return ret;
			}

			bool interp = true;
			try
			{
				interp = param->GetItem<bool>(L"interp");
			}
			catch (...)
			{
				Logger::Get().Report(LogLevel::Info) << "interp sets to default \"true\"" << Logger::endl;
			}

			std::vector<std::pair<int, int>> real_feats;
			FileStream pairfile;
			if (pairfile.Open(pairsinput.c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
			{
				int pairnum = 0;
				pairfile.Read(pairnum, false);

				for (int si=0; si< pairnum; si++)
				{
					std::pair<int, int> rf = { -1, -1 };
					pairfile.Read(rf.first, false);
					pairfile.Read(rf.second, false);
					real_feats.push_back(rf);
				}
				pairfile.Close();

				if (pairnum > 0)
				{
					FileStream infile;
					if (infile.Open(samplesinput.c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
					{
						int desiredframes = 0;
						infile.Read<int>(desiredframes, false);
						int chcount = 0;
						infile.Read<int>(chcount, false);
						int seqrecorded = 0;
						infile.Read<int>(seqrecorded, false);

						std::set<int> ufeatframes;
						for (const auto& rf : real_feats)
						{
							ufeatframes.insert(rf.first);
							ufeatframes.insert(rf.second);
						}

						std::vector<int> featframes;
						for (int f : ufeatframes)
						{
							featframes.push_back(f);
						}
						std::sort(featframes.begin(), featframes.end());

						int featframecount = (int)featframes.size();
						int featcount = (int)real_feats.size();
						std::shared_ptr<GenericBuffer<int>> feats_left_buf = std::make_shared<GenericBuffer<int>>(featcount);
						std::shared_ptr<GenericBuffer<int>> feats_right_buf = std::make_shared<GenericBuffer<int>>(featcount);
						int* feats_left = feats_left_buf->GetCpu();
						int* feats_right = feats_right_buf->GetCpu();
						auto iter = real_feats.begin();
						for (int i = 0; i < featcount; i++, iter++)
						{
							feats_left[i] = iter->first;
							feats_right[i] = iter->second;
						}

						std::vector<std::shared_ptr<FileStream>> outfiles;
						for (int i = 0; i < featframecount; i++)
						{
							auto cofile = std::make_shared<FileStream>();
							std::wstring cofilepath = samplesoutputdir + L"featframe_" + std::to_wstring(featframes[i]) + L".samples";
							if (cofile->Open(cofilepath.c_str(), FileAccess::Write, FileShare::Write, FileMode::Create))
							{
								int i_real = featframes[i];
								cofile->Write((char*)&i_real, sizeof(int), 0, sizeof(int));
								cofile->Write((char*)&chcount, sizeof(int), 0, sizeof(int));
								cofile->Write((char*)&seqrecorded, sizeof(int), 0, sizeof(int));

								outfiles.push_back(cofile);
							}
							else
							{
								for (auto& fs : outfiles)
								{
									fs->CloseAndDelete();
								}
								infile.Close();
								ret->Error = L"can not create samples file: " + cofilepath;
								return ret;
							}
						}

						std::vector<std::shared_ptr<FileStream>> outfiles2;
						for (int i = 0; i < featcount; i++)
						{
							auto cofile = std::make_shared<FileStream>();
							std::wstring fofilepath = samplesoutputdir + L"featframe_" + std::to_wstring(i) + L".featpair";
							if (cofile->Open(fofilepath.c_str(), FileAccess::Write, FileShare::Write, FileMode::Create))
							{
								int i_real = i;
								cofile->Write((char*)&i_real, sizeof(int), 0, sizeof(int));
								cofile->Write((char*)&feats_left[i], sizeof(int), 0, sizeof(int));
								cofile->Write((char*)&feats_right[i], sizeof(int), 0, sizeof(int));
								cofile->Flush();
								cofile->Close();
								outfiles2.push_back(cofile);
							}
							else
							{
								for (auto& fs : outfiles2)
								{
									fs->CloseAndDelete();
								}
								infile.Close();
								ret->Error = L"can not create featpair file: " + fofilepath;
								return ret;
							}
						}

						BufferPtr rawseqchunkbuf = std::make_shared<Buffer>(chcount, desiredframes);
						datatype* rawseqchunk = rawseqchunkbuf->GetCpu();

						if (!interp)
						{
							for (int i = 0; i < seqrecorded; i++)
							{
								int seqlab = -1;
								infile.Read<int>(seqlab, false);
								for (int di = 0; di < desiredframes; di++)
								{
									auto tmpcpu = rawseqchunk + di*chcount;
									infile.Read((char*)tmpcpu, sizeof(datatype)* chcount, 0, sizeof(datatype)* chcount);
								}
								for (int fi = 0; fi < featframecount; fi++)
								{
									outfiles[fi]->Write((char*)&seqlab, sizeof(int), 0, sizeof(int));
									auto tmpcpu = rawseqchunk + featframes[fi] * chcount;
									outfiles[fi]->Write((char*)tmpcpu, sizeof(datatype)* chcount, 0, sizeof(datatype)* chcount);
								}
								Logger::Get().Report(LogLevel::Info) << i + 1 << L"sequences finished" << Logger::endl;
							}
						}
						else
						{
							BufferPtr seqchunkbuf = std::make_shared<Buffer>(chcount, desiredframes);
							datatype* seqchunk = seqchunkbuf->GetCpu();

							std::vector<datatype> iframes;
							iframes.resize(desiredframes);
							for (int j = 0; j < desiredframes; j++) iframes[j] = j;
							for (int i = 0; i < seqrecorded; i++)
							{
								int seqlab = -1;
								infile.Read<int>(seqlab, false);
								for (int di = 0; di < desiredframes; di++)
								{
									auto tmpcpu = rawseqchunk + di*chcount;
									infile.Read((char*)tmpcpu, sizeof(datatype)* chcount, 0, sizeof(datatype)* chcount);
									for (int j = 0; j < chcount; j++)
									{
										(seqchunk + j*desiredframes)[di] = tmpcpu[j];
									}
								}

								{
									#pragma omp parallel for num_threads(Runtime::CpuJobCount())
									for (int j = 0; j < chcount; j++)
									{
										rsdn::learning::timeseries::operators::details::Spline spline;
										if (spline.set_points1(iframes.data(), (seqchunk + j*desiredframes), (size_t)desiredframes))
											spline.interp_points(iframes.data(), (seqchunk + j*desiredframes), (size_t)desiredframes);
									}
								}


								for (int fi = 0; fi < featframecount; fi++)
								{
									outfiles[fi]->Write((char*)&seqlab, sizeof(int), 0, sizeof(int));
									auto tmpcpu = rawseqchunk + featframes[fi] * chcount;
									outfiles[fi]->Write((char*)tmpcpu, sizeof(datatype)* chcount, 0, sizeof(datatype)* chcount);
								}

								Logger::Get().Report(LogLevel::Info) << i + 1 << L" / " << seqrecorded << L" sequences finished" << Logger::endl;
							}
						}
						for (auto& fs : outfiles)
						{
							fs->Close();
						}
						infile.Close();
					}
					ret->State = true;
				}
			}
		}
		catch (const std::exception& excep)
		{
			ret->Error = Converter::Convert(excep.what());
		}
		return ret;
	}));
}


ResultPtr ActionNormalsDataConversionLayer::IsSupportConnectFrom(_type prev)
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionNormalsDataConversionLayer::IsSupportConnectTo(_type next)
{
	return std::make_shared<Result>(false, L"next layer is unsupported");
}

ResultPtr ActionNormalsDataConversionLayer::ReadyCore()
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionNormalsDataConversionLayer::InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev)
{
	return std::make_shared<Result>(true);
}

ResultPtr ActionNormalsDataConversionLayer::OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& parameter, _type next)
{
	parameter = params;
	return std::make_shared<Result>(true);
}

ResultPtr ActionNormalsDataConversionLayer::ConfigCore(data::ParameterPacketPtr param)
{
	params = param;
	return std::make_shared<Result>(true);
}
