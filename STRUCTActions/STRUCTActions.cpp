#include "..\Core\Core.h"
#include "..\Graphics\Graphics.h"
#include "Spline.h"
#include <unordered_map>
#include <filesystem>
using namespace rsdn;
using namespace rsdn::graphics;

/*
void computeSo3feature()
{
	std::vector<std::pair<int, int>> bones;
	bones.push_back(std::make_pair(1, 2));
	bones.push_back(std::make_pair(1, 13));
	bones.push_back(std::make_pair(1, 17));
	bones.push_back(std::make_pair(17, 18));
	bones.push_back(std::make_pair(13, 14));
	bones.push_back(std::make_pair(18, 19));
	bones.push_back(std::make_pair(14, 15));
	bones.push_back(std::make_pair(19, 20));
	bones.push_back(std::make_pair(15, 16));
	bones.push_back(std::make_pair(2, 21));
	bones.push_back(std::make_pair(21, 9));
	bones.push_back(std::make_pair(21, 5));
	bones.push_back(std::make_pair(3, 21));
	bones.push_back(std::make_pair(3, 4));
	bones.push_back(std::make_pair(9, 10));
	bones.push_back(std::make_pair(10, 11));
	bones.push_back(std::make_pair(11, 12));
	bones.push_back(std::make_pair(12, 24));
	bones.push_back(std::make_pair(12, 25));
	bones.push_back(std::make_pair(5, 6));	
	bones.push_back(std::make_pair(6, 7));	
	bones.push_back(std::make_pair(7, 8));	
	bones.push_back(std::make_pair(8, 22));
	bones.push_back(std::make_pair(8, 23));

	std::vector<std::pair<int, int>> angle_bone1_joints;
	std::vector<std::pair<int, int>> angle_bone2_joints;
	for (int i = 0; i < 24; i++)
	{
		for (int j = 0; j < 24; j++)
		{
			if (i != j)
			{
				angle_bone1_joints.push_back(bones[i]);
				angle_bone2_joints.push_back(bones[j]);
			}
		}
	}

	std::wstringstream angle_bone1_joints_str;
	angle_bone1_joints_str << L"angle_bone1_joints=[int]{generic}(" << angle_bone1_joints.size() * 2 << L")";
	for (auto abj : angle_bone1_joints)
	{
		angle_bone1_joints_str << abj.first << ",";
	}
	for (int i = 0; i < angle_bone1_joints.size()-1; i++)
	{
		angle_bone1_joints_str << angle_bone1_joints[i].second << ",";
	}
	angle_bone1_joints_str << angle_bone1_joints.back().second;

	std::wstringstream angle_bone2_joints_str;
	angle_bone2_joints_str << L"angle_bone2_joints=[int]{generic}(" << angle_bone2_joints.size() * 2 << L")"; 
	for (auto abj : angle_bone2_joints)
	{
		angle_bone2_joints_str << abj.first << ",";
	}
	for (int i = 0; i < angle_bone2_joints.size() - 1; i++)
	{
		angle_bone2_joints_str << angle_bone2_joints[i].second << ",";
	}
	angle_bone2_joints_str << angle_bone2_joints.back().second;

	GraphPtr graph = std::make_shared<Graph>();
	DataLayerPtr datalayer = TypeFactory::CreateInstance<DataLayer>(L"rsdn::learning::timeseries::STRUCTDataLayer");
	LayerPtr representationlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::ActionRepresentationLayer");
	OperatorPtr actionnormalizationoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::ActionNormalizationOperator");
	//LayerPtr featureextractionlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayer");
	//OperatorPtr periodicclusteringoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::PeriodicClusteringOperator");
	//OperatorPtr periodicpairwisefeatureextractionoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::PeriodicPairwiseFeatureExtractionOperator");

	std::wstring tmp_bone1 = angle_bone1_joints_str.str();
	std::wstring tmp_bone2 = angle_bone2_joints_str.str();


	std::wstring err;
	auto params_actionnormalizationoperator = data::ParameterStringReader::CreateFromString(L"desired_frames=[int]100", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"mode=[str]So3", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"submode=[str]save", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"samplerate=[int]30", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, angle_bone1_joints_str.str(), err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, angle_bone2_joints_str.str(), err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"samplesoutput=[str]F:\\STRUCT\\train\\normals.samples", err);

	//auto params_periodicclusteringoperator = data::ParameterStringReader::CreateFromString(L"lamda=[float]10.0", err);
	//data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"sparse=[int]100", err);
	//data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"labeldistancebaseterm=[float]2.7182818284590452353602874", err);
	//data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"continuousedge=[float]0.1", err);
	//data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"use_labels=[bool]true", err);
	//data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"label_distancefunction=[int]1", err);
	//data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"labels=[int]{generic}(9)1,2,3,4,5,6,7,8,9", err);

	//auto params_periodicpairwisefeatureextractionoperator = data::ParameterStringReader::CreateFromString(L"pairwise_resolution=[int]50", err);
	//data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"featurecount=[int]-1", err);
	//data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"use_labels=[bool]true", err);
	//data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"label_distancefunction=[int]1", err);
	//data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"labels=[int]{generic}(9)1,2,3,4,5,6,7,8,9", err);
	//data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"midoutput=[str]F:\\STRUCT\\train\\featurepairs.rsdn", err);

	actionnormalizationoperator->Config(params_actionnormalizationoperator);
	//periodicclusteringoperator->Config(params_periodicclusteringoperator);
	//periodicpairwisefeatureextractionoperator->Config(params_periodicpairwisefeatureextractionoperator);

	graph->AddLayer(datalayer);
	graph->AddLayer(representationlayer);
	//graph->AddLayer(featureextractionlayer);
	graph->AddOperator(representationlayer, actionnormalizationoperator);
	//graph->AddOperator(featureextractionlayer, periodicclusteringoperator);
	//graph->AddOperator(featureextractionlayer, periodicpairwisefeatureextractionoperator);

	graph->ConnectFromTo(datalayer, representationlayer);
	//graph->ConnectFromTo(representationlayer, featureextractionlayer);

	bool opened = datalayer->Open(L"F:\\STRUCT\\train\\data.bsdb");

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Feature Extraction] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}
*/

