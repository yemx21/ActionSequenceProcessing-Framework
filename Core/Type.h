#pragma once
#include "Core_Config.h"
#include "Optional.h"
namespace rsdn
{
	enum class TypeTag
	{
		Unknown,
		Object, 
	};


	class CORE_API Type
	{
	private:
		TypeTag _tag;
	protected:
		friend class Type_impl;
		friend class ObjectInfo;

		void destroy();

		/// <summary>
		/// ¹þÏ£Öµ
		/// </summary>
		Optional<size_t> _hashCode;

		Type(TypeTag tag);
	public:
		virtual ~Type();

		virtual const wchar_t* GetName() const;

		TypeTag GetTag() const;

		bool IsType(_type type) const;

		bool IsBaseOf(_type info)const;

		bool IsBaseOfOrSameAs(_type info)const;

		bool IsConvertible(_type type)const;

		bool GetIsObject()const;

		size_t GetHashCode() const;

		static unsigned __int64 GetCount();

		_declspec(property(get = GetName)) const wchar_t* Name;

		_declspec(property(get = GetHashCode)) size_t HashCode;

		_declspec(property(get = GetIsObject)) bool IsObject;

		_declspec(property(get = GetTag)) TypeTag Tag;

		static Type* const unknownType;
		static Type* const objectType;
	};
}