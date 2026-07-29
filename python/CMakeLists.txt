set(SONATA_PYTHON_BUILD ${CMAKE_CURRENT_BINARY_DIR}/libsonata)

set(PYBIND11_FINDPYTHON ON)
if (EXTLIB_FROM_SUBMODULES)
    add_subdirectory(pybind11 EXCLUDE_FROM_ALL)
else()
    find_package(pybind11 REQUIRED CONFIG)
endif()

pybind11_add_module(_libsonata bindings.cpp)

target_link_libraries(_libsonata
    PRIVATE sonata
    PRIVATE HighFive
    PRIVATE fmt::fmt-header-only
    PRIVATE pybind11::module
    PRIVATE nlohmann_json::nlohmann_json
)

install(TARGETS _libsonata LIBRARY DESTINATION libsonata)
