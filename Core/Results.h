#pragma once
#include "Graph.h"
#include <vector>
namespace rsdn
{
	class CORE_API SingleAsyncResult: public AsyncResult
	{
	private:
		friend Graph;
		ResultPtr _result;
		std::unique_ptr<std::future<ResultPtr>> _task;
		std::atomic<bool> _cancellation;

	public:
		SingleAsyncResult();
		~SingleAsyncResult();

	protected:
		void OnCancel() override;
		void OnWait() override;
		ResultPtr OnGet() override;
	};
	typedef std::shared_ptr<SingleAsyncResult> SingleAsyncResultPtr;

	class CORE_API MultiAsyncResult : public AsyncResult
	{
	private:
		friend Graph;
		ResultPtr _result;
		int _level;
		bool _standalonesfirst;
		std::vector<std::vector<NodePtr>> _nodes;
		std::vector<std::unique_ptr<std::future<ResultPtr>>> _standalones;
		std::unique_ptr<std::future<ResultPtr>> _all;
		Graph* _owner;
		std::atomic<bool> _cancellation;
		void Build();

		static ResultPtr Run(MultiAsyncResult* me);
	public:
		MultiAsyncResult();
		~MultiAsyncResult();

	protected:
		void OnCancel() override;
		void OnWait() override;
		ResultPtr OnGet() override;
	};
	typedef std::shared_ptr<MultiAsyncResult> MultiAsyncResultPtr;
}