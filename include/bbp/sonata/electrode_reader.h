#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

#include <highfive/H5File.hpp>

#include <bbp/sonata/common.h>
#include <bbp/sonata/optional.hpp>
#include <bbp/sonata/selection.h>

namespace bbp {
namespace sonata {

/**
 * Container for electrode scaling factor data.
 *
 * Represents a 2D matrix of shape (n_compartments, n_electrodes) with
 * row and column identity information.
 */
struct SONATA_API ElectrodeDataFrame {
    using DataType = std::vector<CompartmentID>;

    /// Per-row identity: [node_id, local_compartment_index]
    DataType ids;

    /// Per-column identity: electrode indices returned.
    /// Uses uint64_t for consistency with Selection::Value and other libsonata public APIs.
    std::vector<uint64_t> electrodes;

    /// Flattened row-major data. data[row * n_cols + col] where n_cols = electrodes.size()
    std::vector<float> data;
};

/**
 * Reader for SONATA electrode weight files (HDF5).
 *
 * Provides access to electrode scaling factors used for LFP computation.
 * Follows the ReportReader/SpikeReader pattern with lazy-loaded populations.
 */
class SONATA_API ElectrodeReader
{
  public:
    class Population
    {
      public:
        /**
         * Return all node IDs present in this population.
         */
        std::vector<NodeID> getNodeIds() const;

        /**
         * Return the number of electrodes in this population.
         */
        size_t getNumberOfElectrodes() const;

        /**
         * Return scaling factors for the given node and electrode selections.
         *
         * \param node_ids selection of node IDs to include. nullopt means all nodes.
         * \param electrode_ids selection of electrode column indices to include.
         *        nullopt means all electrodes.
         * \return ElectrodeDataFrame with the submatrix of scaling factors.
         */
        ElectrodeDataFrame get(
            const nonstd::optional<Selection>& node_ids = nonstd::nullopt,
            const nonstd::optional<Selection>& electrode_ids = nonstd::nullopt) const;

        /**
         * Return electrode names ordered by column index.
         */
        std::vector<std::string> getElectrodeNames() const;

        /**
         * Return electrode positions ordered by column index.
         * Each entry is [x, y, z] in micrometers.
         */
        std::vector<std::array<double, 3>> getElectrodePositions() const;

        /**
         * Return electrode types ordered by column index.
         */
        std::vector<std::string> getElectrodeTypes() const;

      private:
        Population(const HighFive::File& file, const std::string& populationName);

        std::vector<NodeID> node_ids_;
        std::vector<uint64_t> offsets_;
        std::vector<Selection::Range> node_ranges_;  // per-node row range in scaling_factors
        std::vector<uint64_t> node_index_;           // sorted index into node_ids_
        HighFive::Group electrodes_group_;
        std::string population_name_;
        size_t n_electrodes_;

        // Electrode metadata (ordered by column index)
        std::vector<std::string> electrode_names_;
        std::vector<std::array<double, 3>> electrode_positions_;
        std::vector<std::string> electrode_types_;

        friend ElectrodeReader;
    };

    explicit ElectrodeReader(const std::string& filename);

    /**
     * Return a list of all population names found in the file.
     */
    std::vector<std::string> getPopulationNames() const;

    /**
     * Open (or return cached) population by name.
     *
     * \throw SonataError if no such population exists.
     */
    const Population& openPopulation(const std::string& populationName) const;

  private:
    HighFive::File file_;
    std::vector<std::string> population_names_;

    // Lazy loaded populations
    mutable std::map<std::string, Population> populations_;
};

}  // namespace sonata
}  // namespace bbp
