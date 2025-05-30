#pragma once

#include <bbp/sonata/nodes.h>

namespace bbp {
namespace sonata {
namespace detail {
  class CompartmentLocation;
class CompartmentSet;
class CompartmentSets;
}  // namespace detail

class SONATA_API CompartmentLocation
{
  public:
    /**
     * Create CompartmentLocation from JSON
     *
     * See also:
     * TODO
     *
     * \param content is the JSON compartment_set value
     * \throw if content cannot be parsed
     */
    explicit CompartmentLocation(const int64_t gid, const int64_t section_idx, const double offset);
    explicit CompartmentLocation(const std::string& content);                   
    explicit CompartmentLocation(std::unique_ptr<detail::CompartmentLocation>&& impl);
    CompartmentLocation(CompartmentLocation&&) noexcept;
    CompartmentLocation(const CompartmentLocation& other) = delete;
    CompartmentLocation& operator=(CompartmentLocation&&) noexcept;
    bool operator==(const CompartmentLocation& other) const noexcept;
    ~CompartmentLocation();

    /**
     * GID
     */
    uint64_t gid() const;

    /**
     * Absolute section index. Progressive index that uniquely identifies the section. 
     * There is a mapping between neuron section names (i.e. dend[10]) and this index.
     */
    uint64_t sectionIdx() const;

    /**
     * Offset of the compartment along the section.
     */
    double offset() const;

    /**
     * Return the nodesets as a JSON string.
     */
    std::string toJSON() const;

  private:
    std::unique_ptr<detail::CompartmentLocation> impl_;
};


class SONATA_API CompartmentSet
{
  public:
    /**
     * Create CompartmentSet from JSON
     *
     * See also:
     * TODO
     *
     * \param content is the JSON compartment_set value
     * \throw if content cannot be parsed
     */
    explicit CompartmentSet(const std::string& content);
    explicit CompartmentSet(std::unique_ptr<detail::CompartmentSet>&& impl);
    CompartmentSet(CompartmentSet&&) noexcept;
    CompartmentSet(const CompartmentSet& other) = delete;
    CompartmentSet& operator=(CompartmentSet&&) noexcept;
    ~CompartmentSet();

    /**
     * Population name
     */
    const std::string& population() const;

    /**
     * Get the CompartmentLocations.
     */
    std::vector<CompartmentLocation> getCompartmentLocations(const Selection& selection = Selection({})) const;

    /**
     * Return the gids of the compartment locations.
     */
    Selection gids() const;

    /**
     * Return the nodesets as a JSON string.
     */
    std::string toJSON() const;

  private:
    std::unique_ptr<detail::CompartmentSet> impl_;
};

class SONATA_API CompartmentSets
{
  public:
    /**
     * Create CompartmentSets from JSON
     *
     * See also:
     * TODO
     *
     * \param content is the JSON compartment_sets value
     * \throw if content cannot be parsed
     */
    explicit CompartmentSets(const std::string& content);
    explicit CompartmentSets(std::unique_ptr<detail::CompartmentSets>&& impl);
    CompartmentSets(CompartmentSets&&) noexcept;
    CompartmentSets(const CompartmentSets& other) = delete;
    CompartmentSets& operator=(CompartmentSets&&) noexcept;
    ~CompartmentSets();
    size_t size() const;
    bool contains(const std::string& name) const;

    /** Open a SONATA `Compartment sets` file from a path */
    static CompartmentSets fromFile(const std::string& path);

    /**
     * Names of the node sets available
     */
    std::set<std::string> names() const;

    /**
     * Return the nodesets as a JSON string.
     */
    std::string toJSON() const;

    // TODO
    CompartmentSet getCompartmentSet(const std::string& name);

  private:
    std::unique_ptr<detail::CompartmentSets> impl_;
};

}  // namespace sonata
}  // namespace bbp
