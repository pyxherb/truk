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

TRUK_API SymbolObject::SymbolObject(peff::Alloc *allocator) : Object(ObjectType::Symbol), name_entries(allocator) {
}

TRUK_API SymbolObject::~SymbolObject() {
}

TRUK_API void SymbolObject::dealloc() noexcept {
	peff::destroy_and_release<SymbolObject>(name_entries.allocator(), this, alignof(StringObject));
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

TRUK_API ScopeObject::ScopeObject(peff::Alloc *allocator) : Object(ObjectType::Scope), members(allocator), imports(allocator) {
}

TRUK_API ScopeObject::~ScopeObject() {
}

TRUK_API void ScopeObject::dealloc() noexcept {
	peff::destroy_and_release<ScopeObject>(members.allocator(), this, alignof(ScopeObject));
}

TRUK_API MemberObject::MemberObject(peff::Alloc *allocator, ObjectType object_type) : Object(object_type), name(allocator) {
}

TRUK_API MemberObject::~MemberObject() {
}

TRUK_API ModuleObject::ModuleObject(peff::Alloc *allocator) : MemberObject(allocator, ObjectType::Module), token_list(allocator) {
}

TRUK_API ModuleObject::~ModuleObject() {
}

TRUK_API FnObject::FnObject(peff::Alloc *allocator, FnType fn_type) : MemberObject(allocator, ObjectType::Fn), _fn_type(fn_type) {
}
TRUK_API FnObject::~FnObject() {
}

TRUK_API RegularFnObject::RegularFnObject(peff::Alloc *allocator) : FnObject(allocator, FnType::Regular), allocator(allocator) {
}
TRUK_API RegularFnObject::~RegularFnObject() {
}

TRUK_API void RegularFnObject::dealloc() noexcept {
	peff::destroy_and_release<RegularFnObject>(allocator.get(), this, alignof(RegularFnObject));
}

TRUK_API NativeFnObject::NativeFnObject(peff::Alloc *allocator) : FnObject(allocator, FnType::Native), allocator(allocator) {
}
TRUK_API NativeFnObject::~NativeFnObject() {
}

TRUK_API void NativeFnObject::dealloc() noexcept {
	peff::destroy_and_release<NativeFnObject>(allocator.get(), this, alignof(NativeFnObject));
}

TRUK_API SpecialOperatorObject::SpecialOperatorObject(peff::Alloc *allocator) : FnObject(allocator, FnType::SpecialOperator), allocator(allocator) {
}
TRUK_API SpecialOperatorObject::~SpecialOperatorObject() {
}

TRUK_API void SpecialOperatorObject::dealloc() noexcept {
	peff::destroy_and_release<SpecialOperatorObject>(allocator.get(), this, alignof(SpecialOperatorObject));
}

TRUK_API ContextObject::ContextObject(peff::Alloc *allocator) : Object(ObjectType::Context), frames(allocator) {}

TRUK_API ContextObject::~ContextObject() {
}

TRUK_API void ContextObject::dealloc() noexcept {
	peff::destroy_and_release<ContextObject>(frames.allocator(), this, alignof(SpecialOperatorObject));
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
		case ValueType::QuotedObject:
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
		case ObjectType::Scope: {
			ScopeObject *o = (ScopeObject *)i;
			context->push_object(o->owner);
			for (auto j : o->imports)
				context->push_object(j);
			for (auto j : o->members)
				context->push_object(j.second);
			context->push_object(o->lexical_outer);
			break;
		}
		case ObjectType::Module: {
			ModuleObject *o = (ModuleObject *)i;
			_gc_walk(context, o->scope);
			break;
		}
		case ObjectType::Fn: {
			FnObject *o = (FnObject *)i;
			break;
		}
		case ObjectType::Context: {
			ContextObject *o = (ContextObject *)i;

			for (auto &j : o->frames) {
				_gc_walk(context, j.cur_scope);

				context->push_object(j.list_object);
			}

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

TRUK_API MemberObject *Runtime::resolve_member(ScopeObject *scope, SymbolObject *symbol) {
	ScopeObject *cur_scope = scope;
	MemberObject *cur_member = nullptr;

	size_t cur_entry = 0;
	while (cur_entry < symbol->name_entries.size()) {
		if (auto it = cur_scope->members.find(symbol->name_entries.at(cur_entry)); it != cur_scope->members.end()) {
			cur_member = it.value();
			cur_scope = it.value()->scope;
			++cur_entry;
		}

		for (auto i : cur_scope->imports) {
			if (i->scope) {
				if (auto it = i->scope->members.find(symbol->name_entries.at(cur_entry)); it != i->scope->members.end()) {
					cur_member = it.value();
					cur_scope = it.value()->scope;
					++cur_entry;
				}
			}
		}

		if (cur_scope->lexical_outer) {
			cur_scope = cur_scope->lexical_outer;
			continue;
		}

		return nullptr;
	}

	return cur_member;
}

TRUK_API EvalFrame::EvalFrame(peff::Alloc *allocator) : evaluated_operands(allocator) {
}

TRUK_API InternalExceptionPointer Runtime::exec(ContextObject *context) {
	size_t initial_depth = context->frames.size();
	while (context->frames.size() >= initial_depth) {
		auto &cur_frame = context->frames.back();

		if (cur_frame.list_object->elements.size()) {
			if (!cur_frame.eval_index) {
				// The callee has not been evaluated.
				const size_t num_operands = cur_frame.list_object->elements.size() - 1;

				if (num_operands) {
					if (!cur_frame.evaluated_operands.resize(num_operands)) {
						return OutOfMemoryError::alloc();
					}
				}

				auto &fv = cur_frame.list_object->elements.at(0);

				switch (fv.value_type) {
					case ValueType::Object: {
						Object *fo = (Object *)fv.as_object;

						switch (fo->get_object_type()) {
							case ObjectType::Symbol: {
								MemberObject *fm = resolve_member(cur_frame.cur_scope, (SymbolObject *)fo);

								if (!fm)
									// TODO: Use a proper one to replace this simple termination.
									std::terminate();

								switch (fm->get_object_type()) {
									case ObjectType::Fn: {
										FnObject *f = (FnObject *)fm;

										cur_frame.cached_callee = f;

										switch (f->get_fn_type()) {
											case FnType::SpecialOperator:
												// Initialize the frame extended data for the special operator.
												TRUK_RETURN_IF_EXCEPT(((SpecialOperatorObject *)f)->frame_init_callback(&cur_frame));
												break;
											default:
												break;
										}

										++cur_frame.eval_index;
										break;
									}
									default:
										// The target is not callable.
										std::terminate();
										break;
								}

								break;
							}
							case ObjectType::List: {
								++cur_frame.eval_index;

								EvalFrame frame(get_global_allocator());

								frame.cur_scope = cur_frame.cur_scope;
								frame.list_object = ((ListObject *)fo);

								continue;
							}
							default:
								break;
						}

						break;
					}
					default:
						(++context->frames.begin_reversed())->returned_value = cur_frame.list_object;
						break;
				}
			} else if (cur_frame.eval_index >= cur_frame.list_object->elements.size()) {
				// All elements are evaluated.
				switch (cur_frame.cached_callee->get_fn_type()) {
					case FnType::Regular: {
						EvalFrame frame(get_global_allocator());

						// TODO: Allocate a new scope.
						frame.cur_scope = cur_frame.cur_scope;
						frame.list_object = ((RegularFnObject *)cur_frame.cached_callee)->body;

						context->frames.pop_back();

						if (!context->frames.push_back(std::move(frame)))
							return OutOfMemoryError::alloc();
						break;
					}
					case FnType::Native: {
						TRUK_RETURN_IF_EXCEPT(((NativeFnObject *)cur_frame.cached_callee)->callback(&cur_frame));

						(++context->frames.begin_reversed())->returned_value = cur_frame.returned_value;

						context->frames.pop_back();
						break;
					}
					case FnType::SpecialOperator: {
						TRUK_RETURN_IF_EXCEPT(((SpecialOperatorObject *)cur_frame.cached_callee)->invoke_callback(&cur_frame));

						(++context->frames.begin_reversed())->returned_value = cur_frame.returned_value;

						context->frames.pop_back();
						break;
					}
					default:
						std::terminate();
				}
			} else {
				const Value &cur_val = cur_frame.list_object->elements.at(cur_frame.eval_index);

				switch (cur_frame.cached_callee->get_fn_type()) {
					case FnType::SpecialOperator: {
						// The special operator will decide if the operand should be evaluated.
						bool should_evaluate = true;

						TRUK_RETURN_IF_EXCEPT(((SpecialOperatorObject *)cur_frame.cached_callee)->operand_eval_callback(&cur_frame, cur_frame.eval_index, cur_val, should_evaluate));
						break;
					}
					default:
						break;
				}

				switch (cur_val.value_type) {
					case ValueType::Object: {
						auto vo = cur_val.as_object;
						switch (vo->get_object_type()) {
							case ObjectType::Symbol: {
								// Resolve the symbol.
								MemberObject *fm = resolve_member(cur_frame.cur_scope, (SymbolObject *)vo);

								cur_frame.evaluated_operands.at(cur_frame.eval_index) = fm;

								++cur_frame.eval_index;

								break;
							}
							case ObjectType::List: {
								// Evaluate the list.
								++cur_frame.eval_index;

								EvalFrame frame(get_global_allocator());

								frame.cur_scope = cur_frame.cur_scope;
								frame.list_object = ((ListObject *)vo);

								continue;
							}
							default:
								break;
						}
					}
					default:
						// No need to evaluate.
						break;
				}
			}
		} else {
			(++context->frames.begin_reversed())->returned_value = cur_frame.list_object;
		}
	}
}

TRUK_API InternalExceptionPointer Runtime::exec(ScopeObject *cur_scope, ListObject *list) {
	HostObjectRef<ContextObject> context;

	if (!(context = alloc_runtime_managed_object<ContextObject>(get_global_allocator(), get_global_allocator())))
		return OutOfMemoryError::alloc();

	EvalFrame frame(get_global_allocator());

	frame.cur_scope = cur_scope;
	frame.list_object = list;

	if (!context->frames.push_back(std::move(frame)))
		return OutOfMemoryError::alloc();

	TRUK_RETURN_IF_EXCEPT(exec(context.get()));

	return {};
}
