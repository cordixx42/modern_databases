# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.27

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
CMAKE_COMMAND = /opt/homebrew/Cellar/cmake/3.27.7/bin/cmake

# The command to remove a file.
RM = /opt/homebrew/Cellar/cmake/3.27.7/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/xinyicui/Desktop/slotted-pages

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/xinyicui/Desktop/slotted-pages/bench

# Utility rule file for lint_test.

# Include any custom commands dependencies for this target.
include CMakeFiles/lint_test.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/lint_test.dir/progress.make

CMakeFiles/lint_test:
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/xinyicui/Desktop/slotted-pages/bench/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Running lint_test"
	/opt/homebrew/Cellar/cmake/3.27.7/bin/cmake -E chdir /Users/xinyicui/Desktop/slotted-pages /usr/local/bin/clang-tidy -quiet -header-filter=/Users/xinyicui/Desktop/slotted-pages/include -p=/Users/xinyicui/Desktop/slotted-pages/bench test/segment_test.cc test/slotted_page_test.cc

lint_test: CMakeFiles/lint_test
lint_test: CMakeFiles/lint_test.dir/build.make
.PHONY : lint_test

# Rule to build all files generated by this target.
CMakeFiles/lint_test.dir/build: lint_test
.PHONY : CMakeFiles/lint_test.dir/build

CMakeFiles/lint_test.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/lint_test.dir/cmake_clean.cmake
.PHONY : CMakeFiles/lint_test.dir/clean

CMakeFiles/lint_test.dir/depend:
	cd /Users/xinyicui/Desktop/slotted-pages/bench && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/xinyicui/Desktop/slotted-pages /Users/xinyicui/Desktop/slotted-pages /Users/xinyicui/Desktop/slotted-pages/bench /Users/xinyicui/Desktop/slotted-pages/bench /Users/xinyicui/Desktop/slotted-pages/bench/CMakeFiles/lint_test.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/lint_test.dir/depend

