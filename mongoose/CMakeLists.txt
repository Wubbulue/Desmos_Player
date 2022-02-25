cmake_minimum_required(VERSION 3.10)
set (CMAKE_C_STANDARD 17)


add_library(mongoose mongoose.c mongoose.h)
if(WIN32)
  message(in windows)
  target_link_libraries(mongoose wsock32 ws2_32)
endif()

