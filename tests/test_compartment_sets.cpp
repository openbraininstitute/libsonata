#include <catch2/catch.hpp>
#include <bbp/sonata/compartment_sets.h>
#include <string>

#include <nlohmann/json.hpp>

using namespace bbp::sonata;

TEST_CASE("CompartmentLocation public API") {

    SECTION("Construct from valid gid, section_idx, offset") {
        CompartmentLocation loc(1, 10, 0.5);
        REQUIRE(loc.gid() == 1);
        REQUIRE(loc.sectionIdx() == 10);
        REQUIRE(loc.offset() == Approx(0.5));
    }

    SECTION("Construct from valid JSON string") {
        std::string json_str = "[1, 10, 0.5]";
        CompartmentLocation loc(json_str);
        REQUIRE(loc.gid() == 1);
        REQUIRE(loc.sectionIdx() == 10);
        REQUIRE(loc.offset() == Approx(0.5));
    }

    SECTION("Invalid JSON string throws") {
        REQUIRE_THROWS_AS(CompartmentLocation("{\"gid\": 1}"), SonataError);
        REQUIRE_THROWS_AS(CompartmentLocation("[1, 2]"), SonataError);
        REQUIRE_THROWS_AS(CompartmentLocation("[1, 2, 0.1, 1]"), SonataError);
        REQUIRE_THROWS_AS(CompartmentLocation("[\"a\", 2, 0.5]"), SonataError);
        REQUIRE_THROWS_AS(CompartmentLocation("[1, \"a\", 0.5]"), SonataError);
        REQUIRE_THROWS_AS(CompartmentLocation("[1, 2, \"a\"]"), SonataError);
        REQUIRE_THROWS_AS(CompartmentLocation("[1, 2, 2.0]"), SonataError);
        REQUIRE_THROWS_AS(CompartmentLocation("[1, 2, -0.1]"), SonataError);
        REQUIRE_THROWS_AS(CompartmentLocation("[-1, 2, 0.1]"), SonataError);
        REQUIRE_THROWS_AS(CompartmentLocation("[1, -2, 0.1]"), SonataError);
    }

    SECTION("Equality operators") {
        CompartmentLocation loc1(1, 10, 0.5);
        CompartmentLocation loc2(1, 10, 0.5);
        CompartmentLocation loc3(1, 10, 0.6);
        CompartmentLocation loc4(1, 10, 0.5000001);

        REQUIRE(loc1 == loc2);
        REQUIRE_FALSE(loc1 != loc2);
        REQUIRE(loc1 != loc3);
        REQUIRE(loc1 == loc4);
    }

    SECTION("Copy constructor and assignment") {
        CompartmentLocation original(2, 20, 0.7);
        CompartmentLocation copy_constructed(original);
        CompartmentLocation copy_assigned(0, 0, 0);
        copy_assigned = original;

        REQUIRE(copy_constructed == original);
        REQUIRE(copy_assigned == original);
    }

    SECTION("Move constructor and assignment") {
        CompartmentLocation original(3, 30, 0.8);
        CompartmentLocation moved_constructed(std::move(original));

        REQUIRE(moved_constructed.gid() == 3);
        REQUIRE(moved_constructed.sectionIdx() == 30);
        REQUIRE(moved_constructed.offset() == Approx(0.8));

        CompartmentLocation another(0, 0, 0);
        another = std::move(moved_constructed);

        REQUIRE(another.gid() == 3);
        REQUIRE(another.sectionIdx() == 30);
        REQUIRE(another.offset() == Approx(0.8));
    }

    SECTION("toJSON returns valid JSON string") {
        CompartmentLocation loc(4, 40, 0.9);
        auto json_str = loc.toJSON();

        auto json = nlohmann::json::parse(json_str);
        REQUIRE(json.is_array());
        REQUIRE(json[0] == 4);
        REQUIRE(json[1] == 40);
        REQUIRE(json[2] == Approx(0.9));
    }
}


// TEST_CASE("CompartmentSets: fail on invalid JSON strings") {
//     // Top level must be an object
//     REQUIRE_THROWS_AS(CompartmentSets("1"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets("[\"array\"]"), SonataError);

