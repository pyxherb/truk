find_package(peff REQUIRED)
find_package(optxx REQUIRED)
find_package(re2c REQUIRED)

add_re2c_target(slkc_lexer lexer.in.cpp)

file(GLOB SRC *.h *.c *.hh *.cc)

add_executable(truk ${SRC})
target_link_libraries(truk PRIVATE wandjson_static optxx_static peff_advutils_static pbos_common)
set_target_properties(truk PROPERTIES CXX_STANDARD 20)

target_compile_definitions(truk PRIVATE -D_CRT_SECURE_NO_WARNINGS)

install(TARGETS truk)
