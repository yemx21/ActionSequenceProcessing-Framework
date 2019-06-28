#include "..\Core\Core.h"
#include "..\Graphics\Graphics.h"
#include <filesystem>
using namespace rsdn;
using namespace rsdn::graphics;

void generatetrainsamples(int featcount)
{
	GraphPtr graph = std::make_shared<Graph>();
	DataLayerPtr datalayer = TypeFactory::CreateInstance<DataLayer>(L"rsdn::learning::timeseries::SkeletonDataLayer");
	LayerPtr representationlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::SkeletonRepresentationLayer");
	OperatorPtr gaitphaserepresentationoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::GaitPhaseRepresentationOperator");

	std::wstring err;
	auto params_gaitphaserepresentationoperator = data::ParameterStringReader::CreateFromString(L"featinput=[str]F:\\GC\\Train\\model\\" + std::to_wstring(featcount) +L"\\featurepairs_"+std::to_wstring(featcount) +L".rsdn", err);
	data::ParameterStringReader::AttachTo(params_gaitphaserepresentationoperator, L"rawsamplerate=[int]30", err);
	data::ParameterStringReader::AttachTo(params_gaitphaserepresentationoperator, L"resamplerate=[int]120", err);
	data::ParameterStringReader::AttachTo(params_gaitphaserepresentationoperator, L"labels=[int;generic;9]1,2,3,4,5,6,7,8,9", err);
	data::ParameterStringReader::AttachTo(params_gaitphaserepresentationoperator, L"samplesoutput=[str]F:\\GC\\Train\\model\\" + std::to_wstring(featcount) + L"\\train.samples", err);

	gaitphaserepresentationoperator->Config(params_gaitphaserepresentationoperator);

	graph->AddLayer(datalayer);
	graph->AddLayer(representationlayer);
	graph->AddOperator(representationlayer, gaitphaserepresentationoperator);

	graph->ConnectFromTo(datalayer, representationlayer);

	bool opened = datalayer->Open(L"F:\\GC\\Train\\data.bsdb");

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Generating Training Samples] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;

}

void train(const std::wstring& path, int featcount)
{
	GraphPtr graph = std::make_shared<Graph>();
	DataLayerPtr datalayer = TypeFactory::CreateInstance<DataLayer>(L"rsdn::data::SampleChunkLayer");
	LayerPtr randomforestlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::machines::supervised::RandomForestLayer");

	std::wstring err;

	auto params_randomforestlayer = data::ParameterStringReader::CreateFromString(L"mode=[str]rstrain", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"lake=[int]30", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"depth=[int]20", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"job=[int]6", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"output=[str]"+ path, err);

	randomforestlayer->Config(params_randomforestlayer);

	graph->AddLayer(datalayer);
	graph->AddLayer(randomforestlayer);

	graph->ConnectFromTo(datalayer, randomforestlayer);

	bool opened = datalayer->Open(L"F:\\GC\\Train\\model\\" + std::to_wstring(featcount) + L"\\train.samples");

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Training] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}

void generatetestsamples(int featcount)
{
	GraphPtr graph = std::make_shared<Graph>();
	DataLayerPtr datalayer = TypeFactory::CreateInstance<DataLayer>(L"rsdn::learning::timeseries::SkeletonDataLayer");
	LayerPtr representationlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::SkeletonRepresentationLayer");
	OperatorPtr gaitphaserepresentationoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::GaitPhaseRepresentationOperator");

	std::wstring err;
	auto params_gaitphaserepresentationoperator = data::ParameterStringReader::CreateFromString(L"featinput=[str]F:\\GC\\Train\\model\\" + std::to_wstring(featcount) + L"\\featurepairs_" + std::to_wstring(featcount)+L".rsdn", err);
	data::ParameterStringReader::AttachTo(params_gaitphaserepresentationoperator, L"rawsamplerate=[int]30", err);
	data::ParameterStringReader::AttachTo(params_gaitphaserepresentationoperator, L"resamplerate=[int]120", err);
	data::ParameterStringReader::AttachTo(params_gaitphaserepresentationoperator, L"labels=[int;generic;9]1,2,3,4,5,6,7,8,9", err);
	data::ParameterStringReader::AttachTo(params_gaitphaserepresentationoperator, L"samplesoutput=[str]F:\\GC\\Test\\model\\" + std::to_wstring(featcount) + L"\\test.samples", err);

	gaitphaserepresentationoperator->Config(params_gaitphaserepresentationoperator);

	graph->AddLayer(datalayer);
	graph->AddLayer(representationlayer);
	graph->AddOperator(representationlayer, gaitphaserepresentationoperator);

	graph->ConnectFromTo(datalayer, representationlayer);

	bool opened = datalayer->Open(L"F:\\GC\\Test\\data.bsdb");

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Generating Training Samples] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;

}

