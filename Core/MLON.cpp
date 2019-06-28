#include "MLON.h"
#include "FileStreamReader.h"
#include <boost/tokenizer.hpp>
using namespace rsdn;
using namespace rsdn::data;


namespace rsdn
{
	namespace data
	{
		class MLON_Import_impl
		{
		public:
			std::wstring value;

			MLON_Import_impl()
			{

			}
		};

		class MLON_Node_impl
		{
		public:
			std::wstring identifier;
			std::wstring value;
			std::vector<MLON_NodePtr> nodes;
			std::vector<MLON_ObjectPtr> objects;
			int parentnode;
			int parentobject;

			MLON_Node_impl(): parentnode(-1), parentobject(-1)
			{

			}
		};

		class MLON_Object_impl
		{
		public:
			std::wstring identifier;
			std::wstring value;
			std::vector<MLON_NodePtr> nodes;
			std::vector<MLON_ObjectPtr> objects;
			int parentnode;
			int parentobject;

			MLON_Object_impl() : parentnode(-1), parentobject(-1)
			{

			}
		};

		class MLONParser_impl
		{
			typedef boost::tokenizer<boost::char_separator<wchar_t>, std::wstring::const_iterator, std::wstring> Tok;
			enum class State
			{
				None,
				Object,
				Node,
			};

			enum class Token
			{
				None,
				Keyword,
				ObjectIdentifier,
				NodeEqual,
				String
			};

			enum class Keyword
			{
				None,
				Def,
				Import
			};

		public:
			std::vector<MLON_ImportPtr> imports;

			std::vector<MLON_NodePtr> nodes;
			std::vector<MLON_ObjectPtr> objects;

			std::vector<MLON_NodePtr> nodelist;
			std::vector<MLON_ObjectPtr> objectlist;

			void Clear()
			{
				imports.clear();
				nodes.clear();
				objects.clear();
			}

			__inline void GetObjectParent(int curobj, int& parentobj, int& parentnode)
			{
				parentnode = -1;
				parentobj = -1;
				if (objectlist[curobj]->impl->parentobject != -1)
				{
					parentobj = objectlist[curobj]->impl->parentobject;
				}
				else if(objectlist[curobj]->impl->parentnode != -1)
				{
					parentnode = objectlist[curobj]->impl->parentnode;
				}
			}

			__inline void GetNodeParent(int curnode, int& parentobj, int& parentnode)
			{
				parentnode = -1;
				parentobj = -1;
				if (nodelist[curnode]->impl->parentobject != -1)
				{
					parentobj = nodelist[curnode]->impl->parentobject;
				}
				else if (nodelist[curnode]->impl->parentnode != -1)
				{
					parentnode = nodelist[curnode]->impl->parentnode;
				}
			}

