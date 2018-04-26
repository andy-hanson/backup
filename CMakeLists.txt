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

	src/cmd.cpp
	src/cmd.h

	src/compile/check/BuiltinTypes.h
	src/compile/check/check.cpp
	src/compile/check/check.h
	src/compile/check/check_call.cpp
	src/compile/check/check_call.h
	src/compile/check/check_common.h
	src/compile/check/check_effects.cpp
	src/compile/check/check_effects.h
	src/compile/check/check_expr.cpp
	src/compile/check/check_expr.h
	src/compile/check/convert_type.cpp
	src/compile/check/convert_type.h
	src/compile/check/type_utils.cpp
	src/compile/check/type_utils.h
	src/compile/diag/diag.h
	src/compile/diag/parse_diag.h
	src/compile/compile.h
	src/compile/compile.cpp

	src/emit/concrete_fun.cpp
	src/emit/concrete_fun.h
	src/emit/emit.cpp
	src/emit/emit.h
	src/emit/emit_body.cpp
	src/emit/emit_body.h
	src/emit/emit_type.h
	src/emit/function_names.h
	src/emit/mangle.cpp
	src/emit/mangle.h
	src/emit/Names.cpp
	src/emit/Names.h

	src/compile/model/model.cpp
	src/compile/model/model.h
	src/compile/model/effect.cpp
	src/compile/model/effect.h
	src/compile/model/expr.h

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

	src/test/test.cpp
	src/test/test.h

	src/util/Alloc.h
	src/util/collection_util.h
	src/util/Map.h
	src/util/MaxSizeVector.h
	src/util/Option.h
	src/util/ptr.h
	src/util/StringSlice.cpp
	src/util/StringSlice.h
	src/util/hash_util.h
	src/util/io.cpp
	src/util/io.h
	src/util/int.cpp
	src/util/int.h
	src/util/Vec.h
	src/util/Writer.cpp
	src/util/Writer.h

	src/cmd.h
	src/cmd.cpp)