void test(const std::wstring& path, int featcount)
{
	GraphPtr graph = std::make_shared<Graph>();
	DataLayerPtr datalayer = TypeFactory::CreateInstance<DataLayer>(L"rsdn::data::SampleChunkLayer");
	LayerPtr randomforestlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::machines::supervised::RandomForestLayer");

	std::wstring err;

	auto params_randomforestlayer = data::ParameterStringReader::CreateFromString(L"mode=[str]rstest", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"job=[int]1", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"input=[str]"+ path, err);

	randomforestlayer->Config(params_randomforestlayer);

	graph->AddLayer(datalayer);
	graph->AddLayer(randomforestlayer);

	graph->ConnectFromTo(datalayer, randomforestlayer);

	//bool opened = datalayer->Open(L"F:\\GC\\Train\\train.samples");
	bool opened = datalayer->Open(L"F:\\GC\\Test\\model\\" + std::to_wstring(featcount) + L"\\test.samples");

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Training] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;

}

void test1(const std::wstring& path, int featcount)
{
	GraphPtr graph = std::make_shared<Graph>();
	DataLayerPtr datalayer = TypeFactory::CreateInstance<DataLayer>(L"rsdn::data::SampleChunkLayer");
	LayerPtr randomforestlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::machines::supervised::RandomForestLayer");

	std::wstring err;

	auto params_randomforestlayer = data::ParameterStringReader::CreateFromString(L"mode=[str]rstest", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"job=[int]1", err);
	data::ParameterStringReader::AttachTo(params_randomforestlayer, L"input=[str]" + path, err);

	randomforestlayer->Config(params_randomforestlayer);

	graph->AddLayer(datalayer);
	graph->AddLayer(randomforestlayer);

	graph->ConnectFromTo(datalayer, randomforestlayer);

	//bool opened = datalayer->Open(L"F:\\GC\\Train\\train.samples");
	bool opened = datalayer->Open(L"F:\\GC\\Train\\model\\" + std::to_wstring(featcount) + L"\\train.samples");

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Training] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;

}


void miningfeaturepair()
{
	GraphPtr graph = std::make_shared<Graph>();
	DataLayerPtr datalayer = TypeFactory::CreateInstance<DataLayer>(L"rsdn::learning::timeseries::SkeletonDataLayer");
	LayerPtr representationlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::SkeletonRepresentationLayer");
	OperatorPtr gaitcyclenormalizationoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::GaitCycleNormalizationOperator");
	LayerPtr featureextractionlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayer");
	OperatorPtr periodicclusteringoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::PeriodicClusteringOperator");
	OperatorPtr periodicpairwisefeatureextractionoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::PeriodicPairwiseFeatureExtractionOperator");

	std::wstring err;
	auto params_gaitcyclenormalizationoperator = data::ParameterStringReader::CreateFromString(L"desired_frames=[int]100", err);
	data::ParameterStringReader::AttachTo(params_gaitcyclenormalizationoperator, L"rawsamplerate=[int]30", err);
	data::ParameterStringReader::AttachTo(params_gaitcyclenormalizationoperator, L"resamplerate=[int]120", err);
	data::ParameterStringReader::AttachTo(params_gaitcyclenormalizationoperator, L"samplesoutput=[str]F:\\GC\\Train\\normals.samples", err);

	auto params_periodicclusteringoperator = data::ParameterStringReader::CreateFromString(L"lamda=[float]10.0", err);
	data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"sparse=[int]100", err);
	data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"labeldistancebaseterm=[float]2.7182818284590452353602874", err);
	data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"continuousedge=[float]0.1", err);
	data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"use_labels=[bool]true", err);
	data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"label_distancefunction=[int]1", err);
	data::ParameterStringReader::AttachTo(params_periodicclusteringoperator, L"labels=[int;generic;9]1,2,3,4,5,6,7,8,9", err);

	auto params_periodicpairwisefeatureextractionoperator = data::ParameterStringReader::CreateFromString(L"pairwise_eventneighbour=[int]2", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"featurecount=[int]-1", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"use_labels=[bool]true", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"label_distancefunction=[int]1", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"labels=[int;generic;9]1,2,3,4,5,6,7,8,9", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"midoutput=[str]F:\\GC\\Train\\featurepairs.rsdn", err);

	gaitcyclenormalizationoperator->Config(params_gaitcyclenormalizationoperator);
	periodicclusteringoperator->Config(params_periodicclusteringoperator);
	periodicpairwisefeatureextractionoperator->Config(params_periodicpairwisefeatureextractionoperator);

	graph->AddLayer(datalayer);
	graph->AddLayer(representationlayer);
	graph->AddLayer(featureextractionlayer);
	graph->AddOperator(representationlayer, gaitcyclenormalizationoperator);
	graph->AddOperator(featureextractionlayer, periodicclusteringoperator);
	graph->AddOperator(featureextractionlayer, periodicpairwisefeatureextractionoperator);

	graph->ConnectFromTo(datalayer, representationlayer);
	graph->ConnectFromTo(representationlayer, featureextractionlayer);

	bool opened = datalayer->Open(L"F:\\GC\\Train\\data.bsdb");

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Feature Extraction] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}

