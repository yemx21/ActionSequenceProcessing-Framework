#pragma once
#include "ObjectInfo.h"
#include "InertiaString.h"
#include <set>
namespace rsdn
{
	struct ObjectInfo_impl
	{
		InertiaString oname;
		unsigned int size;
	};

	struct ObjectInfo_internal : ZeroStruct<ObjectInfo_internal>
	{
		std::set<const ObjectInfo*>* bcls;
		Objectctorfunc ctor;
		static void LoopCountBaseObject(std::set<const ObjectInfo*>* all, const ObjectInfo* cls)
		{
			if (!cls || !all) return;
			for (auto& b : *cls->internal->bcls)
			{
				if (all->find(b) != all->end())
				{
					all->insert(b);
					LoopCountBaseObject(all, b);
				}
			}
		}

		~ObjectInfo_internal()
		{
			if (bcls)
			{
				bcls->clear();
				delete bcls;
				bcls = nullptr;
			}
		}
	};
}