			ResultPtr MLONParser_impl::Load(const std::wstring& path, const std::locale& loc)
			{
				ResultPtr ret = std::make_shared<Result>(false, L"unknown error");

				try
				{
					FileStreamReader reader{ path.c_str(), Encodings::Utf8, loc };

					boost::char_separator<wchar_t> sep{ L" ", L"{}=#" };

					State stat = State::None;
					bool arraymode = false;

					std::wstring lasttokstr;
					Token lasttok = Token::None;
					Keyword lastkeyword = Keyword::None;

					int curlvl = -1;
					int curobj = -1;
					int curnode = -1;

					while (!reader.IsEndOfStream())
					{
						auto buf = reader.ReadLine();
						Tok tok{ buf, sep };
						for (Tok::iterator tokiter = tok.begin(); tokiter != tok.end(); ++tokiter)
						{
							auto str = *tokiter;
							auto pendinglasttok = Token::None;
							auto pendinglastkeyword = Keyword::None;

							/*keyword*/
							if (str.compare(L"import") == 0)
							{
								if (stat == State::None)
								{
									pendinglasttok = Token::Keyword;
									pendinglastkeyword = Keyword::Import;
								}
							}
							else if (str.compare(L"def") == 0)
							{
								if (stat == State::None)
								{
									stat = State::Object;
									pendinglasttok = Token::Keyword;
									pendinglastkeyword = Keyword::Def;
								}
								else if (stat == State::Node && arraymode)
								{
									stat = State::Object;
									pendinglasttok = Token::Keyword;
									pendinglastkeyword = Keyword::Def;
									arraymode = false;
								}
								else
								{
									ret->Error = L"invalid keyword \"def\"";
									throw 1;
								}
							}
							else if (str.compare(L"{") == 0)
							{
								if (lasttok == Token::ObjectIdentifier)
								{
									MLON_ObjectPtr obj = std::make_shared<MLON_Object>();
									obj->impl->identifier = lasttokstr;
									if (curlvl == -1)
									{
										objects.push_back(obj);
									}
									else
									{
										if (curobj != -1)
										{
											objectlist[curobj]->impl->objects.push_back(obj);
											obj->impl->parentobject = curobj;
										}
										else if (curnode != -1)
										{
											nodelist[curnode]->impl->objects.push_back(obj);
											obj->impl->parentnode = curnode;
										}
									}
									objectlist.push_back(obj);
									curobj = objectlist.size() - 1;
									curnode = -1;									
								}
								else
								{
									ret->Error = L"no \"{\" after \"def\"";
									throw 1;									
								}
								curlvl++;
							}
							else if (str.compare(L"}") == 0)
							{
								if (curobj != -1)
								{
									if (curlvl >= 0)
									{
										GetObjectParent(curobj, curobj, curnode);
										curlvl--;
									}
								}
								else if (curnode != -1)
								{
									if (curlvl >= 0)
									{
										GetNodeParent(curnode, curobj, curnode);
										curlvl--;
									}
								}
							}
							else if (str.compare(L"#") == 0)
							{
								/*skip comment until current line*/
								break;
							}
							else if (str.compare(L"(") == 0)
							{
								if (lasttok != Token::NodeEqual)
								{
									ret->Error = L"\"(\" should be after notation equal";
									throw 1;
								}
								else
								{
									arraymode = true;
								}
							}
							else if (str.compare(L")") == 0)
							{
								if (curobj != -1)
								{
									if (curlvl >= 0)
									{
										GetObjectParent(curobj, curobj, curnode);
										curlvl--;
									}
								}
								else if (curnode != -1)
								{
									if (curlvl >= 0)
									{
										GetNodeParent(curnode, curobj, curnode);
										curlvl--;
									}
								}
							}
							else if (str.compare(L"=") == 0)
							{
								if (lasttok == Token::None && !lasttokstr.empty())
								{
									pendinglasttok = Token::NodeEqual;
									pendinglastkeyword = Keyword::None;

									MLON_NodePtr node = std::make_shared<MLON_Node>();
									node->impl->identifier = lasttokstr;
									if (curlvl == -1)
									{
										nodes.push_back(node);
									}
									else
									{
										if (curobj != -1)
										{
											objectlist[curobj]->impl->nodes.push_back(node);
											node->impl->parentobject = curobj;
										}
										else if (curnode != -1)
										{
											nodelist[curnode]->impl->nodes.push_back(node);
											node->impl->parentnode = curnode;
										}
									}
									nodelist.push_back(node);
									curnode = nodelist.size() - 1;
									curobj = -1;
									curlvl++;
									stat = State::Node;
								}
							}
							else if (stat == State::Object)
							{
								if (lasttok == Token::Keyword)
								{
									if (lastkeyword == Keyword::Def)
									{
										/*should be object identifier*/
										pendinglasttok = Token::ObjectIdentifier;
										pendinglastkeyword = Keyword::None;
										stat = State::None;
									}
								}
							}
							else if (lasttok == Token::Keyword && lastkeyword== Keyword::Import)
							{
								MLON_ImportPtr import = std::make_shared<MLON_Import>();
								import->impl->value = str;
								imports.push_back(import);
							}

							lasttokstr = str;
							lasttok = pendinglasttok;
							lastkeyword = pendinglastkeyword;
						}

						if (stat == State::Node && !arraymode)
						{
							if (!lasttokstr.empty() && curnode!=-1 )
							{
								nodelist[curnode]->impl->value = lasttokstr;
								
								if (curobj != -1)
								{
									if (curlvl >= 0)
									{
										GetObjectParent(curobj, curobj, curnode);
										curlvl--;
									}
								}
								else if (curnode != -1)
								{
									if (curlvl >= 0)
									{
										GetNodeParent(curnode, curobj, curnode);
										curlvl--;
									}
								}
							}
							stat = State::None;
						}
						else if (stat == State::Node && arraymode && lasttokstr.compare(L"(")!=0)
						{
							nodelist[curnode]->impl->value = lasttokstr;

							if (curobj != -1)
							{
								if (curlvl >= 0)
								{
									GetObjectParent(curobj, curobj, curnode);
									curlvl--;
								}
							}
							else if (curnode != -1)
							{
								if (curlvl >= 0)
								{
									GetNodeParent(curnode, curobj, curnode);
									curlvl--;
								}
							}

							arraymode = false;
						}
					}

					ret->State = true;
					ret->Message = L"";
				}
				catch (...)
				{

				}
				return ret;
			}
		};
	}
}

#pragma region MLON_Import

