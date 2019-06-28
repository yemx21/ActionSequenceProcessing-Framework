#include <tchar.h>
#include "..\Core\Core.h"
#include "..\Graphics\Graphics.h"
#include <filesystem>
using namespace rsdn;
using namespace rsdn::graphics;

int _tmain(int argc, _TCHAR *argv[])
{
	if (argc <= 1) { system("pause"); return 1; }

	std::experimental::filesystem::path path = argv[1];

	if (!std::experimental::filesystem::exists(path)) 
	{
		std::wcout << "can not find: " << path << std::endl;
		system("pause"); 
		return 2;
	}

	Random::Initilize();
	Graphics::Initilize();


	ResultPtr ret;
	auto graph = Graph::LoadFromFile(path.wstring(), ret, std::locale{});
	if (!graph)
	{
		std::wcout << "can not load graph: " << ret->Error << std::endl;
		system("pause");
		return 3;
	}

	auto task = graph->RunAsync();
	task->Wait();

	auto taskret = task->Get();

	std::wcout << L"Status: " << (taskret->State ? L"OK" : L"Fail") << std::endl
		<< L"Message: " << taskret->Message << std::endl
		<< L"Error: " << taskret->Error << std::endl;

	Random::Shutdown();
	Graphics::Shutdown();


	system("pause");

    return 0;
}

