# Copyright 2017 The LevelDB Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file. See the AUTHORS file for names of contributors.

cmake_minimum_required(VERSION 3.10)
# Keep the version below in sync with the one in db.h
project(TestVHACD VERSION 1.23.0 LANGUAGES C CXX)

# This project requires C++11.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(TestVHACD TestVHACD.cpp FloatMath.cpp InParser.cpp wavefront.cpp SaveUSDA.cpp)

target_include_directories(TestVHACD PUBLIC "../include")
# On Linux we need to add pthread to the link
if (WIN32)
else (WIN32)
	target_link_libraries(TestVHACD pthread)
endif (WIN32)
