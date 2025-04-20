cmake_minimum_required(VERSION 3.15)
project(exactextract)
set(DEFAULT_BUILD_TYPE "Release")

set(LIB_NAME exactextract)
set(BIN_NAME exactextract_bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include(CTest)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(VersionSource)
find_package(GEOS "3.5" REQUIRED)
if (GEOS_FOUND)
   message(STATUS "GEOS version: " ${GEOS_VERSION})
else()
   message(FATAL_ERROR "GEOS version 3.5 or later is required.")
endif()

message(STATUS "Source version: " ${EXACTEXTRACT_VERSION_SOURCE})
set(SRC_GENERATED ${CMAKE_CURRENT_BINARY_DIR}/generated)
configure_file(src/version.h.in ${SRC_GENERATED}/version.h)

# Define coverage build type
set(CMAKE_CXX_FLAGS_COVERAGE "-fprofile-arcs -ftest-coverage")

# Make sure we know our build type
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}")
endif()

#Configure some options the various components this module can build
option(BUILD_BENCHMARKS "Build exactextract benchmarks" OFF)
option(BUILD_CLI "Build the exactextract cli binary" ON) #requires gdal, cli11
option(BUILD_TEST "Build the exactextract tests" ON) #requires catch
option(BUILD_PYTHON "Build the exactextract Python bindings" ON) # requires pybind11
option(BUILD_DOC "Build documentation" ON) #requires doxygen

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # workaround for MacOS 10.13 and below which does not fully support std::variant
    add_compile_definitions(_LIBCPP_DISABLE_AVAILABILITY)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory("benchmark")
endif()

if(BUILD_CLI)
    # Create our main program, statically linked to our library
    # Unlike the library, this depends on GDAL
    find_package(GDAL CONFIG)
    if (GDAL_FOUND)
        message("GDAL version: ${GDAL_VERSION}")
        if(${GDAL_VERSION} VERSION_LESS 2.0)
            unset(GDAL_FOUND)
        endif()
    endif() #GDAL_FOUND
    if (NOT GDAL_FOUND)
        message(FATAL_ERROR
        "GDAL version >= 2.0 was not found. It is still possible to build and test libexactextract using -DBUILD_CLI=NO, but the "
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
        src/gdal_feature_unnester.h
        src/gdal_writer.h
        src/gdal_writer.cpp
        src/utils_cli.h
        src/utils_cli.cpp
    )

    target_link_libraries(exactextract_gdal
        PUBLIC GDAL::GDAL)

    add_executable(${BIN_NAME} src/exactextract.cpp)
    set_target_properties(${BIN_NAME} PROPERTIES OUTPUT_NAME "exactextract")

    add_executable(subdivide src/subdivide.cpp)
    set_target_properties(subdivide PROPERTIES OUTPUT_NAME "subdivide")

    target_link_libraries(
        subdivide
        PRIVATE GDAL::GDAL ${LIB_NAME})

    target_include_directories(
        subdivide
        PRIVATE
        ${SRC_GENERATED}
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
            ${SRC_GENERATED}
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

    add_test(NAME "cli_tests" 
             COMMAND pytest ${CMAKE_CURRENT_LIST_DIR}/test)

endif() #BUILD_CLI

if(BUILD_TEST)
    #Build the test suite
    add_subdirectory(test)
endif() #BUILD_TEST


set(PROJECT_SOURCES
        src/measures.cpp
        src/measures.h
        src/box.h
        src/box.cpp
        src/cell.cpp
        src/cell.h
        src/coordinate.cpp
        src/coordinate.h
        src/crossing.h
        src/feature.h
        src/feature.cpp
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
        src/memory_raster_source.h
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
        ${SRC_GENERATED}/version.h
        )

add_library(${LIB_NAME} ${PROJECT_SOURCES})


# Check matrix bounds for debug builds
set_target_properties(${LIB_NAME}
        PROPERTIES COMPILE_DEFINITIONS $<$<CONFIG:Debug>:MATRIX_CHECK_BOUNDS>)

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
        GEOS::geos_c
)

set_target_properties(${LIB_NAME} PROPERTIES OUTPUT_NAME ${LIB_NAME})

if (BUILD_PYTHON)
    add_subdirectory(python)
endif() # BUILD_PYTHON

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

        add_custom_command(
                OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/html
                DEPENDS ${LIB_NAME} $<$<BOOL:${BUILD_CLI}>:${BIN_NAME}>
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        )

        add_custom_target(doxygen
                ALL 
                DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/html)

    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")

        

    endif (DOXYGEN_FOUND)
endif() #BUILD_DOC
