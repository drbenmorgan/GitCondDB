cmake_minimum_required(VERSION 3.10)

# Project settings

project(GitCondDB VERSION 1.0)

option(GITCONDDB_BUILD_STATIC_LIBRARY "Build the static library" ON)
option(GITCONDDB_BUILD_SHARED_LIBRARY "Build the shared library" ON)

if(NOT (GITCONDDB_BUILD_STATIC_LIBRARY OR GITCONDDB_BUILD_SHARED_LIBRARY))
  message(FATAL_ERROR "at least one of shared or static should be built")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Wpedantic")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)


# external dependencies

find_package(PkgConfig)
pkg_check_modules(git2 libgit2 REQUIRED)

include(GenerateExportHeader)


# Build instructions

set(HEAERS include/GitCondDB.h)
set(SOURCES src/GitCondDB.cpp)

add_library(objects OBJECT ${HEADERS} ${SOURCES})
generate_export_header(objects BASE_NAME GitCondDB)

target_include_directories(objects PRIVATE include ${git2_INCLUDE_DIRS})
target_compile_definitions(objects PRIVATE -Dobjects_EXPORT)


# link static and/or shared libraries

if(GITCONDDB_BUILD_STATIC_LIBRARY)
  add_library(Static STATIC $<TARGET_OBJECTS:objects>)
  set_target_properties(Static PROPERTIES OUTPUT_NAME GitCondDB)
  install(TARGETS Static EXPORT GitCondDB DESTINATION lib)
endif()

if(GITCONDDB_BUILD_SHARED_LIBRARY)
  add_library(Shared SHARED $<TARGET_OBJECTS:objects>)
  set_target_properties(Shared PROPERTIES OUTPUT_NAME GitCondDB)
  install(TARGETS Shared EXPORT GitCondDB DESTINATION lib)
endif()


# installation

install(FILES ${HEADERS} gitconddb_export.h DESTINATION include)

export(EXPORT GitCondDB NAMESPACE GitCondDB::)

install(EXPORT GitCondDB DESTINATION lib/GitCondDB NAMESPACE GitCondDB::)
