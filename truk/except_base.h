#ifndef _TRUK_EXCEPTBASE_H_
#define _TRUK_EXCEPTBASE_H_

#include "basedefs.h"
#include <cassert>
#include <memory>
#include <peff/base/deallocable.h>
#include <peff/base/panic.h>

namespace truk {
	enum class InternalExceptionKind {
		OutOfMemory = 0,
		CompilationError
	};

	class InternalException {
	public:
		InternalExceptionKind except_kind;

		TRUK_API InternalException(InternalExceptionKind except_kind);
		TRUK_API virtual ~InternalException();

		virtual void dealloc() noexcept = 0;
	};

	class InternalExceptionPointer {
	private:
		std::unique_ptr<InternalException,
			peff::DeallocableDeleter<InternalException>>
			_ptr;

	public:
		PEFF_FORCEINLINE void unwrap() {
			if (_ptr) {
				peff::panic("Unhandled internal exception");
			}
		}

		PEFF_FORCEINLINE InternalExceptionPointer() = default;
		PEFF_FORCEINLINE InternalExceptionPointer(InternalException *exception)
			: _ptr(exception) {}

		PEFF_FORCEINLINE ~InternalExceptionPointer() {
			unwrap();
		}

		InternalExceptionPointer(const InternalExceptionPointer &) = delete;
		InternalExceptionPointer &
		operator=(const InternalExceptionPointer &) = delete;
		PEFF_FORCEINLINE InternalExceptionPointer(InternalExceptionPointer &&other) {
			_ptr = std::move(other._ptr);
		}
		PEFF_FORCEINLINE InternalExceptionPointer &
		operator=(InternalExceptionPointer &&other) {
			unwrap();
			_ptr = std::move(other._ptr);
			return *this;
		}

		PEFF_FORCEINLINE InternalException *get() { return _ptr.get(); }
		PEFF_FORCEINLINE const InternalException *get() const { return _ptr.get(); }

		PEFF_FORCEINLINE void reset() { _ptr.reset(); }

		PEFF_FORCEINLINE explicit operator bool() { return (bool)_ptr; }

		PEFF_FORCEINLINE InternalException *operator->() { return _ptr.get(); }

		PEFF_FORCEINLINE const InternalException *operator->() const {
			return _ptr.get();
		}
	};
}

#define TRUK_RETURN_IF_EXCEPT(expr)                         \
	if (truk::InternalExceptionPointer e = (expr); (bool)e) \
	return e
#define TRUK_RETURN_IF_EXCEPT_WITH_LVAR(name, expr) \
	if ((bool)(name = (expr)))                      \
		return name;

#endif
