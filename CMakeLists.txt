
# file(GLOB_RECURSE SRCS *.cpp)

# set(COMMON_FLAGS "-Wall -march=native -Wfatal-errors -msse2 -pthread")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(okvs)

# add_executable(okvsChain okvsChain.cpp util.cpp)
# target_link_libraries(okvsChain libokvs)

# add_executable(oprf oprf.cpp util.cpp)
# target_link_libraries(oprf libokvs)

add_executable(mpsi mpsi.cpp util.cpp)
target_link_libraries(mpsi libokvs)

target_compile_options(mpsi PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++${LIBOTE_STD_VER}>)

add_executable(oringt1 oring_t1.cpp util.cpp)
target_link_libraries(oringt1 libokvs)

target_compile_options(oringt1 PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++${LIBOTE_STD_VER}>)

#if(MSVC)
#    target_compile_options(mpsi PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/std:c++${LIBOTE_STD_VER}>)
#    target_compile_options(mpsi PRIVATE "/bigobj")
#else()
#    target_compile_options(mpsi PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++${LIBOTE_STD_VER}>)
#endif()
