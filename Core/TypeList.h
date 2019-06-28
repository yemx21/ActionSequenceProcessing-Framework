#pragma once
#include <Windows.h>
#include "Type.h"
#include "OSHelper.h"
#include "InertiaString.h"
#include <type_traits>
#include <array>
#include <valarray>
using namespace std;
namespace rsdn
{
	template <typename> struct is_template : std::false_type {};

	template <template <typename...> class Tmpl, typename ...Args>
	struct is_template<Tmpl<Args...>> : std::true_type {};

	template<typename _Base, typename _Der>
	struct is_same_or_base_of
	{
		static const bool value = std::is_same<_Base, _Der>::value || std::is_base_of < _Base, _Der >::value;
	};

	template<typename T>
	struct has_const_iterator
	{
	private:
		typedef char                      yes;
		typedef struct { char array[2]; } no;

		template<typename C> static yes test(typename C::const_iterator*);
		template<typename C> static no  test(...);
	public:
		static const bool value = sizeof(test<T>(0)) == sizeof(yes);
		typedef T type;
	};

	template<class Container>
	auto begin(Container &&c) -> decltype(c.begin()) { return c.begin(); }

	template<class Container>
	auto end(Container &&c) -> decltype(c.end()) { return c.end(); }

	template<class T, size_t size>
	T *begin(T(&array)[size]) { return (&array[0]); }

	template<class T, size_t size>
	T *end(T(&array)[size]) { return (&array[0] + size); }

	template <typename T>
	struct has_begin_end
	{
		typedef char true_type;
		typedef char false_type[2];

		template <typename U> static true_type& test(decltype(begin(*((U*)0))) *b = 0,
			decltype(end(*((U*)0))) *e = 0);

		template <typename U> static false_type& test(...);

		enum { value = (sizeof(true_type) == sizeof test<T>(0)) };
	};

	template<typename T>
	struct is_container : std::integral_constant<bool, has_const_iterator<T>::value && has_begin_end<T>::value>
	{
	};

	template<typename T, std::size_t N> struct is_container<T[N]> : public ::std::true_type {};

	template<typename T, std::size_t N1, std::size_t N2> struct is_container<T[N1][N2]> : public ::std::true_type {};

	template<typename T, std::size_t N1, std::size_t N2, std::size_t N3> struct is_container<T[N1][N2][N3]> : public ::std::true_type {};

	template<typename T, std::size_t N1, std::size_t N2, std::size_t N3, std::size_t N4> struct is_container<T[N1][N2][N3][N4]> : public ::std::true_type {};

	template<typename T, std::size_t N1, std::size_t N2, std::size_t N3, std::size_t N4, std::size_t N5> struct is_container<T[N1][N2][N3][N4][N5]> : public ::std::true_type {};

	enum class MemoryPtrTypes
	{
		Unknown,
		SharedPtr,
		WeakPtr,
		UniquePtr,
		AutoPtr,
	};

	template<typename T>
	struct is_shared_ptr
	{
		typedef char(&yes)[2];
		template<typename U>
		static yes check(std::shared_ptr<U>*);
		static char check(...);

		static const bool value = (sizeof(check((T*)0)) == sizeof(yes));
	};

	template<typename T>
	struct is_weak_ptr
	{
		typedef char(&yes)[2];
		template<typename U>
		static yes check(std::weak_ptr<U>*);
		static char check(...);

		static const bool value = (sizeof(check((T*)0)) == sizeof(yes));
	};

	template<typename T>
	struct is_unique_ptr
	{
		typedef char(&yes)[2];
		template<typename U>
		static yes check(std::unique_ptr<U>*);
		static char check(...);

		static const bool value = (sizeof(check((T*)0)) == sizeof(yes));
	};
	template<typename T>
	struct is_auto_ptr
	{
		typedef char(&yes)[2];
		template<typename U>
		static yes check(std::auto_ptr<U>*);
		static char check(...);

		static const bool value = (sizeof(check((T*)0)) == sizeof(yes));
	};

	template<typename T>
	struct is_memory_ptr
	{
		typedef typename std::remove_reference<typename std::remove_pointer<typename std::remove_pointer<T>::type>::type>::type base;
		static const bool isShared = is_shared_ptr<base>::value;
		static const bool isWeak = is_weak_ptr<base>::value;
		static const bool isUnique = is_unique_ptr<base>::value;
		static const bool isAuto = is_auto_ptr<base>::value;
	public:
		static const bool value = isShared || isWeak || isUnique || isAuto;
	};

	template<class T>
	struct deduce
	{
		typedef typename std::remove_cv<typename std::remove_pointer<typename std::remove_pointer<typename std::decay<T>::type>::type>::type>::type type;
	};

	template<std::size_t N> struct is_container<char[N]> : public ::std::false_type {};

	template <typename T> struct is_container<std::valarray<T>> : public ::std::true_type {};

	template<typename __T>
	class TypeHelper
	{
	public:
		static _type GetType();
	};

	template<>
	_inline _type TypeHelper<rsdn::Object>::GetType() { return Type::objectType; }


	template<typename T>
	struct is_TypeHelper_supported
	{
		static const bool value = is_same_or_base_of<Object, T>::value;
	};

}

