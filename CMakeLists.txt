cmake_minimum_required(VERSION 3.8)
project(exactextract)
set(DEFAULT_BUILD_TYPE "Release")

set(LIB_NAME exactextract)
set(BIN_NAME exactextract_bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
    # gcc 4.9 doesn't fully support C++14, yet CMake doesn't bail when we
    # set CMAKE_CXX_STANDARD_REQUIRED
    # https://cmake.org/pipermail/cmake/2017-March/065102.html
    message(FATAL_ERROR "gcc 5.0+ is required to build exactextract")
endif()

include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(VersionSource)
find_package(GEOS REQUIRED)

#Configure some options the various components this module can build
option(BUILD_CLI "Build the exactextract cli binary" ON) #requires gdal, cli11
option(BUILD_TEST "Build the exactextract tests" ON) #requires catch
option(BUILD_DOC "Build documentation" ON) #requires doxygen

if(BUILD_CLI)
    # Create our main program, statically linked to our library
    # Unlike the library, this depends on GDAL
    find_package(GDAL)
    if (GDAL_FOUND)
        # Check GDAL version (requires CMake 3.14)
        if (${CMAKE_VERSION} VERSION_LESS 3.14.0)
                message(WARNING "GDAL 2.0+ is required but detected GDAL version is unknown.")
        elseif(${GDAL_VERSION} VERSION_LESS 2.0)
                unset(GDAL_FOUND)
        endif()
    endif() #GDAL_FOUND
    if (NOT GDAL_FOUND)
        message(FATAL_ERROR
        "GDAL version >= 2.0 was not found. It is still possible to build and test libexactextract, but the "
        "exactextract executable cannot be built or installed.")
    endif() #NOT GDAL_FOUND

    # Download CLI11 (header-only library)
    set(CLI11_INCLUDE_DIR ${CMAKE_BINARY_DIR}/CLI11)
    set(CLI11_INCLUDE ${CLI11_INCLUDE_DIR}/CLI11.hpp)

    if (NOT EXISTS ${CLI11_INCLUDE})
        file(DOWNLOAD https://github.com/CLIUtils/CLI11/releases/download/v2.3.2/CLI11.hpp
                ${CLI11_INCLUDE}
                SHOW_PROGRESS)
    endif()

    #Configure the exactextract CLI target
    add_library(exactextract_gdal OBJECT
        src/deferred_gdal_writer.h
        src/deferred_gdal_writer.cpp
        src/gdal_raster_wrapper.h
        src/gdal_raster_wrapper.cpp
        src/gdal_dataset_wrapper.h
        src/gdal_dataset_wrapper.cpp
        src/gdal_feature.h
        src/gdal_writer.h
        src/gdal_writer.cpp
    )

    target_link_libraries(exactextract_gdal
        PRIVATE ${GDAL_LIBRARY})

    target_include_directories(exactextract_gdal
        PUBLIC ${GDAL_INCLUDE_DIR})

    add_executable(${BIN_NAME} src/exactextract.cpp)
    set_target_properties(${BIN_NAME} PROPERTIES OUTPUT_NAME "exactextract")

    add_executable(subdivide src/subdivide.cpp)
    set_target_properties(subdivide PROPERTIES OUTPUT_NAME "subdivide")

    target_link_libraries(
        subdivide
        PRIVATE ${GDAL_LIBRARY} ${LIB_NAME})

    target_include_directories(
        subdivide
        PRIVATE
        ${GDAL_INCLUDE_DIR}
        ${CMAKE_BINARY_DIR}/generated
        )

    target_link_libraries(
            ${BIN_NAME}
            PRIVATE
            ${LIB_NAME}
            exactextract_gdal
    )

    target_include_directories(
            ${BIN_NAME}
            PRIVATE
            ${CMAKE_BINARY_DIR}/generated
            ${CMAKE_SOURCE_DIR}/src
    )

    # Include CLI11 as a system include so that -Wshadow warnings are suppressed.
    target_include_directories(
            ${BIN_NAME}
            SYSTEM PRIVATE ${CLI11_INCLUDE_DIR}
    )

    target_include_directories(
        subdivide
        SYSTEM PRIVATE ${CLI11_INCLUDE_DIR})

    target_compile_options(
            ${BIN_NAME}
            PRIVATE
            $<$<CXX_COMPILER_ID:GNU>:-Werror -Wall -Wextra -Wshadow>
            $<$<CXX_COMPILER_ID:Clang>:-Werror -Wall -Wextra -Wshadow -Wdouble-promotion>)

    install(TARGETS ${BIN_NAME}
            RUNTIME
            DESTINATION bin)

endif() #BUILD_CLI

if(BUILD_TEST)
    #Build the test suite

    # Download Catch (header-only library)
    set(CATCH_INCLUDE_DIR ${CMAKE_BINARY_DIR}/catch)
    set(CATCH_INCLUDE ${CATCH_INCLUDE_DIR}/catch.hpp)

    if (NOT EXISTS ${CATCH_INCLUDE})
        file(DOWNLOAD https://github.com/catchorg/Catch2/releases/download/v2.13.8/catch.hpp
            ${CATCH_INCLUDE}
            SHOW_PROGRESS)
    endif()

    set(TEST_SOURCES
    test/test_box.cpp
    test/test_cell.cpp
    test/test_geos_utils.cpp
    test/test_grid.cpp
    test/test_main.cpp
    test/test_operation.cpp
    test/test_perimeter_distance.cpp
    test/test_raster.cpp
    test/test_raster_area.cpp
    test/test_raster_cell_intersection.cpp
    test/test_raster_iterator.cpp
    test/test_traversal_areas.cpp
    test/test_stats.cpp
    test/test_utils.cpp)

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
                ${LIB_NAME}
                ${GEOS_LIBRARY}
    )

