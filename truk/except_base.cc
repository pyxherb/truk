#include "except_base.h"

using namespace truk;

TRUK_API InternalException::InternalException(
	InternalExceptionKind except_kind) : except_kind(except_kind) {
}

TRUK_API InternalException::~InternalException() {
}