void findfeaturepair(int featcount)
{
	GraphPtr graph = std::make_shared<Graph>();
	LayerPtr featureextractionlayer = TypeFactory::CreateInstance<Layer>(L"rsdn::learning::timeseries::MultivariateTimeSeriesFeatureExtractionLayer");
	OperatorPtr periodicpairwisefeatureextractionoperator = TypeFactory::CreateInstance<Operator>(L"rsdn::learning::timeseries::operators::PeriodicPairwiseFeatureExtractionOperator");

	std::wstring err;
	auto params_featureextractionlayer = data::ParameterStringReader::CreateFromString(L"usedata=[bool]false", err);

	auto params_periodicpairwisefeatureextractionoperator = data::ParameterStringReader::CreateFromString(L"midinput=[str]F:\\GC\\Train\\featurepairs.rsdn", err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"featurecount=[int]"+std::to_wstring(featcount), err);
	data::ParameterStringReader::AttachTo(params_periodicpairwisefeatureextractionoperator, L"output=[str]F:\\GC\\Train\\model\\"+ std::to_wstring(featcount) +L"\\featurepairs_" + std::to_wstring(featcount)+L".rsdn", err);

	featureextractionlayer->Config(params_featureextractionlayer);
	periodicpairwisefeatureextractionoperator->Config(params_periodicpairwisefeatureextractionoperator);

	graph->AddLayer(featureextractionlayer);
	graph->AddOperator(featureextractionlayer, periodicpairwisefeatureextractionoperator);

	auto task = graph->RunAsync();

	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Task [Feature Extraction] Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;
}

int main()
{
	Random::Initilize();
	Runtime::Get().SetCpuJobCount(4);
	Logger::Get().Init(L"F:\\GC\\Train\\log.txt");

	Graphics::Initilize();

	wchar_t exebuf[MAX_PATH];
	GetModuleFileNameW(NULL, exebuf, MAX_PATH);
	std::wstring fullExeName(exebuf);
	std::wstring directory;
	const size_t last_slash_idx = fullExeName.rfind('\\');
	if (std::string::npos != last_slash_idx) directory = fullExeName.substr(0, last_slash_idx);

	TypeFactory::LoadAssembly(directory + L"\\Learning.dll");
	TypeFactory::LoadAssembly(directory + L"\\Human.dll");

	//miningfeaturepair();

	//int feats[] = { 30, 70, 150, 220, 300, 400, 600, 800, 1000 };
	
	int feats[] = { 30, 70, 120, 180, 250, 330, 420, 520, 630, 750, 880, 1020 };

	int featidx = 11;
	int featcount = feats[featidx];

	std::wstring traindir_model = L"F:\\GC\\Train\\model\\" + std::to_wstring(featcount) + L"\\";
	std::wstring testdir_model = L"F:\\GC\\Test\\model\\" + std::to_wstring(featcount) + L"\\";
	if (!std::experimental::filesystem::exists(traindir_model)) std::experimental::filesystem::create_directory(traindir_model);
	if (!std::experimental::filesystem::exists(testdir_model)) std::experimental::filesystem::create_directory(testdir_model);

	//findfeaturepair(featcount);

	//generatetrainsamples(featcount);

	/*
	
	for (int n = 1; n < 3; )
	{
		try
		{
			train(L"F:\\GC\\Train\\model\\" + std::to_wstring(featcount) + L"\\train_" + std::to_wstring(n) + L".model", featcount);
			n++;
		}
		catch (...) {}
	}
	*/
	//generatetestsamples(featcount);
	
	/*
	for (int n = 0; n < 10; n++)
	{
		test(L"F:\\GC\\Train\\model\\" + std::to_wstring(featcount) + L"\\train_" + std::to_wstring(n) + L".model", featcount);
	}*/
	
	for (int n = 0; n < 10; n++)
	{
		test1(L"F:\\GC\\Train\\model\\" + std::to_wstring(featcount) + L"\\train_" + std::to_wstring(n) + L".model", featcount);
	}

	Random::Shutdown();
	Graphics::Shutdown();

	return 0;
}