endif() #BUILD_TEST

message(STATUS "Source version: " ${EXACTEXTRACT_VERSION_SOURCE})
configure_file(src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h)

if (GEOS_VERSION_MAJOR LESS 3 OR GEOS_VERSION_MINOR LESS 5)
   message(FATAL_ERROR "GEOS version 3.5 or later is required.")
endif()

# Define coverage build type
set(CMAKE_CXX_FLAGS_COVERAGE "-fprofile-arcs -ftest-coverage")

# Make sure we know our build type
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}")
endif()

set(PROJECT_SOURCES
        src/measures.cpp
        src/measures.h
        src/box.h
        src/box.cpp
        src/cell.cpp
        src/cell.h
        src/coordinate.cpp
        src/coordinate.h
        src/coverage_operation.h
        src/coverage_processor.h
        src/coverage_processor.cpp
        src/crossing.h
        src/feature.h
        src/feature_sequential_processor.cpp
        src/feature_sequential_processor.h
        src/feature_source.h
        src/floodfill.cpp
        src/floodfill.h
        src/geos_utils.cpp
        src/geos_utils.h
        src/grid.h
        src/grid.cpp
        src/matrix.h
        src/map_feature.h
        src/perimeter_distance.cpp
        src/perimeter_distance.h
        src/processor.h
        src/raster.h
        src/raster_area.h
        src/raster_cell_intersection.cpp
        src/raster_cell_intersection.h
        src/raster_sequential_processor.cpp
        src/raster_sequential_processor.h
        src/raster_stats.h
        src/raster_coverage_iterator.h
        src/side.cpp
        src/side.h
        src/traversal.cpp
        src/traversal.h
        src/traversal_areas.cpp
        src/traversal_areas.h
        src/output_writer.h
        src/output_writer.cpp
        src/operation.h
        src/operation.cpp
        src/raster_source.h
        src/stats_registry.h
        src/stats_registry.cpp
        src/utils.h
        src/utils.cpp
        src/weighted_quantiles.h
        src/weighted_quantiles.cpp
        src/variance.h
        ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
        vend/optional.hpp)

add_library(${LIB_NAME} ${PROJECT_SOURCES})

# Check matrix bounds for debug builds
set_target_properties(${LIB_NAME}
        PROPERTIES COMPILE_DEFINITIONS $<$<CONFIG:Debug>:MATRIX_CHECK_BOUNDS>)

target_include_directories(
        ${LIB_NAME}
        PRIVATE
        ${GEOS_INCLUDE_DIR}
)

target_compile_definitions(
        ${LIB_NAME}
        PRIVATE
        GEOS_USE_ONLY_R_API
)

target_compile_options(
        ${LIB_NAME}
        PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Werror -Wall -Wextra -Wshadow -Wdouble-promotion>
        $<$<CXX_COMPILER_ID:Clang>:-Werror -Wall -Wextra -Wshadow -Wdouble-promotion>
)

target_link_libraries(
        ${LIB_NAME}
        PUBLIC
        ${GEOS_LIBRARY}
)

set_target_properties(${LIB_NAME} PROPERTIES OUTPUT_NAME ${LIB_NAME})

if(BUILD_DOC)
    # Doxygen configuration from https://vicrucann.github.io/tutorials/quick-cmake-doxygen/
    # check if Doxygen is installed
    find_package(Doxygen)
    if (DOXYGEN_FOUND)
        # set input and output files
        set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        # request to configure the file
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        message("Doxygen build started")

        # note the option ALL which allows to build the docs together with the application
        add_custom_target( doc_doxygen ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM )
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif() #BUILD_DOC
