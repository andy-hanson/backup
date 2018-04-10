cmake_minimum_required(VERSION 3.10)
project(oohoo)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_COMPILER "/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++")
# Must have -Wno-used-but-marked-unused because of https://youtrack.jetbrains.com/issue/CPP-2151
set(CMAKE_CXX_FLAGS "-pedantic -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-used-but-marked-unused -Werror")

add_executable(oohoo
	main.cpp
	parse/check_function_body.cpp
	parse/check_function_body.h
	emit.cpp
	emit.h
	parse/Lexer.cpp
	parse/Lexer.h
	util/MaxSizeVector.h
	model/model.cpp
	model/model.h
	util/Option.h
	parse/parse_expr.cpp
	parse/parse_expr.h
	parse/parser_internal.h
	parse/parser.cpp
	parse/parser.h
	util/ptr.h
	util/StringSlice.cpp
	util/StringSlice.h
	parse/type_utils.cpp
	parse/type_utils.h
	util/UnorderedCollection.h
	util/util.h
	util/collection_util.h)
