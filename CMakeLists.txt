cmake_minimum_required(VERSION 3.16)
cmake_policy(VERSION 3.16)

if(DEFINED SKBUILD_PROJECT_VERSION)
    set(SONATA_VERSION "${SKBUILD_PROJECT_VERSION}")
endif()

if(NOT SONATA_VERSION)
    execute_process(COMMAND git describe --tags
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_VERSION_FAILED
                    OUTPUT_VARIABLE GIT_PKG_VERSION_FULL
                    ERROR_VARIABLE GIT_VERSION_ERROR
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(GIT_VERSION_FAILED)
      message(
        FATAL_ERROR
          "Could not retrieve version from command 'git describe --tags'\n"
          ${GIT_VERSION_ERROR})
    endif()

    # keep last line of command output
    string(REPLACE "\n"
                   ";"
                   GIT_PKG_VERSION_FULL
                   "${GIT_PKG_VERSION_FULL}")
    list(GET GIT_PKG_VERSION_FULL -1 SONATA_VERSION)
endif()

# keep MAJOR.MINOR.PATCH (PATCH being optional)
string(REGEX
       REPLACE "v?([0-9]+\\.[0-9]+(\\.[0-9]+)?).*"
               "\\1"
               SONATA_VERSION
               "${SONATA_VERSION}")

# Get "major.minor" from string "major.minor.version"
string(REGEX MATCH "^(.*)\\.[^.]*$" dummy ${SONATA_VERSION})
set(SONATA_VERSION_ABI ${CMAKE_MATCH_1})

project(sonata VERSION ${SONATA_VERSION})

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(EXTLIB_FROM_SUBMODULES "Use Git submodules for header-only dependencies" OFF)
option(SONATA_PYTHON "Build Python extensions" OFF)
option(SONATA_TESTS "Build tests" ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(SONATA_ENABLE_COVERAGE_DEFAULT ON)
else()
  set(SONATA_ENABLE_COVERAGE_DEFAULT OFF)
endif()
option(SONATA_ENABLE_COVERAGE "Enable measuring test coverage." ${SONATA_ENABLE_COVERAGE_DEFAULT})

# For `Findcoverage.cmake` and friends.
set(ENABLE_COVERAGE ${SONATA_ENABLE_COVERAGE})

option(SONATA_CXX_WARNINGS "Compile C++ with warnings as errors, for glibcxx turn on assertions" ON)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SONATA_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

set(SONATA_COMPILE_OPTIONS -Wall -Wextra -pedantic)
if(SONATA_CXX_WARNINGS)
    set(SONATA_COMPILE_OPTIONS ${SONATA_COMPILE_OPTIONS} -Werror -Wp,-D_GLIBCXX_ASSERTIONS)
endif()

# =============================================================================
# Dependencies
# =============================================================================

if (EXTLIB_FROM_SUBMODULES)
    add_subdirectory(extlib EXCLUDE_FROM_ALL)
else()
    find_package(HighFive REQUIRED)
    find_package(fmt REQUIRED)
    find_package(nlohmann_json REQUIRED)
endif()

# =============================================================================
# Targets
# =============================================================================

set(SONATA_SRC
    src/common.cpp
    src/compartment_sets.cpp
    src/config.cpp
    src/edge_index.cpp
    src/edges.cpp
    src/electrode_reader.cpp
    src/hdf5_mutex.cpp
    src/hdf5_reader.cpp
    src/node_sets.cpp
    src/nodes.cpp
    src/population.cpp
    src/report_reader.cpp
    src/selection.cpp
    src/utils.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/src/version.cpp
    )

configure_file (
  ${CMAKE_CURRENT_SOURCE_DIR}/src/version.cpp.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/version.cpp
  )

add_library(sonata ${SONATA_SRC})
set_target_properties(sonata PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
)
target_include_directories(sonata
    PUBLIC $<BUILD_INTERFACE:${SONATA_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_options(sonata PRIVATE ${SONATA_COMPILE_OPTIONS})

if(BUILD_SHARED_LIBS)
    set_target_properties(sonata
        PROPERTIES
            VERSION ${SONATA_VERSION}
            SOVERSION ${SONATA_VERSION_ABI}
        )
    target_compile_definitions(sonata 
        PUBLIC SONATA_DLL 
        PRIVATE SONATA_DLL_EXPORTS)
    target_link_libraries(sonata 
        PRIVATE 
            HighFive 
            fmt::fmt-header-only 
            nlohmann_json::nlohmann_json
        )
else()
    target_compile_definitions(sonata PRIVATE FMT_HEADER_ONLY=1)
    target_include_directories(sonata PRIVATE
        $<TARGET_PROPERTY:fmt::fmt-header-only,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:HighFive,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:nlohmann_json::nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
    )
endif()

if(ENABLE_COVERAGE)
    target_compile_options(sonata PRIVATE -g -O0 --coverage -fprofile-arcs -ftest-coverage)
    target_link_libraries(sonata PRIVATE gcov)
endif()

add_library(sonata::sonata ALIAS sonata)

# =============================================================================
# Install
# =============================================================================

install(TARGETS sonata
    EXPORT sonata-targets
    LIBRARY
        DESTINATION lib
    ARCHIVE
        DESTINATION lib
)

install(DIRECTORY ${SONATA_INCLUDE_DIR}/bbp
    DESTINATION include
)

install(FILES CMake/sonata-config.cmake
    DESTINATION share/sonata/CMake
)

install(EXPORT sonata-targets
    DESTINATION share/sonata/CMake
    NAMESPACE sonata::
)

# =============================================================================
# Testing
# =============================================================================

if (SONATA_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# =============================================================================
# Python bindings
# =============================================================================

if (SONATA_PYTHON)
    add_subdirectory(python)
endif()
