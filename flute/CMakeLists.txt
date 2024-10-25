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
