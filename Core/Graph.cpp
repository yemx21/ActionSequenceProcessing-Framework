#include "Graph.h"
#include "Results.h"
#include <boost\thread\mutex.hpp>
#include <boost\serialization\singleton.hpp> 
#include "Node.h"
#include "MLON.h"
#include "DataLayer.h"
#include "ParameterLoader.h"
using namespace rsdn;
using namespace rsdn::data;
using namespace rsdn::layer;

namespace rsdn
{
	class Graph_impl: public boost::serialization::singleton<Graph_impl>
	{
	private:
		boost::mutex _locker;
		int64_t _generator;
	public:
		int64_t generateId()
		{
			boost::unique_lock<boost::mutex> scoped_lock(_locker);
			return _generator++;
		}

		static std::unique_ptr<std::future<ResultPtr>> RunAsync(LayerPtr layer, std::atomic<bool>& cancel)
		{
			return layer->RunAsync(cancel);
		}

		static std::unique_ptr<std::future<ResultPtr>> RunBatchAsync(LayerPtr layer, int batch, std::atomic<bool>& cancel)
		{
			return layer->RunBatchAsync(cancel, batch);
		}

		static unsigned int GetBatchCount(LayerPtr layer)
		{
			return layer->GetBatchCount();
		}

		static ResultPtr ConfigBatch(LayerPtr layer, unsigned int batch)
		{
			return layer->ConfigBatch(batch);
		}

		static std::unique_ptr<std::future<ResultPtr>> PrepareBatchAsync(LayerPtr layer, std::atomic<bool>& cancel)
		{
			return layer->PrepareBatchAsync(cancel);
		}
	};

	Node::Node(): Finished(false), BatchCount(0), CurrentBatch(0)
	{

	}
}

bool Graph::AddLayer(LayerPtr layer)
{
	if (layer->_id == -1)
	{		
		layer->_id = Graph_impl::get_mutable_instance().generateId();
		pool.insert(std::make_pair(layer->_id, layer));
		children.insert(std::make_pair(layer->_id, std::set<int64_t>{}));
		parents.insert(std::make_pair(layer->_id, std::set<int64_t>{}));
		return true;
	}
	return false;
}

void Graph::RemoveLayer(LayerPtr layer)
{
	if (layer->_id != -1)
	{
		if (pool.find(layer->_id) != pool.end())
		{
			pool.erase(layer->_id);
			children.erase(layer->_id);
			parents.erase(layer->_id);
			for (auto& cc : children)
			{
				cc.second.erase(layer->_id);
			}
			for (auto& pp : parents)
			{
				pp.second.erase(layer->_id);
			}

			layer->_id = -1;
		}
	}
}

bool Graph::ConnectFromTo(LayerPtr layer1, LayerPtr layer2)
{
	bool canadd = false;

	if (layer1->_id != -1 && layer2->_id != -1)
	{
		if (pool.find(layer1->_id) != pool.end() && pool.find(layer2->_id) != pool.end())
		{
			if (children.find(layer2->_id) != children.end())
			{
				if (children[layer2->_id].find(layer1->_id) == children[layer2->_id].end())
					canadd = true;
			}
			else
				canadd = true;			
		}
	}
	if (canadd)
	{
		children[layer1->_id].insert(layer2->_id);
		parents[layer2->_id].insert(layer1->_id);
		return true;
	}
	return false;
}

void Graph::DisconnectFromTo(LayerPtr layer1, LayerPtr layer2)
{
	if (layer1->_id != -1 && layer2->_id != -1)
	{
		if (pool.find(layer1->_id) != pool.end() && pool.find(layer2->_id) != pool.end())
		{
			children[layer1->_id].erase(layer2->_id);
			parents[layer2->_id].erase(layer1->_id);
		}
	}
}

