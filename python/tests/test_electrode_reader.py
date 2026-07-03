import os

import numpy as np
import numpy.testing as npt
import pytest

import libsonata
from libsonata import ElectrodeReader, Selection, SonataError

PATH = os.path.join(os.path.dirname(__file__), "../../tests/data")
ELECTRODE_FILE = os.path.join(PATH, "electrodes/electrode_weights.h5")


@pytest.fixture
def reader():
    return ElectrodeReader(ELECTRODE_FILE)


class TestElectrodeReader:
    def test_population_names(self, reader):
        names = sorted(reader.population_names)
        assert names == ["NodeA", "NodeB"]

    def test_open_population(self, reader):
        pop = reader.open_population("NodeA")
        assert pop.number_of_electrodes == 2

    def test_getitem(self, reader):
        pop = reader["NodeA"]
        assert pop.number_of_electrodes == 2

    def test_nonexistent_population(self, reader):
        with pytest.raises(SonataError):
            reader["NonExistent"]


class TestElectrodePopulation:
    def test_node_ids(self, reader):
        pop = reader["NodeA"]
        npt.assert_array_equal(pop.node_ids, [5, 2, 8])

    def test_number_of_electrodes(self, reader):
        assert reader["NodeA"].number_of_electrodes == 2
        assert reader["NodeB"].number_of_electrodes == 3

    def test_electrode_names(self, reader):
        pop = reader["NodeA"]
        assert pop.electrode_names == ["electrode_A0", "electrode_A1"]

    def test_electrode_positions(self, reader):
        pop = reader["NodeA"]
        npt.assert_allclose(
            pop.electrode_positions,
            [[100.0, 200.0, 300.0], [150.0, 250.0, 350.0]],
        )

    def test_electrode_types(self, reader):
        pop = reader["NodeA"]
        assert pop.electrode_types == ["LineSource", "PointSource"]


class TestElectrodeDataFrame:
    def test_get_all(self, reader):
        pop = reader["NodeA"]
        df = pop.get()

        # 9 compartments total, 2 electrodes, sorted by node_id
        assert df.ids.shape == (9, 2)
        assert df.ids.dtype == np.uint64
        npt.assert_array_equal(df.electrodes, [0, 1])
        assert df.data.shape == (9, 2)
        assert df.data.dtype == np.float32

        # Sorted order: node 2 (2 comp), node 5 (3 comp), node 8 (4 comp)
        expected_ids = np.array([
            [2, 0], [2, 1],
            [5, 0], [5, 1], [5, 2],
            [8, 0], [8, 1], [8, 2], [8, 3],
        ], dtype=np.uint64)
        npt.assert_array_equal(df.ids, expected_ids)

        # Values: node 2 at file rows 3-4, node 5 at rows 0-2, node 8 at rows 5-8
        expected_data = np.array([
            [0.4, 0.41], [0.5, 0.51],
            [0.1, 0.11], [0.2, 0.21], [0.3, 0.31],
            [0.6, 0.61], [0.7, 0.71], [0.8, 0.81], [0.9, 0.91],
        ], dtype=np.float32)
        npt.assert_allclose(df.data, expected_data, atol=1e-6)

    def test_get_node_selection(self, reader):
        pop = reader["NodeA"]
        df = pop.get(node_ids=Selection([(2, 3)]))

        assert df.ids.shape == (2, 2)
        npt.assert_array_equal(df.ids[:, 0], [2, 2])
        npt.assert_allclose(df.data, [[0.4, 0.41], [0.5, 0.51]], atol=1e-6)

    def test_get_electrode_selection(self, reader):
        pop = reader["NodeA"]
        df = pop.get(electrode_ids=Selection([(1, 2)]))

        assert df.data.shape == (9, 1)
        npt.assert_array_equal(df.electrodes, [1])
        expected_col1 = np.array([0.41, 0.51, 0.11, 0.21, 0.31, 0.61, 0.71, 0.81, 0.91],
                                 dtype=np.float32)
        npt.assert_allclose(df.data[:, 0], expected_col1, atol=1e-6)

    def test_get_both_selections(self, reader):
        pop = reader["NodeA"]
        df = pop.get(
            node_ids=Selection([(5, 6)]),
            electrode_ids=Selection([(0, 1)]),
        )

        assert df.ids.shape == (3, 2)
        assert df.data.shape == (3, 1)
        npt.assert_allclose(df.data[:, 0], [0.1, 0.2, 0.3], atol=1e-6)

    def test_get_empty_node_selection(self, reader):
        pop = reader["NodeA"]
        df = pop.get(node_ids=Selection([]))
        assert df.ids.shape[0] == 0

    def test_get_empty_electrode_selection(self, reader):
        pop = reader["NodeA"]
        df = pop.get(electrode_ids=Selection([]))
        assert df.ids.shape[0] == 0

    def test_get_out_of_range_node(self, reader):
        pop = reader["NodeA"]
        df = pop.get(node_ids=Selection([(99, 100)]))
        assert df.ids.shape[0] == 0

    def test_get_out_of_range_electrode(self, reader):
        pop = reader["NodeA"]
        df = pop.get(electrode_ids=Selection([(99, 100)]))
        assert df.ids.shape[0] == 0

    def test_node_b(self, reader):
        pop = reader["NodeB"]
        df = pop.get()

        assert df.ids.shape == (5, 2)
        assert df.data.shape == (5, 3)
        npt.assert_array_equal(df.electrodes, [0, 1, 2])

        # Values: (i+1)*0.5 + j*0.05
        expected = np.array(
            [[(i + 1) * 0.5 + j * 0.05 for j in range(3)] for i in range(5)],
            dtype=np.float32,
        )
        npt.assert_allclose(df.data, expected, atol=1e-6)
