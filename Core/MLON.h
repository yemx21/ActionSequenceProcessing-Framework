#pragma once
#include "Layer.h"

namespace rsdn
{
	namespace data
	{
		class MLONParser_impl;
		class MLON_Import_impl;
		class MLON_Node_impl;
		class MLON_Object_impl;

		class MLON_Import;
		typedef std::shared_ptr<MLON_Import> MLON_ImportPtr;
		class MLON_Node;
		typedef std::shared_ptr<MLON_Node> MLON_NodePtr;
		class MLON_Object;
		typedef std::shared_ptr<MLON_Object> MLON_ObjectPtr;
		class MLONParser;
		typedef std::shared_ptr<MLONParser> MLONParserPtr;

		class CORE_API MLON_Import
		{
		private:
			friend MLONParser;
			friend MLONParser_impl;
			friend MLON_Import_impl;
			MLON_Import_impl* impl;
		public:
			MLON_Import();
			~MLON_Import();

			const std::wstring& QueryValue() const;
			_declspec(property(get = QueryValue)) const std::wstring& Value;

		};

		class CORE_API MLON_Node
		{
		private:
			friend MLONParser;
			friend MLONParser_impl;
			friend MLON_Object;
			friend MLON_Node_impl;
			MLON_Node_impl* impl;
		public:
			MLON_Node();
			~MLON_Node();

			MLON_ObjectPtr QueryObject(const std::wstring& identifier) const;
			MLON_NodePtr QueryNode(const std::wstring& identifier) const;

			const std::wstring& QueryIdentifier() const;
			MLON_ObjectPtr QueryObject(sizetype i) const;
			sizetype QueryObjectCount() const;
			MLON_NodePtr QueryNode(sizetype i) const;
			sizetype QueryNodeCount() const;
			const std::wstring& QueryValue() const;

			_declspec(property(get = QueryIdentifier)) const std::wstring& Identifier;
			_declspec(property(get = QueryObjectCount)) sizetype ObjectCount;
			_declspec(property(get = QueryObject)) MLON_ObjectPtr Object[];
			_declspec(property(get = QueryNodeCount)) sizetype NodeCount;
			_declspec(property(get = QueryNode)) MLON_NodePtr Node[];
			_declspec(property(get = QueryValue)) const std::wstring& Value;
		};

		class CORE_API MLON_Object
		{
		private:
			friend MLONParser;
			friend MLONParser_impl;
			friend MLON_Node;
			friend MLON_Object_impl;
			MLON_Object_impl* impl;
		public:
			MLON_Object();
			~MLON_Object();

			MLON_ObjectPtr QueryObject(const std::wstring& identifier) const;
			MLON_NodePtr QueryNode(const std::wstring& identifier) const;

			const std::wstring& QueryIdentifier() const;
			MLON_ObjectPtr QueryObject(sizetype i) const;
			sizetype QueryObjectCount() const;
			MLON_NodePtr QueryNode(sizetype i) const;
			sizetype QueryNodeCount() const;

			_declspec(property(get = QueryIdentifier)) const std::wstring& Identifier;
			_declspec(property(get = QueryObjectCount)) sizetype ObjectCount;
			_declspec(property(get = QueryObject)) MLON_ObjectPtr Object[];
			_declspec(property(get = QueryNodeCount)) sizetype NodeCount;
			_declspec(property(get = QueryNode)) MLON_NodePtr Node[];
		};

		class CORE_API MLONParser
		{
		private:
			friend MLONParser_impl;
			MLONParser_impl* impl;
		public:
			MLONParser();
			~MLONParser();
			ResultPtr Load(const std::wstring& path, const std::locale& loc);

			MLON_ObjectPtr QueryObject(const std::wstring& identifier) const;
			MLON_NodePtr QueryNode(const std::wstring& identifier) const;

			MLON_ImportPtr QueryImport(sizetype i) const;
			sizetype QueryImportCount() const;
			MLON_ObjectPtr QueryObject(sizetype i) const;
			sizetype QueryObjectCount() const;
			MLON_NodePtr QueryNode(sizetype i) const;
			sizetype QueryNodeCount() const;

			_declspec(property(get = QueryImportCount)) sizetype ImportCount;
			_declspec(property(get = QueryImport)) MLON_ImportPtr Import[];
			_declspec(property(get = QueryObjectCount)) sizetype ObjectCount;
			_declspec(property(get = QueryObject)) MLON_ObjectPtr Object[];
			_declspec(property(get = QueryNodeCount)) sizetype NodeCount;
			_declspec(property(get = QueryNode)) MLON_NodePtr Node[];

		};
	}
}