bool Graph::IsConnected(LayerPtr layer1, LayerPtr layer2)
{
	if (layer1->_id != -1 && layer2->_id != -1)
	{
		if (pool.find(layer1->_id) != pool.end() && pool.find(layer2->_id) != pool.end())
		{
			if (children.find(layer1->_id) != children.end())
			{
				if (children[layer1->_id].find(layer2->_id) != children[layer1->_id].end())
				{
					return true;
				}
			}
			else if(children.find(layer2->_id) != children.end())
			{
				if (children[layer2->_id].find(layer1->_id) != children[layer2->_id].end())
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool Graph::GetLevelForward(int64_t id, std::map<int64_t, Optional<int>>& levels)
{
	auto& lvl = levels[id];
	if (lvl.IsInit()) return true;

	int maxpid = -1;
	for (const auto& pid : parents[id])
	{
		int lvl_pid = GetLevelForward(pid, levels);
		if (lvl_pid > maxpid)
		{
			maxpid = lvl_pid;
		}
	}
	if (maxpid == -1) return false;
	levels[id] = maxpid + 1;
	return true;
}

bool Graph::GetLevelBackward(int64_t id, std::map<int64_t, Optional<int>>& levels)
{
	auto& lvl = levels[id];
	if (lvl.IsInit()) return true;

	int minpid = -1;
	for (const auto& pid : children[id])
	{
		int lvl_pid = GetLevelBackward(pid, levels);
		if (lvl_pid < minpid)
		{
			minpid = lvl_pid;
		}
	}
	if (minpid == -1) return false;
	levels[id] = minpid - 1;
	return true;
}

bool Graph::RearrangeLevels(std::map<int64_t, Optional<int>>& levels)
{
	for (const auto& lay : pool)
	{
		const auto& chs = children[lay.first];
		for (const auto& cid : children[lay.first])
		{
			if (parents[cid].size() > 1)
			{
				int maxplvl = *levels[lay.first];
				for (const auto& pid : parents[cid])
				{
					if (*levels[pid] > maxplvl)
					{
						maxplvl = *levels[pid];
					}
				}

				if (*levels[cid] > maxplvl + 1)
				{
					levels[cid] = maxplvl + 1;
				}
			}
			else if(parents[cid].size() == 1)
			{
				if (children[cid].empty())
				{
					levels[cid] = *levels[lay.first] + 1;
				}
			}
		}
	}
	return true;
}

ResultPtr Graph::AddOperator(LayerPtr layer, OperatorPtr op)
{
	if (pool.find(layer->_id) == pool.end()) return std::make_shared<Result>(false, L"layer is not registered");
	return layer->AddOperator(op);
}

AsyncResultPtr Graph::RunAsync(bool standalonefirst)
{
	MultiAsyncResultPtr ret = std::make_shared<MultiAsyncResult>();
	ret->_owner = this;

	std::map<int64_t, Optional<int>> levels;
	std::map<int64_t, Optional<int>> finallevels;
	std::set<int64_t> starts;

	for (const auto& lay : pool)
	{
		if (parents[lay.first].empty())
		{
			levels.insert(std::make_pair(lay.first, Optional<int>(1)));
			if (children[lay.first].empty())
			{
				starts.insert(lay.first);
			}
		}
		else
		{
			levels.insert(std::make_pair(lay.first, Optional<int>()));
		}
	}

	for (const auto& lay : pool)
	{
		if (!GetLevelForward(lay.first, levels)) throw std::exception("invalid graph route");
	}

	int maxlvl = -1;
	for (const auto& lvl : levels)
	{
		if (lvl.second)
		{
			if (*lvl.second > maxlvl) maxlvl = *lvl.second;
		}
	}

	for (const auto& lay : pool)
	{
		if (children[lay.first].empty() && starts.find(lay.first)==starts.end())
		{
			levels.insert(std::make_pair(lay.first, Optional<int>(maxlvl)));
		}
		else
		{
			levels.insert(std::make_pair(lay.first, Optional<int>()));
		}
	}

	finallevels = levels;

	for (const auto& lay : pool)
	{
		if (!GetLevelBackward(lay.first, finallevels)) throw std::exception("invalid graph route");
	}

	RearrangeLevels(finallevels);

	for (const auto& sid : starts)
	{
		const auto& cids = children[sid];
		if (cids.empty())
		{
			ret->_standalones.push_back(pool[sid]->RunAsync(ret->_cancellation));
		}
	}

	std::map<int, std::set<int64_t>> lvllist;
	std::set<int> lvls;
	std::vector<int> sortedlvls;
	for (const auto& flvl : finallevels)
	{
		lvllist[*flvl.second].insert(flvl.first);
		lvls.insert(*flvl.second);
		if (*flvl.second > maxlvl) maxlvl = *flvl.second;
	}
	for (int l : lvls) sortedlvls.push_back(l);
	std::sort(sortedlvls.begin(), sortedlvls.end());
	size_t lvllen = sortedlvls.size();

	ret->_nodes.resize(lvllen);
	for (size_t i = 0; i< lvllen - 1; i++)
	{
		int curlvl = sortedlvls[i];
		int nxtlvl = sortedlvls[i + 1];
		const auto& curlvllist = lvllist[curlvl];
		const auto& nxtlvllist = lvllist[nxtlvl];

		for (const auto& curid : curlvllist)
		{
			auto pnodefound = std::find_if(ret->_nodes[i].begin(), ret->_nodes[i].end(), [&curid](const NodePtr& nod)
			{
				return nod->Item->Id == curid;
			});
			NodePtr pnode = nullptr;
			if (pnodefound != ret->_nodes[i].end())
			{
				pnode = *pnodefound;
			}
			else
			{
				pnode = std::make_shared<Node>();
				pnode->Item = pool[curid];
				ret->_nodes[i].push_back(pnode);
			}	
			for (const auto& chid : children[curid])
			{
				if (nxtlvllist.find(chid) != nxtlvllist.end())
				{
					auto cnode = std::make_shared<Node>();
					cnode->Item = pool[chid];
					cnode->Previous.push_back(pnode);
					pnode->Next.push_back(cnode);
					ret->_nodes[i + 1].push_back(cnode);
				}
			}
		}
	}

	ret->Build();

	return ret;
}

constexpr wchar_t Graph_root[] = L"graph";
constexpr wchar_t Graph_connection[] = L"connection";
constexpr wchar_t Graph_datalayer[] = L"datalayer";
constexpr wchar_t Graph_layer[] = L"layer";
constexpr wchar_t Graph_type[] = L"type";
constexpr wchar_t Graph_name[] = L"name";
constexpr wchar_t Graph_params[] = L"params";
constexpr wchar_t Graph_operators[] = L"operators";
constexpr wchar_t Graph_operator[] = L"operator";
constexpr wchar_t Graph_datalayer_open[] = L"open";
constexpr wchar_t Graph_connection_from[] = L"from";
constexpr wchar_t Graph_connection_to[] = L"to";

constexpr wchar_t Graph_runtime[] = L"runtime";
constexpr wchar_t Graph_runtime_mode[] = L"mode";
constexpr wchar_t Graph_runtime_cpujob[] = L"cpujob";

GraphPtr Graph::LoadFromFile(const std::wstring& path, ResultPtr& ret, const std::locale& loc)
{
	ret = std::make_shared<Result>(false, L"unknown");

	GraphPtr graph = std::make_shared<Graph>();

	MLONParser parser{};
	ret = parser.Load(path, loc);

	std::map<std::wstring, LayerPtr> table;
	std::vector<std::pair<std::wstring, std::wstring>> connections;


	if (ret->State)
	{
		int importnum = parser.ImportCount;
		for (int i = 0; i < importnum; i++)
		{
			if (!Runtime::Load(parser.Import[i]->Value))
			{
				ret->Error = L"can not load dll " + parser.Import[i]->Value;
				goto ERR;
			}
		}

		/*runtime*/
		auto runtime = parser.QueryObject(Graph_runtime);
		if (runtime)
		{
			auto runtime_mode = runtime->QueryNode(Graph_runtime_mode);
			if (runtime_mode)
			{
				if (runtime_mode->Value.compare(L"gpu") != 0)
				{
					Runtime::Get().SetMode(ComputationMode::CPU);
					Logger::Get().Report(LogLevel::Info) << "runtime mode is set to cpu" << Logger::endl;
				}
				else
				{
					Runtime::Get().SetMode(ComputationMode::GPU);
					Logger::Get().Report(LogLevel::Info) << "runtime mode is set to gpu" << Logger::endl;
				}
			}
			auto runtime_cpujob = runtime->QueryNode(Graph_runtime_cpujob);
			if (runtime_cpujob)
			{
				try
				{
					int cpujob = std::stoi(runtime_cpujob->Value);
					Runtime::Get().SetCpuJobCount(cpujob);
					Logger::Get().Report(LogLevel::Info) << "CPU Job is set to " << std::to_wstring(cpujob) << Logger::endl;
				}
				catch (...)
				{
					SYSTEM_INFO sysinfo;
					GetSystemInfo(&sysinfo);
					int numCPU = sysinfo.dwNumberOfProcessors;
					numCPU /= 2;
					if (numCPU < 1) numCPU = 1;
					Runtime::Get().SetCpuJobCount(numCPU);
					Logger::Get().Report(LogLevel::Info) << "CPU Job is set to default which is half of maxinum core numbers: " << std::to_wstring(numCPU) << Logger::endl;
				}
			}
		}

		/*root*/
		auto root = parser.QueryObject(Graph_root);
		if (root)
		{
			int rootobjs = root->ObjectCount;

			for (int i = 0; i < rootobjs; i++)
			{
				auto robj = root->Object[i];
				if (robj->Identifier.compare(Graph_datalayer) == 0)
				{
					auto name = robj->QueryNode(Graph_name);
					if (name)
					{
						auto type = robj->QueryNode(Graph_type);
						if (type)
						{
							DataLayerPtr datalayer = TypeFactory::CreateInstance<DataLayer>(type->Value);
							if (datalayer)
							{
								auto params = robj->QueryNode(Graph_params);
								if (params)
								{
									int paramnum = params->NodeCount;
									ParameterPacketPtr parampacket = nullptr;
									std::wstring paramerr;
									for (int p = 0; p < paramnum; p++)
									{
										auto par = params->Node[p];
										if (!parampacket)
										{
											parampacket = ParameterStringReader::CreateFromString(par->Identifier, par->Value, paramerr);
											if (!parampacket)
											{
												ret->Error = paramerr;
												goto ERR;
											}
										}
										else
										{
											if (!ParameterStringReader::AttachTo(parampacket, par->Identifier, par->Value, paramerr))
											{
												ret->Error = paramerr;
												goto ERR;
											}
										}
									}
									if (parampacket)
									{
										ret = datalayer->Config(parampacket);
										if (!ret->State) goto ERR;
									}
								}
								auto open = robj->QueryNode(Graph_datalayer_open);
								if (open)
								{
									if (!datalayer->Open(open->Value))
									{
										ret->Error = L"can not open " + open->Value + L" by " + type->Value;
										goto ERR;
									}
								}
								auto operators = robj->QueryNode(Graph_operators);
								if (operators)
								{
									int operatornum = operators->ObjectCount;
									for (int o = 0; o < operatornum; o++)
									{
										auto op_obj = operators->Object[o];
										if (op_obj->Identifier.compare(Graph_operator) == 0)
										{
											auto opname = op_obj->QueryNode(Graph_name);
											if (opname)
											{
												auto optype = op_obj->QueryNode(Graph_type);
												if (optype)
												{
													OperatorPtr op = TypeFactory::CreateInstance<Operator>(optype->Value);
													if (op)
													{
														auto params = op_obj->QueryNode(Graph_params);
														if (params)
														{
															int paramnum = params->NodeCount;
															ParameterPacketPtr parampacket = nullptr;
															std::wstring paramerr;
															for (int p = 0; p < paramnum; p++)
															{
																auto par = params->Node[p];
																if (!parampacket)
																{
																	parampacket = ParameterStringReader::CreateFromString(par->Identifier, par->Value, paramerr);
																	if (!parampacket)
																	{
																		ret->Error = paramerr;
																		goto ERR;
																	}
																}
																else
																{
																	if (!ParameterStringReader::AttachTo(parampacket, par->Identifier, par->Value, paramerr))
																	{
																		ret->Error = paramerr;
																		goto ERR;
																	}
																}
															}
															if (parampacket)
															{
																ret = op->Config(parampacket);
																if (!ret->State) goto ERR;
															}
														}

														ret = datalayer->AddOperator(op);
														if (!ret->State) goto ERR;
													}
												}
											}
										}
									}
								}
								table[name->Value] = datalayer;
							}
						}
					}
				}
				else if (robj->Identifier.compare(Graph_layer) == 0)
				{
					auto name = robj->QueryNode(Graph_name);
					if (name)
					{
						auto type = robj->QueryNode(Graph_type);
						if (type)
						{
							LayerPtr layer = TypeFactory::CreateInstance<Layer>(type->Value);
							if (layer)
							{
								auto params = robj->QueryNode(Graph_params);
								if (params)
								{
									int paramnum = params->NodeCount;
									ParameterPacketPtr parampacket = nullptr;
									std::wstring paramerr;
									for (int p = 0; p < paramnum; p++)
									{
										auto par = params->Node[p];
										if (!parampacket)
										{
											parampacket = ParameterStringReader::CreateFromString(par->Identifier, par->Value, paramerr);
											if (!parampacket)
											{
												ret->Error = paramerr;
												goto ERR;
											}
										}
										else
										{
											if (!ParameterStringReader::AttachTo(parampacket, par->Identifier, par->Value, paramerr))
											{
												ret->Error = paramerr;
												goto ERR;
											}
										}
									}
									if (parampacket)
									{
										ret = layer->Config(parampacket);
										if (!ret->State) goto ERR;
									}
								}

								auto operators = robj->QueryNode(Graph_operators);
								if (operators)
								{
									int operatornum = operators->ObjectCount;
									for (int o = 0; o < operatornum; o++)
									{
										auto op_obj = operators->Object[o];
										if (op_obj->Identifier.compare(Graph_operator) == 0)
										{
											auto opname = op_obj->QueryNode(Graph_name);
											if (opname)
											{
												auto optype = op_obj->QueryNode(Graph_type);
												if (optype)
												{
													OperatorPtr op = TypeFactory::CreateInstance<Operator>(optype->Value);
													if (op)
													{
														auto params = op_obj->QueryNode(Graph_params);
														if (params)
														{
															int paramnum = params->NodeCount;
															ParameterPacketPtr parampacket = nullptr;
															std::wstring paramerr;
															for (int p = 0; p < paramnum; p++)
															{
																auto par = params->Node[p];
																if (!parampacket)
																{
																	parampacket = ParameterStringReader::CreateFromString(par->Identifier, par->Value, paramerr);
																	if (!parampacket)
																	{
																		ret->Error = paramerr;
																		goto ERR;
																	}
																}
																else
																{
																	if (!ParameterStringReader::AttachTo(parampacket, par->Identifier, par->Value, paramerr))
																	{
																		ret->Error = paramerr;
																		goto ERR;
																	}
																}
															}
															if (parampacket)
															{
																ret = op->Config(parampacket);
																if (!ret->State) goto ERR;
															}
														}

														ret = layer->AddOperator(op);
														if (!ret->State) goto ERR;
													}
												}
											}
										}
									}
								}
								table[name->Value] = layer;
							}
						}
					}
				}
				else if (robj->Identifier.compare(Graph_connection) == 0)
				{
					auto connection_from = robj->QueryNode(Graph_connection_from);
					auto connection_to = robj->QueryNode(Graph_connection_to);

					if (connection_from && connection_to)
					{
						auto from_name = connection_from->Value;
						auto to_name = connection_to->Value;
						if (!from_name.empty() && !to_name.empty())
						{
							connections.push_back(std::make_pair(from_name, to_name));
						}	
					}
				}
			}

			if (!table.empty())
			{
				for (auto& it : table)
				{
					if (!graph->AddLayer(it.second))
					{
						ret->Error = L"failed to add layer " + it.first;
						goto ERR;
					}
				}

				for (const auto& cc : connections)
				{
					if (table.find(cc.first) != table.end())
					{
						if (table.find(cc.second) != table.end())
						{
							if (!graph->ConnectFromTo(table[cc.first], table[cc.second]))
							{
								ret->Error = L"failed to connect layers " + cc.first + L" to " + cc.second;
								goto ERR;
							}
						}
						else
						{
							ret->Error = L"can not find layer " + cc.second;
							goto ERR;
						}
					}
					else
					{
						ret->Error = L"can not find layer " + cc.first;
						goto ERR;
					}
				}
				return graph;
			}
		}
	}
ERR:



	return nullptr;
}


MultiAsyncResult::MultiAsyncResult():_level(-1), _standalonesfirst(true), _cancellation(false)
{

}

ResultPtr MultiAsyncResult::Run(MultiAsyncResult* me)
{
	ResultPtr ret = std::make_shared<Result>(false);
	if (me->_standalonesfirst)
	{
		for (auto& task : me->_standalones)
		{
			ResultPtr tmpret = task->get();
			if (!tmpret->State)
			{
				return tmpret;
			}
		}
	}

	int maxlvl = (int)me->_nodes.size();
	/*level 0*/
	{
		me->_level = 0;
		const auto& mnodes = me->_nodes[me->_level];
		for (const auto& node : mnodes)
		{
			auto readyret = node->Item->Ready();
			if (readyret->State)
			{
				auto tpb= Graph_impl::PrepareBatchAsync(node->Item, me->_cancellation);
				if (tpb)
				{
					tpb->wait();
					auto tmptpb = tpb->get();
					if (!tmptpb->State)
					{
						ret = tmptpb;
						return ret;
					}
				}
				node->BatchCount = Graph_impl::GetBatchCount(node->Item);
				node->CurrentBatch = 0;

				if (node->BatchCount == 0)
				{
					auto tret = Graph_impl::RunAsync(node->Item, me->_cancellation);
					tret->wait();
					auto tmpasync = tret->get();
					if (!tmpasync->State)
					{
						ret = tmpasync;
						return ret;
					}
				}
			}
			else
			{
				ret->Message = L"layer is not ready: " + std::wstring(node->Item->GetType()->GetName()) + L"[" + std::to_wstring(node->Item->GetId()) + L"]";
				ret->Error = readyret->Message;
				return ret;
			}
		}
		me->_level = 1;
	}

	for (; me->_level < maxlvl; me->_level++)
	{
		const auto& curnodes = me->_nodes[me->_level];
		for (const auto& node : curnodes)
		{
			auto nodety = node->Item->GetType();
			for (const auto& prnode : node->Previous)
			{
				if (prnode->BatchCount > 0)
				{
					for (; prnode->CurrentBatch < prnode->BatchCount; prnode->CurrentBatch++)
					{
						auto tret = Graph_impl::RunBatchAsync(prnode->Item, prnode->CurrentBatch, me->_cancellation);
						tret->wait();
						auto tmpasync = tret->get();
						if (!tmpasync->State)
						{
							ret = tmpasync;
							break;
						}
						else
						{
							rsdn::data::DataPacketPtr prdata;
							rsdn::data::ParameterPacketPtr prparam;
							auto tmpret = prnode->Item->Out(prdata, prparam, nodety);
							if (!tmpret->State)
							{
								return tmpret;
							}
							else
							{
								tmpret = node->Item->In(prdata, prparam, prnode->Item->GetType());
								if (!tmpret->State)
								{
									return tmpret;
								}
							}
							auto tret1 = Graph_impl::ConfigBatch(node->Item, prnode->CurrentBatch);
							if (!tret1->State)
							{
								ret = tret1;
								break;
							}
							else
							{
								auto readyret = node->Item->Ready();
								if (readyret->State)
								{
									auto tret2 = Graph_impl::RunAsync(node->Item, me->_cancellation);
									tret2->wait();
									auto tmp2async = tret2->get();
									if (!tmp2async->State)
									{
										ret = tmp2async;
										break;
									}
								}
							}
						}
					}
				}
				else
				{
					rsdn::data::DataPacketPtr prdata;
					rsdn::data::ParameterPacketPtr prparam;
					auto tmpret = prnode->Item->Out(prdata, prparam, nodety);
					if (!tmpret->State)
					{
						return tmpret;
					}
					else
					{
						tmpret = node->Item->In(prdata, prparam, prnode->Item->GetType());
						if (!tmpret->State)
						{
							return tmpret;
						}
					}

					auto readyret = node->Item->Ready();
					if (readyret->State)
					{
						auto tret = Graph_impl::RunAsync(node->Item, me->_cancellation);
						tret->wait();
						auto tmpasync = tret->get();
						if (!tmpasync->State)
						{
							ret = tmpasync;
						}
					}
					else
					{
						ret->Message = L"layer is not ready: " + std::wstring(node->Item->GetType()->GetName()) + L"[" + std::to_wstring(node->Item->GetId()) + L"]";
						ret->Error = readyret->Message;
						return ret;
					}
				}
			}
		}
	}

	ret->State = true;

	return ret;
}

void MultiAsyncResult::Build()
{
	_all = std::make_unique<std::future<ResultPtr>>(std::async(std::launch::deferred, &MultiAsyncResult::Run, this));
}

MultiAsyncResult::~MultiAsyncResult()
{
	_cancellation = true;
	if(!_all->_Is_ready()) _all->wait();
}

void MultiAsyncResult::OnCancel()
{
	_cancellation = true;
}

void MultiAsyncResult::OnWait()
{
	_all->wait();
}

ResultPtr MultiAsyncResult::OnGet()
{
	return _all->get();
}