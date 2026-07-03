# Implementation Plan: Electrode File Reader

## Overview

Add `ElectrodeReader` to libsonata (C++ + Python bindings) following the
`ReportReader`/`SpikeReader` pattern. Reads SONATA electrode weight HDF5 files
with Selection-based access on both node and electrode axes.

## Tasks

- [ ] 1. Create test data fixture
  - [ ] 1.1 Create a small HDF5 electrode file with known values
    - Two populations ("NodeA", "NodeB")
    - NodeA: 3 nodes (unsorted node_ids: [5, 2, 8]), 2 electrodes, varying compartment counts
    - NodeB: 2 nodes (sorted node_ids: [0, 1]), 3 electrodes
    - Electrode metadata: names, positions, types, layer, region
    - Place in `tests/data/electrodes/electrode_weights.h5`
    - Write a Python script to generate it (placed in `tests/data/electrodes/generate.py`)

- [ ] 2. Implement C++ header
  - [ ] 2.1 Create `include/bbp/sonata/electrode_reader.h`
    - Define `ElectrodeDataFrame` struct (ids, electrodes, data)
    - Define `ElectrodeReader` class with `Population` inner class
    - Public API: `getPopulationNames()`, `openPopulation()`, `Population::get()`,
      `Population::getNodeIds()`, `Population::getNumberOfElectrodes()`,
      `Population::getElectrodeNames()`, `Population::getElectrodePositions()`,
      `Population::getElectrodeTypes()`

  - [ ] 2.2 Update `include/bbp/sonata/sonata.h` (or equivalent umbrella header)
    - Add `#include <bbp/sonata/electrode_reader.h>`

- [ ] 3. Implement C++ source
  - [ ] 3.1 Create `src/electrode_reader.cpp`
    - `ElectrodeReader` constructor: open HDF5 file
    - `getPopulationNames()`: scan root groups for those with `node_ids` + `offsets`
    - `openPopulation()`: lazy construct + cache in `std::map`
    - `Population` constructor: read node_ids, offsets, validate, build sorted index,
      scan electrode metadata (names → column index mapping)
    - `Population::get()`: resolve node Selection → row ranges, resolve electrode
      Selection → column indices, read submatrix from HDF5, build ElectrodeDataFrame
    - `Population::getNodeIds()`, `getNumberOfElectrodes()`, metadata accessors

  - [ ] 3.2 Update `src/CMakeLists.txt`
    - Add `electrode_reader.cpp` to the source list

- [ ] 4. Implement Python bindings
  - [ ] 4.1 Add bindings in `python/bindings.cpp`
    - Register `ElectrodeDataFrame` class (`.ids`, `.electrodes`, `.data` as numpy arrays)
    - Register `ElectrodeReader` class (`__init__`, `population_names`, `open_population`, `__getitem__`)
    - Register `ElectrodeReader::Population` class (`get`, `node_ids`, `number_of_electrodes`,
      `electrode_names`, `electrode_positions`, `electrode_types`)
    - Use zero-copy numpy exposure where possible (managedMemoryArray pattern)

- [ ] 5. Checkpoint — verify build compiles
  - Build C++ and Python. Fix any compile errors before proceeding to tests.

- [ ] 6. Write C++ tests
  - [ ] 6.1 Create `tests/test_electrode_reader.cpp`
    - Test population discovery (correct names returned)
    - Test opening nonexistent population throws SonataError
    - Test getNodeIds() returns all node IDs
    - Test getNumberOfElectrodes()
    - Test get() with no selection returns full matrix with correct shape
    - Test get() with node_ids Selection returns subset
    - Test get() with electrode_ids Selection returns column subset
    - Test get() with both selections returns submatrix
    - Test get() with empty Selection returns empty result
    - Test get() with out-of-range node_ids returns only valid ones
    - Test unsorted node_ids are handled correctly (lookup works regardless of file order)
    - Test ids field has correct (node_id, compartment_index) pairs
    - Test electrode metadata accessors (names, positions, types)
    - Test file validation (missing datasets → SonataError)

  - [ ] 6.2 Update `tests/CMakeLists.txt`
    - Add test_electrode_reader to the test suite

- [ ] 7. Write Python tests
  - [ ] 7.1 Create `python/tests/test_electrode_reader.py`
    - Test construction from file path
    - Test population_names property
    - Test open_population and __getitem__ access
    - Test get() returns correct numpy arrays (ids shape, data shape, electrodes shape)
    - Test Selection-based filtering (node_ids, electrode_ids, both)
    - Test electrode metadata properties
    - Test invalid file raises SonataError

- [ ] 8. Final checkpoint — run all tests
  - `ctest` for C++ tests
  - `pytest` for Python tests
  - Fix any failures

## Notes

- The `ElectrodeDataFrame` struct is new and independent from the existing `DataFrame<KeyType>`.
- Population discovery filters out the `/electrodes` root group (it's metadata, not a population).
- Electrode column ordering comes from the `/{electrodename}/{population}` dataset value
  (the integer stored there is the column index in `scaling_factors`).
- Test fixture uses unsorted node_ids to verify the sorted index works correctly.

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["1.1"] },
    { "id": 1, "tasks": ["2.1", "2.2"] },
    { "id": 2, "tasks": ["3.1", "3.2"] },
    { "id": 3, "tasks": ["4.1"] },
    { "id": 4, "tasks": ["5"] },
    { "id": 5, "tasks": ["6.1", "6.2", "7.1"] },
    { "id": 6, "tasks": ["8"] }
  ]
}
```
