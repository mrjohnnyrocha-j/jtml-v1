# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build"

# Include any dependencies generated for this target.
include CMakeFiles/jtml_tests.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/jtml_tests.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/jtml_tests.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/jtml_tests.dir/flags.make

CMakeFiles/jtml_tests.dir/main_tests.cpp.o: CMakeFiles/jtml_tests.dir/flags.make
CMakeFiles/jtml_tests.dir/main_tests.cpp.o: ../main_tests.cpp
CMakeFiles/jtml_tests.dir/main_tests.cpp.o: CMakeFiles/jtml_tests.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/jtml_tests.dir/main_tests.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/jtml_tests.dir/main_tests.cpp.o -MF CMakeFiles/jtml_tests.dir/main_tests.cpp.o.d -o CMakeFiles/jtml_tests.dir/main_tests.cpp.o -c "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/main_tests.cpp"

CMakeFiles/jtml_tests.dir/main_tests.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/jtml_tests.dir/main_tests.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/main_tests.cpp" > CMakeFiles/jtml_tests.dir/main_tests.cpp.i

CMakeFiles/jtml_tests.dir/main_tests.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/jtml_tests.dir/main_tests.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/main_tests.cpp" -o CMakeFiles/jtml_tests.dir/main_tests.cpp.s

CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.o: CMakeFiles/jtml_tests.dir/flags.make
CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.o: ../src/jtml_ast.cpp
CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.o: CMakeFiles/jtml_tests.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.o -MF CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.o.d -o CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.o -c "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_ast.cpp"

CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_ast.cpp" > CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.i

CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_ast.cpp" -o CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.s

CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.o: CMakeFiles/jtml_tests.dir/flags.make
CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.o: ../src/jtml_lexer.cpp
CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.o: CMakeFiles/jtml_tests.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.o -MF CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.o.d -o CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.o -c "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_lexer.cpp"

CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_lexer.cpp" > CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.i

CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_lexer.cpp" -o CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.s

CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.o: CMakeFiles/jtml_tests.dir/flags.make
CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.o: ../src/jtml_parser.cpp
CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.o: CMakeFiles/jtml_tests.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.o -MF CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.o.d -o CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.o -c "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_parser.cpp"

CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_parser.cpp" > CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.i

CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_parser.cpp" -o CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.s

CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.o: CMakeFiles/jtml_tests.dir/flags.make
CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.o: ../src/jtml_interpreter.cpp
CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.o: CMakeFiles/jtml_tests.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.o -MF CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.o.d -o CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.o -c "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_interpreter.cpp"

CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_interpreter.cpp" > CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.i

CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/jtml_interpreter.cpp" -o CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.s

CMakeFiles/jtml_tests.dir/src/transpiler.cpp.o: CMakeFiles/jtml_tests.dir/flags.make
CMakeFiles/jtml_tests.dir/src/transpiler.cpp.o: ../src/transpiler.cpp
CMakeFiles/jtml_tests.dir/src/transpiler.cpp.o: CMakeFiles/jtml_tests.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/jtml_tests.dir/src/transpiler.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/jtml_tests.dir/src/transpiler.cpp.o -MF CMakeFiles/jtml_tests.dir/src/transpiler.cpp.o.d -o CMakeFiles/jtml_tests.dir/src/transpiler.cpp.o -c "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/transpiler.cpp"

CMakeFiles/jtml_tests.dir/src/transpiler.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/jtml_tests.dir/src/transpiler.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/transpiler.cpp" > CMakeFiles/jtml_tests.dir/src/transpiler.cpp.i

CMakeFiles/jtml_tests.dir/src/transpiler.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/jtml_tests.dir/src/transpiler.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/src/transpiler.cpp" -o CMakeFiles/jtml_tests.dir/src/transpiler.cpp.s

# Object files for target jtml_tests
jtml_tests_OBJECTS = \
"CMakeFiles/jtml_tests.dir/main_tests.cpp.o" \
"CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.o" \
"CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.o" \
"CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.o" \
"CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.o" \
"CMakeFiles/jtml_tests.dir/src/transpiler.cpp.o"

# External object files for target jtml_tests
jtml_tests_EXTERNAL_OBJECTS =

jtml_tests: CMakeFiles/jtml_tests.dir/main_tests.cpp.o
jtml_tests: CMakeFiles/jtml_tests.dir/src/jtml_ast.cpp.o
jtml_tests: CMakeFiles/jtml_tests.dir/src/jtml_lexer.cpp.o
jtml_tests: CMakeFiles/jtml_tests.dir/src/jtml_parser.cpp.o
jtml_tests: CMakeFiles/jtml_tests.dir/src/jtml_interpreter.cpp.o
jtml_tests: CMakeFiles/jtml_tests.dir/src/transpiler.cpp.o
jtml_tests: CMakeFiles/jtml_tests.dir/build.make
jtml_tests: CMakeFiles/jtml_tests.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_7) "Linking CXX executable jtml_tests"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/jtml_tests.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/jtml_tests.dir/build: jtml_tests
.PHONY : CMakeFiles/jtml_tests.dir/build

CMakeFiles/jtml_tests.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/jtml_tests.dir/cmake_clean.cmake
.PHONY : CMakeFiles/jtml_tests.dir/clean

CMakeFiles/jtml_tests.dir/depend:
	cd "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements" "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements" "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build" "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build" "/mnt/c/Users/Joao Rocha/Documents/GitHub/jtml-v2/jtml_elements/build/CMakeFiles/jtml_tests.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : CMakeFiles/jtml_tests.dir/depend

