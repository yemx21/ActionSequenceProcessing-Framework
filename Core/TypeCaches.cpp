#include "TypeCaches.h"
#include "TypeCaches_impl.h"

using namespace rsdn;

TypeCaches::TypeCaches()
{
	impl = new TypeCaches_impl();
}

TypeCaches::~TypeCaches()
{
	if (impl) { delete impl; impl = nullptr; }
}

bool TypeCaches::Insert(_type type)
{
	if (impl) return impl->Insert(type); else return false;
}

void TypeCaches::Remove(_type type)
{
	if (impl) impl->Remove(type);
}

_type TypeCaches::GetItem(size_t index) const
{
	if (impl) return impl->GetItem(index); else return nullptr;
}

size_t TypeCaches::GetItemSize() const
{
	if (impl) return impl->GetItemSize(); else return 0;
}