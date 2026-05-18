#include "parser.h"

using namespace truk;

#define TRUK_CO_RETURN_IF_CO_PARSE_ERROR(expr)                                    \
	if (InternalExceptionPointer _ = co_await ((expr)(&this->coro_scheduler)); _) \
		co_return _;                                                              \
	else

TRUK_API InternalExceptionPointer ParseCoroutine::resume(ParseCoroutineScheduler *scheduler) {
	if (!coro_handle)
		return OutOfMemoryError::alloc();

	coro_handle.resume();

	while (scheduler->task_list.size()) {
		auto h = scheduler->task_list.back();
		scheduler->task_list.pop_back();
		if (!h.done())
			h.resume();
		if (coro_handle.promise().result)
			return std::move(coro_handle.promise().result);
	}

	if (coro_handle.promise().result)
		return std::move(coro_handle.promise().result);
	if (!coro_handle.done())
		std::terminate();

	return {};
}

TRUK_API ParseCoroutine::Awaitable::Awaitable(
	ParseCoroutine &co,
	ParseCoroutineScheduler *scheduler,
	Handle handle)
	: co(co),
	  scheduler(scheduler),
	  handle(std::move(handle)) {
}

TRUK_API bool ParseCoroutine::Awaitable::await_ready() {
	return false;
}

TRUK_API void ParseCoroutine::Awaitable::await_suspend(Handle h) {
	if (!scheduler->task_list.push_back(std::move(h))) {
		co.coro_handle.promise().result = OutOfMemoryError::alloc();
		return;
	}
	if (!scheduler->task_list.push_back(Handle(handle))) {
		co.coro_handle.promise().result = OutOfMemoryError::alloc();
		return;
	}
}

TRUK_API InternalExceptionPointer ParseCoroutine::Awaitable::await_resume() {
	if (handle) {
		if (handle.promise().result)
			return std::move(handle.promise().result);
		return {};
	}
	return OutOfMemoryError::alloc();
}

TRUK_API ParseCoroutine::Awaitable ParseCoroutine::operator()(ParseCoroutineScheduler *scheduler) {
	return Awaitable(*this, scheduler, coro_handle);
}

TRUK_API ParseCoroutineScheduler::ParseCoroutineScheduler(peff::Alloc *allocator) : task_list(allocator) {
}

TRUK_API Parser::Parser(Runtime *runtime, TokenList &token_list)
	: runtime(runtime),
	  token_list(token_list),
	  coro_scheduler(runtime->get_global_allocator()),
	  errors(runtime->get_global_allocator()),
	  warnings(runtime->get_global_allocator()) {
}

TRUK_API Token *Parser::next_token(bool keep_new_line, bool keep_whitespace, bool keep_comment) {
	size_t &i = parse_context.idx_current_token;

	while (i < token_list.size()) {
		Token *current_token = &token_list.at(i);

		switch (current_token->token_id) {
			case TokenId::NewLine:
				if (keep_new_line) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return current_token;
				}
				break;
			case TokenId::Whitespace:
				if (keep_whitespace) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return current_token;
				}
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keep_comment) {
					parse_context.idx_prev_token = parse_context.idx_current_token;
					++i;
					return current_token;
				}
				break;
			default:
				parse_context.idx_prev_token = parse_context.idx_current_token;
				++i;
				return current_token;
		}

		++i;
	}

	return &token_list.back();
}

TRUK_API Token *Parser::peek_token(bool keep_new_line, bool keep_whitespace, bool keep_comment) {
	size_t i = parse_context.idx_current_token;

	while (i < token_list.size()) {
		Token *current_token = &token_list.at(i);

		switch (current_token->token_id) {
			case TokenId::NewLine:
				if (keep_new_line)
					return current_token;
				break;
			case TokenId::Whitespace:
				if (keep_whitespace)
					return current_token;
				break;
			case TokenId::LineComment:
			case TokenId::BlockComment:
			case TokenId::DocumentationComment:
				if (keep_comment)
					return current_token;
				break;
			default:
				return current_token;
		}

		++i;
	}

	return &token_list.back();
}

