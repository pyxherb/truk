#include "except.h"

using namespace truk;

static OutOfMemoryError _g_oom_singleton;

TRUK_API OutOfMemoryError::OutOfMemoryError() noexcept : InternalException(InternalExceptionKind::OutOfMemory) {
}

TRUK_API OutOfMemoryError::~OutOfMemoryError() {
}

TRUK_API void OutOfMemoryError::dealloc() noexcept {
}

TRUK_API OutOfMemoryError *OutOfMemoryError::alloc() noexcept {
	return &_g_oom_singleton;
}

TRUK_API CompilationError::CompilationError(
	peff::Alloc *allocator,
	CompilationErrorCode error_code) noexcept : InternalException(InternalExceptionKind::CompilationError), self_allocator(allocator), error_code(error_code) {
}

TRUK_API CompilationError::~CompilationError() {
}

TRUK_API LexicalError::LexicalError(
	peff::Alloc *allocator,
	const SourcePosition &source_position) noexcept : CompilationError(allocator, CompilationErrorCode::LexicalError), source_position(source_position) {
}

TRUK_API LexicalError::~LexicalError() {
}

TRUK_API void LexicalError::dealloc() noexcept {
	peff::destroy_and_release<LexicalError>(self_allocator.get(), this, alignof(LexicalError));
}

TRUK_API LexicalError *LexicalError::alloc(
	peff::Alloc *allocator,
	const SourcePosition &source_position) noexcept {
	return peff::alloc_and_construct<LexicalError>(allocator, alignof(LexicalError), allocator, source_position);
}

TRUK_API SyntaxError::SyntaxError(
	peff::Alloc *allocator,
	SyntaxErrorKind syntax_error_kind) noexcept : CompilationError(allocator, CompilationErrorCode::SyntaxError), syntax_error_kind(syntax_error_kind) {
}

TRUK_API SyntaxError::~SyntaxError() {
}

TRUK_API void SyntaxError::dealloc() noexcept {
	peff::destroy_and_release<SyntaxError>(self_allocator.get(), this, alignof(SyntaxError));
}

TRUK_API SyntaxError *SyntaxError::alloc(
	peff::Alloc *allocator,
	SyntaxErrorKind syntax_error_kind) noexcept {
	return peff::alloc_and_construct<SyntaxError>(allocator, alignof(SyntaxError), allocator, syntax_error_kind);
}
