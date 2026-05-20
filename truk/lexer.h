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

	template <typename T>
	using AstNodePtr = peff::SharedPtr<T>;

	class ModuleObject;

	struct SourceLocation {
		ModuleObject *module_node;
		size_t begin_index, end_index;

		PEFF_FORCEINLINE SourceLocation(): module_node(nullptr), begin_index(SIZE_MAX), end_index(SIZE_MAX) {}
		PEFF_FORCEINLINE SourceLocation(ModuleObject *module_node, size_t index) : module_node(module_node), begin_index(index), end_index(index) {}
		PEFF_FORCEINLINE SourceLocation(ModuleObject *module_node, size_t begin_index, size_t end_index) : module_node(module_node), begin_index(begin_index), end_index(end_index) {}
	};

	class Lexer;

	enum class TokenId : int {
		End = 0,

		Unknown,

		LParenthese,
		RParenthese,

		ScopeOp,

		Quote,
		Colon,
		Dollar,
		Question,
		Hash,
		Percent,
		Xor,
		Dot,
		And,
		At,
		Exclamation,
		Tilde,
		Asterisk,
		Assign,
		Eq,
		Neq,
		Ellipsis,
		Backquote,

		LBrace,
		RBrace,
		LBracket,
		RBracket,

		IntLiteral,
		LongLiteral,
		UIntLiteral,
		ULongLiteral,
		FloatLiteral,
		DoubleLiteral,
		StringLiteral,

		Id,

		Whitespace,
		NewLine,
		LineComment,
		BlockComment,
		DocumentationComment,

		MaxToken
	};

	enum class IntTokenType : uint8_t {
		Decimal,
		Hexadecimal,
		Octal,
		Binary
	};

	struct Token {
		size_t token_index;
		TokenId token_id;
		std::string_view source_text;
		SourcePosition begin_pos, end_pos;
		std::variant<std::monostate, peff::String, IntTokenType> exdata;
	};

	using TokenList = peff::DynArray<Token>;

	[[nodiscard]] TRUK_API InternalExceptionPointer lex(ModuleObject *module_node, const std::string_view &src, peff::Alloc *allocator, TokenList &token_list_out);

	TRUK_API const char *get_token_name(TokenId token_id);
}

#endif
