#ifndef _TRUK_PARSER_H_
#define _TRUK_PARSER_H_

#include "runtime.h"
#include <coroutine>

namespace truk {
	class ParseCoroutineScheduler;

	struct ParseCoroutine {
		struct promise_type;

		using Handle = std::coroutine_handle<promise_type>;

		struct promise_type {
			InternalExceptionPointer result;

			PEFF_FORCEINLINE static ParseCoroutine get_return_object_on_allocation_failure() noexcept {
				return ParseCoroutine({});
			}

			PEFF_FORCEINLINE ParseCoroutine get_return_object() noexcept {
				return ParseCoroutine(Handle::from_promise(*this));
			}

			PEFF_FORCEINLINE std::suspend_always initial_suspend() noexcept {
				return {};
			}

			PEFF_FORCEINLINE std::suspend_always final_suspend() noexcept {
				return {};
			}

			PEFF_FORCEINLINE std::suspend_always yield_value(InternalExceptionPointer &&value) noexcept {
				result = std::move(value);
				return {};
			}

			PEFF_FORCEINLINE void return_value(InternalExceptionPointer &&value) noexcept {
				result = std::move(value);
			}

			PEFF_FORCEINLINE void unhandled_exception() { std::terminate(); }

			struct AllocatorInfo {
				peff::Alloc *allocator;
#if PEFF_ENABLE_RCOBJ_DEBUGGING
				size_t c;
#endif
			};

			template <typename First, typename... Args>
			PEFF_FORCEINLINE static void *operator new(size_t size, First &&first, peff::Alloc *allocator, Args &&...args) noexcept {
				char *p = (char *)allocator->alloc(size + sizeof(AllocatorInfo), alignof(std::max_align_t));

				if (!p)
					return nullptr;

				memset(p, 0, size);

#if PEFF_ENABLE_RCOBJ_DEBUGGING
				auto ref_count = peff::acquire_global_rcobj_ptr_counter();
#endif

				AllocatorInfo allocator_info = {
					allocator
#if PEFF_ENABLE_RCOBJ_DEBUGGING
					,
					ref_count
#endif
				};

				memcpy(p + size, &allocator_info, sizeof(allocator_info));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
				allocator->inc_ref(ref_count);
#endif

				return p;
			}

			PEFF_FORCEINLINE static void operator delete(void *p, size_t size) noexcept {
				AllocatorInfo allocator_info;

				memcpy(&allocator_info, (char *)p + size, sizeof(allocator_info));

				peff::RcObjectPtr<peff::Alloc> allocator_holder = allocator_info.allocator;

				allocator_holder->release(p, size + sizeof(AllocatorInfo), alignof(std::max_align_t));
#if PEFF_ENABLE_RCOBJ_DEBUGGING
				allocator_holder->dec_ref(allocator_info.c);
#endif
			}
		};

		Handle coro_handle;

		static inline bool recursed = false;

		ParseCoroutine(Handle coro_handle) : coro_handle(coro_handle) {}
		~ParseCoroutine() {
			// assert(!recursed);
			// recursed = true;
			if (coro_handle)
				coro_handle.destroy();
			// recursed = false;
		}

		PEFF_FORCEINLINE bool done() {
			return coro_handle.done();
		}

		TRUK_API InternalExceptionPointer resume(ParseCoroutineScheduler *scheduler);

		struct Awaitable {
			ParseCoroutine &co;
			ParseCoroutineScheduler *scheduler;
			Handle handle;

			TRUK_API Awaitable(ParseCoroutine &co, ParseCoroutineScheduler *scheduler, Handle handle);
			TRUK_API bool await_ready();
			TRUK_API void await_suspend(Handle h);
			[[nodiscard]] TRUK_API InternalExceptionPointer await_resume();
		};

		TRUK_API Awaitable operator()(ParseCoroutineScheduler *scheduler);
	};

	class ParseCoroutineScheduler {
	public:
		peff::DynArray<std::coroutine_handle<ParseCoroutine::promise_type>> task_list;

		TRUK_API ParseCoroutineScheduler(peff::Alloc *allocator);
	};

	class Parser final {
	private:
		struct ParseContext {
			ModuleObject *mod = nullptr;
			size_t idx_prev_token = 0, idx_current_token = 0;
		};
		ParseContext parse_context;

		Runtime *runtime;
		TokenList &token_list;
		ParseCoroutineScheduler coro_scheduler;

		ParseCoroutine parse_list_element(HostRefHolder &host_ref_holder, Value &value_out);
		ParseCoroutine parse_list_body(HostRefHolder &host_ref_holder, HostObjectRef<ListObject> &list_out);
		ParseCoroutine parse_program(HostRefHolder &host_ref_holder, HostObjectRef<ListObject> &list_out);

	public:
		peff::DynArray<InternalExceptionPointer> errors;
		peff::DynArray<InternalExceptionPointer> warnings;

		TRUK_API Parser(Runtime *runtime, TokenList &token_list);

		TRUK_API Token *next_token(bool keep_new_line = false, bool keep_whitespace = false, bool keep_comment = false);
		TRUK_API Token *peek_token(bool keep_new_line = false, bool keep_whitespace = false, bool keep_comment = false);

		[[nodiscard]] PEFF_FORCEINLINE InternalExceptionPointer expect_token(Token *token, TokenId token_id) {
			if (token->token_id != token_id) {
				return SyntaxError::alloc(
					runtime->get_global_allocator(),
					{ parse_context.mod, token->token_index },
					SyntaxErrorKind::UnexpectedToken);
			}

			return {};
		}

		[[nodiscard]] PEFF_FORCEINLINE InternalExceptionPointer expect_token(Token *token) {
			if (token->token_id == TokenId::End) {
				return SyntaxError::alloc(
					runtime->get_global_allocator(),
					{ parse_context.mod, token->token_index },
					SyntaxErrorKind::UnexpectedToken);
			}

			return {};
		}

		PEFF_FORCEINLINE InternalExceptionPointer push_literal_overflowed_error(Token *token) noexcept {
			auto e = SyntaxError::alloc(runtime->get_global_allocator(), SourceLocation{ parse_context.mod, token->token_index }, SyntaxErrorKind::LiteralOverflowed);
			if ((!e) || (!errors.resize(errors.size() + 1)))
				return OutOfMemoryError::alloc();

			errors.back() = e;

			return {};
		}

		InternalExceptionPointer parse(peff::Alloc *misc_allocator, HostObjectRef<ListObject> &list_out);
	};
}

#endif
