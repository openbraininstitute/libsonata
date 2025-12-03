Include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.11.0
)

FetchContent_MakeAvailable(Catch2)

set(TESTS_SRC
  main.cpp
  test_compartment_sets.cpp
  test_config.cpp
  test_edges.cpp
  test_node_sets.cpp
  test_nodes.cpp
  test_report_reader.cpp
  test_selection.cpp
)

add_executable(unittests ${TESTS_SRC})
target_link_libraries(unittests
    PRIVATE
    sonata_shared
    HighFive
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

if (ENABLE_COVERAGE)
    include(CodeCoverage)
    SETUP_TARGET_FOR_COVERAGE_LCOV(
        NAME coverage
        EXECUTABLE ctest
        DEPENDENCIES unittests
        EXCLUDE "/usr/*" "${PROJECT_SOURCE_DIR}/include/*" "${PROJECT_SOURCE_DIR}/extlib/*"
    )
endif()

include(Catch)
catch_discover_tests(unittests
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )

