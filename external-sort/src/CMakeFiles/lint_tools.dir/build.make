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
CMAKE_SOURCE_DIR = /Users/xinyicui/Desktop/external-sort

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/xinyicui/Desktop/external-sort/src

# Utility rule file for lint_tools.

# Include any custom commands dependencies for this target.
include CMakeFiles/lint_tools.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/lint_tools.dir/progress.make

CMakeFiles/lint_tools:
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --blue --bold --progress-dir=/Users/xinyicui/Desktop/external-sort/src/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Running lint_tools"
	/opt/homebrew/Cellar/cmake/3.27.7/bin/cmake -E chdir /Users/xinyicui/Desktop/external-sort /usr/local/bin/clang-tidy -quiet -header-filter=/Users/xinyicui/Desktop/external-sort/include -p=/Users/xinyicui/Desktop/external-sort/src tools/external_sort.cc

lint_tools: CMakeFiles/lint_tools
lint_tools: CMakeFiles/lint_tools.dir/build.make
.PHONY : lint_tools

# Rule to build all files generated by this target.
CMakeFiles/lint_tools.dir/build: lint_tools
.PHONY : CMakeFiles/lint_tools.dir/build

CMakeFiles/lint_tools.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/lint_tools.dir/cmake_clean.cmake
.PHONY : CMakeFiles/lint_tools.dir/clean

CMakeFiles/lint_tools.dir/depend:
	cd /Users/xinyicui/Desktop/external-sort/src && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/xinyicui/Desktop/external-sort /Users/xinyicui/Desktop/external-sort /Users/xinyicui/Desktop/external-sort/src /Users/xinyicui/Desktop/external-sort/src /Users/xinyicui/Desktop/external-sort/src/CMakeFiles/lint_tools.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/lint_tools.dir/depend

