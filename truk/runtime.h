#ifndef _TRUK_MODULE_H_
#define _TRUK_MODULE_H_

#include "lexer.h"
#include "except.h"
#include <peff/containers/hashmap.h>
#include <peff/containers/set.h>

namespace truk {
	class Runtime;
	class Object;

	class CountablePoolAlloc : public peff::Alloc {
	protected:
		std::atomic_size_t ref_count = 0;

	public:
		Runtime *runtime;

		peff::RcObjectPtr<peff::Alloc> upstream;
		std::atomic_size_t sz_allocated = 0;

		TRUK_API CountablePoolAlloc(Runtime *runtime, peff::Alloc *upstream);

		TRUK_API virtual size_t inc_ref(size_t global_rc) noexcept override;
		TRUK_API virtual size_t dec_ref(size_t global_rc) noexcept override;

		TRUK_API virtual void *alloc(size_t size, size_t alignment) noexcept override;
		TRUK_API virtual void *realloc(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		TRUK_API virtual void *realloc_in_place(void *ptr, size_t size, size_t alignment, size_t new_size, size_t new_alignment) noexcept override;
		TRUK_API virtual void release(void *p, size_t size, size_t alignment) noexcept override;

		TRUK_API virtual bool is_replaceable(const peff::Alloc *rhs) const noexcept override;

		TRUK_API virtual peff::UUID type_identity() const noexcept override;
		TRUK_API virtual void on_ref_zero() noexcept;
	};

	enum class ValueType : uint8_t {
		Int,
		UInt,
		Long,
		ULong,
		SizeInt,
		USizeInt,

		Float,
		Double,

		Bool,

		Object
	};

	using ssize_t = std::make_signed_t<size_t>;

	struct Value {
		struct SizeWidthMarker {};
		union {
			Object *as_object;
			int32_t as_int;
			uint32_t as_uint;
			int64_t as_long;
			uint64_t as_ulong;
			ssize_t as_sizeint;
			size_t as_usizeint;
			float as_float;
			double as_double;
			bool as_bool;
		};
		ValueType value_type;

		constexpr Value() = default;
		constexpr Value(const Value &) = default;
		constexpr Value(Value &&) = default;
		constexpr ~Value() = default;
		PEFF_FORCEINLINE constexpr explicit Value(int32_t data) noexcept : value_type(ValueType::Int), as_int(data) {
		}
		PEFF_FORCEINLINE constexpr explicit Value(uint32_t data) noexcept : value_type(ValueType::UInt), as_uint(data) {
		}
		PEFF_FORCEINLINE constexpr explicit Value(int64_t data) noexcept : value_type(ValueType::Long), as_long(data) {
		}
		PEFF_FORCEINLINE constexpr explicit Value(uint64_t data) noexcept : value_type(ValueType::ULong), as_ulong(data) {
		}
		PEFF_FORCEINLINE constexpr Value(SizeWidthMarker marker, ssize_t data) noexcept : value_type(ValueType::SizeInt), as_sizeint(data) {
		}
		PEFF_FORCEINLINE constexpr Value(SizeWidthMarker marker, size_t data) noexcept : value_type(ValueType::USizeInt), as_usizeint(data) {
		}
		PEFF_FORCEINLINE constexpr explicit Value(float data) noexcept : value_type(ValueType::Float), as_float(data) {
		}
		PEFF_FORCEINLINE constexpr explicit Value(double data) noexcept : value_type(ValueType::Double), as_double(data) {
		}
		PEFF_FORCEINLINE constexpr explicit Value(bool data) noexcept : value_type(ValueType::Bool), as_bool(data) {
		}
		PEFF_FORCEINLINE constexpr explicit Value(Object *data) noexcept : value_type(ValueType::Object), as_object(data) {
		}

		PEFF_FORCEINLINE constexpr Value &operator=(int32_t data) noexcept {
			value_type = ValueType::Int;
			this->as_int = data;
			return *this;
		}

		PEFF_FORCEINLINE constexpr Value &operator=(uint32_t data) noexcept {
			value_type = ValueType::UInt;
			this->as_uint = data;
			return *this;
		}

		PEFF_FORCEINLINE constexpr Value &operator=(int64_t data) noexcept {
			value_type = ValueType::Long;
			this->as_long = data;
			return *this;
		}

