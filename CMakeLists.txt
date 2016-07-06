cmake_minimum_required(VERSION 2.8)
PROJECT(eal)

set(CMAKE_VERBOSE_MAKEFILE on)

IF( ${CMAKE_CXX_COMPILER} MATCHES  "[cg][+][+]")
    MESSAGE("G++ ")
    set(CMAKE_CXX_FLAGS "-Wall -std=c++11")
    set(CMAKE_CXX_FLAGS_DEBUG "-g3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    set(CMAKE_C_FLAGS "-Wall ")
    set(CMAKE_C_FLAGS_DEBUG "-g3")
    set(CMAKE_C_FLAGS_RELEASE "-o2")
ELSE()

ENDIF()

INCLUDE_DIRECTORIES(.
    eal
    lib/FemasAPIv2/linux64/include
    lib/libbson/include
    lib/libforecaster
    app/lmiced/)

FIND_LIBRARY(femas_libs NAMES USTPtraderapi
            	PATHS ${CMAKE_SOURCE_DIR}/lib/FemasAPIv2/linux64/lib)

FIND_LIBRARY(femasmd_libs NAMES USTPmduserapi
                PATHS ${CMAKE_SOURCE_DIR}/lib/FemasAPIv2/linux64/lib)

FIND_LIBRARY(bson_libs NAMES bson
                PATHS ${CMAKE_SOURCE_DIR}/lib/libbson/lib)

FIND_LIBRARY(fc_libs NAMES forecaster
                PATHS ${CMAKE_SOURCE_DIR}/lib/libforecaster)

message("Femas Lib:" ${femas_libs})
message("Femas MD lib:" ${femasmd_libs})
message("Bson lib:" ${bson_libs})
message("forecaster lib:" ${fc_libs})

#
#LINK_DIRECTORIES(/home/hehao/work/ZeusingApi_20150331)

#LIB EAL
LIST(APPEND eal_SRCS  eal/lmice_ring.c
    eal/lmice_trace.c
    eal/lmice_eal_shm.c
    eal/lmice_eal_hash.c
    eal/lmice_eal_spinlock.c
    eal/lmice_eal_malloc.c
    eal/lmice_eal_event.c
    eal/lmice_eal_time.c
    eal/lmice_core.c
    eal/lmice_eal_aio.c
    eal/lmice_eal_thread.c
    eal/lmice_bloomfilter.c
    eal/lmice_eal_log.cpp
    eal/lmice_eal_bson.cpp )

#Shared EAL Library
#ADD_LIBRARY(eal SHARED ${EAL_SRCS})
#Static EAL Library
ADD_LIBRARY(eal_static STATIC ${eal_SRCS})

SET_TARGET_PROPERTIES(eal_static PROPERTIES OUTPUT_NAME "eal")
SET_TARGET_PROPERTIES(eal_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
#SET_TARGET_PROPERTIES(eal_static PROPERTIES VERSION 1.2 SOVERSION 1)


#Add Model static lib
LIST(APPEND model_SRC app/lmiced/lmspi.cpp
    app/lmiced/udss.c
)

add_library(model_static STATIC ${model_SRC})
SET_TARGET_PROPERTIES(model_static PROPERTIES OUTPUT_NAME "model")
SET_TARGET_PROPERTIES(model_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
add_dependencies(model_static eal_static)


#App Lmiced
LIST(APPEND lmiced_SRCS app/lmiced/lmiced.c
    app/lmiced/udss.c
    app/lmiced/lmshm.c
    app/lmiced/lmclient.c)

ADD_EXECUTABLE(lmiced ${lmiced_SRCS})
target_link_libraries(lmiced eal_static pthread rt ${bson_libs})
add_dependencies(lmiced eal_static)

#App udstest
LIST(APPEND udstest_SRCS app/lmiced/udsc.c
    app/lmiced/udss.c)

ADD_EXECUTABLE(udstest ${udstest_SRCS})
target_link_libraries(udstest eal_static pthread rt)
add_dependencies(udstest eal_static)

#App Zeusing Trader
#LIST(APPEND zstrader_SRC app/lmiced/zstrader.cpp
#    app/lmiced/lmicedtrader.cpp)
#
#ADD_EXECUTABLE(zstrader ${zstrader_SRC})
#target_link_libraries(zstrader eal_static pthread rt zeusingtraderapi )
#add_dependencies(zstrader eal_static)

#App femas trader
LIST(APPEND fmtrader_SRC app/lmiced/fmtrader.cpp
    app/lmiced/fmspi.cpp
    app/lmiced/publicfuncs.cpp
    app/lmiced/udss.c
    app/lmiced/lmspi.cpp
)
ADD_EXECUTABLE(fmtrader ${fmtrader_SRC})
target_link_libraries(fmtrader eal_static pthread rt ${femas_libs} ${bson_libs})
add_dependencies(fmtrader eal_static)

#App guava md
LIST(APPEND guavamd_SRC app/guavamd/guava_demo.cpp
    app/guavamd/guava_quote.cpp
    app/guavamd/guavamd.cpp
    app/guavamd/profile.cpp
    app/guavamd/socket_multicast.cpp )

ADD_EXECUTABLE(guavamd ${guavamd_SRC})
target_link_libraries(guavamd model_static eal_static pthread rt dl ${bson_libs})
add_dependencies(guavamd model_static)
add_dependencies(guavamd eal_static)


#Add Femas md
LIST(APPEND fmmd_SRC app/femasmd/MsClient.cpp
    app/femasmd/profile.cpp
app/femasmd/USTPMDClient.cpp
app/femasmd/USTPMDHandler.cpp
)

add_executable(femasmd ${fmmd_SRC})
target_link_libraries(femasmd eal_static pthread rt ${femasmd_libs} ${bson_libs})
add_dependencies(femasmd eal_static)


#Add demo model
LIST(APPEND demo_SRC app/lmiced/demo.cpp
)

add_executable(demomodel ${demo_SRC})
target_link_libraries(demomodel model_static eal_static pthread rt ${femasmd_libs} ${bson_libs})
add_dependencies(demomodel model_static)
add_dependencies(demomodel eal_static)

#Add demo model
LIST(APPEND rb1610_SRC app/lmiced/rb1610.cpp
)

add_executable(rb1610 ${rb1610_SRC})
target_link_libraries(rb1610 model_static eal_static pthread rt ${femasmd_libs} ${bson_libs})
add_dependencies(rb1610 model_static)
add_dependencies(rb1610 eal_static)


#Add SW guava md model
LIST(APPEND guavasw_SRC app/guavasw/guava_demo.cpp
    app/guavasw/guava_quote.cpp
    app/guavasw/guavasw.cpp
    app/guavasw/profile.cpp
    app/guavasw/socket_multicast.cpp )

add_executable(guavasw ${guavasw_SRC})
target_link_libraries(guavasw model_static eal_static pthread rt ${femasmd_libs} ${bson_libs})
add_dependencies(guavasw model_static)
add_dependencies(guavasw eal_static)

#Add Net md model
LIST(APPEND netmd_SRC app/netmd/netmd.c
)
add_executable(netmd ${netmd_SRC})
target_link_libraries(netmd model_static eal_static pthread rt pcap )
add_dependencies(netmd model_static)
add_dependencies(netmd eal_static)


#Add HT guava md model
LIST(APPEND guavaht_SRC app/mdht/guava_demo.cpp
    app/mdht/guava_quote.cpp
    app/mdht/guavaht.cpp
    app/mdht/profile.cpp
    app/mdht/socket_multicast.cpp )

add_executable(guavaht ${guavaht_SRC})
target_link_libraries(guavaht model_static eal_static pthread rt ${femas_libs} ${femasmd_libs}  ${bson_libs} ${fc_libs})
add_dependencies(guavaht model_static)
add_dependencies(guavaht eal_static)

#Add genid tool
LIST(APPEND genid_SRC app/lmiced/genid.c )

add_executable(genid ${genid_SRC})
target_link_libraries(genid  eal_static pthread rt)
add_dependencies(genid eal_static)
