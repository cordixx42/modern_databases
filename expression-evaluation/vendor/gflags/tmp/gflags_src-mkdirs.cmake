# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/xinyicui/Desktop/expression-evaluation/vendor/gflags/src/gflags_src"
  "/Users/xinyicui/Desktop/expression-evaluation/vendor/gflags/src/gflags_src-build"
  "/Users/xinyicui/Desktop/expression-evaluation/vendor/gflags"
  "/Users/xinyicui/Desktop/expression-evaluation/vendor/gflags/tmp"
  "/Users/xinyicui/Desktop/expression-evaluation/vendor/gflags/src/gflags_src-stamp"
  "/Users/xinyicui/Desktop/expression-evaluation/vendor/gflags/src"
  "/Users/xinyicui/Desktop/expression-evaluation/vendor/gflags/src/gflags_src-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/xinyicui/Desktop/expression-evaluation/vendor/gflags/src/gflags_src-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/xinyicui/Desktop/expression-evaluation/vendor/gflags/src/gflags_src-stamp${cfgdir}") # cfgdir has leading slash
endif()
