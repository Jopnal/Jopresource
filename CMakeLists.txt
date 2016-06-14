cmake_minimum_required(VERSION 2.6)

project(Jopresource CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/extlibs/include)

set(INCDIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRCDIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(jopres ${SRCDIR}/main.cpp
					  ${SRCDIR}/Compiler.cpp
					  ${INCDIR}/Compiler.hpp)