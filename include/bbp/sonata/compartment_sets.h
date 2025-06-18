#pragma once

#include <bbp/sonata/nodes.h>

#include <nlohmann/json.hpp>

namespace bbp {
namespace sonata {
namespace detail {
class CompartmentSetFilteredIterator;
class CompartmentSet;
class CompartmentSets;
}  // namespace detail

/**
 * CompartmentLocation.
 *
 * This struct uniquely identifies a compartment by a set of node_id, section_index and offset:
 *
 * - node_id: Global ID of the cell (Neuron) to which the compartment belongs. No
 * overlaps among populations.
 * - section_index: Absolute section index. Progressive index that uniquely identifies the section.
 *  There is a mapping between neuron section names (i.e. dend[10]) and this index.
 * - offset: Offset of the compartment along the section. The offset is a value between 0 and 1
 *
 * Note: it cannot go inside CompartmentSet because then CompartmentSetFilteredIterator needs the
 * full definition of CompartmentSet and CompartmentSet needs the full definition of
 * CompartmentSetFilteredIterator.
 */
struct CompartmentLocation {
  public:
    uint64_t nodeId = 0;
    uint64_t sectionId = 0;
    double offset = 0.0;

    /// Comparator. Used to compare vectors in CompartmentSet. More idiomatic than defining a
    /// comaprator on the fly
    bool operator==(const CompartmentLocation& other) const {
        return nodeId == other.nodeId && sectionId == other.sectionId && offset == other.offset;
    }

    bool operator!=(const CompartmentLocation& other) const {
        return !(*this == other);
    }

    bool operator<(const CompartmentLocation& other) const {
        if (nodeId != other.nodeId)
            return nodeId < other.nodeId;
        if (sectionId != other.sectionId)
            return sectionId < other.sectionId;
        return offset < other.offset;
    }

    bool operator>(const CompartmentLocation& other) const {
        return other < *this;
    }

    bool operator<=(const CompartmentLocation& other) const {
        return !(other < *this);
    }

    bool operator>=(const CompartmentLocation& other) const {
        return !(*this < other);
    }
};

/// Ostream << operator used by catch2 when there are problems for example
inline std::ostream& operator<<(std::ostream& os, const CompartmentLocation& cl) {
    os << "CompartmentLocation("
       << "nodeId: " << cl.nodeId << ", "
       << "sectionId: " << cl.sectionId << ", "
       << "offset: " << cl.offset << ")";
    return os;
}

class SONATA_API CompartmentSetFilteredIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = CompartmentLocation;
    using difference_type = std::ptrdiff_t;
    using pointer = const CompartmentLocation*;
    using reference = const CompartmentLocation&;

    explicit CompartmentSetFilteredIterator(
        std::unique_ptr<detail::CompartmentSetFilteredIterator> impl);
    CompartmentSetFilteredIterator(const CompartmentSetFilteredIterator& other);
    CompartmentSetFilteredIterator& operator=(const CompartmentSetFilteredIterator& other);
    CompartmentSetFilteredIterator(CompartmentSetFilteredIterator&&) noexcept;
    CompartmentSetFilteredIterator& operator=(CompartmentSetFilteredIterator&&) noexcept;
    ~CompartmentSetFilteredIterator();

    const CompartmentLocation& operator*() const;
    const CompartmentLocation* operator->() const;

    CompartmentSetFilteredIterator& operator++();    // prefix ++
    CompartmentSetFilteredIterator operator++(int);  // postfix ++
    bool operator==(const CompartmentSetFilteredIterator& other) const;
    bool operator!=(const CompartmentSetFilteredIterator& other) const;

  private:
    std::unique_ptr<detail::CompartmentSetFilteredIterator> impl_;
};


/**
 * CompartmentSet public API.
 *
 * This class represents a set of compartment locations associated with a neuron population.
 * Each compartment is uniquely defined by a (node_id, section_index, offset) triplet.
 * This API supports filtering based on a node_id selection.
 */
class SONATA_API CompartmentSet
{
  public:
    CompartmentSet() = delete;

    explicit CompartmentSet(const std::string& json_content);
    explicit CompartmentSet(std::shared_ptr<detail::CompartmentSet>&& impl);

    std::pair<CompartmentSetFilteredIterator, CompartmentSetFilteredIterator> filtered_crange(
        Selection selection = Selection({})) const;

    /// Size of the set, optionally filtered by selection
    std::size_t size(const Selection& selection = Selection({})) const;

    // Is empty?
    bool empty() const;

    /// Population name
    const std::string& population() const;

    /// Access element by index. It returns a copy!
    CompartmentLocation operator[](std::size_t index) const;

    Selection nodeIds() const;

    CompartmentSet filter(const Selection& selection = Selection({})) const;

    /// Serialize to JSON string
    std::string toJSON() const;

    bool operator==(const CompartmentSet& other) const;
    bool operator!=(const CompartmentSet& other) const;

  private:
    std::shared_ptr<detail::CompartmentSet> impl_;
};


/**
 * @class CompartmentSets
 * @brief A container class that manages a collection of named CompartmentSet objects.
 *
 * This class provides methods for accessing, querying, and serializing a collection of
 * compartment sets identified by string keys. It supports construction from a JSON string
 * or a file, and encapsulates its internal implementation using the PIMPL idiom.
 *
 * The class is non-copyable but movable, and offers value-style accessors for ease of use.
 */
class SONATA_API CompartmentSets
{
  public:
    CompartmentSets(const std::string& content);
    CompartmentSets(std::unique_ptr<detail::CompartmentSets>&& impl);
    CompartmentSets(detail::CompartmentSets&& impl);
    CompartmentSets(CompartmentSets&&) noexcept;
    CompartmentSets(const CompartmentSets& other) = delete;
    CompartmentSets& operator=(CompartmentSets&&) noexcept;
    ~CompartmentSets();

    /// Create new CompartmentSets from file. In this way we distinguish from
    /// the basic string constructor.
    static CompartmentSets fromFile(const std::string& path);

    /// Access element by key (throws if not found)
    CompartmentSet getCompartmentSet(const std::string& key) const;

    /// Number of compartment sets
    std::size_t size() const;

    /// Is empty?
    bool empty() const;

    /// Check if key exists
    bool contains(const std::string& key) const;

    /// Get names of CompartmentSet(s) as a vector
    std::vector<std::string> names() const;

    /// Get all compartment sets as vector
    std::vector<CompartmentSet> getAllCompartmentSets() const;

    /// Get items (key + compartment set) as vector of pairs
    std::vector<std::pair<std::string, CompartmentSet>> items() const;

    /// Serialize all compartment sets to JSON string
    std::string toJSON() const;

    bool operator==(const CompartmentSets& other) const;
    bool operator!=(const CompartmentSets& other) const;

  private:
    std::unique_ptr<detail::CompartmentSets> impl_;
};

}  // namespace sonata
}  // namespace bbp