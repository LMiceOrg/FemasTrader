cmake_minimum_required (VERSION 2.6)
PROJECT(mduserdemo C CXX)

# Which compilers to use for C and C++
#set(CMAKE_C_COMPILER cc -m32)
#set(CMAKE_CXX_COMPILER cc -m32)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")

#Header file path
INCLUDE_DIRECTORIES(
../../../Doc/FemasAPI_V1.01/include/
)

#source directory
AUX_SOURCE_DIRECTORY(. mduserdemo_src )

#set extern libraries
SET(LIBRARIES
#libm-2.12.so
libUSTPmduserapi.so
)

LINK_DIRECTORIES(
#../../../Doc/FemasAPI_V1.01/lnx32
#/compat/linux/lib
/home/hehao/work/FemasTrader/Doc/FemasAPI_V1.01/lnx32
)

message(${mduserdemo_src})

#add executable file
ADD_EXECUTABLE( mduserdemo ${mduserdemo_src} )

#add link library
TARGET_LINK_LIBRARIES(mduserdemo ${LIBRARIES})

#LINK_DIRECTORIES(
#../../../Doc/FemasAPI_V1.01/lnx32
#)


#add executable file
#ADD_EXECUTABLE( mduserdemo ${mduserdemo_src} )
