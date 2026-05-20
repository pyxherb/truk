#include "parser.h"
#include "utils/file.h"
#include <optxx/optxx.h>

#define _print_error(fmt, ...) fprintf(stderr, "Error: " fmt, ##__VA_ARGS__)

std::string_view script_filename = "Trukfile", target_name, input_base_dir, output_base_dir;

struct MatchUserData {
	peff::DynArray<std::string_view> *extra_include_items;
};

const optxx::ArglessOptionMap g_argless_opts = {

};

const optxx::SingleArgOptionMap g_single_arg_opts = {
	{ "-t", [](const optxx::OptionMatchContext &match_context, const std::string_view &option, const std::string_view &arg) -> int {
		 target_name = arg;

		 return 0;
	 } },
	{ "-S", [](const optxx::OptionMatchContext &match_context, const std::string_view &option, const std::string_view &arg) -> int {
		 input_base_dir = arg;

		 return 0;
	 } },
	{ "-o", [](const optxx::OptionMatchContext &match_context, const std::string_view &option, const std::string_view &arg) -> int {
		 output_base_dir = arg;

		 return 0;
	 } },
	{ "-I", [](const optxx::OptionMatchContext &match_context, const std::string_view &option, const std::string_view &arg) -> int {
		 if (!((MatchUserData *)match_context.get_user_data())->extra_include_items->push_back(std::string_view(arg))) {
			 _print_error("Out of memory");
			 return ENOMEM;
		 }

		 return 0;
	 } }
};

const optxx::CustomOptionMap g_custom_opts = {

};

int main(int argc, char *argv[]) {
	peff::DynArray<std::string_view> extra_include_dirs(peff::default_allocator());

	optxx::CompiledOptionMap compiled_option_map(
		peff::default_allocator(),
		[](optxx::OptionMatchContext &match_context, const std::string_view &option_str) -> int {
			_print_error("Unknown option %s", option_str.data());
			return EINVAL;
		},
		[](const optxx::OptionMatchContext &match_context, const optxx::SingleArgOption &option) {
			_print_error("Option %s requires one more argument", option.name);
		});
	if (!optxx::build_option_map(compiled_option_map, g_argless_opts, g_single_arg_opts, g_custom_opts)) {
		_print_error("Out of memory");
		return ENOMEM;
	}

	peff::DynArray<std::string_view> extra_include_items(peff::default_allocator());

	MatchUserData match_user_data = {
		&extra_include_items
	};
	if (int result = optxx::match_args(compiled_option_map, argc, argv, &match_user_data); result)
		return result;

	if (script_filename.empty()) {
		_print_error("Build script file name must not be empty");
		return EINVAL;
	}

	FILE *fp;
	{
		peff::String input_script_full_path(peff::default_allocator());

		if (!input_script_full_path.build(input_base_dir)) {
			_print_error("Out of memory");
			return ENOMEM;
		}

		if (!input_script_full_path.push_back('/')) {
			_print_error("Out of memory");
			return ENOMEM;
		}

		if (!input_script_full_path.append(script_filename)) {
			_print_error("Out of memory");
			return ENOMEM;
		}

		if (!(fp = fopen(input_script_full_path.data(), "rb"))) {
			_print_error("Error opening the build script file: %s", input_script_full_path.data());
			return ENOMEM;
		}
	}

	truk::utils::File script_file(fp);

	fseek(fp, SEEK_END, 0);
	long size = ftell(fp);
	if (size < 0) {
		_print_error("I/O error");
	}
	fseek(fp, SEEK_SET, 0);

	{
		char *script_content = (char *)peff::default_allocator()->alloc((size_t)size + 1, alignof(char));

		if (!script_content) {
			_print_error("Out of memory");
			return ENOMEM;
		}

		script_content[size] = '\0';

		peff::Deferred release_script_content([script_content, size]() noexcept {
			peff::default_allocator()->release(script_content, (size_t)size + 1, alignof(char));
		});

		truk::Runtime runtime(peff::default_allocator());

		truk::HostObjectRef<truk::ModuleObject> root_mod;
		if (!(root_mod = truk::alloc_managed_object<truk::ModuleObject>(runtime.get_global_allocator(), runtime.get_global_allocator()))) {
			_print_error("Out of memory");
			return ENOMEM;
		}

		truk::TokenList token_list(peff::default_allocator());
		if (auto e = truk::lex(root_mod.get(), std::string_view(script_content, (size_t)size), runtime.get_global_allocator(), token_list); e)
			std::terminate();

		truk::Parser parser(&runtime, token_list);

		truk::HostObjectRef<truk::ListObject> list;
		if (auto e = parser.parse(peff::default_allocator(), list); e)
			std::terminate();

		truk::HostObjectRef<truk::ScopeObject> scope;
		if (!(scope = truk::alloc_managed_object<truk::ScopeObject>(runtime.get_global_allocator(), runtime.get_global_allocator()))) {
			_print_error("Out of memory");
			return ENOMEM;
		}

		if (auto e = runtime.exec(scope.get(), list.get()); e)
			std::terminate();
	}

	return 0;
}
