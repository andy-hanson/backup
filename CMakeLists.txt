cmake_minimum_required(VERSION 3.10)
project(oohoo)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_COMPILER "/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++")
# Must have -Wno-used-but-marked-unused because of https://youtrack.jetbrains.com/issue/CPP-2151
# -Wno-missing-no-return because I often write `f() { throw "todo"; }`
# -stdlib=libstdc++ supposedly improves debugging (https://blog.jetbrains.com/clion/2015/05/debug-clion/)
set(CMAKE_CXX_FLAGS "-pedantic -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-used-but-marked-unused -Wno-missing-noreturn -Werror -stdlib=libstdc++")

add_executable(oohoo
	src/main.cpp

	src/compile/check/BuiltinTypes.h
	src/compile/check/check.cpp
	src/compile/check/check.h
	src/compile/check/check_call.cpp
	src/compile/check/check_call.h
	src/compile/check/check_common.h
	src/compile/check/check_expr.cpp
	src/compile/check/check_expr.h
	src/compile/check/convert_type.cpp
	src/compile/check/convert_type.h
	src/compile/check/type_utils.cpp
	src/compile/check/type_utils.h

	src/compile/diag/diag.h
	src/compile/diag/parse_diag.h

	src/compile/emit/concrete_fun.cpp
	src/compile/emit/concrete_fun.h
	src/compile/emit/emit.cpp
	src/compile/emit/emit.h
	src/compile/emit/emit_body.cpp
	src/compile/emit/emit_body.h
	src/compile/emit/emit_type.h
	src/compile/emit/function_names.h
	src/compile/emit/mangle.cpp
	src/compile/emit/mangle.h
	src/compile/emit/Names.cpp
	src/compile/emit/Names.h
	src/compile/emit/Writer.cpp
	src/compile/emit/Writer.h

	src/compile/model/model.cpp
	src/compile/model/model.h
	src/compile/model/expr.h
	src/compile/model/Identifier.h

	src/compile/parse/ast.h
	src/compile/parse/expr_ast.h
	src/compile/parse/Lexer.cpp
	src/compile/parse/Lexer.h
	src/compile/parse/parse_expr.cpp
	src/compile/parse/parse_expr.h
	src/compile/parse/parse_type.cpp
	src/compile/parse/parse_type.h
	src/compile/parse/parser.cpp
	src/compile/parse/parser.h
	src/compile/parse/type_ast.h

	src/util/Alloc.h
	src/util/collection_util.h
	src/util/Map.h
	src/util/MaxSizeVector.h
	src/util/Option.h
	src/util/ptr.h
	src/util/StringSlice.cpp
	src/util/StringSlice.h
	src/util/util.h
	src/util/hash_util.h)
