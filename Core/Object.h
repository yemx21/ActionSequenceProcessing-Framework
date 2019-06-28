#pragma once
#include "Type.h"
#include <memory>

namespace rsdn
{
	typedef void(*CreateMetaFunc)(Type&);
	class CORE_API Object
	{
	protected:
		constexpr Object() {}

		static _type Meta;

	public:

		virtual ~Object();

		virtual _type GetType() const;

		bool Is(_type type) const;

		virtual bool Equals(const Object* ref) const;

		virtual unsigned long long GetHashCode() const;
	};

	template<class T>
	inline T* SafeCast(Object* base)
	{
		if (base && base->Is(TypeHelper<T>::GetType()))
		{
			return static_cast<T*>(base);
		}
		else
		{
			return NULL;
		}
	}

	template<class T>
	inline const T* SafeCast(const Object* base)
	{
		if (base && base->Is(TypeHelper<T>::GetType()))
		{
			return static_cast<const T*>(base);
		}
		else
		{
			return NULL;
		}
	}

	typedef std::shared_ptr<Object> ObjectPtr;

}

