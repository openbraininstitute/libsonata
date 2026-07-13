#include <catch2/catch_all.hpp>

#include <bbp/sonata/electrode_reader.h>

using namespace bbp::sonata;

namespace {
const std::string TEST_FILE = "./data/electrodes/electrode_weights.h5";
}

TEST_CASE("ElectrodeReader::getPopulationNames", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    auto names = reader.getPopulationNames();
    std::sort(names.begin(), names.end());
    REQUIRE(names == std::vector<std::string>{"NodeA", "NodeB"});
}

TEST_CASE("ElectrodeReader::openPopulation", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);

    SECTION("existing population") {
        const auto& pop = reader.openPopulation("NodeA");
        REQUIRE(pop.getNumberOfElectrodes() == 2);
    }

    SECTION("nonexistent population throws") {
        REQUIRE_THROWS_AS(reader.openPopulation("NonExistent"), SonataError);
    }
}

TEST_CASE("ElectrodeReader::Population::getNodeIds", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);

    SECTION("unsorted population") {
        const auto& pop = reader.openPopulation("NodeA");
        REQUIRE(pop.getNodeIds() == std::vector<NodeID>{5, 2, 8});
    }

    SECTION("sorted population") {
        const auto& pop = reader.openPopulation("NodeB");
        REQUIRE(pop.getNodeIds() == std::vector<NodeID>{0, 1});
    }
}

TEST_CASE("ElectrodeReader::Population::getNumberOfElectrodes", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    REQUIRE(reader.openPopulation("NodeA").getNumberOfElectrodes() == 2);
    REQUIRE(reader.openPopulation("NodeB").getNumberOfElectrodes() == 3);
}

TEST_CASE("ElectrodeReader::Population::get all", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    const auto& pop = reader.openPopulation("NodeA");

    auto df = pop.get();

    // 9 total compartments (node 2: 2, node 5: 3, node 8: 4), sorted by node_id
    REQUIRE(df.ids.size() == 9);
    REQUIRE(df.electrodes == std::vector<uint64_t>{0, 1});
    REQUIRE(df.data.size() == 9 * 2);

    // First rows belong to node 2 (sorted order)
    REQUIRE(df.ids[0] == CompartmentID{2, 0});
    REQUIRE(df.ids[1] == CompartmentID{2, 1});
    REQUIRE(df.ids[2] == CompartmentID{5, 0});
    REQUIRE(df.ids[5] == CompartmentID{8, 0});
    REQUIRE(df.ids[8] == CompartmentID{8, 3});

    // Verify data values: node 2 is at file rows 3-4, so values are (4*0.1, 5*0.1)
    REQUIRE(df.data[0] == Catch::Approx(0.4f));   // node 2, comp 0, electrode 0
    REQUIRE(df.data[1] == Catch::Approx(0.41f));  // node 2, comp 0, electrode 1
    REQUIRE(df.data[2] == Catch::Approx(0.5f));   // node 2, comp 1, electrode 0
}

TEST_CASE("ElectrodeReader::Population::get with node selection", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    const auto& pop = reader.openPopulation("NodeA");

    auto df = pop.get(Selection({{8, 9}}));

    // Node 8 has 4 compartments
    REQUIRE(df.ids.size() == 4);
    REQUIRE(df.ids[0] == CompartmentID{8, 0});
    REQUIRE(df.ids[3] == CompartmentID{8, 3});
    REQUIRE(df.data.size() == 4 * 2);

    // Node 8 is at file rows 5-8, values: (6*0.1+j*0.01, ..., 9*0.1+j*0.01)
    REQUIRE(df.data[0] == Catch::Approx(0.6f));
    REQUIRE(df.data[1] == Catch::Approx(0.61f));
    REQUIRE(df.data[6] == Catch::Approx(0.9f));
    REQUIRE(df.data[7] == Catch::Approx(0.91f));
}

TEST_CASE("ElectrodeReader::Population::get with electrode selection", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    const auto& pop = reader.openPopulation("NodeA");

    auto df = pop.get(nonstd::nullopt, Selection({{1, 2}}));

    // All 9 rows, but only electrode 1
    REQUIRE(df.ids.size() == 9);
    REQUIRE(df.electrodes == std::vector<uint64_t>{1});
    REQUIRE(df.data.size() == 9);

    // First value: node 2, comp 0, electrode 1 → 0.41
    REQUIRE(df.data[0] == Catch::Approx(0.41f));
}

