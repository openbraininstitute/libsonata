file(GLOB TESTS_SRC "*.cpp")

if(NOT EXTLIB_FROM_SUBMODULES)
  # When using submodules `include(.../Catch)` is performed
  # in `extlib/CMakeLists.txt`.
  include(Catch)
endif()

add_executable(unittests ${TESTS_SRC})
target_link_libraries(unittests
    PRIVATE
    sonata_shared
    HighFive
    Catch2::Catch2
    nlohmann_json::nlohmann_json
)

catch_discover_tests(unittests
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )
