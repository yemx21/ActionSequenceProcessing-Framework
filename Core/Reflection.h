#pragma once
#include "Core_Config.h"
#include <vector>
#include "ObjectInfo.h"
#include "TypeList.h"
#include "TypeCaches.h"

#ifdef REFLECTION_SUPPORT
#define REFLECTION_API  __declspec(dllexport)
#else
#define REFLECTION_API  __declspec(dllimport)
#endif

#define i_CONCAT(a, b) (a ## b)
#define CONCAT(a, b) i_CONCAT(a, b)

#define i_CONCATNAME(a, b) a ## b
#define CONCATNAME(a, b) i_CONCATNAME(a, b)

#define i__ref_CLASS(T, num) \
private:\
static void DefineMeta(rsdn::ObjectInfo&);\
template<class T>\
friend void rsdn::ObjectInfo::Create(rsdn::Type const*& pointer, const wchar_t* name, unsigned int size);\
public:\
_type GetType() const override;

#define REFLECT_CLASS(T)\
	public:\
	i__ref_CLASS(T, __COUNTER__)

#define i2__ref_register(REG,...)\
static struct REG{\
	REG() { __VA_ARGS__ } \
}CONCATNAME(U_PREMAIN_ENTRY, __COUNTER__);

#define i__ref_register_class(T, num)\
	static _type CONCATNAME(GLOBAL_META_, num);\
	_type T::GetType() const { return CONCATNAME(GLOBAL_META_, num);}\
	template<> \
	REFLECTION_API _type rsdn::TypeHelper<T>::GetType() { rsdn::ObjectInfo::Create<T>(CONCATNAME(GLOBAL_META_, num), L## #T, sizeof(T)); return CONCATNAME(GLOBAL_META_, num);  }\
	i2__ref_register(CONCATNAME(CONCATNAME(REFLECT_PREMAIN_CLASS, T), __COUNTER__), rsdn::ObjectInfo::Create<T>(CONCATNAME(GLOBAL_META_, num), L## #T, sizeof(T));)\
	void T::DefineMeta(rsdn::ObjectInfo& type)

#define i__ref_register_class1(T, T1, num)\
	static _type CONCATNAME(GLOBAL_META_, num);\
	_type T::GetType() const { return CONCATNAME(GLOBAL_META_, num);}\
	template<> \
	REFLECTION_API _type rsdn::TypeHelper<T>::GetType() { rsdn::ObjectInfo::Create<T>(CONCATNAME(GLOBAL_META_, num), L## #T, sizeof(T)); return CONCATNAME(GLOBAL_META_, num);  }\
	i2__ref_register(CONCATNAME(CONCATNAME(REFLECT_PREMAIN_CLASS, T1), __COUNTER__), rsdn::ObjectInfo::Create<T>(CONCATNAME(GLOBAL_META_, num), L## #T, sizeof(T));)\
	void T::DefineMeta(rsdn::ObjectInfo& type)

#define __uregister_class(T)   i__ref_register_class(T, __COUNTER__)
#define __uregister_class1(T, T1)   i__ref_register_class1(T, T1, __COUNTER__)


