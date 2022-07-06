cmake_minimum_required (VERSION 3.8)
project(nanojson2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" 
  OR "${CMAKE_CXX_SIMULATE_ID}" STREQUAL "MSVC" )
    add_compile_options(/utf-8 /W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_executable (nanojson2 "nanojson2.h" "nanojson2.samples.cpp")
