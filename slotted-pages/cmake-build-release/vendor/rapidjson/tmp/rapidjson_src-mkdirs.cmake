# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/xinyicui/Desktop/slotted-pages/cmake-build-release/vendor/rapidjson/src/rapidjson_src"
  "/Users/xinyicui/Desktop/slotted-pages/cmake-build-release/vendor/rapidjson/src/rapidjson_src-build"
  "/Users/xinyicui/Desktop/slotted-pages/cmake-build-release/vendor/rapidjson"
  "/Users/xinyicui/Desktop/slotted-pages/cmake-build-release/vendor/rapidjson/tmp"
  "/Users/xinyicui/Desktop/slotted-pages/cmake-build-release/vendor/rapidjson/src/rapidjson_src-stamp"
  "/Users/xinyicui/Desktop/slotted-pages/cmake-build-release/vendor/rapidjson/src"
  "/Users/xinyicui/Desktop/slotted-pages/cmake-build-release/vendor/rapidjson/src/rapidjson_src-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/xinyicui/Desktop/slotted-pages/cmake-build-release/vendor/rapidjson/src/rapidjson_src-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/xinyicui/Desktop/slotted-pages/cmake-build-release/vendor/rapidjson/src/rapidjson_src-stamp${cfgdir}") # cfgdir has leading slash
endif()
