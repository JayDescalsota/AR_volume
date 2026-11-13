cmake_minimum_required(VERSION 3.20)

project(volume_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(volume_core STATIC
    src/geom.cpp
    src/triangulate.cpp
    src/plane.cpp
    src/polygon.cpp
    src/solid.cpp
    src/measurement.cpp
    src/volume_c.cpp
)

target_include_directories(volume_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(volume_core PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(volume_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(volume_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Static library consumed by the iOS and Android native modules and by the JSI
# bridge. Position-independent code for Android shared-object linking.
set_target_properties(volume_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

option(VOLUME_CORE_BUILD_TESTS "Build the volume core tests" ON)

if(VOLUME_CORE_BUILD_TESTS)
    enable_testing()
    add_executable(volume_tests
        tests/test_main.cpp
        tests/test_math.cpp
        tests/test_volume.cpp
    )
    target_link_libraries(volume_tests PRIVATE volume_core)
    add_test(NAME volume_tests COMMAND volume_tests)
endif()
