set(ROUTE_HOME ${CMAKE_CURRENT_SOURCE_DIR})

##################
# route
##################

add_library(route
  ${ROUTE_HOME}/app/App.cpp
  ${ROUTE_HOME}/lefdef/LefDefDatabase.cpp
  ${ROUTE_HOME}/object/GRContext.cpp
  ${ROUTE_HOME}/timing/MakeWireParasitics.cpp
  ${ROUTE_HOME}/tcl/RouteTcl.cpp
  ${ROUTE_HOME}/util/log.cpp
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

