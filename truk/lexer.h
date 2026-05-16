#ifndef _SLKC_AST_LEXER_H_
#define _SLKC_AST_LEXER_H_

#include "basedefs.h"
#include <peff/base/deallocable.h>
#include <peff/containers/dynarray.h>
#include <peff/containers/string.h>
#include <peff/advutils/shared_ptr.h>
#include <peff/utils/option.h>
#include <variant>
#include "except_base.h"

namespace truk {
	struct SourcePosition {
		size_t line, column;

		PEFF_FORCEINLINE SourcePosition() : line(SIZE_MAX), column(SIZE_MAX) {}
		PEFF_FORCEINLINE SourcePosition(size_t line, size_t column) : line(line), column(column) {}

		PEFF_FORCEINLINE bool operator<(const SourcePosition &loc) const {
			if (line < loc.line)
				return true;
			if (line > loc.line)
				return false;
			return column < loc.column;
		}

		PEFF_FORCEINLINE bool operator>(const SourcePosition &loc) const {
			if (line > loc.line)
				return true;
			if (line < loc.line)
				return false;
			return column > loc.column;
		}

		PEFF_FORCEINLINE bool operator==(const SourcePosition &loc) const {
			return (line == loc.line) && (column == loc.column);
		}

		PEFF_FORCEINLINE bool operator>=(const SourcePosition &loc) const {
			return ((*this) == loc) || ((*this) > loc);
		}

		PEFF_FORCEINLINE bool operator<=(const SourcePosition &loc) const {
			return ((*this) == loc) || ((*this) < loc);
		}
	};

	template<typename T>
	using AstNodePtr = peff::SharedPtr<T>;

	class Module;

	struct SourceLocation {
		Module *module_node;
		SourcePosition begin_position, end_position;
	};

	class Lexer;

	enum class TokenId : int {
		End = 0,

		Unknown,

		LParenthese,
		RParenthese,

		Text,

		Quote,

		Whitespace,
		NewLine,
		LineComment,
		BlockComment,
		DocumentationComment,

		MaxToken
	};

	struct Token {
		TokenId token_id;
		std::string_view source_text;
		SourceLocation source_location;
	};

	using TokenList = peff::DynArray<Token>;

	[[nodiscard]] TRUK_API InternalExceptionPointer lex(Module *module_node, const std::string_view &src, peff::Alloc *allocator, TokenList &token_list_out);

	TRUK_API const char *get_token_name(TokenId token_id);
}

#endif
