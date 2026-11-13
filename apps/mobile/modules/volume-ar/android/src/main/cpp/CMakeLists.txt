cmake_minimum_required(VERSION 3.22)

project(volume_core_jni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../core)

add_library(volume_core STATIC
  ${CORE_DIR}/src/geom.cpp
  ${CORE_DIR}/src/triangulate.cpp
  ${CORE_DIR}/src/plane.cpp
  ${CORE_DIR}/src/polygon.cpp
  ${CORE_DIR}/src/solid.cpp
  ${CORE_DIR}/src/measurement.cpp
  ${CORE_DIR}/src/volume_c.cpp
)

target_include_directories(volume_core PUBLIC ${CORE_DIR}/include)

add_library(volume_core_jni SHARED VolumeCoreJNI.cpp)
target_link_libraries(volume_core_jni volume_core log)
