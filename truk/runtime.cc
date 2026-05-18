#include "runtime.h"

using namespace truk;

TRUK_API CountablePoolAlloc::CountablePoolAlloc(Runtime *runtime, peff::Alloc *upstream) : runtime(runtime), upstream(upstream) {}

TRUK_API peff::UUID CountablePoolAlloc::type_identity() const noexcept {
	return PEFF_UUID(1a4b6c8d, 0e2f, 4a6b, 8c1d, 2e4f6a8b0c2e);
}

TRUK_API size_t CountablePoolAlloc::inc_ref(size_t global_rc) noexcept {
	return ++ref_count;
}

TRUK_API size_t CountablePoolAlloc::dec_ref(size_t global_rc) noexcept {
	if (!--ref_count) {
		on_ref_zero();
		return 0;
	}

	return ref_count;
}

TRUK_API void CountablePoolAlloc::on_ref_zero() noexcept {
}

TRUK_API void *CountablePoolAlloc::alloc(size_t size, size_t alignment) noexcept {
	assert(size);
	void *p = upstream->alloc(size, alignment);
	if (!p)
		return nullptr;

	sz_allocated += size;

	return p;
}

TRUK_API void *CountablePoolAlloc::realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	assert(new_size);
	void *p = upstream->realloc(ptr, size, alignment, new_size, new_alignment);
	if (!p)
		return nullptr;

	sz_allocated -= size;
	sz_allocated += new_size;

	return p;
}

TRUK_API void *CountablePoolAlloc::realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept {
	assert(new_size);
	void *p = upstream->realloc_in_place(ptr, size, alignment, new_size, new_alignment);
	if (!p)
		return nullptr;

	sz_allocated -= size;
	sz_allocated += new_size;

	return p;
}

TRUK_API void CountablePoolAlloc::release(void *p, size_t size, size_t alignment) noexcept {
	assert(size <= sz_allocated);

	upstream->release(p, size, alignment);

	sz_allocated -= size;
}

TRUK_API bool CountablePoolAlloc::is_replaceable(const peff::Alloc *rhs) const noexcept {
	if (type_identity() != rhs->type_identity())
		return false;

	CountablePoolAlloc *r = (CountablePoolAlloc *)rhs;

	if (runtime != r->runtime)
		return false;

	if (upstream != r->upstream)
		return false;

	return true;
}

TRUK_API Object::Object(ObjectType object_type) {
}

TRUK_API Object::~Object() {
}

TRUK_API StringObject::StringObject(peff::Alloc *allocator) : Object(ObjectType::String), data(allocator) {
}

TRUK_API StringObject::~StringObject() {
}

TRUK_API void StringObject::dealloc() noexcept {
	peff::destroy_and_release<StringObject>(this->data.allocator(), this, alignof(StringObject));
}

TRUK_API SymbolObject::SymbolObject(peff::Alloc *allocator) : Object(ObjectType::Symbol), name(allocator) {
}

TRUK_API SymbolObject::~SymbolObject() {
}

TRUK_API void SymbolObject::dealloc() noexcept {
	peff::destroy_and_release<SymbolObject>(name.allocator(), this, alignof(StringObject));
}

TRUK_API ListObject::ListObject(peff::Alloc *allocator) : Object(ObjectType::List), elements(allocator) {
}

TRUK_API ListObject::~ListObject() {
}

TRUK_API void ListObject::dealloc() noexcept {
	peff::destroy_and_release<ListObject>(this->elements.allocator(), this, alignof(StringObject));
}

TRUK_API ArrayObject::ArrayObject(peff::Alloc *allocator) : Object(ObjectType::Array), elements(allocator) {
}

TRUK_API ArrayObject::~ArrayObject() {
}

TRUK_API void ArrayObject::dealloc() noexcept {
	peff::destroy_and_release<ArrayObject>(this->elements.allocator(), this, alignof(StringObject));
}

TRUK_API MemberObject::MemberObject(peff::Alloc *allocator, ObjectType object_type) : Object(object_type), name(allocator) {
}