template <typename T>
PEFF_FORCEINLINE InternalExceptionPointer _parse_int(Parser *parser, Token *token, bool is_negative, const std::string_view &body_view, T &data_out) {
	InternalExceptionPointer syntax_error;

	bool overflow_warned = false;
	char c;
	T data = 0;
	size_t i = 0;

	switch (std::get<IntTokenType>(token->exdata)) {
		case IntTokenType::Decimal:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 10 > data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 10;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 10 < data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 10;
					data += c - '0';
				}
				++i;
			}
			break;
		case IntTokenType::Hexadecimal:
			while (i < body_view.size()) {
				char c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 16 > data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 16;
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							data -= c - '0';
							break;
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							data -= c - 'a' + 10;
							break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							data -= c - 'A' + 10;
							break;
					}
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 10 < data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 16;
					switch (c) {
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
							data += c - '0';
							break;
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
							data += c - 'a' + 10;
							break;
						case 'A':
						case 'B':
						case 'C':
						case 'D':
						case 'E':
						case 'F':
							data += c - 'A' + 10;
							break;
					}
				}
				++i;
			}
			break;
		case IntTokenType::Octal:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && (std::numeric_limits<T>::min() / 8 > data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 8;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && (std::numeric_limits<T>::max() / 8 < data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data *= 8;
					data += c - '0';
				}
				++i;
			}
			break;
		case IntTokenType::Binary:
			while (i < body_view.size()) {
				c = body_view.at(i);
				if (is_negative) {
					if ((!overflow_warned) && ((std::numeric_limits<T>::min() >> 1) > data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data <<= 1;
					data -= c - '0';
				} else {
					if ((!overflow_warned) && ((std::numeric_limits<T>::max() >> 1) < data)) {
						if ((syntax_error = parser->push_literal_overflowed_error(token)))
							return syntax_error;
						overflow_warned = true;
					}
					data <<= 1;
					data += c - '0';
				}
				++i;
			}
			break;
		default:
			std::terminate();
	}

	data_out = data;

	return {};
}

