#include "hdf5_reader.hpp"

#include <bbp/sonata/electrode_reader.h>

#include <fmt/format.h>

#include <algorithm>
#include <numeric>
#include <string>

namespace bbp {
namespace sonata {

namespace {

/// Resolved node selection: parallel vectors of node IDs and their row ranges.
struct NodeLayout {
    std::vector<NodeID> node_ids;
    Selection::Ranges ranges;

    void reserve(size_t n) {
        node_ids.reserve(n);
        ranges.reserve(n);
    }

    void add(NodeID node_id, Selection::Range range) {
        node_ids.emplace_back(node_id);
        ranges.emplace_back(range);
    }

    void add(const std::vector<NodeID>& ids,
             const Selection::Ranges& rngs,
             const std::vector<uint64_t>& indices) {
        assert(ids.size() == rngs.size());
        reserve(indices.size());
        for (size_t idx : indices) {
            add(ids[idx], rngs[idx]);
        }
    }

    bool empty() const {
        return node_ids.empty();
    }
    size_t size() const {
        return node_ids.size();
    }
};


/// Build an index that maps sorted position to original position in node_ids.
std::vector<uint64_t> buildSortedIndex(const std::vector<NodeID>& node_ids) {
    std::vector<uint64_t> index(node_ids.size());
    std::iota(index.begin(), index.end(), 0);
    std::sort(index.begin(), index.end(), [&](size_t i, size_t j) {
        return node_ids[i] < node_ids[j];
    });
    return index;
}


/// Scan /electrodes/{name}/ subgroups and populate metadata vectors for a population.
/// Vectors are ordered by column index (HDF5 group iteration order is not guaranteed).
void discoverElectrodeMetadata(const HighFive::Group& electrodes_group,
                               const std::string& populationName,
                               std::vector<std::string>& names,
                               std::vector<std::array<float, 3>>& positions,
                               std::vector<std::string>& types) {
    struct Entry {
        uint64_t electrode_id;
        std::string name;
        std::array<float, 3> position;
        std::string type;
    };
    std::vector<Entry> entries;

    for (const auto& ename : electrodes_group.listObjectNames()) {
        if (ename == populationName) {
            continue;
        }

        const auto egrp = electrodes_group.getGroup(ename);

        if (!egrp.exist(populationName)) {
            continue;
        }

        Entry entry;
        entry.name = ename;

        egrp.getDataSet(populationName).read(entry.electrode_id);

        std::vector<float> pos_f32;
        egrp.getDataSet("position").read(pos_f32);
        if (pos_f32.size() != 3) {
            throw SonataError(
                fmt::format("Electrode '{}': position dataset must have exactly 3 elements",
                            ename));
        }
        entry.position = {pos_f32[0], pos_f32[1], pos_f32[2]};

        egrp.getDataSet("type").read(entry.type);

        entries.push_back(std::move(entry));
    }

    std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
        return a.electrode_id < b.electrode_id;
    });

