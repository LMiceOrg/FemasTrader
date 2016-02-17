PROJECT("Femas")

#Header file path
INCLUDE_DIRECTORIES(
../../../Doc/FemasAPI_V1.01/include/
)

#source directory
AUX_SOURCE_DIRECTORY(mduserdemo .)

#set extern libraries
SET(LIBRARIES
libm.so
)

#add executable file
ADD_EXECUTABLE(mduserdemo ${mduserdemo})

#add link library
TARGET_LINK_LIBRARIES(mduserdemo ${LIBRARIES})

