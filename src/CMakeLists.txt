cmake_minimum_required(VERSION 3.10)
project(oohoo)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_COMPILER "/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++")
# -stdlib=libstdc++ supposedly improves debugging (https://blog.jetbrains.com/clion/2015/05/debug-clion/)
set(CMAKE_CXX_FLAGS "-pedantic -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Werror -stdlib=libstdc++")

add_executable(oohoo
	./main.cpp

	./compile/check/check.cpp
	./compile/check/check.h
	./compile/check/CheckCtx.h
	./compile/check/check_call.cpp
	./compile/check/check_call.h
	./compile/check/check_expr_call_common.cpp
	./compile/check/check_expr_call_common.h
	./compile/check/check_expr.cpp
	./compile/check/check_expr.h
	./compile/check/check_lifetime.cpp
	./compile/check/check_lifetime.h
	./compile/check/check_param_or_local_shadows_fun.cpp
	./compile/check/check_param_or_local_shadows_fun.h
	./compile/check/convert_type.cpp
	./compile/check/convert_type.h
	./compile/check/Candidate.cpp
	./compile/check/Candidate.h

	./compile/diag/diag.cpp
	./compile/diag/diag.h
	./compile/diag/parse_diag.cpp
	./compile/diag/parse_diag.h
	./compile/diag/LineAndColumnGetter.cpp
	./compile/diag/LineAndColumnGetter.h
	./compile/diag/SourceRange.h

	./compile/model/BuiltinTypes.h
	./compile/model/model.cpp
	./compile/model/model.h
	./compile/model/effect.cpp
	./compile/model/effect.h
	./compile/model/expr.cpp
	./compile/model/expr.h
	./compile/model/type_of_expr.cpp
	./compile/model/type_of_expr.h
	./compile/model/types_equal_ignore_lifetime.cpp
	./compile/model/types_equal_ignore_lifetime.h

	./compile/parse/ast.cpp
	./compile/parse/ast.h
	./compile/parse/expr_ast.cpp
	./compile/parse/expr_ast.h
	./compile/parse/Lexer.cpp
	./compile/parse/Lexer.h
	./compile/parse/parse_expr.cpp
	./compile/parse/parse_expr.h
	./compile/parse/parse_type.cpp
	./compile/parse/parse_type.h
	./compile/parse/parser.cpp
	./compile/parse/parser.h
	./compile/parse/type_ast.h

	./compile/compile.h
	./compile/compile.cpp

	./emit/emit_type.cpp
	./emit/ConcreteFun.cpp
	./emit/ConcreteFun.h
	./emit/emit.cpp
	./emit/emit.h
	./emit/emit_body.cpp
	./emit/emit_body.h
	./emit/emit_comment.cpp
	./emit/emit_comment.h
	./emit/emit_type.h
	./emit/mangle.cpp
	./emit/mangle.h
	./emit/Names.cpp
	./emit/Names.h
	./emit/substitute_type_arguments.cpp
	./emit/substitute_type_arguments.h

	./host/DocumentProvider.cpp
	./host/DocumentProvider.h

	./test/test.cpp
	./test/test.h
	./test/test_single.cpp
	./test/test_single.h
	./test/TestMode.h
	./test/TestFailure.h

	./util/store/Arena.cpp
	./util/store/Arena.h
	./util/store/ArenaArrayBuilders.h
	./util/store/ArenaString.cpp
	./util/store/ArenaString.h
	./util/store/BlockedList.h
	./util/store/collection_util.h
	./util/store/KeyValuePair.h
	./util/store/List.h
	./util/store/ListBuilder.h
	./util/store/ListNode.h
	./util/store/Map.h
	./util/store/MaxSizeMap.h
	./util/store/MaxSizeVector.h
	./util/store/MaxSizeSet.h
	./util/store/MaxSizeString.cpp
	./util/store/MaxSizeString.h
	./util/store/MultiMap.h
	./util/store/NonEmptyList.h
	./util/store/NonEmptyList_utils.h
	./util/PathCache.h
	./util/PathCache.cpp
	./util/PathImpl.h
	./util/store/SmallMap.h
	./util/store/Slice.h
	./util/store/slice_util.h
	./util/store/StringSlice.cpp
	./util/store/StringSlice.h

	./util/assert.h
	./util/Option.h
	./util/Ref.h
	./util/hash_util.h
	./util/io.cpp
	./util/io.h
	./util/int.cpp
	./util/int.h
	./util/rlimit.cpp
	./util/rlimit.h
	./util/Path.cpp
	./util/Path.h
	./util/unique_ptr.h
	./util/Writer.cpp
	./util/Writer.h

	./clang.cpp
	./clang.h)
