#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <bbp/sonata/common.h>

TEST_CASE("SONATA", "version") {
    CHECK(!bbp::sonata::version().empty());
}
