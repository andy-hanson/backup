# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/andy/bin/clion-2018.1/bin/cmake/bin/cmake

# The command to remove a file.
RM = /home/andy/bin/clion-2018.1/bin/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/andy/CLionProjects/oohoo

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/andy/CLionProjects/oohoo/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/oohoo.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/oohoo.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/oohoo.dir/flags.make

CMakeFiles/oohoo.dir/src/main.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/main.cpp.o: ../src/main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/oohoo.dir/src/main.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/main.cpp.o -c /home/andy/CLionProjects/oohoo/src/main.cpp

CMakeFiles/oohoo.dir/src/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/main.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/main.cpp > CMakeFiles/oohoo.dir/src/main.cpp.i

CMakeFiles/oohoo.dir/src/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/main.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/main.cpp -o CMakeFiles/oohoo.dir/src/main.cpp.s

CMakeFiles/oohoo.dir/src/main.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/main.cpp.o.requires

CMakeFiles/oohoo.dir/src/main.cpp.o.provides: CMakeFiles/oohoo.dir/src/main.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/main.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/main.cpp.o.provides

CMakeFiles/oohoo.dir/src/main.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/main.cpp.o


CMakeFiles/oohoo.dir/src/emit/emit.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/emit/emit.cpp.o: ../src/emit/emit.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/oohoo.dir/src/emit/emit.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/emit/emit.cpp.o -c /home/andy/CLionProjects/oohoo/src/emit/emit.cpp

CMakeFiles/oohoo.dir/src/emit/emit.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/emit/emit.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/emit/emit.cpp > CMakeFiles/oohoo.dir/src/emit/emit.cpp.i

CMakeFiles/oohoo.dir/src/emit/emit.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/emit/emit.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/emit/emit.cpp -o CMakeFiles/oohoo.dir/src/emit/emit.cpp.s

CMakeFiles/oohoo.dir/src/emit/emit.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/emit/emit.cpp.o.requires

CMakeFiles/oohoo.dir/src/emit/emit.cpp.o.provides: CMakeFiles/oohoo.dir/src/emit/emit.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/emit/emit.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/emit/emit.cpp.o.provides

CMakeFiles/oohoo.dir/src/emit/emit.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/emit/emit.cpp.o


CMakeFiles/oohoo.dir/src/model/model.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/model/model.cpp.o: ../src/model/model.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/oohoo.dir/src/model/model.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/model/model.cpp.o -c /home/andy/CLionProjects/oohoo/src/model/model.cpp

CMakeFiles/oohoo.dir/src/model/model.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/model/model.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/model/model.cpp > CMakeFiles/oohoo.dir/src/model/model.cpp.i

CMakeFiles/oohoo.dir/src/model/model.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/model/model.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/model/model.cpp -o CMakeFiles/oohoo.dir/src/model/model.cpp.s

CMakeFiles/oohoo.dir/src/model/model.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/model/model.cpp.o.requires

CMakeFiles/oohoo.dir/src/model/model.cpp.o.provides: CMakeFiles/oohoo.dir/src/model/model.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/model/model.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/model/model.cpp.o.provides

CMakeFiles/oohoo.dir/src/model/model.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/model/model.cpp.o


CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o: ../src/parse/check_function_body.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o -c /home/andy/CLionProjects/oohoo/src/parse/check_function_body.cpp

CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/parse/check_function_body.cpp > CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.i

CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/parse/check_function_body.cpp -o CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.s

CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o.requires

CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o.provides: CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o.provides

CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o


CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o: ../src/parse/Lexer.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o -c /home/andy/CLionProjects/oohoo/src/parse/Lexer.cpp

CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/parse/Lexer.cpp > CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.i

CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/parse/Lexer.cpp -o CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.s

CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o.requires

CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o.provides: CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o.provides

CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o


CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o: ../src/parse/parse_expr.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o -c /home/andy/CLionProjects/oohoo/src/parse/parse_expr.cpp

CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/parse/parse_expr.cpp > CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.i

CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/parse/parse_expr.cpp -o CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.s

CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o.requires

CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o.provides: CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o.provides

CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o


CMakeFiles/oohoo.dir/src/parse/parser.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/parse/parser.cpp.o: ../src/parse/parser.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/oohoo.dir/src/parse/parser.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/parse/parser.cpp.o -c /home/andy/CLionProjects/oohoo/src/parse/parser.cpp

CMakeFiles/oohoo.dir/src/parse/parser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/parse/parser.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/parse/parser.cpp > CMakeFiles/oohoo.dir/src/parse/parser.cpp.i

CMakeFiles/oohoo.dir/src/parse/parser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/parse/parser.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/parse/parser.cpp -o CMakeFiles/oohoo.dir/src/parse/parser.cpp.s

CMakeFiles/oohoo.dir/src/parse/parser.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/parse/parser.cpp.o.requires

CMakeFiles/oohoo.dir/src/parse/parser.cpp.o.provides: CMakeFiles/oohoo.dir/src/parse/parser.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/parse/parser.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/parse/parser.cpp.o.provides

