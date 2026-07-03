# Design Document: Electrode File Reader

## Overview

Add an `ElectrodeReader` to libsonata that reads SONATA electrode weight files
(HDF5). Follows the `ReportReader`/`SpikeReader` pattern: top-level reader with
lazy-loaded Population inner class.

## Architecture

```
ElectrodeReader (holds HighFive::File)
 └── Population (lazy-loaded, cached in std::map)
      ├── Internal sorted index (node_id → file row position)
      ├── get() → ElectrodeDataFrame (selection on both axes)
      └── Electrode metadata accessors
```

**Data flow:**
1. `ElectrodeReader(filename)` opens the HDF5 file
2. `getPopulationNames()` discovers populations by listing groups at `/` that
   contain `node_ids` and `offsets` datasets
3. `openPopulation(name)` reads `node_ids` + `offsets` arrays (small), builds
   sorted index for O(log n) lookup, caches the Population object
4. `Population::get(node_ids, electrode_ids)` reads scaling_factors from HDF5
   on demand for the requested submatrix

## New Data Structure: ElectrodeDataFrame

The existing `DataFrame<KeyType>` has axes `times × ids`. Electrode data has axes
`compartments × electrodes` — a fundamentally different shape. A dedicated struct
is warranted.

```cpp
struct SONATA_API ElectrodeDataFrame {
    using DataType = std::vector<CompartmentID>;
    DataType ids;                          // per row: [node_id, local_compartment_index]
    std::vector<uint64_t> electrodes;      // per column: electrode indices returned
    // data[ids][electrodes], flattened row-major. n_cols is electrodes.size()
    std::vector<float> data;
};
```

- `ids`: one `CompartmentID` (= `std::array<uint64_t, 2>`) per row. `[0]` = node_id,
  `[1]` = local compartment index (0-based within that node).
- `electrodes`: column indices returned. When all electrodes are requested, this is
  `{0, 1, ..., n_electrodes-1}`. When a subset is selected, only those indices.
- `data`: flat row-major matrix. `data[row * n_cols + col]` gives the scaling factor
  for compartment `ids[row]` and electrode `electrodes[col]`.

Python exposure:
- `.ids` → numpy array `(n_rows, 2)`, dtype uint64
- `.electrodes` → numpy array `(n_electrodes_returned,)`, dtype uint64
- `.data` → numpy array reshaped to `(n_rows, n_electrodes_returned)`, dtype float32

## Class Interface

```cpp
class SONATA_API ElectrodeReader {
public:
    class Population {
    public:
        /// All node IDs present in this population
        std::vector<NodeID> getNodeIds() const;

        /// Number of electrodes in this population
        size_t getNumberOfElectrodes() const;

        /// Scaling factors for selected nodes and electrodes
        /// node_ids: nullopt = all nodes; electrode_ids: nullopt = all electrodes
        ElectrodeDataFrame get(
            const nonstd::optional<Selection>& node_ids = nonstd::nullopt,
            const nonstd::optional<Selection>& electrode_ids = nonstd::nullopt) const;

        /// Electrode names (ordered by column index)
        std::vector<std::string> getElectrodeNames() const;

        /// Electrode positions (ordered by column index), each is [x, y, z] in µm
        std::vector<std::array<double, 3>> getElectrodePositions() const;

        /// Electrode types (ordered by column index)
        std::vector<std::string> getElectrodeTypes() const;

    private:
        Population(const HighFive::File& file, const std::string& populationName);

        std::vector<NodeID> node_ids_;
        std::vector<uint64_t> offsets_;
        std::vector<uint64_t> node_index_;  // sorted index into node_ids_
        HighFive::Group electrodes_group_;
        size_t n_electrodes_;

        friend ElectrodeReader;
    };

    explicit ElectrodeReader(const std::string& filename);

    /// Discover all population names in the file
    std::vector<std::string> getPopulationNames() const;

    /// Open (or return cached) population by name
    const Population& openPopulation(const std::string& populationName) const;

private:
    HighFive::File file_;
    mutable std::map<std::string, Population> populations_;
};
```

## Population Discovery

Populations are discovered by inspecting the root of the HDF5 file. A group at
`/{name}` is considered a population if it contains both `node_ids` and `offsets`
datasets. The `/electrodes` group is excluded from this scan.

## Internal Sorted Index

On `openPopulation`, the constructor:
1. Reads `/{pop}/node_ids` and `/{pop}/offsets`
2. Validates `offsets.size() == node_ids.size() + 1`
3. Builds `node_index_`: a vector of indices `[0, 1, ..., n-1]` sorted by `node_ids_[i]`

Lookup for a given node_id uses `std::lower_bound` on `node_index_` — O(log n).
This is identical to how `ReportReader::Population` handles unsorted node_ids.

## Selection Semantics

Following existing libsonata policy (`ReportReader::Population::get`):
- `nullopt` → all (select everything)
- Empty Selection (`Selection({})`) → nothing (empty result)
- Selection with values → those specific IDs

For `electrode_ids`: values are column indices `[0, n_electrodes)`. Out-of-range
indices are silently skipped (consistent with how node_ids out of range are handled
in ReportReader).

## Electrode Metadata

Electrode metadata lives under `/electrodes/{electrodename}/`. The mapping from
electrode name to column index is stored in `/electrodes/{electrodename}/{population}`.

On `openPopulation`, the reader scans `/electrodes/` subgroups (excluding
`{population}/scaling_factors`) to build an ordered list of electrode names sorted
by their column index. This enables `getElectrodeNames()[i]` to correspond to
column `i` of scaling_factors.

## Error Handling

| Condition | Behavior |
|-----------|----------|
| File does not exist / cannot be opened | Throw `SonataError` |
| Population not found | Throw `SonataError` |
| `offsets.size() != node_ids.size() + 1` | Throw `SonataError` |
| `scaling_factors` dataset missing | Throw `SonataError` |
| Node ID not found in population | Silently skip (empty result for that node) |
| Electrode index out of range | Silently skip |

## Python Bindings

Registered in `python/bindings.cpp`:

```python
# Usage
from libsonata import ElectrodeReader, Selection

reader = ElectrodeReader("electrode_weights.h5")
reader.population_names  # ['NodeA', 'NodeB']

pop = reader["NodeA"]  # or reader.open_population("NodeA")
pop.node_ids            # numpy array
pop.number_of_electrodes

df = pop.get()                                    # all nodes, all electrodes
df = pop.get(node_ids=Selection([0, 1, 5]))       # specific nodes
df = pop.get(electrode_ids=Selection([0, 2]))     # specific electrodes
df = pop.get(Selection([0, 1]), Selection([0]))   # both

df.ids        # numpy (n_rows, 2)
df.electrodes # numpy (n_cols,)
df.data       # numpy (n_rows, n_cols)

pop.electrode_names      # ['E0', 'E1', ...]
pop.electrode_positions  # numpy (n_elec, 3)
pop.electrode_types      # ['LineSource', 'PointSource', ...]
```

## File Layout

New files:
- `include/bbp/sonata/electrode_reader.h` — header
- `src/electrode_reader.cpp` — implementation
- `tests/test_electrode_reader.cpp` — C++ tests
- `python/tests/test_electrode_reader.py` — Python tests
- `tests/data/electrodes/` — test HDF5 fixtures
