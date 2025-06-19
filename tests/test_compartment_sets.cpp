#include <catch2/catch.hpp>
#include <bbp/sonata/compartment_sets.h>
#include <string>

#include <nlohmann/json.hpp>

using namespace bbp::sonata;
using json = nlohmann::json;

TEST_CASE("CompartmentLocation public API") {

    std::string json_content = R"(
        {
            "population": "test_population",
            "compartment_set": [
                [1, 10, 0.5]
            ]
        }
    )";
    CompartmentSet cs(json_content);

    SECTION("Construct from valid nodeId, section_idx, offset") {
        const auto& loc = cs[0];
        REQUIRE(loc.nodeId == 1);
        REQUIRE(loc.sectionId == 10);
        REQUIRE(loc.offset == Approx(0.5));
        REQUIRE(cs[0] == CompartmentLocation{1, 10, 0.5});
    }

    SECTION("Invalid JSON string throws") {
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ ["bla", 2, 0.1] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [1, 2] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [1, 2, 0.1, 1] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ ["a", 2, 0.5] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [1, "a", 0.5] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [1, 2, "a"] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [1, 2, 2.0] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [1, 2, -0.1] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [-1, 2, 0.1] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [1, -2, 0.1] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [1, 0, 0.5], [0, 0, 0.5] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [0, 1, 0.5], [0, 0, 0.5] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [0, 0, 0.6], [0, 0, 0.5] ]})"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSet(R"({"population": "pop0", "compartment_set": [ [0, 0, 0.5], [0, 0, 0.5] ]})"), SonataError);
    }

    SECTION("Equality operators") {
        CompartmentLocation loc1{1, 10, 0.5};
        CompartmentLocation loc2{1, 10, 0.5};
        CompartmentLocation loc3{1, 10, 0.6};

        REQUIRE(loc1 == loc2);
        REQUIRE_FALSE(loc1 != loc2);
        REQUIRE(loc1 != loc3);
    }
}


