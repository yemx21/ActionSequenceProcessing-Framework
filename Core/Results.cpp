#include "Results.h"
using namespace rsdn;


SingleAsyncResult::SingleAsyncResult() : _cancellation(false), _task(nullptr)
{

}

SingleAsyncResult::~SingleAsyncResult()
{
	_cancellation = true;
	if (_task)
	{
		if (!_task->_Is_ready())_task->wait();
	}
}

void SingleAsyncResult::OnCancel()
{
	_cancellation = true;
}

void SingleAsyncResult::OnWait()
{
	if (_task)
	{
		if (!_task->_Is_ready())_task->wait();
	}
}

ResultPtr SingleAsyncResult::OnGet()
{
	if (!_result)
	{
		if (_task)
		{
			_result = _task->get();
		}
	}
	return _result ? _result : std::make_shared<Result>(false, L"no task");
}