ParseCoroutine Parser::parse_list_element(HostRefHolder &host_ref_holder, Value &value_out) {
	InternalExceptionPointer syntax_error;
	Token *prefix_token = peek_token();

	switch (prefix_token->token_id) {
		case TokenId::IntLiteral: {
			next_token();

			int32_t data = 0;
			bool is_negative = false;

			if (prefix_token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0));

			if ((syntax_error = _parse_int<int32_t>(this, prefix_token, is_negative, body_view, data)))
				co_return syntax_error;

			value_out = data;
			break;
		}
		case TokenId::LongLiteral: {
			next_token();

			int64_t data = 0;
			bool is_negative = false;

			if (prefix_token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("L") - 1));

			if ((syntax_error = _parse_int<int64_t>(this, prefix_token, is_negative, body_view, data)))
				co_return syntax_error;

			value_out = data;
			break;
		}
		case TokenId::UIntLiteral: {
			next_token();

			uint32_t data = 0;
			bool is_negative = false;

			if (prefix_token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("U") - 1));

			if ((syntax_error = _parse_int<uint32_t>(this, prefix_token, is_negative, body_view, data)))
				co_return syntax_error;

			value_out = data;
			break;
		}
		case TokenId::ULongLiteral: {
			next_token();

			uint64_t data = 0;
			bool is_negative = false;

			if (prefix_token->source_text.at(0) == '-')
				is_negative = true;

			std::string_view body_view = prefix_token->source_text.substr(is_negative ? 1 : 0, prefix_token->source_text.size() - (is_negative ? 1 : 0) - (sizeof("UL") - 1));

			if ((syntax_error = _parse_int<uint64_t>(this, prefix_token, is_negative, body_view, data)))
				co_return syntax_error;

			value_out = data;
			break;
		}
		case TokenId::StringLiteral: {
			next_token();

			const peff::String &data = std::move(std::get<StringTokenExData>(prefix_token->exdata).str);

			HostObjectRef<StringObject> obj;

			if (!(obj = alloc_runtime_managed_object<StringObject>(runtime->get_global_allocator(), runtime->get_global_allocator())))
				co_return OutOfMemoryError::alloc();

			if (!host_ref_holder.add_object(obj.get()))
				co_return OutOfMemoryError::alloc();

			if (!(obj->data.build(data)))
				co_return OutOfMemoryError::alloc();

			value_out = obj.get();

			break;
		}
		case TokenId::FloatLiteral: {
			next_token();

			value_out = strtof(prefix_token->source_text.data(), nullptr);
			break;
		}
		case TokenId::DoubleLiteral: {
			next_token();

			value_out = strtod(prefix_token->source_text.data(), nullptr);
			break;
		}
		case TokenId::Id: {
			HostObjectRef<SymbolObject> obj;

			if (!(obj = alloc_runtime_managed_object<SymbolObject>(runtime->get_global_allocator(), runtime->get_global_allocator())))
				co_return OutOfMemoryError::alloc();

			peff::String first_entry(runtime->get_global_allocator());

			if (!first_entry.build(prefix_token->source_text))
				co_return OutOfMemoryError::alloc();

			if (!obj->name_entries.push_back(std::move(first_entry)))
				co_return OutOfMemoryError::alloc();

			Token *cur_token;
			while (true) {
				if ((cur_token = peek_token())->token_id != TokenId::ScopeOp)
					break;

				peff::String entry(runtime->get_global_allocator());

				if (!first_entry.build(cur_token->source_text))
					co_return OutOfMemoryError::alloc();

				if (!obj->name_entries.push_back(std::move(entry)))
					co_return OutOfMemoryError::alloc();
			}

			if (!host_ref_holder.add_object(obj.get()))
				co_return OutOfMemoryError::alloc();

			value_out = obj.get();

			break;
		}
		case TokenId::LParenthese: {
			HostObjectRef<ListObject> obj;

			TRUK_CO_RETURN_IF_CO_PARSE_ERROR(parse_list_body(host_ref_holder, obj));

			if (!host_ref_holder.add_object(obj.get()))
				co_return OutOfMemoryError::alloc();

			value_out = obj.get();

			break;
		}
		default:
			co_return SyntaxError::alloc(runtime->get_global_allocator(),
				SourceLocation{
					parse_context.mod,
					prefix_token->token_index },
				SyntaxErrorKind::UnexpectedToken);
	}

	co_return {};
}

ParseCoroutine Parser::parse_list_body(HostRefHolder &host_ref_holder, HostObjectRef<ListObject> &list_out) {
	if (auto e = expect_token(next_token(), TokenId::LParenthese))
		co_return e;

	if (!(list_out = alloc_runtime_managed_object<ListObject>(runtime->get_global_allocator(), runtime->get_global_allocator())))
		co_return OutOfMemoryError::alloc();

	while (true) {
		auto t = peek_token();

		if (t->token_id == TokenId::RParenthese)
			break;

		Value element;

		TRUK_CO_RETURN_IF_CO_PARSE_ERROR(parse_list_element(host_ref_holder, element));

		if (!list_out->elements.push_back(std::move(element)))
			co_return OutOfMemoryError::alloc();
	}

	if (auto e = expect_token(next_token(), TokenId::LParenthese))
		co_return e;

	co_return {};
}

ParseCoroutine Parser::parse_program(HostRefHolder &host_ref_holder, HostObjectRef<ListObject> &list_out) {
	if (!(list_out = alloc_runtime_managed_object<ListObject>(runtime->get_global_allocator(), runtime->get_global_allocator())))
		co_return OutOfMemoryError::alloc();

	while (true) {
		auto t = peek_token();

		if (t->token_id == TokenId::End)
			break;

		Value element;

		TRUK_CO_RETURN_IF_CO_PARSE_ERROR(parse_list_element(host_ref_holder, element));

		if (!list_out->elements.push_back(std::move(element)))
			co_return OutOfMemoryError::alloc();
	}

	co_return {};
}

InternalExceptionPointer Parser::parse(Runtime *runtime, HostObjectRef<ListObject> &list_out) {
	HostRefHolder holder(runtime->get_global_allocator());
	return parse_program(holder, list_out).resume(&this->coro_scheduler);
}