TRUK_API MemberObject::~MemberObject() {
}

TRUK_API ModuleObject::ModuleObject(peff::Alloc *allocator) : MemberObject(allocator, ObjectType::Module), token_list(allocator), members(allocator) {
}

TRUK_API ModuleObject::~ModuleObject() {
}

TRUK_API HostRefHolder::HostRefHolder(peff::Alloc *self_allocator)
	: holded_objects(self_allocator) {
}

TRUK_API HostRefHolder::~HostRefHolder() {
	for (auto i : holded_objects)
		--i->host_ref_count;
}

TRUK_API bool HostRefHolder::add_object(Object *object) {
	if (!holded_objects.contains(object)) {
		if (!holded_objects.insert(+object))
			return false;
		++object->host_ref_count;
	}
	return true;
}

TRUK_API void HostRefHolder::remove_object(Object *object) noexcept {
	assert(holded_objects.contains(object));
	holded_objects.remove(object);
	--object->host_ref_count;
}

TRUK_API void ModuleObject::dealloc() noexcept {
	peff::destroy_and_release<ModuleObject>(this->token_list.allocator(), this, alignof(StringObject));
}

TRUK_API void Runtime::GCWalkContext::push_object(Object *object) {
	if (!object)
		return;

	switch (object->_gc_status) {
		case ObjectGCStatus::Unwalked:
			object->_gc_status = ObjectGCStatus::ReadyToWalk;

			// object->gc_spinlock.lock();

			if (object == unwalked_list)
				unwalked_list = object->_next_object;

			// remove_from_cur_gcset(object);
			push_walkable(object);
			break;
		case ObjectGCStatus::ReadyToWalk:
			break;
		case ObjectGCStatus::Walked:
			break;
		default:
			std::terminate();
	}
}

TRUK_API bool Runtime::GCWalkContext::is_walkable_list_empty() {
	return !walkable_list;
}

TRUK_API Object *Runtime::GCWalkContext::get_walkable_list() {
	std::lock_guard access_mutex_guard(access_mutex);

	Object *p = walkable_list;
	walkable_list = nullptr;

	return p;
}

TRUK_API void Runtime::GCWalkContext::push_walkable(Object *walkable_object) {
	std::lock_guard access_mutex_guard(access_mutex);

	if (unwalked_list == walkable_object)
		unwalked_list = walkable_object->_next_object;

	walkable_object->_next_object = walkable_list;
	walkable_list = walkable_object;
}

TRUK_API Object *Runtime::GCWalkContext::get_unwalked_list(bool clear_list) {
	std::lock_guard access_mutex_guard(access_mutex);

	Object *p = unwalked_list;
	if (clear_list)
		unwalked_list = nullptr;

	return p;
}

TRUK_API void Runtime::GCWalkContext::push_unwalked(Object *walkable_object) {
	std::lock_guard access_mutex_guard(access_mutex);

	if (unwalked_list)
		unwalked_list->_prev_object = walkable_object;

	walkable_object->_next_object = unwalked_list;

	unwalked_list = walkable_object;
}

TRUK_API void Runtime::GCWalkContext::update_unwalked_list(Object *deleted_object) {
	std::lock_guard access_mutex_guard(access_mutex);

	if (unwalked_list == deleted_object)
		unwalked_list = deleted_object->_next_object;

	// remove_from_cur_gcset(deleted_object);
}

TRUK_API Object *Runtime::GCWalkContext::get_walked_list() {
	return walked_list;
}

TRUK_API void Runtime::GCWalkContext::push_walked(Object *walked_object) {
	std::lock_guard access_mutex_guard(access_mutex);

	if (walked_list)
		walked_list->_prev_object = walked_object;

	walked_object->_next_object = walked_list;

	walked_list = walked_object;
}

