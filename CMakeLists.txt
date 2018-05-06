cmake_minimum_required(VERSION 3.10)
project(oohoo)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_COMPILER "/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++")
# -stdlib=libstdc++ supposedly improves debugging (https://blog.jetbrains.com/clion/2015/05/debug-clion/)
set(CMAKE_CXX_FLAGS "-pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Werror -stdlib=libstdc++")

add_executable(oohoo
	src/main.cpp

	src/compile/check/BuiltinTypes.h
	src/compile/check/check.cpp
	src/compile/check/check.h
	src/compile/check/CheckCtx.h
	src/compile/check/check_call.cpp
	src/compile/check/check_call.h
	src/compile/check/check_expr_call_common.cpp
	src/compile/check/check_expr_call_common.h
	src/compile/check/check_effects.cpp
	src/compile/check/check_effects.h
	src/compile/check/check_expr.cpp
	src/compile/check/check_expr.h
	src/compile/check/check_param_or_local_shadows_fun.cpp
	src/compile/check/check_param_or_local_shadows_fun.h
	src/compile/check/convert_type.cpp
	src/compile/check/convert_type.h
	src/compile/check/Candidate.cpp
	src/compile/check/Candidate.h

	src/compile/diag/diag.cpp
	src/compile/diag/diag.h
	src/compile/diag/parse_diag.cpp
	src/compile/diag/parse_diag.h
	src/compile/diag/LineAndColumnGetter.cpp
	src/compile/diag/LineAndColumnGetter.h
	src/compile/diag/SourceRange.h

	src/compile/model/model.cpp
	src/compile/model/model.h
	src/compile/model/effect.cpp
	src/compile/model/effect.h
	src/compile/model/expr.cpp
	src/compile/model/expr.h

	src/compile/parse/ast.cpp
	src/compile/parse/ast.h
	src/compile/parse/expr_ast.cpp
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

	src/compile/compile.h
	src/compile/compile.cpp

	src/emit/emit_type.cpp
	src/emit/ConcreteFun.cpp
	src/emit/ConcreteFun.h
	src/emit/emit.cpp
	src/emit/emit.h
	src/emit/emit_body.cpp
	src/emit/emit_body.h
	src/emit/emit_comment.cpp
	src/emit/emit_comment.h
	src/emit/emit_type.h
	src/emit/mangle.cpp
	src/emit/mangle.h
	src/emit/Names.cpp
	src/emit/Names.h
	src/emit/substitute_type_arguments.cpp
	src/emit/substitute_type_arguments.h

	src/host/DocumentProvider.cpp
	src/host/DocumentProvider.h

	src/test/test.cpp
	src/test/test.h
	src/test/test_single.cpp
	src/test/test_single.h
	src/test/TestMode.h
	src/test/TestFailure.h

	src/util/store/Arena.cpp
	src/util/store/Arena.h
	src/util/store/ArenaArrayBuilders.h
	src/util/store/ArenaString.cpp
	src/util/store/ArenaString.h
	src/util/store/BlockedList.h
	src/util/store/collection_util.h
	src/util/store/KeyValuePair.h
	src/util/store/List.h
	src/util/store/ListBuilder.h
	src/util/store/ListNode.h
	src/util/store/Map.h
	src/util/store/MaxSizeMap.h
	src/util/store/MaxSizeVector.h
	src/util/store/MaxSizeSet.h
	src/util/store/MaxSizeString.cpp
	src/util/store/MaxSizeString.h
	src/util/store/MultiMap.h
	src/util/store/NonEmptyList.h
	src/util/store/SmallMap.h
	src/util/store/Slice.h
	src/util/store/slice_util.h
	src/util/store/StringSlice.cpp
	src/util/store/StringSlice.h

	src/util/assert.h
	src/util/Option.h
	src/util/Ref.h
	src/util/hash_util.h
	src/util/io.cpp
	src/util/io.h
	src/util/int.cpp
	src/util/int.h
	src/util/rlimit.cpp
	src/util/rlimit.h
	src/util/Path.cpp
	src/util/Path.h
	src/util/unique_ptr.h
	src/util/Writer.cpp
	src/util/Writer.h

	src/clang.cpp
	src/clang.h src/util/PathCache.h src/util/PathCache.cpp src/util/PathImpl.h)
