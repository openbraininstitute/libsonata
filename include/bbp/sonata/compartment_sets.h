#pragma once

#include <bbp/sonata/nodes.h>

namespace bbp {
namespace sonata {
namespace detail {
class CompartmentLocation;
class CompartmentSetFilteredIterator;
class CompartmentSet;
class CompartmentSets;
}  // namespace detail
/**
 * CompartmentLocation public API.
 *
 * This class uniquely identifies a compartment by a set of node_id, section_index and offset:
 *
 * - node_id: Global ID of the cell (Neuron) to which the compartment belongs. No
 * overlaps among populations.
 * - section_index: Absolute section index. Progressive index that uniquely identifies the section.
 *  There is a mapping between neuron section names (i.e. dend[10]) and this index.
 * - offset: Offset of the compartment along the section. The offset is a value between 0 and 1
 */
class SONATA_API CompartmentLocation
{
  public:
    CompartmentLocation();
    CompartmentLocation(const int64_t node_id, const int64_t section_index, const double offset);
    explicit CompartmentLocation(const std::string& content);
    explicit CompartmentLocation(std::unique_ptr<detail::CompartmentLocation>&& impl);
    CompartmentLocation(const CompartmentLocation& other);
    CompartmentLocation& operator=(const CompartmentLocation& other);
    CompartmentLocation(CompartmentLocation&&) noexcept;
    CompartmentLocation& operator=(CompartmentLocation&&) noexcept;
    ~CompartmentLocation();

    bool operator==(const CompartmentLocation& other) const noexcept;
    bool operator!=(const CompartmentLocation& other) const noexcept;

    uint64_t nodeId() const;
    uint64_t sectionIndex() const;
    double offset() const;

    std::string toJSON() const;

  private:
    std::unique_ptr<detail::CompartmentLocation> impl_;
};

class SONATA_API CompartmentSetFilteredIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = CompartmentLocation;
    using difference_type = std::ptrdiff_t;
    using pointer = void;                // dereference returns by value
    using reference = CompartmentLocation; // dereference returns by value

    explicit CompartmentSetFilteredIterator(std::unique_ptr<detail::CompartmentSetFilteredIterator> impl);
    CompartmentSetFilteredIterator(const CompartmentSetFilteredIterator& other);
    CompartmentSetFilteredIterator& operator=(const CompartmentSetFilteredIterator& other);
    CompartmentSetFilteredIterator(CompartmentSetFilteredIterator&&) noexcept;
    CompartmentSetFilteredIterator& operator=(CompartmentSetFilteredIterator&&) noexcept;
    ~CompartmentSetFilteredIterator();

    /// Dereference operator. It makes a copy!
    CompartmentLocation operator*() const;
    /// Arrow operator is voluntarely disabled because we can only return copies of CompartmentLocation.
    /// In any way we need to find a location to store a temp CompartmentLocation and memory leaks become possible.
    CompartmentSetFilteredIterator& operator++();            // prefix ++
    CompartmentSetFilteredIterator operator++(int);          // postfix ++
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

    std::pair<CompartmentSetFilteredIterator, CompartmentSetFilteredIterator>
    filtered_crange(Selection selection = Selection({})) const;

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
    CompartmentSet at(const std::string& key) const;

    /// Number of compartment sets
    std::size_t size() const;

    /// Is empty?
    bool empty() const;

    /// Check if key exists
    bool contains(const std::string& key) const;

    /// Get keys as a vector (use vector here)
    std::vector<std::string> keys() const;

    /// Get all compartment sets as vector
    std::vector<CompartmentSet> values() const;

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
