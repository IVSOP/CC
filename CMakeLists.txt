cmake_minimum_required(VERSION 3.22.1)
project(TP2)

set(CMAKE_CXX_STANDARD 17)

SET(GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -pedantic -Wno-unused-parameter -mavx2") # -O2??? -ftree-vectorize -fopt-info-vec-missed or all or optimized
# SET(GCC_COVERAGE_LINK_FLAGS    "-lgcov")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
# SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

add_executable(TP2 main.cpp)
