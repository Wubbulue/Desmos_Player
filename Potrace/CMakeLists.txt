cmake_minimum_required(VERSION 3.10)
set (CMAKE_C_STANDARD 17)

 
message (${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB potrace_src
     "*.h"
     "*.c"
	 "*.cpp"
	 
)

set( EXEC_SOURCES ${potrace_src} ${CMAKE_CURRENT_SOURCE_DIR}/include/getopt/getopt.h)

message(${EXEC_SOURCES})


add_library(Potrace ${EXEC_SOURCES})

INCLUDE_DIRECTORIES(Potrace ${CMAKE_CURRENT_SOURCE_DIR}/include)
