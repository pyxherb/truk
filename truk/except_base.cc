#include "except_base.h"

using namespace mklisp;

MKLISP_API InternalException::InternalException(
	std::pmr::memory_resource *memoryResource,
	InternalExceptionKind exceptionKind) : memoryResource(memoryResource), exceptionKind(exceptionKind) {
}

MKLISP_API InternalException::~InternalException() {
}
