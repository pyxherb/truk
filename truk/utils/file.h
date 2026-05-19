#ifndef _TRUK_UTILS_FILE_H_
#define _TRUK_UTILS_FILE_H_

#include "../except.h"
#include <cstdio>

namespace truk {
	namespace utils {
		class File final {
	private:
		FILE *_fp;

	public:
		PEFF_FORCEINLINE void close() {
			if (_fp)
				fclose(_fp);
			_fp = nullptr;
		}
		PEFF_FORCEINLINE File() : _fp(nullptr) {}
		PEFF_FORCEINLINE File(FILE *fp) : _fp(fp) {}
		PEFF_FORCEINLINE ~File() {
			close();
		}

		PEFF_FORCEINLINE InternalExceptionPointer read(char *buffer, size_t size) const {
			if (!size)
				return {};
			if (fread(buffer, size, 1, _fp) < 1)
				return IOError::alloc();
			return {};
		}

		PEFF_FORCEINLINE InternalExceptionPointer write(const char *buffer, size_t size) const {
			if (!size)
				return {};
			if (fwrite(buffer, size, 1, _fp) < 1)
				return IOError::alloc();
			return {};
		}

		PEFF_FORCEINLINE InternalExceptionPointer write(const std::string_view &s) const {
			if (!s.size())
				return {};
			if (fwrite(s.data(), s.size(), 1, _fp) < 1)
				return IOError::alloc();
			return {};
		}

		PEFF_FORCEINLINE void set_c_file(FILE *fp) {
			close();
			_fp = fp;
		}

		PEFF_FORCEINLINE FILE* c_file() const {
			return _fp;
		}
	};
	}
}

#endif
