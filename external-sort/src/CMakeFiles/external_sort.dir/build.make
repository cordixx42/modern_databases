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

# Include any dependencies generated for this target.
include CMakeFiles/external_sort.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/external_sort.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/external_sort.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/external_sort.dir/flags.make

CMakeFiles/external_sort.dir/tools/external_sort.cc.o: CMakeFiles/external_sort.dir/flags.make
CMakeFiles/external_sort.dir/tools/external_sort.cc.o: /Users/xinyicui/Desktop/external-sort/tools/external_sort.cc
CMakeFiles/external_sort.dir/tools/external_sort.cc.o: CMakeFiles/external_sort.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/Users/xinyicui/Desktop/external-sort/src/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/external_sort.dir/tools/external_sort.cc.o"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/external_sort.dir/tools/external_sort.cc.o -MF CMakeFiles/external_sort.dir/tools/external_sort.cc.o.d -o CMakeFiles/external_sort.dir/tools/external_sort.cc.o -c /Users/xinyicui/Desktop/external-sort/tools/external_sort.cc

CMakeFiles/external_sort.dir/tools/external_sort.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/external_sort.dir/tools/external_sort.cc.i"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/xinyicui/Desktop/external-sort/tools/external_sort.cc > CMakeFiles/external_sort.dir/tools/external_sort.cc.i

CMakeFiles/external_sort.dir/tools/external_sort.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/external_sort.dir/tools/external_sort.cc.s"
	/Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/xinyicui/Desktop/external-sort/tools/external_sort.cc -o CMakeFiles/external_sort.dir/tools/external_sort.cc.s

# Object files for target external_sort
external_sort_OBJECTS = \
"CMakeFiles/external_sort.dir/tools/external_sort.cc.o"

# External object files for target external_sort
external_sort_EXTERNAL_OBJECTS =

external_sort: CMakeFiles/external_sort.dir/tools/external_sort.cc.o
external_sort: CMakeFiles/external_sort.dir/build.make
external_sort: libmoderndbs.a
external_sort: vendor/gflags/lib/libgflags.a
external_sort: CMakeFiles/external_sort.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/Users/xinyicui/Desktop/external-sort/src/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable external_sort"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/external_sort.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/external_sort.dir/build: external_sort
.PHONY : CMakeFiles/external_sort.dir/build

CMakeFiles/external_sort.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/external_sort.dir/cmake_clean.cmake
.PHONY : CMakeFiles/external_sort.dir/clean

CMakeFiles/external_sort.dir/depend:
	cd /Users/xinyicui/Desktop/external-sort/src && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/xinyicui/Desktop/external-sort /Users/xinyicui/Desktop/external-sort /Users/xinyicui/Desktop/external-sort/src /Users/xinyicui/Desktop/external-sort/src /Users/xinyicui/Desktop/external-sort/src/CMakeFiles/external_sort.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/external_sort.dir/depend

