#include <catch2/catch.hpp>
#include <bbp/sonata/compartment_sets.h>
#include <string>
#include <fstream>
#include <stdexcept>

using namespace bbp::sonata;

TEST_CASE("CompartmentLocation: construction and JSON round-trip") {
    CompartmentLocation loc(42, 3, 0.75);
    REQUIRE(loc.gid() == 42);
    REQUIRE(loc.sectionIdx() == 3);
    REQUIRE(loc.offset() == Approx(0.75));

    std::string json = loc.toJSON();
    CompartmentLocation parsed(json);
    REQUIRE(parsed == loc);
}

TEST_CASE("CompartmentSet: valid JSON parsing and access") {
    std::string json = R"({
        "population": "exc",
        "compartment_set": [
            [1, 0, 0.0],
            [2, 1, 0.5],
            [1, 2, 1.0]
        ]
    })";

    CompartmentSet set(json);
    REQUIRE(set.population() == "exc");

    auto gids = set.gids();
    REQUIRE(gids.flatten().size() == 2);
    REQUIRE(gids.contains(1));
    REQUIRE(gids.contains(2));

    auto locs = set.getCompartmentLocations();
    REQUIRE(locs.size() == 3);
    REQUIRE(locs[0].gid() == 1);
    REQUIRE(locs[1].sectionIdx() == 1);
    REQUIRE(locs[2].offset() == Approx(1.0));
}

TEST_CASE("CompartmentSets: parse multiple sets and query") {
    std::string json = R"({
        "setA": {
            "population": "pop1",
            "compartment_set": [[10, 0, 0.0]]
        },
        "setB": {
            "population": "pop1",
            "compartment_set": [[11, 1, 0.5], [12, 2, 0.7]]
        }
    })";

    CompartmentSets sets(json);
    REQUIRE(sets.size() == 2);
    REQUIRE(sets.contains("setA"));
    REQUIRE(sets.contains("setB"));

    auto names = sets.names();
    REQUIRE(names.size() == 2);
    REQUIRE(names.find("setA") != names.end());
    REQUIRE(names.find("setB") != names.end());

    auto setA = sets.getCompartmentSet("setA");
    REQUIRE(setA.gids().contains(10));

    auto setB = sets.getCompartmentSet("setB");
    auto locs = setB.getCompartmentLocations();
    REQUIRE(locs.size() == 2);
    REQUIRE(locs[1].offset() == Approx(0.7));
}

TEST_CASE("CompartmentSets: round-trip serialization") {
    std::string json = R"({
        "cs0": {
            "population": "P",
            "compartment_set": [[1, 1, 0.1]]
        }
    })";

    CompartmentSets sets(json);
    std::string out = sets.toJSON();
    CompartmentSets reloaded(out);

    REQUIRE(reloaded.contains("cs0"));
    auto set = reloaded.getCompartmentSet("cs0");
    auto locs = set.getCompartmentLocations();
    REQUIRE(locs.size() == 1);
    REQUIRE(locs[0].gid() == 1);
    REQUIRE(locs[0].offset() == Approx(0.1));
}

TEST_CASE("CompartmentSets: load from valid file") {
    CompartmentSets sets = CompartmentSets::fromFile("./data/compartment_sets.json");
    REQUIRE(sets.size() > 0);

    for (const auto& name : sets.names()) {
        auto set = sets.getCompartmentSet(name);
        REQUIRE_FALSE(set.getCompartmentLocations().empty());
    }
}


TEST_CASE("CompartmentSet and CompartmentSets: throw SonataError on malformed JSON") {
    SECTION("CompartmentSet: missing offset") {
        std::string json = R"({ "population": "P", "compartment_set": [ [1, 1] ] })";
        CHECK_THROWS_AS(CompartmentSet(json), SonataError);
    }

    SECTION("CompartmentSet: malformed array structure") {
        std::string json = R"({ "population": "P", "compartment_set": [ 1, 2, 3 ] })";
        CHECK_THROWS_AS(CompartmentSet(json), SonataError);
    }

    SECTION("CompartmentSet: missing population field") {
        std::string json = R"({ "compartment_set": [ [1, 1, 0.5] ] })";
        CHECK_THROWS_AS(CompartmentSet(json), SonataError);
    }

    SECTION("CompartmentSets: missing compartment_set in one entry") {
        std::string json = R"({ "set1": { "population": "P" } })";
        CHECK_THROWS_AS(CompartmentSets(json), SonataError);
    }

    SECTION("CompartmentSets: invalid inner structure") {
        std::string json = R"({ "set1": [1, 2, 3] })";
        CHECK_THROWS_AS(CompartmentSets(json), SonataError);
    }

    SECTION("CompartmentSets: malformed JSON string") {
        std::string json = R"({ "set1": { "population": "P", "compartment_set": [ [1, 1 ] })"; // unclosed bracket
        CHECK_THROWS_AS(CompartmentSets(json), std::exception);
    }
}
