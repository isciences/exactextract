# Build the Python bindings, which require pybind11
set(PYBIND11_FINDPYTHON ON)
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
set_target_properties(_exactextract PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/exactextract)
set_property(TARGET ${LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(_exactextract PRIVATE ${LIB_NAME} GEOS::geos_c)
install(TARGETS _exactextract DESTINATION exactextract)

add_test(NAME "pybindings"
         COMMAND ${CMAKE_COMMAND} -E env
         PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}
         python3 -m pytest ${CMAKE_CURRENT_LIST_DIR}/tests)


# Symlink Python files from the package into the folder where the
# _exactextract lib will be placed. This allows tests to be run
# on the package without installing.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/exactextract")
file(GLOB py_paths "${CMAKE_CURRENT_LIST_DIR}/src/exactextract/*.py")
foreach(py_path IN ITEMS ${py_paths})
    get_filename_component(py_file ${py_path} NAME)
    file(CREATE_LINK ${py_path} ${CMAKE_CURRENT_BINARY_DIR}/exactextract/${py_file} SYMBOLIC)
endforeach()

if(BUILD_DOC)
    file(CREATE_LINK ${CMAKE_CURRENT_LIST_DIR}/doc ${CMAKE_CURRENT_BINARY_DIR}/doc SYMBOLIC)

    set(_RST_DIR ${CMAKE_CURRENT_BINARY_DIR}/doc)
    set(_HTML_DIR ${CMAKE_CURRENT_BINARY_DIR}/html)

    add_custom_command(
        OUTPUT ${_HTML_DIR}/index.html
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/doc
        DEPENDS ${CMAKE_CURRENT_LIST_DIR}/src/exactextract
        COMMAND sphinx-apidoc -f -o ${_RST_DIR} ${CMAKE_CURRENT_BINARY_DIR}/exactextract
        COMMAND sphinx-build -b html ${_RST_DIR} ${_HTML_DIR})

    add_custom_target(python_html DEPENDS ${_HTML_DIR}/index.html)

    unset(_RST_DIR)
    unset(_HTML_DIR)
endif() #BUILD_DOC
