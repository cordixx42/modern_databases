# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/xinyicui/Desktop/buffer-manager/test/vendor/gtm/src/googletest/googletest"
  "/Users/xinyicui/Desktop/buffer-manager/test/vendor/gtm/src/gtest_src-build"
  "/Users/xinyicui/Desktop/buffer-manager/test/vendor/gtm/gtest"
  "/Users/xinyicui/Desktop/buffer-manager/test/vendor/gtm/tmp"
  "/Users/xinyicui/Desktop/buffer-manager/test/vendor/gtm/src/gtest_src-stamp"
  "/Users/xinyicui/Desktop/buffer-manager/test/vendor/gtm/src"
  "/Users/xinyicui/Desktop/buffer-manager/test/vendor/gtm/src/gtest_src-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/xinyicui/Desktop/buffer-manager/test/vendor/gtm/src/gtest_src-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/xinyicui/Desktop/buffer-manager/test/vendor/gtm/src/gtest_src-stamp${cfgdir}") # cfgdir has leading slash
endif()
