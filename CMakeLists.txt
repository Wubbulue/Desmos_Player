cmake_minimum_required(VERSION 3.10)
set (CMAKE_CXX_STANDARD 17)

 
PROJECT (DesmosTracer)
find_package(OpenCV REQUIRED )
INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} )
message(${OpenCV_INCLUDE_DIRS})

# add the Potrace library
add_subdirectory(Potrace)

# add the Mongoose library
add_subdirectory(mongoose)

set( NAME_SRC
    main.cpp 
)



INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/Potrace ${CMAKE_CURRENT_SOURCE_DIR}/mongoose)

add_executable( DesmosTracer ${NAME_SRC} ${NAME_HEADERS})

TARGET_LINK_LIBRARIES (DesmosTracer PUBLIC ${OpenCV_LIBS} Potrace mongoose)         


