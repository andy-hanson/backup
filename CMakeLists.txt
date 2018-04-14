cmake_minimum_required(VERSION 3.10)
project(oohoo)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_COMPILER "/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++")
# Must have -Wno-used-but-marked-unused because of https://youtrack.jetbrains.com/issue/CPP-2151
set(CMAKE_CXX_FLAGS "-pedantic -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-used-but-marked-unused -Werror")

add_executable(oohoo
	src/main.cpp

	src/diag/diag.h
	src/diag/parse_diag.h

	src/emit/emit.cpp
	src/emit/emit.h
	src/emit/mangle.cpp
	src/emit/mangle.h
	src/emit/Writer.cpp
	src/emit/Writer.h

	src/model/model.cpp
	src/model/model.h

	src/parse/check/convert_type.h
	src/parse/check/check_call.cpp
	src/parse/check/check_call.h
	src/parse/check/check_common.h
	src/parse/check/check_function_body.cpp
	src/parse/check/check_function_body.h
	src/parse/check/parse_expr.cpp
	src/parse/check/parse_expr.h
	src/parse/Lexer.cpp
	src/parse/Lexer.h
	src/parse/parser.cpp
	src/parse/parser.h

	src/parse/type_utils.cpp
	src/parse/type_utils.h

	src/util/Alloc.h
	src/util/collection_util.h
	src/util/Map.h
	src/util/MaxSizeVector.h
	src/util/Option.h
	src/util/ptr.h
	src/util/StringSlice.cpp
	src/util/StringSlice.h
	src/util/util.h src/parse/check/check_expr.h src/parse/check/check_expr.cpp src/parse/check/expr_ast.h)
