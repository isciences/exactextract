# Download Catch (header-only library)
set(CATCH_INCLUDE_DIR ${CMAKE_BINARY_DIR}/catch)
set(CATCH_INCLUDE ${CATCH_INCLUDE_DIR}/catch.hpp)

if (NOT EXISTS ${CATCH_INCLUDE})
    file(DOWNLOAD https://github.com/catchorg/Catch2/releases/download/v2.13.8/catch.hpp
    ${CATCH_INCLUDE}
    SHOW_PROGRESS)
endif()

set(TEST_SOURCES
test_box.cpp
test_cell.cpp
test_geos_utils.cpp
test_grid.cpp
test_main.cpp
test_operation.cpp
test_perimeter_distance.cpp
test_raster.cpp
test_raster_area.cpp
test_raster_cell_intersection.cpp
test_raster_iterator.cpp
test_traversal_areas.cpp
test_stats.cpp
test_utils.cpp)

# Create an executable to run the unit tests
add_executable(catch_tests ${TEST_SOURCES})

add_test(NAME "catch_tests" COMMAND catch_tests)

target_include_directories(
        catch_tests
        PRIVATE
            ${CATCH_INCLUDE_DIR}
            ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(
        catch_tests
        PRIVATE
            ${LIB_NAME}
            GEOS::geos_c
)

if (GDAL_FOUND)
    target_sources(catch_tests PRIVATE test_gdal.cpp)
    target_link_libraries(catch_tests PRIVATE exactextract_gdal ${GDAL_LIBRARY})
    target_include_directories(exactextract_gdal PRIVATE ${GDAL_INCLUDE_DIR})
endif()

