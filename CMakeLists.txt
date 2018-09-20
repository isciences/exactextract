cmake_minimum_required(VERSION 3.7)
project(exactextract)

set(LIB_NAME exactextract)
set(BIN_NAME exactextract_bin)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(GEOS REQUIRED)
find_package(GDAL)

# Download Catch (header-only library)
set(CATCH_INCLUDE_DIR ${CMAKE_BINARY_DIR}/catch)
set(CATCH_INCLUDE ${CATCH_INCLUDE_DIR}/catch.hpp)

if (NOT EXISTS ${CATCH_INCLUDE})
    file(DOWNLOAD https://github.com/catchorg/Catch2/releases/download/v2.2.2/catch.hpp
         ${CATCH_INCLUDE}
         SHOW_PROGRESS)
endif()

# Download CLI11 (header-only library)
set(CLI11_INCLUDE_DIR ${CMAKE_BINARY_DIR}/CLI11)
set(CLI11_INCLUDE ${CLI11_INCLUDE_DIR}/CLI11.hpp)

if (NOT EXISTS ${CLI11_INCLUDE})
    file(DOWNLOAD https://github.com/CLIUtils/CLI11/releases/download/v1.6.0/CLI11.hpp
            ${CLI11_INCLUDE}
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
        src/floodfill.cpp
        src/floodfill.h
        src/geos_utils.cpp
        src/geos_utils.h
        src/grid.h
        src/grid.cpp
        src/matrix.h
        src/perimeter_distance.cpp
        src/perimeter_distance.h
        src/raster.h
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
        test/test_grid.cpp
        test/test_main.cpp
        test/test_perimeter_distance.cpp
        test/test_raster.cpp
        test/test_raster_cell_intersection.cpp
        test/test_traversal_areas.cpp
        test/test_stats.cpp)

# Make a shared and static version of the library
foreach(LINKING SHARED STATIC)
    add_library(${LIB_NAME}_${LINKING} ${LINKING} ${PROJECT_SOURCES})

    target_include_directories(
            ${LIB_NAME}_${LINKING}
            PRIVATE
            ${GEOS_INCLUDE_DIR}
    )

    set_target_properties(${LIB_NAME}_${LINKING} PROPERTIES OUTPUT_NAME ${LIB_NAME})
endforeach(LINKING)

# Create our main program, statically linked to our library
# Unlike the library, this depends on GDAL
if (GDAL_FOUND)
    add_executable(${BIN_NAME}
            src/exactextract.cpp
            )
    set_target_properties(${BIN_NAME} PROPERTIES OUTPUT_NAME "exactextract")

    target_link_libraries(
            ${BIN_NAME}
            PRIVATE
                ${LIB_NAME}_STATIC
                ${GDAL_LIBRARY}
                ${GEOS_LIBRARY}
    )

    target_include_directories(
            ${BIN_NAME}
            PRIVATE
                ${GDAL_INCLUDE_DIR}
                ${GEOS_INCLUDE_DIR}
                ${CLI11_INCLUDE_DIR}
                ${CMAKE_SOURCE_DIR}/src
    )

    install(TARGETS ${BIN_NAME}
            RUNTIME
            DESTINATION bin)
endif(GDAL_FOUND)

# Create an executable to run the unit tests
add_executable(catch_tests ${TEST_SOURCES})

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
            ${LIB_NAME}_SHARED
            ${GEOS_LIBRARY}
)
