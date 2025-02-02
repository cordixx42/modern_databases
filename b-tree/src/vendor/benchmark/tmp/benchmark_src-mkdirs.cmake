# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/xinyicui/Desktop/b-tree/src/vendor/benchmark/src/benchmark_src"
  "/Users/xinyicui/Desktop/b-tree/src/vendor/benchmark/src/benchmark_src-build"
  "/Users/xinyicui/Desktop/b-tree/src/vendor/benchmark"
  "/Users/xinyicui/Desktop/b-tree/src/vendor/benchmark/tmp"
  "/Users/xinyicui/Desktop/b-tree/src/vendor/benchmark/src/benchmark_src-stamp"
  "/Users/xinyicui/Desktop/b-tree/src/vendor/benchmark/src"
  "/Users/xinyicui/Desktop/b-tree/src/vendor/benchmark/src/benchmark_src-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/xinyicui/Desktop/b-tree/src/vendor/benchmark/src/benchmark_src-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/xinyicui/Desktop/b-tree/src/vendor/benchmark/src/benchmark_src-stamp${cfgdir}") # cfgdir has leading slash
endif()
