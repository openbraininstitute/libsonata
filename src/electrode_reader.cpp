#include "hdf5_reader.hpp"

#include <bbp/sonata/electrode_reader.h>

#include <fmt/format.h>

#include <algorithm>
#include <numeric>
#include <string>

namespace bbp {
namespace sonata {

namespace {

struct ElectrodeInfo {
    uint64_t column_index;
    std::string name;
    std::array<double, 3> position;
    std::string type;
};

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


/// Scan /electrodes/{name}/ subgroups to collect per-electrode metadata for a population.
std::vector<ElectrodeInfo> discoverElectrodeMetadata(const HighFive::Group& electrodes_group,
                                                     const std::string& populationName) {
    std::vector<ElectrodeInfo> infos;

    for (const auto& ename : electrodes_group.listObjectNames()) {
        // Skip the population's scaling_factors group
        if (ename == populationName) {
            continue;
        }

        const auto egrp = electrodes_group.getGroup(ename);

        // Check if this electrode belongs to our population
        if (!egrp.exist(populationName)) {
            continue;
        }

        ElectrodeInfo info;
        info.name = ename;

        // Read column index
        egrp.getDataSet(populationName).read(info.column_index);

        // Read position (float32 in file, store as double)
        std::vector<float> pos_f32;
        egrp.getDataSet("position").read(pos_f32);
        if (pos_f32.size() == 3) {
            info.position = {static_cast<double>(pos_f32[0]),
                             static_cast<double>(pos_f32[1]),
                             static_cast<double>(pos_f32[2])};
        } else {
            info.position = {0.0, 0.0, 0.0};
        }

        // Read type
        egrp.getDataSet("type").read(info.type);

        infos.push_back(std::move(info));
    }

    // Sort by column index
    std::sort(infos.begin(), infos.end(), [](const auto& a, const auto& b) {
        return a.column_index < b.column_index;
    });

    return infos;
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


/// Coalesce sorted I/O order into contiguous read blocks.
/// Adjacent nodes with gap <= block_gap_limit are merged into a single read.
Selection::Ranges coalesceIOBlocks(const NodeLayout& slices,
                                   const std::vector<size_t>& io_order,
                                   size_t block_gap_limit) {
    Selection::Ranges blocks;
    size_t block_start = 0;
    for (size_t i = 0; (i + 1) < io_order.size(); ++i) {
        const auto cur_end = slices.ranges[io_order[i]][1];
        const auto next_start = slices.ranges[io_order[i + 1]][0];
        if (next_start - cur_end > block_gap_limit) {
            blocks.push_back({block_start, i + 1});
            block_start = i + 1;
        }
    }
    blocks.push_back({block_start, io_order.size()});
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


/// Read scaling factors from HDF5 in coalesced blocks and scatter into output.
void readAndScatter(const HighFive::DataSet& sf_dset,
                    const NodeLayout& slices,
                    const std::vector<size_t>& io_order,
                    const Selection::Ranges& io_blocks,
                    const std::vector<size_t>& output_offsets,
                    const std::vector<uint64_t>& selected_electrodes,
                    size_t n_electrodes,
                    ElectrodeDataFrame& result) {
    const size_t n_cols = selected_electrodes.size();

    for (const auto& block : io_blocks) {
        const size_t first_io = block[0];
        const size_t last_io = block[1] - 1;
        const size_t file_row_start = slices.ranges[io_order[first_io]][0];
        const size_t file_row_end = slices.ranges[io_order[last_io]][1];

        std::vector<std::vector<double>> block_data;
        sf_dset.select({file_row_start, 0}, {file_row_end - file_row_start, n_electrodes})
            .read(block_data);

        for (size_t i = first_io; i <= last_io; ++i) {
            const size_t n = io_order[i];
            const auto& range = slices.ranges[n];
            const size_t n_compartments = range[1] - range[0];
            const size_t local_row_start = range[0] - file_row_start;
            const size_t out_start = output_offsets[n];

            for (size_t comp = 0; comp < n_compartments; ++comp) {
                const size_t out_row = out_start + comp;
                result.ids[out_row] = {slices.node_ids[n], comp};

                const auto& src_row = block_data[local_row_start + comp];
                for (size_t col = 0; col < n_cols; ++col) {
                    result.data[out_row * n_cols + col] =
                        static_cast<float>(src_row[selected_electrodes[col]]);
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

    // Build sorted index for O(log n) lookup by node_id
    node_index_ = buildSortedIndex(node_ids_);

    // Read scaling_factors shape to get n_electrodes
    const auto sf_path = std::string("electrodes/") + populationName + "/scaling_factors";
    const auto sf_dset = file.getDataSet(sf_path);
    const auto dims = sf_dset.getDimensions();
    if (dims.size() != 2) {
        throw SonataError(
            fmt::format("Population '{}': scaling_factors must be 2D", populationName));
    }
    n_electrodes_ = dims[1];

    // Discover electrode metadata
    const auto electrode_infos = discoverElectrodeMetadata(electrodes_group_, populationName);
    electrode_names_.reserve(electrode_infos.size());
    electrode_positions_.reserve(electrode_infos.size());
    electrode_types_.reserve(electrode_infos.size());
    for (const auto& info : electrode_infos) {
        electrode_names_.push_back(info.name);
        electrode_positions_.push_back(info.position);
        electrode_types_.push_back(info.type);
    }
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


std::vector<std::array<double, 3>> ElectrodeReader::Population::getElectrodePositions() const {
    return electrode_positions_;
}


std::vector<std::string> ElectrodeReader::Population::getElectrodeTypes() const {
    return electrode_types_;
}


ElectrodeDataFrame ElectrodeReader::Population::get(
    const nonstd::optional<Selection>& node_ids,
    const nonstd::optional<Selection>& electrode_ids) const {
    ElectrodeDataFrame result;

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
    constexpr size_t block_gap_limit = 32;
    const auto io_blocks = coalesceIOBlocks(slices, io_order, block_gap_limit);

    const auto sf_path = std::string("electrodes/") + population_name_ + "/scaling_factors";
    const auto sf_dset = electrodes_group_.getFile().getDataSet(sf_path);

    readAndScatter(
        sf_dset, slices, io_order, io_blocks, output_offsets, selected_electrodes, n_electrodes_,
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
