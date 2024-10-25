cmake_minimum_required(VERSION 3.12)

project(route_sta)

add_subdirectory(OpenSTA)
add_subdirectory(lef)
add_subdirectory(def)
add_subdirectory(flute)
add_subdirectory(route)

add_executable(route_sta main.cpp)

target_link_libraries(route_sta
  sta_swig
  OpenSTA
  route
)