/*
void miningfeaturepairs()
{
	GraphPtr graph = std::make_shared<Graph>();
	LayerPtr representationlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::ActionRepresentationLayer");
	OperatorPtr actionnormalizationoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::ActionNormalizationOperator");
	LayerPtr featureextractionlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayer");
	OperatorPtr periodicclusteringoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::PeriodicClusteringOperator");
	OperatorPtr periodicpairwisefeatureextractionoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::PeriodicPairwiseFeatureExtractionOperator");

	std::wstring err;

	auto params_actionnormalizationoperator = data::ParameterStringReader::CreateFromString(L"desired_frames=[int]100", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"mode=[str]So3", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"submode=[str]load", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"batch=[bool]true", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"batchstart=[int]0", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"samplerate=[int]30", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"samplesinputdir=[str]H:\\STRUCT\\train\\normals\\", err);

	auto params_periodicclusteringoperator = data::ParameterStringReader::CreateFromString(L"sparse=[int]100", err);
	data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"continuousedge=[float]0.1", err);
	data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"use_labels=[bool]true", err);
	data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"label_distancefunction=[int]2", err);
	data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"dbscan_order=[int]3", err);

	auto params_periodicpairwisefeatureextractionoperator = data::ParameterStringReader::CreateFromString(L"pairwise_resolution=[int]50", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"featurecount=[int]-1", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"use_labels=[bool]true", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"pairwise_centroidwise=[int]0", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"labels=[int]{generic}(60)1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"midoutputdir=[str]F:\\STRUCT\\train\\featurepairs\\", err);

	actionnormalizationoperator->Config(params_actionnormalizationoperator);
	periodicclusteringoperator->Config(params_periodicclusteringoperator);
	periodicpairwisefeatureextractionoperator->Config(params_periodicpairwisefeatureextractionoperator);

	graph->AddLayer(representationlayer);
	graph->AddLayer(featureextractionlayer);
	graph->AddOperator(representationlayer, actionnormalizationoperator);
	graph->AddOperator(featureextractionlayer, periodicclusteringoperator);
	graph->AddOperator(featureextractionlayer, periodicpairwisefeatureextractionoperator);

	graph->ConnectFromTo(representationlayer, featureextractionlayer);

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Feature Extraction] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}
*/

