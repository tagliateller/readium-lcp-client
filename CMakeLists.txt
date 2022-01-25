cmake_minimum_required(VERSION 3.16)

project(ReadiumLcp)

add_subdirectory(src/third-parties/cryptopp)
add_subdirectory(src/lcp-client-lib)
add_subdirectory(test/lcp-client-lib)