		PEFF_FORCEINLINE constexpr Value &operator=(uint64_t data) noexcept {
			value_type = ValueType::ULong;
			this->as_ulong = data;
			return *this;
		}

		PEFF_FORCEINLINE constexpr Value &operator=(std::pair<SizeWidthMarker, ssize_t> data) noexcept {
			value_type = ValueType::SizeInt;
			this->as_sizeint = data.second;
			return *this;
		}

		PEFF_FORCEINLINE constexpr Value &operator=(std::pair<SizeWidthMarker, size_t> data) noexcept {
			value_type = ValueType::USizeInt;
			this->as_usizeint = data.second;
			return *this;
		}

		PEFF_FORCEINLINE constexpr Value &operator=(float data) noexcept {
			value_type = ValueType::Float;
			this->as_float = data;
			return *this;
		}

		PEFF_FORCEINLINE constexpr Value &operator=(double data) noexcept {
			value_type = ValueType::Double;
			this->as_double = data;
			return *this;
		}

		PEFF_FORCEINLINE constexpr Value &operator=(bool data) noexcept {
			value_type = ValueType::Bool;
			this->as_bool = data;
			return *this;
		}
	};

	enum class ObjectType {
		String = 0,
		Symbol,
		List,
		Array,
		Module
	};

	enum class ObjectGCStatus : uint8_t {
		Unwalked = 0,
		ReadyToWalk,
		Walked
	};

	class Object {
	private:
		Object *_prev_object = nullptr, *_next_object = nullptr;
		ObjectType _object_type;
		ObjectGCStatus _gc_status = ObjectGCStatus::Unwalked;

		friend class Runtime;

	public:
		std::atomic_size_t host_ref_count;

		TRUK_API Object(ObjectType object_type);
		TRUK_API ~Object();

		virtual void dealloc() noexcept = 0;

		PEFF_FORCEINLINE ObjectType get_object_type() const noexcept {
			return _object_type;
		}
	};

	class StringObject : public Object {
	public:
		peff::String data;

		TRUK_API StringObject(peff::Alloc *allocator);
		TRUK_API ~StringObject();

		TRUK_API virtual void dealloc() noexcept override;
	};

	class SymbolObject : public Object {
	public:
		peff::String name;

		TRUK_API SymbolObject(peff::Alloc *allocator);
		TRUK_API ~SymbolObject();

		TRUK_API virtual void dealloc() noexcept override;
	};

	class ListObject : public Object {
	public:
		peff::DynArray<Value> elements;

		TRUK_API ListObject(peff::Alloc *allocator);
		TRUK_API ~ListObject();

		TRUK_API virtual void dealloc() noexcept override;
	};

	class ArrayObject : public Object {
	public:
		peff::DynArray<Value> elements;
		size_t size = 0;
		size_t rank = 0;

		TRUK_API ArrayObject(peff::Alloc *allocator);
		TRUK_API ~ArrayObject();

		TRUK_API virtual void dealloc() noexcept override;
	};

	class MemberObject : public Object {
	public:
		peff::String name;

		TRUK_API MemberObject(peff::Alloc *allocator, ObjectType object_type);
		TRUK_API ~MemberObject();
	};

	class ModuleObject final : public MemberObject {
	public:
		MemberObject *parent;
		TokenList token_list;
		peff::HashMap<std::string_view, MemberObject *> members;

		TRUK_API ModuleObject(peff::Alloc *allocator);
		TRUK_API ~ModuleObject();

		TRUK_API virtual void dealloc() noexcept override;
	};

	template <typename T = Object>
	class HostObjectRef final {
	public:
		T *_value = nullptr;

		PEFF_FORCEINLINE void reset() noexcept {
			if (_value) {
				--_value->host_ref_count;
				_value = nullptr;
			}
		}

		PEFF_FORCEINLINE T *release() noexcept {
			T *v = _value;
			--_value->host_ref_count;
			_value = nullptr;
			return v;
		}

		PEFF_FORCEINLINE void discard() noexcept { _value = nullptr; }