/*
void miningeventpairs()
{
	GraphPtr graph = std::make_shared<Graph>();
	LayerPtr eventpairlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::ActionEventPairLayer");

	std::wstring err;

	auto params_eventpairlayer = data::ParameterStringReader::CreateFromString(L"samplesinput=[str]H:\\STRUCT\\train\\normals.samples", err);
	data::ParameterStringReader::AttachTo(params_eventpairlayer, L"output=[str]F:\\STRUCT\\train\\normals.eventpairs", err);
	data::ParameterStringReader::AttachTo(params_eventpairlayer, L"pairwise_eventneighbour=[int]1", err);
	data::ParameterStringReader::AttachTo(params_eventpairlayer, L"pairwise_eventslope=[float]0.25", err);

	eventpairlayer->Config(params_eventpairlayer);

	graph->AddLayer(eventpairlayer);

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Event Pairs Extraction] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}

void selecteventpairs(int desiredpaircount = 100)
{
	GraphPtr graph = std::make_shared<Graph>();
	LayerPtr pairwiselayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::ActionPairwiseLayer");

	std::wstring err;

	auto params_pairwiselayer = data::ParameterStringReader::CreateFromString(L"input=[str]F:\\STRUCT\\train\\normals.eventpairs", err);
	data::ParameterStringReader::AttachTo(params_pairwiselayer, L"output=[str]F:\\STRUCT\\train\\selection_" + std::to_wstring(desiredpaircount) + L".eventpairs", err);
	data::ParameterStringReader::AttachTo(params_pairwiselayer, L"desiredpaircount =[int]"+ std::to_wstring(desiredpaircount), err);

	pairwiselayer->Config(params_pairwiselayer);

	graph->AddLayer(pairwiselayer);

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Pairwise Extraction] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}

void convertSo3feature(int desiredpaircount = 100)
{
	GraphPtr graph = std::make_shared<Graph>();
	LayerPtr dataconvlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::ActionNormalsDataConversionLayer");

	std::wstring err;

	auto params_dataconvlayer = data::ParameterStringReader::CreateFromString(L"samplesinput=[str]H:\\STRUCT\\train\\normals.samples", err);
	data::ParameterStringReader::AttachTo(params_dataconvlayer, L"pairsinput=[str]F:\\STRUCT\\train\\selection_" + std::to_wstring(desiredpaircount) + L".eventpairs", err);
	data::ParameterStringReader::AttachTo(params_dataconvlayer, L"samplesoutputdir=[str]F:\\STRUCT\\train\\normals\\", err);

	dataconvlayer->Config(params_dataconvlayer);

	graph->AddLayer(dataconvlayer);

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Normal Data Conversion] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}

void collectfeaturepairs()
{
	GraphPtr graph = std::make_shared<Graph>();
	LayerPtr representationlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::ActionRepresentationLayer");
	OperatorPtr actionnormalizationoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::ActionNormalizationOperator");
	LayerPtr featureextractionlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayer");
	OperatorPtr pairwisefieldoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::PairwiseFieldOperator");

	std::wstring err;

	auto params_actionnormalizationoperator = data::ParameterStringReader::CreateFromString(L"desired_frames=[int]100", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"mode=[str]So3", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"submode=[str]load", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"batch=[bool]true", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"batchstart=[int]0", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"batchend=[int]-1", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"samplerate=[int]30", err);
	data::ParameterStringReader::AttachTo(params_actionnormalizationoperator, L"samplesinputdir=[str]F:\\STRUCT\\train\\normals\\", err);

	auto params_pairwisefieldoperator = data::ParameterStringReader::CreateFromString(L"batchstability=[int]100", err);
	data::ParameterStringReader::AttachTo(params_pairwisefieldoperator, L"mode=[str]save", err);
	data::ParameterStringReader::AttachTo(params_pairwisefieldoperator, L"branch=[int]6", err);
	data::ParameterStringReader::AttachTo(params_pairwisefieldoperator, L"labels=[int]{generic}(60)1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60", err);
	data::ParameterStringReader::AttachTo(params_pairwisefieldoperator, L"outputdir=[str]H:\\STRUCT\\train\\fields\\", err);


	actionnormalizationoperator->Config(params_actionnormalizationoperator);
	pairwisefieldoperator->Config(params_pairwisefieldoperator);

	graph->AddLayer(representationlayer);
	graph->AddLayer(featureextractionlayer);
	graph->AddOperator(representationlayer, actionnormalizationoperator);
	graph->AddOperator(featureextractionlayer, pairwisefieldoperator);

	graph->ConnectFromTo(representationlayer, featureextractionlayer);

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Feature Extraction 1] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}

void miningfeaturepairs(int desiredpaircount = 100, float fieldimportancelimit=0.5f)
{
	GraphPtr graph = std::make_shared<Graph>();
	LayerPtr featureextractionlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayer");
	OperatorPtr pairwisefieldoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::PairwiseFieldOperator");

	std::wstring err;

	auto params_featureextractionlayer = data::ParameterStringReader::CreateFromString(L"usedata=[bool]false", err);


	auto params_pairwisefieldoperator = data::ParameterStringReader::CreateFromString(L"mode=[str]load", err);
	data::ParameterStringReader::AttachTo(params_pairwisefieldoperator, L"fieldimportancelimit=[float]" + std::to_wstring(fieldimportancelimit), err);
	data::ParameterStringReader::AttachTo(params_pairwisefieldoperator, L"pairsinput=[str]F:\\STRUCT\\train\\selection_" + std::to_wstring(desiredpaircount) + L".eventpairs", err);
	data::ParameterStringReader::AttachTo(params_pairwisefieldoperator, L"inputdir=[str]F:\\STRUCT\\train\\fields\\", err);
	data::ParameterStringReader::AttachTo(params_pairwisefieldoperator, L"pairsoutput=[str]F:\\STRUCT\\train\\featpairs_" + std::to_wstring(desiredpaircount) + L".rsdn", err);

	featureextractionlayer->Config(params_featureextractionlayer);
	pairwisefieldoperator->Config(params_pairwisefieldoperator);

	graph->AddLayer(featureextractionlayer);
	graph->AddOperator(featureextractionlayer, pairwisefieldoperator);


	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Feature Extraction 2] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}

class PairCandicate
{
public:
	std::pair<int, int> Combination;
	int A;
	int B;
	std::vector<datatype> Factors;
	std::vector<sizetype> Indices;
	datatype Sparsity;
	datatype Quality;
	datatype Split;

	PairCandicate() : Sparsity(0.0)
	{

	}
};

template<>
struct std::hash<std::pair<int, int>>
{
	size_t operator()(const std::pair<int, int>& x) const throw()
	{
		return std::hash<int>()(x.first) ^ std::hash<int>()(x.second);
	}
};

class PairCandicate_EqualFn
{
public:
	bool operator() (const std::pair<int, int>& x1, const std::pair<int, int>& x2) const
	{
		return (x1.first == x2.second && x1.second == x2.first) || (x1.first == x2.first && x1.second == x2.second);
	}
};

void toprankfeaturepairs(int desiredpaircount = 100, int topn = 1200)
{
	std::wstring pairsinput = L"F:\\STRUCT\\train\\featpairs_" + std::to_wstring(desiredpaircount) + L".rsdn";
	rsdn::data::BSDBReaderPtr reader = std::make_shared<rsdn::data::BSDBReader>();
	if (reader->Load(pairsinput, [](const std::wstring& head, int ver)-> bool
	{
		return head.compare(L"rsdn::learning::timeseries::operators:PairwiseFieldOperator=>featurecandicates") == 0 && ver > -1;
	}))
	{
		BinaryBufferPtr buf = std::make_shared<BinaryBuffer>();
		if (reader->Read(buf, 0))
		{
			std::vector<PairCandicate> selcandicates;
			std::unordered_map<std::pair<int, int>, std::vector<PairCandicate>, std::hash<std::pair<int,int>>, PairCandicate_EqualFn> candicates;
			std::unordered_map<std::pair<int, int>, datatype> candavgqual;
			double cansumavgqual = 0;
			std::unordered_map<std::pair<int, int>, int> candselnums;
			auto cpubuf = buf->GetCpuReadonlyBuffer();
			__int64 candicatenum = 0ll;
			cpubuf->Read(&candicatenum, 1);
			for (__int64 i = 0; i < candicatenum; i++)
			{
				int vinum = 0;
				cpubuf->Read(&vinum, 1);
				PairCandicate pc{};
				pc.Factors.resize(vinum);
				pc.Indices.resize(vinum);
				cpubuf->Read(&pc.A, 1);
				cpubuf->Read(&pc.B, 1);
				cpubuf->Read(&pc.Combination.first, 1);
				cpubuf->Read(&pc.Combination.second, 1);
				cpubuf->Read(pc.Indices.data(), vinum);
				cpubuf->Read(pc.Factors.data(), vinum);
				cpubuf->Read(&pc.Sparsity, 1);
				cpubuf->Read(&pc.Quality, 1);
				cpubuf->Read(&pc.Split, 1);
				candicates[std::make_pair(pc.A, pc.B)].push_back(pc);
			}

			for (const auto& cand : candicates)
			{
				datatype avgq = 0;
				for (const auto& c : cand.second)
				{
					avgq += c.Quality;
				}
				candavgqual[cand.first] = avgq / (datatype)cand.second.size();
				cansumavgqual += candavgqual[cand.first];
			}
			
			for (const auto& candq : candavgqual)
			{
				candselnums[candq.first] = (int)floor((double)candq.second / cansumavgqual* (double)topn + 0.5);
			}

			for (auto& cand : candicates)
			{
				auto& candlist = cand.second;
				int cn = candlist.size();
				std::vector<__int64> indices;
				indices.resize(cn);
				for (__int64 i = 0; i < cn; i++) indices[i] = i;
				std::sort(indices.begin(), indices.end(), [&candlist](__int64 a, __int64 b)-> bool
				{
					return candlist[a].Sparsity < candlist[b].Sparsity;
				});

				int ln = candselnums[cand.first];
				for (int nn = 0; nn < ln && nn< cn; nn++)
				{
					selcandicates.push_back(candlist[nn]);
				}
			}
			
			std::wstring pairsoutput = L"F:\\STRUCT\\train\\feats_" + std::to_wstring(topn) + L".rsdn";

			__int64 topnum = selcandicates.size();

			rsdn::data::BSDBWriterPtr writer = std::make_shared<rsdn::data::BSDBWriter>();
			if (writer->Open(pairsoutput, L"rsdn::learning::timeseries::operators:PairwiseFieldOperator=>featurecandicates", 0))
			{
				writer->FinishItems();
				writer->ReserveChunks(1);
				writer->StartChunk();
				writer->WriteToChunk(&topnum, sizeof(__int64));
				for (int n=0; n<topnum; n++)
				{
					const auto& pc = selcandicates[n];
					int vinum = (int)pc.Indices.size();
					writer->WriteToChunk(&vinum, sizeof(int));
					writer->WriteToChunk(&pc.A, sizeof(int));
					writer->WriteToChunk(&pc.B, sizeof(int));
					writer->WriteToChunk(&pc.Combination.first, sizeof(int));
					writer->WriteToChunk(&pc.Combination.second, sizeof(int));
					writer->WriteToChunk(pc.Indices.data(), sizeof(sizetype)* vinum);
					writer->WriteToChunk(pc.Factors.data(), sizeof(datatype)* vinum);				
					writer->WriteToChunk(&pc.Sparsity, sizeof(datatype));
					writer->WriteToChunk(&pc.Quality, sizeof(datatype));
					writer->WriteToChunk(&pc.Split, sizeof(datatype));
				}
				writer->EndChunk();
				writer->FinishChunks();
				writer->Close();
			}
		}
	}

}

void generatetrainingsamples(int topn = 1200)
{
	GraphPtr graph = std::make_shared<Graph>();
	LayerPtr samplerlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::ActionNormalsSamplerLayer");

	std::wstring err;

	auto params_samplerlayer = data::ParameterStringReader::CreateFromString(L"samplesinput=[str]H:\\STRUCT\\train\\normals.samples", err);
	data::ParameterStringReader::AttachTo(params_samplerlayer, L"pairsinput=[str]F:\\STRUCT\\train\\feats_" + std::to_wstring(topn) + L".rsdn", err);
	data::ParameterStringReader::AttachTo(params_samplerlayer, L"labels=[int]{generic}(60)1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60", err);
	data::ParameterStringReader::AttachTo(params_samplerlayer, L"samplesoutput=[str]F:\\STRUCT\\train\\training_" + std::to_wstring(topn) + L".samples", err);

	samplerlayer->Config(params_samplerlayer);

	graph->AddLayer(samplerlayer);

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Generating Training Samples] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;

}

void train(int topn = 1200, int tree=60, int depth=20)
{
	GraphPtr graph = std::make_shared<Graph>();
	DataLayerPtr datalayer = TypeFactory::CreateInstance<DataLayer>(L"rsdn::data::SampleChunkLayer");
	LayerPtr randomforestlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::machines::supervised::RandomForestLayer");

	std::wstring err;

	auto params_randomforestlayer = data::ParameterStringReader::CreateFromString(L"mode=[str]rstrain", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"lake=[int]"+std::to_wstring(tree), err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"depth=[int]"+std::to_wstring(depth), err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"job=[int]6", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"output=[str]F:\\STRUCT\\train\\train_" + std::to_wstring(topn) + L".model", err);

	randomforestlayer->Config(params_randomforestlayer);

	graph->AddLayer(datalayer);
	graph->AddLayer(randomforestlayer);

	graph->ConnectFromTo(datalayer, randomforestlayer);

	bool opened = datalayer->Open(L"F:\\STRUCT\\train\\training_" + std::to_wstring(topn) + L".samples");

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Training] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}

void test(int topn = 1200)
{
	GraphPtr graph = std::make_shared<Graph>();
	DataLayerPtr datalayer = TypeFactory::CreateInstance<DataLayer>(L"rsdn::data::SampleChunkLayer");
	LayerPtr randomforestlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::machines::supervised::RandomForestLayer");

	std::wstring err;

	auto params_randomforestlayer = data::ParameterStringReader::CreateFromString(L"mode=[str]rstest", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"job=[int]1", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"input=[str]F:\\STRUCT\\train\\train_" + std::to_wstring(topn) + L".model", err);

	randomforestlayer->Config(params_randomforestlayer);

	graph->AddLayer(datalayer);
	graph->AddLayer(randomforestlayer);

	graph->ConnectFromTo(datalayer, randomforestlayer);

	bool opened = datalayer->Open(L"F:\\STRUCT\\train\\training_" + std::to_wstring(topn) + L".samples");

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Training] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;

}

void render1()
{
	std::wstring samplesinput = L"H:\\STRUCT\\train\\normals.samples";

	FileStream infile;
	if (infile.Open(samplesinput.c_str(), FileAccess::Read, FileShare::Read, FileMode::Open))
	{
		int desiredframes = 0;
		infile.Read<int>(desiredframes, false);
		int chcount = 0;
		infile.Read<int>(chcount, false);
		int seqrecorded = 0;
		infile.Read<int>(seqrecorded, false);

		BufferPtr rawseqchunkbuf = std::make_shared<Buffer>(chcount, desiredframes);
		datatype* rawseqchunk = rawseqchunkbuf->GetCpu();

		BufferPtr seqchunkbuf = std::make_shared<Buffer>(chcount, desiredframes);
		datatype* seqchunk = seqchunkbuf->GetCpu();

		std::vector<datatype> iframes;
		iframes.resize(desiredframes);
		int job = Runtime::Get().GetCpuJobCount();
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
				#pragma omp parallel for num_threads(job)
				for (int j = 0; j < chcount; j++)
				{
					rsdn::learning::timeseries::operators::details::Spline spline;
					if (spline.set_points1(iframes.data(), (seqchunk + j*desiredframes), (size_t)desiredframes))
					{
						spline.interp_points(iframes.data(), (seqchunk + j*desiredframes), (size_t)desiredframes);
					}
				}
			}

			graphics::ImagePtr img = std::make_shared<graphics::Image>(chcount, desiredframes);
			for (int j = 0; j < chcount; j++)
			{
				auto chcpu = seqchunk + j * desiredframes;
				for (int di = 0; di < desiredframes; di++)
				{
					datatype trgb = 255;
					unsigned char r = 255;
					unsigned char g = 255;
					unsigned char b = 255;
					datatype vgb = chcpu[di];
					if (vgb >= 0.0)
					{
						r = 255;
						g = vgb > 1.0 ? 0 : (1.0 - vgb) * 255;
						b = g;
					}
					else
					{
						r = vgb < -1.0 ? 0 : -255 * (vgb + 1.0);
						g = vgb < -1.0 ? 200 : 255 + 55 * vgb;
						b = 255;
					}
					img->Draw(j, di, r, g, b);
				}
			}
			std::wstring dir = L"F:\\STRUCT\\train\\render\\1\\Action_" + std::to_wstring(seqlab);
			if (!std::experimental::filesystem::exists(dir)) std::experimental::filesystem::create_directory(dir);
			img->SaveAsPng(dir + L"\\" + std::to_wstring(i) + L".png");

			Logger::Get().Report(LogLevel::Info) << i + 1 << L" / " << seqrecorded << L" sequences finished" << Logger::endl;
		}
		infile.Close();
	}
}

int main()
{
	Random::Initilize();
	Runtime::Get().SetCpuJobCount(4);
	Logger::Get().Init(L"H:\\STRUCT\\train\\log.txt");

	Graphics::Initilize();

	computeSo3feature();

	//computeSo3feature();	
	//miningeventpairs();
	//selecteventpairs(120);
	//convertSo3feature(120);
	//collectfeaturepairs();
	//miningfeaturepairs(120, 0.7);

	//int topn = 1600;

	//toprankfeaturepairs(120, topn);

	//generatetrainingsamples(topn);

	//train(topn, 720, 30);
	//test(topn);

	render1();

	Random::Shutdown();
	Graphics::Shutdown();

    return 0;
}

*/

int main()
{
	ResultPtr ret;
	auto graph = Graph::LoadFromFile(L"C:\\Users\\yemx21\\Desktop\\cross-subject train.graph.txt", ret, std::locale{});





	return 0;
}

