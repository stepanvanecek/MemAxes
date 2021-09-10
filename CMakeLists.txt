project(MemAxes C CXX)

# Qt5 requires CMake 2.8.9 or higher.
cmake_minimum_required(VERSION 3.1)

# Qt5 + Modules
find_package(Qt5 REQUIRED Core Widgets OpenGL Xml)

# OpenGL
find_package(OpenGL)

# sys-topo
#find_library(NAMES sys-topo)
#find_package(sys-topo REQUIRED)
##include(FindPkgConfig)
##pkg_search_module(SYS_TOPO REQUIRED sys-topo)
#include_directories(${SYS_TOPO_INCLUDE_DIRS})
#target_link_libraries(${PROJECT_NAME} ${SYS_TOPO_LIBRARIES})
#execute_process(COMMAND xml2-config --cflags OUTPUT_VARIABLE XML2_CFLAGS)
#execute_process(COMMAND xml2-config --libs OUTPUT_VARIABLE XML2_LIBS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include -I/Users/stepan/Documents/phd/sys-topo/install/include")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib -lxml2 -lz -lpthread -licucore -lm -L/Users/stepan/Documents/phd/sys-topo/install/lib -lsys-topo")
message(STATUS "CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}")

# Top-level build just includes subdirectories.
add_subdirectory(src)
add_subdirectory(example_data)
