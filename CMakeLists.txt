cmake_minimum_required(VERSION 3.22.1)
project(TP2)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED true)

SET(GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -pedantic") # -Wno-unused-parameter -O2??? -ftree-vectorize -fopt-info-vec-missed or all or optimized
#SET(GCC_COVERAGE_LINK_FLAGS "-lssl -lcrypto")    # "-lgcov"

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
#SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

# para usarmos sha-1
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# include_directories(include)

add_executable(Server
	src/server/main.cpp
	src/common/fs_track.cpp
	src/common/TCP_socket.cpp
	src/common/convert_types.cpp
	src/common/fs_track_test.cpp)

add_executable(Node
	src/client/main.cpp
	src/client/fs_transfer.cpp
	src/common/fs_track.cpp
	src/common/TCP_socket.cpp
	src/common/convert_types.cpp
	src/common/UDP_socket.cpp)

target_include_directories(Server
	PUBLIC include/common
	PUBLIC include/server)

target_include_directories(Node
	PUBLIC include/common
	PUBLIC include/client)

# para usarmos sha-1
target_link_libraries(Server ${OPENSSL_LIBRARIES})
target_link_libraries(Node ${OPENSSL_LIBRARIES})

