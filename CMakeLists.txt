cmake_minimum_required(VERSION 3.10)
set (CMAKE_CXX_STANDARD 17)

 
PROJECT (HelloWorldProject)
find_package(OpenCV REQUIRED )
INCLUDE_DIRECTORIES( ${OpenCV_INCLUDE_DIRS} )
message(${OpenCV_INCLUDE_DIRS})

# add the Potrace library
add_subdirectory(Potrace)

set( NAME_SRC
    main.cpp    
)



INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/Potrace)
#INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/include)

add_executable( HelloWorldProject ${NAME_SRC} ${NAME_HEADERS} )

#TARGET_LINK_LIBRARIES (HelloWorldProject ${OpenCV_LIBS} ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/lib/PotraceLib.lib)         
TARGET_LINK_LIBRARIES (HelloWorldProject PUBLIC ${OpenCV_LIBS} Potrace)         


#target_link_libraries( HelloWorldProject ${OpenCV_LIBS} )