		PEFF_FORCEINLINE HostObjectRef(const HostObjectRef<T> &x) noexcept : _value(x._value) {
			if (x._value) {
				++_value->host_ref_count;
			}
		}
		PEFF_FORCEINLINE HostObjectRef(HostObjectRef<T> &&x) noexcept : _value(x._value) {
			if (x._value) {
				x._value = nullptr;
			}
		}
		PEFF_FORCEINLINE HostObjectRef(T *value = nullptr) noexcept : _value(value) {
			if (_value) {
				++_value->host_ref_count;
			}
		}
		PEFF_FORCEINLINE ~HostObjectRef() {
			reset();
		}

		PEFF_FORCEINLINE const T *get() const noexcept { return _value; }
		PEFF_FORCEINLINE T *get() noexcept { return _value; }
		PEFF_FORCEINLINE const T *operator->() const noexcept { return _value; }
		PEFF_FORCEINLINE T *operator->() noexcept { return _value; }

		PEFF_FORCEINLINE HostObjectRef<T> &operator=(const HostObjectRef<T> &x) noexcept {
			reset();

			if ((_value = x._value)) {
				++_value->host_ref_count;
			}

			return *this;
		}
		PEFF_FORCEINLINE HostObjectRef<T> &operator=(HostObjectRef<T> &&x) noexcept {
			reset();

			if ((_value = x._value)) {
				x._value = nullptr;
			}

			return *this;
		}

		PEFF_FORCEINLINE HostObjectRef<T> &operator=(T *other) noexcept {
			reset();

			if ((_value = other)) {
				++_value->host_ref_count;
			}

			return *this;
		}

		PEFF_FORCEINLINE bool operator<(const HostObjectRef<T> &rhs) const noexcept {
			return _value < rhs._value;
		}
		PEFF_FORCEINLINE bool operator>(const HostObjectRef<T> &rhs) const noexcept {
			return _value > rhs._value;
		}
		PEFF_FORCEINLINE bool operator==(const HostObjectRef<T> &rhs) const noexcept {
			return _value == rhs._value;
		}

		PEFF_FORCEINLINE operator bool() const {
			return _value;
		}
	};

	class HostRefHolder final {
	public:
		peff::Set<Object *> holded_objects;

		TRUK_API HostRefHolder(
			peff::Alloc *self_allocator);
		TRUK_API ~HostRefHolder();

		[[nodiscard]] TRUK_API bool add_object(Object *object);
		TRUK_API void remove_object(Object *object) noexcept;
	};

	template <typename T, typename... Args>
	PEFF_FORCEINLINE HostObjectRef<T> alloc_runtime_managed_object(peff::Alloc *allocator, Args &&...args) {
		return peff::alloc_and_construct<T>(allocator, alignof(T), std::forward<Args>(args)...);
	}

	class Runtime {
	private:
		Object *_object_list = nullptr;
		bool _is_deleting = false;

		struct GCWalkContext {
		private:
			Object *walkable_list = nullptr;
			std::mutex access_mutex;
			Object *unwalked_list = nullptr;
			Object *walked_list = nullptr;

		public:
			TRUK_API void push_object(Object *object);
			TRUK_API void remove_from_unwalked_list(Object *v);
			TRUK_API void remove_from_walkable_list(Object *v);

			TRUK_API bool is_walkable_list_empty();
			TRUK_API Object *get_walkable_list();
			TRUK_API void push_walkable(Object *walkable_object);

			TRUK_API Object *get_unwalked_list(bool clear_list);
			TRUK_API void push_unwalked(Object *walkable_object);
			TRUK_API void update_unwalked_list(Object *deleted_object);

			TRUK_API Object *get_walked_list();
			TRUK_API void push_walked(Object *walked_object);

			TRUK_API void reset();
		};

		TRUK_API void _gc_walk(GCWalkContext *context, const Value &i);
		TRUK_API void _gc_walk(GCWalkContext *context, Object *i);

		size_t _num_objects = 0;
		ModuleObject *_root_module = nullptr;
		CountablePoolAlloc _global_alloc;

	public:
		TRUK_API Runtime(peff::Alloc *upstream) noexcept;
		TRUK_API void gc(Object *&end_object_out, size_t &num_objects) noexcept;

		PEFF_FORCEINLINE peff::Alloc *get_global_allocator() noexcept {
			return &_global_alloc;
		}
	};
}

#endif
