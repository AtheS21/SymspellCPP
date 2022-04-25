project(symspell)
cmake_minimum_required(VERSION 2.8)
include_directories(include)

set(CMAKE_CXX_FLAGS "-O3 -g3")

set(CMAKE_CXX_STANDARD 17)

add_library(symspell src/Symspell.cpp)
add_executable(symspelltest src/SymSpell.cpp SymSpellTest.cpp)