set(ROUTE_HOME ${CMAKE_CURRENT_SOURCE_DIR})

##################
# route
##################

add_library(route
  ${ROUTE_HOME}/LefDefDatabase.cpp
  ${ROUTE_HOME}/LefDefReader.cpp
  ${ROUTE_HOME}/RouteTcl.cpp
)

target_link_libraries(route
  PRIVATE
  lef
  lefzlib
  def
  defzlib
  ${TCL_LIBRARY}
  OpenSTA
)

target_include_directories(route
  PUBLIC
  ${ROUTE_HOME}
  PRIVATE
  ${TCL_INCLUDE_PATH}
)

