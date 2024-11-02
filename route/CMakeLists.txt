set(ROUTE_HOME ${CMAKE_CURRENT_SOURCE_DIR})

##################
# route
##################

find_package(LEMON CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED)

add_library(route
  ${ROUTE_HOME}/context/Context.cpp
  ${ROUTE_HOME}/object/GRNetwork.cpp
  ${ROUTE_HOME}/object/GRTechnology.cpp
  ${ROUTE_HOME}/object/GRTree.cpp
  ${ROUTE_HOME}/lefdef/LefDatabase.cpp
  ${ROUTE_HOME}/lefdef/DefDatabase.cpp
  ${ROUTE_HOME}/timing/MakeWireParasitics.cpp
  ${ROUTE_HOME}/tcl/RouteTcl.cpp
  ${ROUTE_HOME}/util/log.cpp

  ${ROUTE_HOME}/stt/pd.cpp
  ${ROUTE_HOME}/stt/flute.cpp

  ${ROUTE_HOME}/cugr2/GlobalRouter.cpp
  ${ROUTE_HOME}/cugr2/GridGraph.cpp
  ${ROUTE_HOME}/cugr2/MazeRoute.cpp
  ${ROUTE_HOME}/cugr2/PatternRoute.cpp
)

add_library(special_warnings INTERFACE)
target_compile_options(special_warnings INTERFACE
  -Wall
  -Wextra
  # -Weffc++
  -Wuninitialized
  -Wreturn-type
  # -Wconversion
  -Wsign-compare
  -Wunused-result
  -Wsuggest-override
  -Wzero-as-null-pointer-constant
  -Wmissing-declarations
  -Wold-style-cast
  -Wnon-virtual-dtor
  # -Werror
)

target_link_libraries(route PRIVATE
  special_warnings
  lef
  lefzlib
  def
  defzlib
  ${TCL_LIBRARY}
  OpenSTA
  ${LEMON_LIBRARY}
  ${Boost_LIBRARIES}
)

target_include_directories(route PUBLIC ${ROUTE_HOME})

target_include_directories(route PRIVATE
  ${TCL_INCLUDE_PATH}
  ${LEMON_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
)


add_custom_command(
  TARGET route POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy 
    ${ROUTE_HOME}/stt/POST9.dat 
    $<TARGET_FILE_DIR:route>/POST9.dat
  COMMAND ${CMAKE_COMMAND} -E copy 
    ${ROUTE_HOME}/stt/POWV9.dat
    $<TARGET_FILE_DIR:route>/POWV9.dat
)

