#ifndef _MKLISP_EXCEPT_H_
#define _MKLISP_EXCEPT_H_

#include "except_base.h"
#include "lexer.h"

namespace truk {
	class OutOfMemoryError final : public InternalException {
	public:
		TRUK_API OutOfMemoryError() noexcept;
		TRUK_API virtual ~OutOfMemoryError();
		TRUK_API virtual void dealloc() noexcept override;

		TRUK_API static OutOfMemoryError *alloc() noexcept;
	};

	class IOError final : public InternalException {
	public:
		TRUK_API IOError() noexcept;
		TRUK_API virtual ~IOError();
		TRUK_API virtual void dealloc() noexcept override;

		TRUK_API static IOError *alloc() noexcept;
	};

	enum class CompilationErrorCode {
		LexicalError = 0,
		SyntaxError
	};

	class CompilationError : public InternalException {
	public:
		peff::RcObjectPtr<peff::Alloc> self_allocator;
		SourceLocation source_location;
		CompilationErrorCode error_code;

		TRUK_API CompilationError(
			peff::Alloc *allocator,
			CompilationErrorCode error_code) noexcept;
		TRUK_API virtual ~CompilationError();
	};

	enum class LexicalErrorKind : uint8_t {
		UnrecognizedToken = 0,
		PrematuredEOF,
		UnterminatedString,
		InvalidEscape
	};

	class LexicalError : public CompilationError {
	public:
		SourcePosition source_position;
		LexicalErrorKind lexical_error_kind;

		TRUK_API LexicalError(
			peff::Alloc *allocator,
			const SourcePosition &source_position,
			LexicalErrorKind lexical_error_kind) noexcept;
		TRUK_API virtual ~LexicalError();
		TRUK_API virtual void dealloc() noexcept override;

		TRUK_API static LexicalError *alloc(
			peff::Alloc *allocator,
			const SourcePosition &source_position,
			LexicalErrorKind lexical_error_kind) noexcept;
	};

	enum class SyntaxErrorKind {
		UnexpectedToken,
		LiteralOverflowed,
	};

	class SyntaxError : public CompilationError {
	public:
		SourceLocation source_location;
		SyntaxErrorKind syntax_error_kind;

		TRUK_API SyntaxError(
			peff::Alloc *allocator,
			SourceLocation source_location,
			SyntaxErrorKind syntax_error_kind) noexcept;
		TRUK_API virtual ~SyntaxError();
		TRUK_API virtual void dealloc() noexcept override;

		TRUK_API static SyntaxError *alloc(
			peff::Alloc *allocator,
			SourceLocation source_location,
			SyntaxErrorKind syntax_error_kind) noexcept;
	};

	PEFF_FORCEINLINE InternalException *with_oom_error_if_alloc_failed(InternalException *e) noexcept {
		if (!e)
			return OutOfMemoryError::alloc();
		return e;
	}
}

#endif
