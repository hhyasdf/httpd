# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.6

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/hhy/github/httpd

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/hhy/github/httpd

# Include any dependencies generated for this target.
include CMakeFiles/httpd_server.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/httpd_server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/httpd_server.dir/flags.make

CMakeFiles/httpd_server.dir/httpd.o: CMakeFiles/httpd_server.dir/flags.make
CMakeFiles/httpd_server.dir/httpd.o: httpd.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hhy/github/httpd/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/httpd_server.dir/httpd.o"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/httpd_server.dir/httpd.o   -c /home/hhy/github/httpd/httpd.c

CMakeFiles/httpd_server.dir/httpd.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/httpd_server.dir/httpd.i"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/hhy/github/httpd/httpd.c > CMakeFiles/httpd_server.dir/httpd.i

CMakeFiles/httpd_server.dir/httpd.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/httpd_server.dir/httpd.s"
	/usr/bin/cc  $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/hhy/github/httpd/httpd.c -o CMakeFiles/httpd_server.dir/httpd.s

CMakeFiles/httpd_server.dir/httpd.o.requires:

.PHONY : CMakeFiles/httpd_server.dir/httpd.o.requires

CMakeFiles/httpd_server.dir/httpd.o.provides: CMakeFiles/httpd_server.dir/httpd.o.requires
	$(MAKE) -f CMakeFiles/httpd_server.dir/build.make CMakeFiles/httpd_server.dir/httpd.o.provides.build
.PHONY : CMakeFiles/httpd_server.dir/httpd.o.provides

CMakeFiles/httpd_server.dir/httpd.o.provides.build: CMakeFiles/httpd_server.dir/httpd.o


# Object files for target httpd_server
httpd_server_OBJECTS = \
"CMakeFiles/httpd_server.dir/httpd.o"

# External object files for target httpd_server
httpd_server_EXTERNAL_OBJECTS =

httpd_server: CMakeFiles/httpd_server.dir/httpd.o
httpd_server: CMakeFiles/httpd_server.dir/build.make
httpd_server: CMakeFiles/httpd_server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/hhy/github/httpd/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable httpd_server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/httpd_server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/httpd_server.dir/build: httpd_server

.PHONY : CMakeFiles/httpd_server.dir/build

CMakeFiles/httpd_server.dir/requires: CMakeFiles/httpd_server.dir/httpd.o.requires

.PHONY : CMakeFiles/httpd_server.dir/requires

CMakeFiles/httpd_server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/httpd_server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/httpd_server.dir/clean

CMakeFiles/httpd_server.dir/depend:
	cd /home/hhy/github/httpd && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/hhy/github/httpd /home/hhy/github/httpd /home/hhy/github/httpd /home/hhy/github/httpd /home/hhy/github/httpd/CMakeFiles/httpd_server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/httpd_server.dir/depend