TEST_CASE("ElectrodeReader::Population::get with both selections", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    const auto& pop = reader.openPopulation("NodeA");

    auto df = pop.get(Selection({{5, 6}}), Selection({{0, 1}}));

    // Node 5 has 3 compartments, electrode 0 only
    REQUIRE(df.ids.size() == 3);
    REQUIRE(df.electrodes == std::vector<uint64_t>{0});
    REQUIRE(df.data.size() == 3);

    // Node 5 is at file rows 0-2, values: (1*0.1, 2*0.1, 3*0.1)
    REQUIRE(df.data[0] == Catch::Approx(0.1f));
    REQUIRE(df.data[1] == Catch::Approx(0.2f));
    REQUIRE(df.data[2] == Catch::Approx(0.3f));
}

TEST_CASE("ElectrodeReader::Population::get with empty selection", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    const auto& pop = reader.openPopulation("NodeA");

    SECTION("empty node selection") {
        auto df = pop.get(Selection({}));
        REQUIRE(df.ids.empty());
        REQUIRE(df.data.empty());
    }

    SECTION("empty electrode selection") {
        auto df = pop.get(nonstd::nullopt, Selection({}));
        REQUIRE(df.ids.empty());
        REQUIRE(df.data.empty());
    }
}

TEST_CASE("ElectrodeReader::Population::get with out-of-range node_ids", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    const auto& pop = reader.openPopulation("NodeA");

    // Node 99 doesn't exist, node 5 does
    auto df = pop.get(Selection({{99, 100}}));
    REQUIRE(df.ids.empty());

    // Mix of valid and invalid
    auto df2 = pop.get(Selection::fromValues({99, 5, 100}));
    REQUIRE(df2.ids.size() == 3);  // only node 5's 3 compartments
    REQUIRE(df2.ids[0] == CompartmentID{5, 0});
}

TEST_CASE("ElectrodeReader::Population::get with out-of-range electrode_ids", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    const auto& pop = reader.openPopulation("NodeA");

    // Electrode 99 doesn't exist
    auto df = pop.get(nonstd::nullopt, Selection({{99, 100}}));
    REQUIRE(df.ids.empty());
    REQUIRE(df.data.empty());
}

TEST_CASE("ElectrodeReader::Population electrode metadata", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    const auto& pop = reader.openPopulation("NodeA");

    SECTION("names") {
        auto names = pop.getElectrodeNames();
        REQUIRE(names == std::vector<std::string>{"electrode_A0", "electrode_A1"});
    }

    SECTION("positions") {
        auto positions = pop.getElectrodePositions();
        REQUIRE(positions.size() == 2);
        REQUIRE(positions[0][0] == Catch::Approx(100.0));
        REQUIRE(positions[0][1] == Catch::Approx(200.0));
        REQUIRE(positions[0][2] == Catch::Approx(300.0));
        REQUIRE(positions[1][0] == Catch::Approx(150.0));
    }

    SECTION("types") {
        auto types = pop.getElectrodeTypes();
        REQUIRE(types == std::vector<std::string>{"LineSource", "PointSource"});
    }
}

TEST_CASE("ElectrodeReader::Population NodeB", "[electrode]") {
    const ElectrodeReader reader(TEST_FILE);
    const auto& pop = reader.openPopulation("NodeB");

    auto df = pop.get();
    REQUIRE(df.ids.size() == 5);
    REQUIRE(df.electrodes == std::vector<uint64_t>{0, 1, 2});
    REQUIRE(df.data.size() == 5 * 3);

    // Node 0 (2 compartments), node 1 (3 compartments) — already sorted
    REQUIRE(df.ids[0] == CompartmentID{0, 0});
    REQUIRE(df.ids[2] == CompartmentID{1, 0});

    // Values: (i+1)*0.5 + j*0.05
    REQUIRE(df.data[0] == Catch::Approx(0.5f));    // row 0, col 0
    REQUIRE(df.data[1] == Catch::Approx(0.55f));   // row 0, col 1
    REQUIRE(df.data[2] == Catch::Approx(0.6f));    // row 0, col 2
    REQUIRE(df.data[12] == Catch::Approx(2.5f));   // row 4, col 0
    REQUIRE(df.data[14] == Catch::Approx(2.6f));   // row 4, col 2
}

TEST_CASE("ElectrodeReader bad position throws", "[electrode]") {
    const std::string bad_file = "./data/electrodes/electrode_bad_position.h5";
    const ElectrodeReader reader(bad_file);
    REQUIRE_THROWS_AS(reader.openPopulation("BadPop"), SonataError);
}