MLON_Import::MLON_Import()
{
	impl = new MLON_Import_impl();
}

MLON_Import::~MLON_Import()
{
	SafeDelete(impl);
}

const std::wstring& MLON_Import::QueryValue() const
{
	return impl->value;
}
#pragma endregion


#pragma region MLON_Node

MLON_Node::MLON_Node()
{
	impl = new MLON_Node_impl();
}

MLON_Node::~MLON_Node()
{
	SafeDelete(impl);
}

const std::wstring& MLON_Node::QueryIdentifier() const
{
	return impl->identifier;
}

MLON_ObjectPtr MLON_Node::QueryObject(const std::wstring& identifier) const
{
	for (auto obj : impl->objects)
	{
		if (obj->impl->identifier.compare(identifier) == 0) return obj;
	}
	return nullptr;
}

MLON_NodePtr MLON_Node::QueryNode(const std::wstring& identifier) const
{
	for (auto node : impl->nodes)
	{
		if (node->impl->identifier.compare(identifier) == 0) return node;
	}
	return nullptr;
}

MLON_ObjectPtr MLON_Node::QueryObject(sizetype i) const
{
	if (i < 0 || i > impl->objects.size()) return nullptr;
	return impl->objects[i];
}

sizetype MLON_Node::QueryObjectCount() const
{
	return impl->objects.size();
}

MLON_NodePtr MLON_Node::QueryNode(sizetype i) const
{
	if (i < 0 || i > impl->nodes.size()) return nullptr;
	return impl->nodes[i];
}

sizetype MLON_Node::QueryNodeCount() const
{
	return impl->nodes.size();
}

const std::wstring& MLON_Node::QueryValue() const
{
	return impl->value;
}
#pragma endregion

#pragma region MLON_Object
MLON_Object::MLON_Object()
{
	impl = new MLON_Object_impl();
}

MLON_Object::~MLON_Object()
{
	SafeDelete(impl);
}

const std::wstring& MLON_Object::QueryIdentifier() const
{
	return impl->identifier;
}

MLON_ObjectPtr MLON_Object::QueryObject(const std::wstring& identifier) const
{
	for (auto obj : impl->objects)
	{
		if (obj->impl->identifier.compare(identifier) == 0) return obj;
	}
	return nullptr;
}

MLON_NodePtr MLON_Object::QueryNode(const std::wstring& identifier) const
{
	for (auto node : impl->nodes)
	{
		if (node->impl->identifier.compare(identifier) == 0) return node;
	}
	return nullptr;
}

MLON_ObjectPtr MLON_Object::QueryObject(sizetype i) const
{
	if (i < 0 || i > impl->objects.size()) return nullptr;
	return impl->objects[i];
}

sizetype MLON_Object::QueryObjectCount() const
{
	return impl->objects.size();
}

MLON_NodePtr MLON_Object::QueryNode(sizetype i) const
{
	if (i < 0 || i > impl->nodes.size()) return nullptr;
	return impl->nodes[i];
}

sizetype MLON_Object::QueryNodeCount() const
{
	return impl->nodes.size();
}
#pragma endregion

MLONParser::MLONParser()
{
	impl = new MLONParser_impl();
}

MLONParser::~MLONParser()
{
	SafeDelete(impl);
}

ResultPtr MLONParser::Load(const std::wstring& path, const std::locale& loc)
{
	impl->Clear();
	return impl->Load(path, loc);
}

MLON_ImportPtr MLONParser::QueryImport(sizetype i)  const
{
	if (i < 0 || i > impl->imports.size()) return nullptr;
	return impl->imports[i];
}

sizetype MLONParser::QueryImportCount() const
{
	return impl->imports.size();
}

MLON_ObjectPtr MLONParser::QueryObject(const std::wstring& identifier) const
{
	for (auto obj : impl->objects)
	{
		if (obj->impl->identifier.compare(identifier) == 0) return obj;
	}
	return nullptr;
}

MLON_NodePtr MLONParser::QueryNode(const std::wstring& identifier) const
{
	for (auto node : impl->nodes)
	{
		if (node->impl->identifier.compare(identifier) == 0) return node;
	}
	return nullptr;
}

MLON_ObjectPtr MLONParser::QueryObject(sizetype i) const
{
	if (i < 0 || i > impl->objects.size()) return nullptr;
	return impl->objects[i];
}

sizetype MLONParser::QueryObjectCount() const
{
	return impl->objects.size();
}

MLON_NodePtr MLONParser::QueryNode(sizetype i) const
{
	if (i < 0 || i > impl->nodes.size()) return nullptr;
	return impl->nodes[i];
}

sizetype MLONParser::QueryNodeCount() const
{
	return impl->nodes.size();
}