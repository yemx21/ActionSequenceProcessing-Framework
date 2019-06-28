#pragma once
#include "Core_Config.h"
#include <guiddef.h>
#include <cguid.h>
#include <future>
#include <atomic>
#include <string>
#include <map>
#include <boost\any.hpp>
#include <boost\multiprecision\cpp_int.hpp>
#include "ObjectDef.h"
#include "Buffer.h"
namespace rsdn
{
	class Graph_impl;
	class Graph;
	namespace layer
	{
		class Layer;
	}

	class CORE_API Result
	{
	private:
		bool _state;
		std::wstring _msg;
		std::wstring _err;
	public:
		Result();
		Result(bool state);
		Result(bool state, const std::wstring& msg);
		Result(bool state, const std::wstring& msg, const std::wstring& err);

		bool GetState() const;
		void SetState(bool);

		const std::wstring& GetMsg() const;
		void SetMsg(const std::wstring&);

		const std::wstring& GetError() const;
		void SetError(const std::wstring&);

		operator bool() const;

		_declspec(property(get = GetState, put = SetState)) bool State;
		_declspec(property(get = GetMsg, put = SetMsg)) const std::wstring& Message;
		_declspec(property(get = GetError, put = SetError)) const std::wstring& Error;
	};
	typedef std::shared_ptr<Result> ResultPtr;

	class CORE_API AsyncResult
	{
	protected:
		virtual void OnCancel();
		virtual void OnWait();
		virtual ResultPtr OnGet();
	public:
		AsyncResult();
		~AsyncResult();
		void Cancel();
		void Wait();

		ResultPtr Get();
	};
	typedef std::shared_ptr<AsyncResult> AsyncResultPtr;

	namespace data
	{
		class IDataSection;
		typedef std::shared_ptr<IDataSection> IDataSectionPtr;

		class CORE_API IDataSection
		{
		protected:
			virtual boost::any GetDataCore(const std::wstring& key);
			virtual void SetDataCore(const std::wstring& key, const boost::any& data);

			virtual void* GetDataCore1(const std::wstring& key);
		public:
			virtual bool HasSection(const std::wstring& key);
			virtual IDataSectionPtr GetSection(const std::wstring& key);
			virtual void SetSection(const std::wstring& key, IDataSectionPtr sec);

			virtual __int64 GetChildCount();
			virtual IDataSectionPtr GetChild(const __int64 index);
			virtual void SetChild(const __int64 index, IDataSectionPtr sec);

			template<typename T>
			std::shared_ptr<T> GetData(const std::wstring& key)
			{
				return boost::any_cast<std::shared_ptr<T>>(GetDataCore(key));
			}

			template<typename T>
			void SetData(const std::wstring& key, std::shared_ptr<T> data)
			{
				SetDataCore(key, data);
			}

			template<typename T>
			T* GetData1(const std::wstring& key)
			{
				return (T*)GetDataCore1(key);
			}
		};

		class CORE_API DataPacket: __object
		{
		private:
			std::map<std::wstring, IDataSectionPtr> sections;
			std::map<std::wstring, std::shared_ptr<void>> temporary;
			void SetTemporaryCore(const std::wstring& key, std::shared_ptr<void> val);
			std::shared_ptr<void> GetTemporaryCore(const std::wstring& key);
		public:
			bool HasSection(const std::wstring& key);
			IDataSectionPtr GetSection(const std::wstring& key);
			void SetSection(const std::wstring& key, IDataSectionPtr sec);

			template<typename T>
			void SetTemporary(const std::wstring& key, std::shared_ptr<T> val)
			{
				SetTemporaryCore(key, val);
			}

			template<typename T>
			std::shared_ptr<T> GetTemporary(const std::wstring& key)
			{
				return std::static_pointer_cast<T>(GetTemporaryCore(key));
			}

			void ClearTemporary(const std::wstring& key);
			
			REFLECT_CLASS(DataPacket)
		};
		typedef std::shared_ptr<DataPacket> DataPacketPtr;

		enum class ParameterItemType
		{
			None,
			Value,
			Buffer,
		};

		class CORE_API ParameterPacket: __object
		{
		protected:
			virtual int GetCountCore() const;
			virtual ParameterItemType GetTypeCore(const std::wstring& key) const;
			virtual ParameterItemType GetTypeCore(int index) const;

			virtual const boost::any& GetCore(const std::wstring& key) const;
			virtual const boost::any& GetAtIndexCore(int index) const;

			virtual IBufferPtr GetCoreEx(const std::wstring& key) const;
			virtual IBufferPtr GetAtIndexCoreEx(int index) const;
		public:
			int GetCount() const;

			ParameterItemType GetItemType(const std::wstring& key) const;
			ParameterItemType GetItemType(int index) const;

			const boost::any& GetItem(const std::wstring& key) const;
			const boost::any& GetItemAtIndex(int index) const;

			template<typename T>
			T GetItem(const std::wstring& key) const
			{
				const boost::any& ret = GetItem(key);
				if (ret.empty())
				{
					throw std::exception{};
				}
				return boost::any_cast<T>(ret);
			}

			template<typename T>
			T GetItemAtIndex(int index) const
			{
				const boost::any& ret = GetItemAtIndex(index);
				if (ret.empty())
				{
					throw std::exception{};
				}
				return boost::any_cast<T>(ret);
			}

			IBufferPtr GetItemEx(const std::wstring& key) const;
			IBufferPtr GetItemAtIndexEx(int index) const;

			REFLECT_CLASS(ParameterPacket)
		};
		typedef std::shared_ptr<ParameterPacket> ParameterPacketPtr;
	}

	namespace layer
	{
		class CORE_API Operator :__object
		{
		protected:
			virtual ResultPtr ConfigCore(data::ParameterPacketPtr param);
		public:
			ResultPtr Config(data::ParameterPacketPtr param);

			virtual bool IsExclusive() const;

			REFLECT_CLASS(Operator)
		};
		typedef std::shared_ptr<Operator> OperatorPtr;

		class CORE_API Layer:__object
		{
		protected:
			friend Graph_impl;
			friend Graph;
			int64_t _id;
			virtual ResultPtr ConfigCore(data::ParameterPacketPtr param);
			virtual ResultPtr ReadyCore();
			virtual ResultPtr InCore(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev);
			virtual ResultPtr OutCore(data::DataPacketPtr& data, data::ParameterPacketPtr& outparameter, _type next);
			virtual std::unique_ptr<std::future<ResultPtr>> RunAsync(std::atomic<bool>& cancel);

			virtual ResultPtr AddOperator(OperatorPtr op);

			virtual unsigned int GetBatchCount();
			virtual ResultPtr ConfigBatch(unsigned int batch);
			virtual std::unique_ptr<std::future<ResultPtr>> PrepareBatchAsync(std::atomic<bool>& cancel);
			virtual std::unique_ptr<std::future<ResultPtr>> RunBatchAsync(std::atomic<bool>& cancel, unsigned int batch);
		public:
			Layer();
			int64_t GetId() const;		
			_declspec(property(get = GetId)) int64_t Id;

		public:
			virtual ResultPtr IsSupportConnectFrom(_type prev);
			virtual ResultPtr IsSupportConnectTo(_type next);
			ResultPtr Config(data::ParameterPacketPtr param);
			ResultPtr Ready();
			ResultPtr In(data::DataPacketPtr data, data::ParameterPacketPtr prevparameter, _type prev);
			ResultPtr Out(data::DataPacketPtr& data, data::ParameterPacketPtr& outparameter, _type next);

			REFLECT_CLASS(Layer)
		};
		typedef std::shared_ptr<Layer> LayerPtr;
	}
}