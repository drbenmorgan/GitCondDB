cmake_minimum_required(VERSION 3.10)

project(GitCondDB VERSION 1.0)

include_directories(include)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

add_library(${PROJECT_NAME} SHARED include/GitCondDB.h src/GitCondDB.cpp)