TEST_CASE("CompartmentSet public API") {

    // Example JSON representing a CompartmentSet (adjust to match real expected format)
    std::string json_content = R"(
        {
            "population": "test_population",
            "compartment_set": [
                [1, 10, 0.5],
                [2, 20, 0.25],
                [2, 20, 0.250001],
                [3, 30, 0.75]
            ]
        }
    )";

    SECTION("Construct from JSON string, round-trip serialization") {
        CompartmentSet cs(json_content);

        REQUIRE(cs.population() == "test_population");
        REQUIRE(cs.size() == 4);

        // Access elements by index
        REQUIRE(cs[0] == CompartmentLocation{1, 10, 0.5});
        REQUIRE(cs[1] == CompartmentLocation{2, 20, 0.25});
        REQUIRE(cs[2] == CompartmentLocation{2, 20, 0.250001});
        REQUIRE(cs[3] == CompartmentLocation{3, 30, 0.75});
        REQUIRE_THROWS_AS(cs[4], std::out_of_range);

        auto expected = json::parse(json_content);

        std::stable_sort(
            expected["compartment_set"].begin(),
            expected["compartment_set"].end(),
            [](const json& a, const json& b) {
                return a[0] < b[0];
            }
        );

        REQUIRE(cs.toJSON() == expected.dump());
    }

    SECTION("JSON constructor throws on invalid input") {
        // Not an object (array instead)
        REQUIRE_THROWS_AS(CompartmentSet("[1, 2, 3]"), SonataError);

        // Missing population key
        REQUIRE_THROWS_AS(
            CompartmentSet(R"({"compartment_set": []})"),
            SonataError
        );

        // population not a string
        REQUIRE_THROWS_AS(
            CompartmentSet(R"({"population": 123, "compartment_set": []})"),
            SonataError
        );

        // Missing compartment_set key
        REQUIRE_THROWS_AS(
            CompartmentSet(R"({"population": "test_population"})"),
            SonataError
        );

        // compartment_set not an array
        REQUIRE_THROWS_AS(
            CompartmentSet(R"({"population": "test_population", "compartment_set": "not an array"})"),
            SonataError
        );
    }


    SECTION("Size with selection filter") {
        CompartmentSet cs(json_content);

        REQUIRE(cs.size(Selection::fromValues({1, 2})) == 3);
        REQUIRE(cs.size(Selection::fromValues({3, 8, 9, 10, 13})) == 1);
        REQUIRE(cs.size(Selection::fromValues({999})) == 0);
    }

    SECTION("Filtered iteration") {
        CompartmentSet cs(json_content);

        auto pp = cs.filtered_crange();

        std::vector<int> nodeIds;
        for (auto it = pp.first; it != pp.second; ++it) {
            nodeIds.push_back((*it).nodeId);
        }

        REQUIRE(nodeIds.size() == 4);
        REQUIRE((nodeIds == std::vector<int>{1, 2, 2, 3}));
        nodeIds.clear();
        for (auto it = cs.filtered_crange(Selection::fromValues({2, 3})).first; it != pp.second; ++it) {
            nodeIds.push_back((*it).nodeId);
        }
        REQUIRE(nodeIds.size() == 3);
        REQUIRE((nodeIds == std::vector<int>{2, 2, 3}));
    }

    SECTION("Filter returns subset") {
        CompartmentSet cs(json_content);
        auto filtered = cs.filter(Selection::fromValues({2, 3}));

        REQUIRE(filtered.size() == 3);

        // Check filtered compartments only contain nodeIds 1 and 2
        auto nodeIds = filtered.nodeIds().flatten();
        REQUIRE(nodeIds == std::vector<uint64_t>({2, 3}));
        auto no_filtered = cs.filter();
        REQUIRE(no_filtered.size() == 4);
        auto no_filtered_nodeIds = no_filtered.nodeIds().flatten();
        REQUIRE(no_filtered_nodeIds == std::vector<uint64_t>({1, 2, 3}));
    }

    SECTION("Equality and inequality operators") {
        std::string json1 = R"(
            {
                "population": "pop1",
                "compartment_set": [
                    [1, 10, 0.5],
                    [2, 20, 0.25]
                ]
            }
        )";

        std::string json2 = R"(
            {
                "population": "pop1",
                "compartment_set": [
                    [1, 10, 0.5],
                    [2, 20, 0.25]
                ]
            }
        )";

        std::string json_different = R"(
            {
                "population": "pop1",
                "compartment_set": [
                    [1, 10, 0.5],
                    [2, 20, 0.3]
                ]
            }
        )";

        std::string json_different2 = R"(
            {
                "population": "pop2",
                "compartment_set": [
                    [1, 10, 0.5],
                    [2, 20, 0.25]
                ]
            }
        )";

        CompartmentSet cs1(json1);
        CompartmentSet cs2(json2);
        CompartmentSet cs3(json_different);
        CompartmentSet cs4(json_different2);

        REQUIRE(cs1 == cs2);
        REQUIRE_FALSE(cs1 != cs2);

        REQUIRE(cs1 != cs3);
        REQUIRE_FALSE(cs1 == cs3);
        REQUIRE_FALSE(cs1 == cs4);
    }

}

