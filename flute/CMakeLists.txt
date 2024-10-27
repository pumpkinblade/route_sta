set(PATH_FLUTE ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB FLUTE_SOURCE_FILES
    ${PATH_FLUTE}/*.c
    ${PATH_FLUTE}/*.cc
    ${PATH_FLUTE}/*.cpp
)

file(GLOB FLUTE_HEADER_FILES
    ${PATH_FLUTE}/*.h
    ${PATH_FLUTE}/*.hpp
)

add_library(flute
    ${FLUTE_SOURCE_FILES}
    ${FLUTE_HEADER_FILES}
)

target_include_directories(flute
    PUBLIC ${PATH_FLUTE}
)

add_custom_command(
  TARGET flute POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy 
    ${PATH_FLUTE}/POST9.dat 
    ${CMAKE_BINARY_DIR}/POST9.dat
  COMMAND ${CMAKE_COMMAND} -E copy 
    ${PATH_FLUTE}/POWV9.dat
    ${CMAKE_BINARY_DIR}/POWV9.dat
)