//     // Each CompartmentSet must be an object with 'population' and 'compartment_set' keys
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": 1 })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": "string" })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": null })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": true })"), SonataError);

//     // Missing keys
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "compartment_set": [] } })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0" } })"), SonataError);

//     // Invalid types
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": 123, "compartment_set": [] } })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": null, "compartment_set": [] } })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": "not an array" } })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": 123 } })"), SonataError);

//     // Invalid compartment_set elements
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [1] } })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [[1, 2]] } })"), SonataError);

//     // Wrong types inside compartment_set elements
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [["not uint64", 0, 0.5]] } })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [[1, "not uint64", 0.5]] } })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [[1, 0, "not a number"]] } })"), SonataError);

//     // Location out of bounds
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [[1, 0, -0.1]] } })"), SonataError);
//     REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [[1, 0, 1.1]] } })"), SonataError);
// }

// TEST_CASE("CompartmentSets: load from valid JSON file") {
//     const auto sets = CompartmentSets::fromFile("./data/compartment_sets.json");

//     REQUIRE(sets.size() == 2);

//     SECTION("Key presence") {
//         REQUIRE(sets.contains("cs0"));
//         REQUIRE(sets.contains("cs1"));
//     }

//     SECTION("Key ordering") {
//         const auto keys = sets.keys();
//         REQUIRE(keys == std::vector<std::string>{"cs0", "cs1"});
//     }

//     SECTION("Values and items") {
//         const auto values = sets.values();
//         REQUIRE(values.size() == 2);
//         REQUIRE(values[0].population() == "pop0");
//         REQUIRE(values[1].population() == "pop1");

//         const auto items = sets.items();
//         REQUIRE(items.size() == 2);
//         REQUIRE(items[0].first == "cs0");
//         REQUIRE(items[0].second.population() == "pop0");
//     }

//     SECTION("Get by name") {
//         const auto cs0 = sets.get("cs0");
//         REQUIRE(cs0.population() == "pop0");
//     }
// }

// TEST_CASE("CompartmentSets: round-trip serialization") {
//     std::string json = R"({
//         "cs0": {
//             "population": "P",
//             "compartment_set": [[1, 1, 0.1]]
//         }
//     })";

//     CompartmentSets sets(json);
//     std::string out = sets.toJSON();
//     CompartmentSets reloaded(out);

//     REQUIRE(reloaded.contains("cs0"));
//     auto set = reloaded.get("cs0");
//     auto locs = set.locations();
//     REQUIRE(locs.size() == 1);
//     REQUIRE(locs[0].gid() == 1);
//     REQUIRE(locs[0].offset() == Approx(0.1));
// }


// TEST_CASE("CompartmentLocation: construction and JSON round-trip") {
//     CompartmentLocation loc(42, 3, 0.75);
//     REQUIRE(loc.gid() == 42);
//     REQUIRE(loc.sectionIdx() == 3);
//     REQUIRE(loc.offset() == Approx(0.75));

//     std::string json = loc.toJSON();
//     CompartmentLocation parsed(json);
//     REQUIRE(parsed == loc);
//     CompartmentLocation loc0("[42, 3, 0.75]");
//     REQUIRE(loc == loc0);
// }

// TEST_CASE("CompartmentSet: valid JSON parsing and access") {
//     std::string json = R"({
//         "population": "exc",
//         "compartment_set": [
//             [1, 0, 0.0],
//             [2, 1, 0.5],
//             [1, 2, 1.0]
//         ]
//     })";

//     CompartmentSet set(json);
//     REQUIRE(set.population() == "exc");

//     auto gids = set.gids();
//     REQUIRE(gids.flatten().size() == 2);
//     REQUIRE(gids.contains(1));
//     REQUIRE(gids.contains(2));

//     auto locs = set.getCompartmentLocations();
//     REQUIRE(locs.size() == 3);
//     REQUIRE(locs[0].gid() == 1);
//     REQUIRE(locs[1].sectionIdx() == 1);
//     REQUIRE(locs[2].offset() == Approx(1.0));
// }

