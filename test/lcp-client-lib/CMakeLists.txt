cmake_minimum_required(VERSION 3.16)

include_directories(${CMAKE_SOURCE_DIR}/src/lcp-client-lib)
include_directories(${CMAKE_SOURCE_DIR}/test/lcp-client-lib/tools)
include_directories(${CMAKE_SOURCE_DIR}/src/third-parties)

set(testSrcs main.cpp)
add_executable(lcptest ${testSrcs})

target_link_libraries(lcptest lcplib)

install(TARGETS lcptest DESTINATION bin)