TEST_CASE("CompartmentSets public API") {

    const std::string json = R"({
        "cs1": {
            "population": "pop1",
            "compartment_set": [
                [0, 10, 0.1],
                [0, 10, 0.2],
                [0, 10, 0.3],
                [2, 3, 0.1],
                [3, 6, 0.3]
            ]
        },
        "cs0": {
            "population": "pop0",
            "compartment_set": []
        }
    })";

    const auto cs1 = CompartmentSet(R"({
            "population": "pop1",
            "compartment_set": [
                [0, 10, 0.1],
                [0, 10, 0.2],
                [0, 10, 0.3],
                [2, 3, 0.1],
                [3, 6, 0.3]
            ]
        })");

    const auto cs0 = CompartmentSet(R"({
            "population": "pop0",
            "compartment_set": []
        })");

    SECTION("Load from file and basic properties") {
        auto sets = CompartmentSets::fromFile("./data/compartment_sets.json");

        CHECK(sets.size() == 2);
        CHECK_FALSE(sets.empty());

        auto keys = sets.names();
        REQUIRE(sets.names() == std::vector<std::string>{"cs0", "cs1"});

        CHECK(sets.contains("cs0"));
        CHECK(sets.contains("cs1"));

        const auto& cs0 = sets.getCompartmentSet("cs0");
        CHECK(cs0.empty());

        const auto& cs1 = sets.getCompartmentSet("cs1");
        CHECK_FALSE(cs1.empty());
    }

    SECTION("Equality operator from file and string") {
        // Load from file
        auto sets_from_file = CompartmentSets::fromFile("./data/compartment_sets.json");

        // The JSON string as in the file
        const std::string json = R"({
        "cs1": {
            "population": "pop1",
            "compartment_set": [
            [0, 10, 0.1],
            [0, 10, 0.2],
            [0, 10, 0.3],
            [2, 3, 0.1],
            [3, 6, 0.3]
            ]
        },
        "cs0": {
            "population": "pop0",
            "compartment_set": []
        }
        })";

        // Construct from JSON string directly
        CompartmentSets sets_from_string(json);

        // They should be equal
        CHECK(sets_from_file == sets_from_string);
        CHECK_FALSE(sets_from_file != sets_from_string);

        // Now change the string slightly and check inequality
        const std::string json_modified = R"({
        "cs1": {
            "population": "pop1",
            "compartment_set": [
            [0, 10, 0.15],
            [0, 10, 0.2],
            [0, 10, 0.3],
            [2, 3, 0.1],
            [3, 6, 0.3]
            ]
        },
        "cs0": {
            "population": "pop0",
            "compartment_set": []
        }
        })";

        CompartmentSets sets_modified(json_modified);

        CHECK(sets_from_file != sets_modified);
        CHECK_FALSE(sets_from_file == sets_modified);
    }

    SECTION("Throws on missing key") {
        auto sets = CompartmentSets::fromFile("./data/compartment_sets.json");
        CHECK_THROWS_AS(sets.getCompartmentSet("not_there"), std::out_of_range);
    }

    SECTION("JSON serialization round-trip") {
        auto sets = CompartmentSets::fromFile("./data/compartment_sets.json");

        auto json = sets.toJSON();
        CompartmentSets roundtrip(json);

        CHECK(sets == roundtrip);
    }

    SECTION("Keys returns correct vector") {
        CompartmentSets sets(json);
        auto keys = sets.names();
        CHECK(keys == std::vector<std::string>{"cs0", "cs1"});
    }

    SECTION("GetAllCompartmentSets returns vector of CompartmentSet") {
        CompartmentSets sets(json);
        CHECK(sets.getAllCompartmentSets() == std::vector<CompartmentSet>{cs0, cs1});
    }

    SECTION("Items returns vector of pairs (key, CompartmentSet)") {
        CompartmentSets sets(json);
        CHECK(sets.items() == std::vector<std::pair<std::string, CompartmentSet>>{{"cs0", cs0}, {"cs1", cs1}});
    }

    SECTION("Size method") {
        CompartmentSets sets(json);
        CHECK(sets.size() == 2);
    }

    SECTION("Contains method") {
        CompartmentSets sets(json);
        CHECK(sets.contains("cs0"));
        CHECK(sets.contains("cs1"));
        CHECK_FALSE(sets.contains("missing_key"));
    }

    SECTION("Invalid JSON parsings") {
        // Top level must be an object
        REQUIRE_THROWS_AS(CompartmentSets("1"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets("[\"array\"]"), SonataError);

        // Each CompartmentSet must be an object with 'population' and 'compartment_set' keys
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": 1 })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": "string" })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": null })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": true })"), SonataError);

        // Missing keys
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "compartment_set": [] } })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0" } })"), SonataError);

        // Invalid types
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": 123, "compartment_set": [] } })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": null, "compartment_set": [] } })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": "not an array" } })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": 123 } })"), SonataError);

        // Invalid compartment_set elements
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [1] } })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [[1, 2]] } })"), SonataError);

        // Wrong types inside compartment_set elements
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [["not uint64", 0, 0.5]] } })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [[1, "not uint64", 0.5]] } })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [[1, 0, "not a number"]] } })"), SonataError);

        // Location out of bounds
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [[1, 0, -0.1]] } })"), SonataError);
        REQUIRE_THROWS_AS(CompartmentSets(R"({ "cs0": { "population": "pop0", "compartment_set": [[1, 0, 1.1]] } })"), SonataError);
    }

}

