# Build the Python bindings, which require pybind11
find_package(pybind11 QUIET)
if (pybind11_FOUND)
    # TODO: which pybind11 version
endif() #pybind11_FOUND
if (NOT pybind11_FOUND)
    message(FATAL_ERROR
    "pybind11 was not found. It is still possible to build and test libexactextract using -DBUILD_PYTHON=NO.")
endif() #NOT pybind11_FOUND


set(PYBIND_SOURCES
src/pybindings/bindings.cpp
src/pybindings/feature_bindings.cpp
src/pybindings/feature_bindings.h
src/pybindings/feature_source_bindings.cpp
src/pybindings/feature_source_bindings.h
src/pybindings/operation_bindings.cpp
src/pybindings/operation_bindings.h
src/pybindings/processor_bindings.cpp
src/pybindings/processor_bindings.h
src/pybindings/raster_source_bindings.cpp
src/pybindings/raster_source_bindings.h
src/pybindings/writer_bindings.cpp
src/pybindings/writer_bindings.h
)

pybind11_add_module(_exactextract MODULE ${PYBIND_SOURCES})
target_include_directories(_exactextract PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SRC_GENERATED})
set_property(TARGET ${LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(_exactextract PRIVATE ${LIB_NAME} GEOS::geos_c)
install(TARGETS _exactextract DESTINATION exactextract)

add_test(NAME "pybindings" 
         COMMAND ${CMAKE_COMMAND} -E env
         PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_LIST_DIR}/src
         python3 -m pytest ${CMAKE_CURRENT_LIST_DIR}/tests)
