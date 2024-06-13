include_directories(${CMAKE_SOURCE_DIR})

# file(GLOB_RECURSE SRCS *.cpp)

set(XXHASHDIR "${CMAKE_CURRENT_SOURCE_DIR}/xxHash")


#add_library(libokvs paxos.cpp gf2e_mat_solve.cpp)
add_library(libokvs paxos.cpp linear_solver.cpp)

target_link_libraries(libokvs libOTe_Tests tests_cryptoTools ntl gmp ${XXHASHDIR}/libxxhash.a)

add_executable(test3hgct test_3hgct.cpp)
target_link_libraries(test3hgct libokvs)

if(MSVC)
    target_compile_options(libokvs PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/std:c++${LIBOTE_STD_VER}>)
    target_compile_options(libokvs PRIVATE "/bigobj")
else()
    target_compile_options(libokvs PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++${LIBOTE_STD_VER}>)

    target_compile_options(test3hgct PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++${LIBOTE_STD_VER}>)

endif()

