cmake_minimum_required(VERSION 3.10)
project(exactextract)

set(LIB_NAME exactextract)

include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(GEOS REQUIRED)
find_package(GDAL REQUIRED)

set(CATCH_INCLUDE_DIR ${CMAKE_BINARY_DIR}/catch)
set(CATCH_INCLUDE ${CATCH_INCLUDE_DIR}/catch.hpp)

if (NOT EXISTS ${CATCH_INCLUDE})
    file(DOWNLOAD https://github.com/catchorg/Catch2/releases/download/v2.2.2/catch.hpp
         ${CATCH_INCLUDE}
         SHOW_PROGRESS)
endif()

set(PROJECT_SOURCES
        src/area.cpp
        src/area.h
        src/box.h
        src/box.cpp
        src/cell.cpp
        src/cell.h
        src/coordinate.cpp
        src/coordinate.h
        src/crossing.h
        src/extent.cpp
        src/extent.h
        src/floodfill.cpp
        src/floodfill.h
        src/geos_utils.cpp
        src/geos_utils.h
        src/matrix.h
        src/perimeter_distance.cpp
        src/perimeter_distance.h
        src/raster_cell_intersection.cpp
        src/raster_cell_intersection.h
        src/raster_stats.h
        src/segment_orientation.h
        src/side.cpp
        src/side.h
        src/traversal.cpp
        src/traversal.h
        src/traversal_areas.cpp
        src/traversal_areas.h
)

set(TEST_SOURCES
        test/test_box.cpp
        test/test_cell.cpp
        test/test_extent.cpp
        test/test_main.cpp
        test/test_perimeter_distance.cpp
        test/test_raster_cell_intersection.cpp
        test/test_traversal_areas.cpp
)

add_library(${LIB_NAME} SHARED ${PROJECT_SOURCES})

target_compile_features(
        ${LIB_NAME} PUBLIC cxx_std_14
)

target_include_directories(
        ${LIB_NAME}
        PRIVATE
            ${GEOS_INCLUDE_DIR}
)

add_executable(catch_tests ${TEST_SOURCES})

add_executable(zonal_example
        example/zonal_example.cpp
    )

target_compile_features(
        zonal_example PUBLIC cxx_std_14
)

target_link_libraries(
        zonal_example
        PRIVATE
            ${LIB_NAME}
            ${GDAL_LIBRARY}
            ${GEOS_LIBRARY}
)

target_include_directories(
        zonal_example
        PRIVATE
            ${GDAL_INCLUDE_DIR}
            ${GEOS_INCLUDE_DIR}
            ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(
        catch_tests PUBLIC cxx_std_14
)

target_include_directories(
        catch_tests
        PRIVATE
            ${CATCH_INCLUDE_DIR}
            ${GEOS_INCLUDE_DIR}
            ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(
        catch_tests
        PRIVATE
            ${LIB_NAME}
            ${GEOS_LIBRARY}
)

