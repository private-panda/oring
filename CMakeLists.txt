
include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(okvs)

add_executable(mpsi mpsi.cpp util.cpp)
target_link_libraries(mpsi libokvs)

target_compile_options(mpsi PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++${LIBOTE_STD_VER}>)

add_executable(oringt1 oring_t1.cpp util.cpp)
target_link_libraries(oringt1 libokvs)

target_compile_options(oringt1 PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++${LIBOTE_STD_VER}>)