TRUK_API void Runtime::_gc_walk(GCWalkContext *context, const Value &i) {
	bool is_walkable_object_detected = false;
	switch (i.value_type) {
		case ValueType::Int:
		case ValueType::UInt:
		case ValueType::Long:
		case ValueType::ULong:
		case ValueType::SizeInt:
		case ValueType::USizeInt:
		case ValueType::Float:
		case ValueType::Double:
			break;
		case ValueType::Object:
			context->push_object(i.as_object);
			break;
		default:
			peff::panic("Unhandled value type");
	}
}

TRUK_API void Runtime::_gc_walk(GCWalkContext *context, Object *i) {
	switch (i->get_object_type()) {
		case ObjectType::String:
		case ObjectType::Symbol:
			break;
		case ObjectType::List: {
			ListObject *o = (ListObject *)i;
			for (auto j : o->elements)
				_gc_walk(context, j);
			break;
		}
		case ObjectType::Array: {
			ArrayObject *o = (ArrayObject *)i;
			for (auto j : o->elements)
				_gc_walk(context, j);
			break;
		}
		case ObjectType::Module: {
			ModuleObject *o = (ModuleObject *)i;
			for (auto j : o->members)
				context->push_object(j.second);
			break;
		}
	}
}

TRUK_API Runtime::Runtime(peff::Alloc *upstream) noexcept : _global_alloc(this, upstream) {
}

TRUK_API void Runtime::gc(Object *&end_object_out, size_t &num_objects) noexcept {
	size_t iteration_times = 0;
rescan:
	++iteration_times;
	GCWalkContext context;

	Object *host_ref_list = nullptr;

	{
		Object *prev = nullptr;

		for (Object *i = _object_list; i; i = i->_next_object) {
			i->_gc_status = ObjectGCStatus::Unwalked;

			// Check if the object is referenced by the host, if so, exclude them into a separated list.
			size_t host_ref_count = i->host_ref_count;
			switch (host_ref_count) {
				case 0:
					context.push_unwalked(i);
					break;
				default:
					if (host_ref_list)
						host_ref_list->_prev_object = i;
					i->_next_object = host_ref_list;
					host_ref_list = i;
			}

			prev = i;
		}

		end_object_out = prev;
	}

	for (Object *i = host_ref_list, *next; i; i = next) {
		next = i->_next_object;
		context.push_object(i);
	}

	if (!_is_deleting) {
		// Walk the root node.
		context.push_object(_root_module);
	}

	for (Object *p = context.get_walkable_list(), *i; p;) {
		i = p;

		while (i) {
			Object *next = i->_next_object;

			switch (i->_gc_status) {
				case ObjectGCStatus::Unwalked:
					std::terminate();
					break;
				case ObjectGCStatus::ReadyToWalk:
					_gc_walk(&context, i);
					break;
				case ObjectGCStatus::Walked:
					std::terminate();
					break;
			}

			i = next;
		}

		p = context.get_walkable_list();
	}

	bool is_rescan_needed = false;

	/*
	if (InstanceObject *p = context.get_destructible_list(); p) {
		_destruct_destructible_objects(p);
		is_rescan_needed = true;
	}*/

	if (is_rescan_needed) {
		goto rescan;
	}

	size_t num_deleted_objects = 0;
	// Delete unreachable objects.
	bool clear_unwalked_list = false;

	Object *i = context.get_unwalked_list(clear_unwalked_list);

	for (Object *next; i; i = next) {
		next = i->_next_object;

		if (i == _object_list) {
			_object_list = i->_next_object;
			assert(!i->_prev_object);
		}

		if (end_object_out == i) {
			if (i->_prev_object) {
				end_object_out = i->_prev_object;
			} else {
				end_object_out = nullptr;
			}
			assert(!i->_next_object);
		}

		if (i->_prev_object) {
			i->_prev_object->_next_object = i->_next_object;
		}

		if (i->_next_object) {
			i->_next_object->_prev_object = i->_prev_object;
		}

		i->dealloc();

		++num_deleted_objects;
	}

	clear_unwalked_list = true;

	num_objects -= num_deleted_objects;
}