    names.reserve(entries.size());
    positions.reserve(entries.size());
    types.reserve(entries.size());
    for (auto& e : entries) {
        names.push_back(std::move(e.name));
        positions.push_back(e.position);
        types.push_back(std::move(e.type));
    }
}


/// Resolve an optional electrode Selection into concrete column indices.
/// Returns all indices if nullopt, filtered valid indices if provided, empty if empty selection.
std::vector<uint64_t> resolveElectrodeSelection(const nonstd::optional<Selection>& electrode_ids,
                                                size_t n_electrodes) {
    std::vector<uint64_t> selected;
    if (!electrode_ids) {
        selected.resize(n_electrodes);
        std::iota(selected.begin(), selected.end(), 0);
    } else if (!electrode_ids->empty()) {
        for (auto idx : electrode_ids->flatten()) {
            if (idx < n_electrodes) {
                selected.push_back(idx);
            }
        }
    }
    return selected;
}


/// Build I/O order sorted by file row position.
std::vector<size_t> buildIOOrder(const NodeLayout& slices) {
    std::vector<size_t> io_order(slices.size());
    std::iota(io_order.begin(), io_order.end(), 0);
    std::sort(io_order.begin(), io_order.end(), [&](size_t a, size_t b) {
        return slices.ranges[a][0] < slices.ranges[b][0];
    });
    return io_order;
}


/// A coalesced I/O block. All ranges are half-open [start, end).
/// io_begin/io_end index into io_order; file_row_start/file_row_end are HDF5 row offsets.
struct IOBlock {
    size_t io_begin;
    size_t io_end;
    size_t file_row_start;
    size_t file_row_end;
};


/// Coalesce sorted I/O order into contiguous read blocks.
/// Adjacent nodes with gap <= block_gap_limit are merged into a single read.
std::vector<IOBlock> coalesceIOBlocks(const NodeLayout& slices,
                                      const std::vector<size_t>& io_order,
                                      size_t block_gap_limit) {
    std::vector<IOBlock> blocks;
    size_t block_start = 0;
    for (size_t i = 1; i < io_order.size(); ++i) {
        const auto prev_end = slices.ranges[io_order[i - 1]][1];
        const auto cur_start = slices.ranges[io_order[i]][0];
        if (cur_start - prev_end > block_gap_limit) {
            blocks.push_back({block_start, i, slices.ranges[io_order[block_start]][0], prev_end});
            block_start = i;
        }
    }
    blocks.push_back({block_start,
                      io_order.size(),
                      slices.ranges[io_order[block_start]][0],
                      slices.ranges[io_order.back()][1]});
    return blocks;
}


/// Compute per-node output offsets (cumulative compartment counts).
std::vector<size_t> computeOutputOffsets(const NodeLayout& slices) {
    const size_t n = slices.size();
    std::vector<size_t> offsets(n + 1, 0);
    for (size_t i = 0; i < n; ++i) {
        offsets[i + 1] = offsets[i] + (slices.ranges[i][1] - slices.ranges[i][0]);
    }
    return offsets;
}


/// Read scaling factors from HDF5 in coalesced blocks and scatter into result.
void readAndScatter(const HighFive::DataSet& sf_dset,
                    const NodeLayout& slices,
                    const std::vector<size_t>& io_order,
                    const std::vector<IOBlock>& io_blocks,
                    const std::vector<size_t>& output_offsets,
                    const std::vector<uint64_t>& selected_electrodes,
                    size_t n_electrodes,
                    ElectrodeScalingFactors& result) {
    const size_t n_cols = selected_electrodes.size();
    std::vector<double> block_data;

    for (const auto& block : io_blocks) {
        const size_t block_rows = block.file_row_end - block.file_row_start;

        block_data.resize(block_rows * n_electrodes);
        sf_dset.select({block.file_row_start, 0}, {block_rows, n_electrodes})
            .read_raw(block_data.data());

        for (size_t i = block.io_begin; i < block.io_end; ++i) {
            const size_t n = io_order[i];
            const auto& range = slices.ranges[n];
            const size_t n_compartments = range[1] - range[0];
            const size_t local_row_start = range[0] - block.file_row_start;
            const size_t out_start = output_offsets[n];

            for (size_t comp = 0; comp < n_compartments; ++comp) {
                const size_t out_row = out_start + comp;
                result.ids[out_row] = {slices.node_ids[n], comp};

                const size_t src_offset = (local_row_start + comp) * n_electrodes;
                for (size_t col = 0; col < n_cols; ++col) {
                    result.data[out_row * n_cols + col] =
                        block_data[src_offset + selected_electrodes[col]];
                }
            }
        }
    }
}


/// Uses binary search on the sorted index. Nodes not found are silently skipped.
NodeLayout resolveNodeSelection(const nonstd::optional<Selection>& node_ids,
                                const std::vector<NodeID>& all_node_ids,
                                const Selection::Ranges& all_ranges,
                                const std::vector<uint64_t>& sorted_index) {
    NodeLayout layout;

    if (!node_ids) {
        layout.add(all_node_ids, all_ranges, sorted_index);
    } else if (!node_ids->empty()) {
        for (const auto node_id : node_ids->flatten()) {
            const auto it =
                std::lower_bound(sorted_index.begin(),
                                 sorted_index.end(),
                                 node_id,
                                 [&](size_t i, NodeID nid) { return all_node_ids[i] < nid; });

            if (it != sorted_index.end() && all_node_ids[*it] == node_id) {
                layout.add(node_id, all_ranges[*it]);
            }
        }
    }

    return layout;
}

}  // anonymous namespace


//--------------------------------------------------------------------------------------------------
// ElectrodeReader::Population
//--------------------------------------------------------------------------------------------------

