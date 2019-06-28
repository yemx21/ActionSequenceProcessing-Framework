#include "Object.h"
#include "ObjectInfo.h"
#include "TypeList.h"
#include "Reflection.h"
using namespace rsdn;

_type Object::Meta = nullptr;

Object::~Object()
{

}

_type Object::GetType() const
{
	return Meta;
}


bool Object::Is(_type type) const
{
	_type thisType = GetType();
	return thisType->IsType(type);
}



bool Object::Equals(const Object* ref) const
{
	return ref == this;
}

unsigned long long Object::GetHashCode() const
{
	return static_cast<unsigned long long>((intptr_t)this);
}