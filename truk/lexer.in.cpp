#include "lexer.h"
#include "except.h"
#include <algorithm>

using namespace truk;

enum LexCondition {
	yycInitialCondition = 0,

	yycStringCondition,
	yycEscapeCondition,

	yycCommentCondition,
	yycLineCommentCondition,
};

TRUK_API InternalExceptionPointer truk::lex(Module *module_node, const std::string_view &src, peff::Alloc *allocator, TokenList &token_list_out) {
	const char *YYCURSOR = src.data(), *YYMARKER = YYCURSOR, *YYLIMIT = src.data() + src.size();
	const char *prev_YYCURSOR = YYCURSOR;

	LexCondition YYCONDITION = yycInitialCondition;

#define YYSETCONDITION(cond) (YYCONDITION = (yyc##cond))
#define YYGETCONDITION() (YYCONDITION)

	Token token;

	while (true) {
		peff::String str_literal(allocator);

		while (true) {
			/*!re2c
				re2c:yyfill:enable = 0;
				re2c:define:YYCTYPE = char;
				re2c:eof = 1;

				<InitialCondition>"///"		{ YYSETCONDITION(LineCommentCondition); token->token_id = TokenId::DocumentationComment; continue; }
				<InitialCondition>"//"		{ YYSETCONDITION(LineCommentCondition); token->token_id = TokenId::LineComment; continue; }
				<InitialCondition>"/*"		{ YYSETCONDITION(CommentCondition); token->token_id = TokenId::BlockComment; continue; }

				<InitialCondition>"'"		{ token->token_id = TokenId::Quote; break; }

				<InitialCondition>"("		{ token->token_id = TokenId::LParenthese; break; }
				<InitialCondition>")"		{ token->token_id = TokenId::RParenthese; break; }

				<InitialCondition>[a-zA-Z_\x80-\xff][a-zA-Z0-9_\x80-\xff]* {
					token->token_id = TokenId::Id;
					break;
				}

				<InitialCondition>"\""		{ YYSETCONDITION(StringCondition); continue; }

				<InitialCondition>"\n"		{ token->token_id = TokenId::NewLine; break; }
				<InitialCondition>$			{ goto end; }

				<InitialCondition>[ \r\t]+	{ token->token_id = TokenId::Whitespace; break; }

				<InitialCondition>[^]		{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return LexicalError {
						SourceLocation {
						module_node,
						{ (size_t)std::count(str_to_begin.begin(), str_to_begin.end(), '\n'), prev_YYCURSOR_pos },
						{ (size_t)std::count(str_to_end.begin(), str_to_end.end(), '\n'), YYCURSOR_pos }
					}, LexicalErrorKind::UnrecognizedToken};
				}

				<StringCondition>"\""		{
					YYSETCONDITION(InitialCondition);
					token->token_id = TokenId::StringLiteral;
					token->ex_data = std::unique_ptr<TokenExtension, peff::DeallocableDeleter<TokenExtension>>(
						peff::alloc_and_construct<StringTokenExtension>(allocator, alignof(std::max_align_t), allocator, std::move(str_literal)));
					break;
				}
				<StringCondition>"\\"		{ YYSETCONDITION(EscapeCondition); continue; }
				<StringCondition>"\n"		{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					// Unexpected end of line.
					return with_oom_error_if_alloc_failed(
						LexicalError::alloc(allocator, SourcePosition { module_node, (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), pos })
					);
				}
				<StringCondition>$	{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return with_oom_error_if_alloc_failed(
						LexicalError::alloc(allocator, SourcePosition { module_node, (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), pos })
					);
				}
				<StringCondition>[^]		{ if(!str_literal.push_back(+YYCURSOR[-1])) goto oom; continue; }

				<EscapeCondition>"'"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\'')) goto oom; continue; }
				<EscapeCondition>"\""	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('"')) goto oom; continue; }
				<EscapeCondition>"?"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('?')) goto oom; continue; }
				<EscapeCondition>"\\"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\\')) goto oom; continue; }
				<EscapeCondition>"a"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\a')) goto oom; continue; }
				<EscapeCondition>"b"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\b')) goto oom; continue; }
				<EscapeCondition>"f"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\f')) goto oom; continue; }
				<EscapeCondition>"n"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\n')) goto oom; continue; }
				<EscapeCondition>"r"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\r')) goto oom; continue; }
				<EscapeCondition>"t"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\t')) goto oom; continue; }
				<EscapeCondition>"v"	{ YYSETCONDITION(StringCondition); if(!str_literal.push_back('\v')) goto oom; continue; }
				<EscapeCondition>[0-7]{1,3}	{
					YYSETCONDITION(StringCondition);

					size_t size = YYCURSOR - prev_YYCURSOR;

					char c = 0;
					for(uint_fast8_t i = 0; i < size; ++i) {
						c *= 8;
						c += prev_YYCURSOR[i] - '0';
					}

					if(!str_literal.push_back(+c))
						goto oom;
				}
				<EscapeCondition>[xX][0-9a-fA-F]{1,2}	{
					YYSETCONDITION(StringCondition);

					size_t size = YYCURSOR - prev_YYCURSOR;

					char c = 0, j;

					for(uint_fast8_t i = 1; i < size; ++i) {
						c *= 16;

						j = prev_YYCURSOR[i];
						if((j >= '0') && (j <= '9'))
							c += prev_YYCURSOR[i] - '0';
						else if((j >= 'a') && (j <= 'f'))
							c += prev_YYCURSOR[i] - 'a';
						else if((j >= 'A') && (j <= 'F'))
							c += prev_YYCURSOR[i] - 'A';
					}

					if(!str_literal.push_back(+c))
						goto oom;
				}
				<EscapeCondition>$	{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return with_oom_error_if_alloc_failed(
						LexicalError::alloc(allocator, SourcePosition { module_node, (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), pos })
					);
				}
				<EscapeCondition>[^]{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return with_oom_error_if_alloc_failed(
						LexicalError::alloc(allocator, SourcePosition { module_node, (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), pos })
					);
				}

				<CommentCondition>"*"[/]	{ YYSETCONDITION(InitialCondition); break; }
				<CommentCondition>[^]		{ continue; }
				<CommentCondition>$	{
					size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();
					std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

					size_t prev_YYCURSOR_index = prev_YYCURSOR - src.data();
					auto prev_YYCURSOR_pos = src.find_last_of('\n', prev_YYCURSOR_index);
					if(prev_YYCURSOR_pos == std::string::npos)
						prev_YYCURSOR_pos = 0;
					prev_YYCURSOR_pos = prev_YYCURSOR_index - prev_YYCURSOR_pos;

					size_t YYCURSOR_index = YYCURSOR - src.data();
					auto YYCURSOR_pos = src.find_last_of('\n', YYCURSOR_index);
					if(YYCURSOR_pos == std::string::npos)
						YYCURSOR_pos = 0;
					YYCURSOR_pos = YYCURSOR_index - YYCURSOR_pos;

					return with_oom_error_if_alloc_failed(
						LexicalError::alloc(allocator, SourcePosition { module_node, (size_t)std::count(strToBegin.begin(), strToBegin.end(), '\n'), pos })
					);
				}

				<LineCommentCondition>"\n"	{ YYSETCONDITION(InitialCondition); break; }
				<LineCommentCondition>$		{ YYSETCONDITION(InitialCondition); break; }
				<LineCommentCondition>[^]	{ continue; }
			*/
		}

		size_t begin_index = prev_YYCURSOR - src.data(), endIndex = YYCURSOR - src.data();

		std::string_view str_to_begin = src.substr(0, begin_index), str_to_end = src.substr(0, endIndex);

		token.source_text = std::string_view(prev_YYCURSOR, YYCURSOR - prev_YYCURSOR);

		size_t idxLastBeginNewline = src.find_last_of('\n', begin_index),
			   idxLastEndNewline = src.find_last_of('\n', endIndex);

		token.source_location.module_node = module_node;
		token.source_location.begin_position = { (size_t)std::count(str_to_begin.begin(), str_to_begin.end(), '\n'),
			(idxLastBeginNewline == std::string_view::npos
					? begin_index
					: begin_index - idxLastBeginNewline - 1) };
		token.source_location.end_position = { (size_t)std::count(str_to_end.begin(), str_to_end.end(), '\n'),
			(idxLastEndNewline == std::string_view::npos
					? endIndex
					: endIndex - idxLastEndNewline) };
		if (!token_list_out.push_back(std::move(token)))
			goto oom;

		token = Token{};

		prev_YYCURSOR = YYCURSOR;
	}

end: {
	SourceLocation end_location = token.source_location;

	token = Token{};
	token.token_id = TokenId::End;
	token.source_location = end_location;

	if (!token_list_out.push_back(std::move(token)))
		goto oom;
}

	return {};

oom:
	return OutOfMemoryError::alloc();
}