ElectrodeReader::Population::Population(const HighFive::File& file,
                                        const std::string& populationName)
    : electrodes_group_(file.getGroup("electrodes"))
    , population_name_(populationName) {
    // Read node_ids and offsets
    const auto pop_group = file.getGroup(populationName);
    pop_group.getDataSet("node_ids").read(node_ids_);
    pop_group.getDataSet("offsets").read(offsets_);

    if (offsets_.size() != node_ids_.size() + 1) {
        throw SonataError(fmt::format(
            "Population '{}': 'offsets' size ({}) must be 'node_ids' size ({}) plus one",
            populationName,
            offsets_.size(),
            node_ids_.size()));
    }

    // Precompute per-node row ranges from offsets
    for (size_t i = 0; i < node_ids_.size(); ++i) {
        node_ranges_.push_back({offsets_[i], offsets_[i + 1]});
    }

    node_index_ = buildSortedIndex(node_ids_);

    const auto sf_path = std::string("electrodes/") + populationName + "/scaling_factors";
    const auto sf_dset = file.getDataSet(sf_path);
    const auto dims = sf_dset.getDimensions();
    if (dims.size() != 2) {
        throw SonataError(
            fmt::format("Population '{}': scaling_factors must be 2D", populationName));
    }
    n_electrodes_ = dims[1];

    discoverElectrodeMetadata(electrodes_group_,
                              populationName,
                              electrode_names_,
                              electrode_positions_,
                              electrode_types_);
}


std::vector<NodeID> ElectrodeReader::Population::getNodeIds() const {
    return node_ids_;
}


size_t ElectrodeReader::Population::getNumberOfElectrodes() const {
    return n_electrodes_;
}


std::vector<std::string> ElectrodeReader::Population::getElectrodeNames() const {
    return electrode_names_;
}


std::vector<std::array<float, 3>> ElectrodeReader::Population::getElectrodePositions() const {
    return electrode_positions_;
}


std::vector<std::string> ElectrodeReader::Population::getElectrodeTypes() const {
    return electrode_types_;
}


ElectrodeScalingFactors ElectrodeReader::Population::get(
    const nonstd::optional<Selection>& node_ids,
    const nonstd::optional<Selection>& electrode_ids) const {
    ElectrodeScalingFactors result;

    const auto selected_electrodes = resolveElectrodeSelection(electrode_ids, n_electrodes_);
    if (selected_electrodes.empty()) {
        return result;
    }

    const auto slices = resolveNodeSelection(node_ids, node_ids_, node_ranges_, node_index_);
    if (slices.empty()) {
        return result;
    }

    result.electrodes = selected_electrodes;
    const size_t n_cols = selected_electrodes.size();

    const auto output_offsets = computeOutputOffsets(slices);
    const size_t total_rows = output_offsets.back();

    result.ids.resize(total_rows);
    result.data.resize(total_rows * n_cols);

    const auto io_order = buildIOOrder(slices);
    // Coalescing gap: merge adjacent reads if the gap between them is within budget.
    // Use a 4 MB over-read tolerance (same as ReportReader), converted to rows.
    constexpr size_t block_gap_bytes = 4 * 1024 * 1024;
    const size_t row_bytes = n_electrodes_ * sizeof(double);
    const size_t block_gap_limit = block_gap_bytes / row_bytes;
    const auto io_blocks = coalesceIOBlocks(slices, io_order, block_gap_limit);

    const auto sf_path = std::string("electrodes/") + population_name_ + "/scaling_factors";
    const auto sf_dset = electrodes_group_.getFile().getDataSet(sf_path);

    readAndScatter(sf_dset,
                   slices,
                   io_order,
                   io_blocks,
                   output_offsets,
                   selected_electrodes,
                   n_electrodes_,
                   result);

    return result;
}


//--------------------------------------------------------------------------------------------------
// ElectrodeReader
//--------------------------------------------------------------------------------------------------

ElectrodeReader::ElectrodeReader(const std::string& filename)
    : file_(openHDF5withoutLock(filename)) {
    // Discover populations: root-level groups that contain node_ids + offsets
    for (const auto& name : file_.listObjectNames()) {
        if (name == "electrodes") {
            continue;
        }

        if (file_.getObjectType(name) != HighFive::ObjectType::Group) {
            continue;
        }

        const auto grp = file_.getGroup(name);
        if (grp.exist("node_ids") && grp.exist("offsets")) {
            population_names_.push_back(name);
        }
    }
}


std::vector<std::string> ElectrodeReader::getPopulationNames() const {
    return population_names_;
}


const ElectrodeReader::Population& ElectrodeReader::openPopulation(
    const std::string& populationName) const {
    if (populations_.find(populationName) == populations_.end()) {
        // Verify population exists
        if (std::find(population_names_.begin(), population_names_.end(), populationName) ==
            population_names_.end()) {
            throw SonataError(
                fmt::format("No population '{}' found in electrode file", populationName));
        }
        populations_.emplace(populationName, Population{file_, populationName});
    }

    return populations_.at(populationName);
}

}  // namespace sonata
}  // namespace bbp
