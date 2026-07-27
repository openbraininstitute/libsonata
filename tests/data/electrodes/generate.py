#!/usr/bin/env python3
"""Generate a test electrode weights HDF5 file for ElectrodeReader tests.

File layout follows the SONATA electrode file spec:
  /{population}/node_ids          — uint64, shape (N_nodes,)
  /{population}/offsets           — uint64, shape (N_nodes + 1,)
  /electrodes/{population}/scaling_factors — float64, shape (Total_comp, N_elec)
  /electrodes/{electrodename}/position     — float32, shape (3,)
  /electrodes/{electrodename}/type         — utf8 scalar
  /electrodes/{electrodename}/layer        — utf8 scalar
  /electrodes/{electrodename}/region       — utf8 scalar
  /electrodes/{electrodename}/{population} — uint, electrode_id (column index in scaling_factors)

Creates two populations:
  - "NodeA": 3 nodes (unsorted: [5, 2, 8]), 2 electrodes, varying compartments
  - "NodeB": 2 nodes (sorted: [0, 1]), 3 electrodes
"""

import os

import h5py
import numpy as np


def create_test_file(filepath: str) -> None:
    with h5py.File(filepath, "w") as h5:
        _create_population_a(h5)
        _create_population_b(h5)


def _create_population_a(h5: h5py.File) -> None:
    """Population 'NodeA': 3 nodes, 2 electrodes, unsorted node_ids."""
    pop_name = "NodeA"

    # Unsorted node_ids: [5, 2, 8]
    node_ids = np.array([5, 2, 8], dtype=np.uint64)

    # Compartment counts: node 5 has 3, node 2 has 2, node 8 has 4
    # offsets: [0, 3, 5, 9]
    offsets = np.array([0, 3, 5, 9], dtype=np.uint64)

    # scaling_factors: (9 total compartments, 2 electrodes)
    # Fill with deterministic values: row i, col j => (i + 1) * 0.1 + j * 0.01
    n_rows = 9
    n_elec = 2
    scaling = np.zeros((n_rows, n_elec), dtype=np.float64)
    for i in range(n_rows):
        for j in range(n_elec):
            scaling[i, j] = (i + 1) * 0.1 + j * 0.01

    # Population datasets
    h5.create_dataset(f"{pop_name}/node_ids", data=node_ids)
    h5.create_dataset(f"{pop_name}/offsets", data=offsets)

    # Scaling factors
    h5.create_dataset(f"electrodes/{pop_name}/scaling_factors", data=scaling)

    # Electrode metadata — 2 electrodes: "electrode_A0" and "electrode_A1"
    electrodes = [
        {
            "name": "electrode_A0",
            "position": np.array([100.0, 200.0, 300.0], dtype=np.float32),
            "type": "LineSource",
            "layer": "L5",
            "region": "S1",
            "electrode_id": 0,
        },
        {
            "name": "electrode_A1",
            "position": np.array([150.0, 250.0, 350.0], dtype=np.float32),
            "type": "PointSource",
            "layer": "L3",
            "region": "S1",
            "electrode_id": 1,
        },
    ]

    for elec in electrodes:
        prefix = f"electrodes/{elec['name']}"
        h5.create_dataset(f"{prefix}/position", data=elec["position"])
        h5.create_dataset(f"{prefix}/type", data=elec["type"])
        h5.create_dataset(f"{prefix}/layer", data=elec["layer"])
        h5.create_dataset(f"{prefix}/region", data=elec["region"])
        h5.create_dataset(f"{prefix}/{pop_name}", data=np.uint64(elec["electrode_id"]))


def _create_population_b(h5: h5py.File) -> None:
    """Population 'NodeB': 2 nodes, 3 electrodes, sorted node_ids."""
    pop_name = "NodeB"

    # Sorted node_ids: [0, 1]
    node_ids = np.array([0, 1], dtype=np.uint64)

    # Compartment counts: node 0 has 2, node 1 has 3
    # offsets: [0, 2, 5]
    offsets = np.array([0, 2, 5], dtype=np.uint64)

    # scaling_factors: (5 total compartments, 3 electrodes)
    n_rows = 5
    n_elec = 3
    scaling = np.zeros((n_rows, n_elec), dtype=np.float64)
    for i in range(n_rows):
        for j in range(n_elec):
            scaling[i, j] = (i + 1) * 0.5 + j * 0.05

    # Population datasets
    h5.create_dataset(f"{pop_name}/node_ids", data=node_ids)
    h5.create_dataset(f"{pop_name}/offsets", data=offsets)

    # Scaling factors
    h5.create_dataset(f"electrodes/{pop_name}/scaling_factors", data=scaling)

    # Electrode metadata — 3 electrodes
    electrodes = [
        {
            "name": "electrode_B0",
            "position": np.array([10.0, 20.0, 30.0], dtype=np.float32),
            "type": "Reciprocity",
            "layer": "L1",
            "region": "V1",
            "electrode_id": 0,
        },
        {
            "name": "electrode_B1",
            "position": np.array([40.0, 50.0, 60.0], dtype=np.float32),
            "type": "DipoleReciprocity",
            "layer": "L2",
            "region": "V1",
            "electrode_id": 1,
        },
        {
            "name": "electrode_B2",
            "position": np.array([70.0, 80.0, 90.0], dtype=np.float32),
            "type": "LineSource",
            "layer": "Outside",
            "region": "NA",
            "electrode_id": 2,
        },
    ]

    for elec in electrodes:
        prefix = f"electrodes/{elec['name']}"
        h5.create_dataset(f"{prefix}/position", data=elec["position"])
        h5.create_dataset(f"{prefix}/type", data=elec["type"])
        h5.create_dataset(f"{prefix}/layer", data=elec["layer"])
        h5.create_dataset(f"{prefix}/region", data=elec["region"])
        h5.create_dataset(f"{prefix}/{pop_name}", data=np.uint64(elec["electrode_id"]))


if __name__ == "__main__":
    output_path = os.path.join(os.path.dirname(__file__), "electrode_weights.h5")
    create_test_file(output_path)
    print(f"Generated: {output_path}")