CMakeFiles/oohoo.dir/src/parse/parser.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/parse/parser.cpp.o


CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o: ../src/parse/type_utils.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o -c /home/andy/CLionProjects/oohoo/src/parse/type_utils.cpp

CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/parse/type_utils.cpp > CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.i

CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/parse/type_utils.cpp -o CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.s

CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o.requires

CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o.provides: CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o.provides

CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o


CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o: ../src/util/StringSlice.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building CXX object CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o -c /home/andy/CLionProjects/oohoo/src/util/StringSlice.cpp

CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/util/StringSlice.cpp > CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.i

CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/util/StringSlice.cpp -o CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.s

CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o.requires

CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o.provides: CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o.provides

CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o


CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o: ../src/emit/Writer.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building CXX object CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o -c /home/andy/CLionProjects/oohoo/src/emit/Writer.cpp

CMakeFiles/oohoo.dir/src/emit/Writer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/emit/Writer.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/emit/Writer.cpp > CMakeFiles/oohoo.dir/src/emit/Writer.cpp.i

CMakeFiles/oohoo.dir/src/emit/Writer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/emit/Writer.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/emit/Writer.cpp -o CMakeFiles/oohoo.dir/src/emit/Writer.cpp.s

CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o.requires

CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o.provides: CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o.provides

CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o


CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o: CMakeFiles/oohoo.dir/flags.make
CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o: ../src/emit/mangle.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building CXX object CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o -c /home/andy/CLionProjects/oohoo/src/emit/mangle.cpp

CMakeFiles/oohoo.dir/src/emit/mangle.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/oohoo.dir/src/emit/mangle.cpp.i"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/andy/CLionProjects/oohoo/src/emit/mangle.cpp > CMakeFiles/oohoo.dir/src/emit/mangle.cpp.i

CMakeFiles/oohoo.dir/src/emit/mangle.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/oohoo.dir/src/emit/mangle.cpp.s"
	/home/andy/bin/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/andy/CLionProjects/oohoo/src/emit/mangle.cpp -o CMakeFiles/oohoo.dir/src/emit/mangle.cpp.s

CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o.requires:

.PHONY : CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o.requires

CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o.provides: CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o.requires
	$(MAKE) -f CMakeFiles/oohoo.dir/build.make CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o.provides.build
.PHONY : CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o.provides

CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o.provides.build: CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o


# Object files for target oohoo
oohoo_OBJECTS = \
"CMakeFiles/oohoo.dir/src/main.cpp.o" \
"CMakeFiles/oohoo.dir/src/emit/emit.cpp.o" \
"CMakeFiles/oohoo.dir/src/model/model.cpp.o" \
"CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o" \
"CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o" \
"CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o" \
"CMakeFiles/oohoo.dir/src/parse/parser.cpp.o" \
"CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o" \
"CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o" \
"CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o" \
"CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o"

# External object files for target oohoo
oohoo_EXTERNAL_OBJECTS =

oohoo: CMakeFiles/oohoo.dir/src/main.cpp.o
oohoo: CMakeFiles/oohoo.dir/src/emit/emit.cpp.o
oohoo: CMakeFiles/oohoo.dir/src/model/model.cpp.o
oohoo: CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o
oohoo: CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o
oohoo: CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o
oohoo: CMakeFiles/oohoo.dir/src/parse/parser.cpp.o
oohoo: CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o
oohoo: CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o
oohoo: CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o
oohoo: CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o
oohoo: CMakeFiles/oohoo.dir/build.make
oohoo: CMakeFiles/oohoo.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Linking CXX executable oohoo"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/oohoo.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/oohoo.dir/build: oohoo

.PHONY : CMakeFiles/oohoo.dir/build

CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/main.cpp.o.requires
CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/emit/emit.cpp.o.requires
CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/model/model.cpp.o.requires
CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/parse/check_function_body.cpp.o.requires
CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/parse/Lexer.cpp.o.requires
CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/parse/parse_expr.cpp.o.requires
CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/parse/parser.cpp.o.requires
CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/parse/type_utils.cpp.o.requires
CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/util/StringSlice.cpp.o.requires
CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/emit/Writer.cpp.o.requires
CMakeFiles/oohoo.dir/requires: CMakeFiles/oohoo.dir/src/emit/mangle.cpp.o.requires

.PHONY : CMakeFiles/oohoo.dir/requires

CMakeFiles/oohoo.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/oohoo.dir/cmake_clean.cmake
.PHONY : CMakeFiles/oohoo.dir/clean

CMakeFiles/oohoo.dir/depend:
	cd /home/andy/CLionProjects/oohoo/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/andy/CLionProjects/oohoo /home/andy/CLionProjects/oohoo /home/andy/CLionProjects/oohoo/cmake-build-debug /home/andy/CLionProjects/oohoo/cmake-build-debug /home/andy/CLionProjects/oohoo/cmake-build-debug/CMakeFiles/oohoo.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/oohoo.dir/depend

