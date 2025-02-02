# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/xinyicui/Desktop/external-sort/src/vendor/gtm/src/googletest/googlemock"
  "/Users/xinyicui/Desktop/external-sort/src/vendor/gtm/src/gmock_src-build"
  "/Users/xinyicui/Desktop/external-sort/src/vendor/gtm/gmock"
  "/Users/xinyicui/Desktop/external-sort/src/vendor/gtm/tmp"
  "/Users/xinyicui/Desktop/external-sort/src/vendor/gtm/src/gmock_src-stamp"
  "/Users/xinyicui/Desktop/external-sort/src/vendor/gtm/src"
  "/Users/xinyicui/Desktop/external-sort/src/vendor/gtm/src/gmock_src-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/xinyicui/Desktop/external-sort/src/vendor/gtm/src/gmock_src-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/xinyicui/Desktop/external-sort/src/vendor/gtm/src/gmock_src-stamp${cfgdir}") # cfgdir has leading slash
endif()
