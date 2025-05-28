#pragma once

#include <bbp/sonata/nodes.h>

namespace bbp {
namespace sonata {
namespace detail {
  class CompartmentSetElement;
class CompartmentSet;
class CompartmentSets;
}  // namespace detail

class SONATA_API CompartmentSetElement
{
  public:
    /**
     * Create CompartmentSetElement from JSON
     *
     * See also:
     * TODO
     *
     * \param content is the JSON compartment_set value
     * \throw if content cannot be parsed
     */
    explicit CompartmentSetElement(const uint64_t gid, const std::string& section_name,
                            const uint64_t section_index, const double location);
    explicit CompartmentSetElement(const std::string& content);                   
    explicit CompartmentSetElement(std::unique_ptr<detail::CompartmentSetElement>&& impl);
    CompartmentSetElement(CompartmentSetElement&&) noexcept;
    CompartmentSetElement(const CompartmentSetElement& other) = delete;
    CompartmentSetElement& operator=(CompartmentSetElement&&) noexcept;
    ~CompartmentSetElement();

    /**
     * GID
     */
    uint64_t gid() const;
    /**
     * Section name
     */
    const std::string& sectionName() const;
    /**
     * Section index
     */
    uint64_t sectionIndex() const;

    /**
     * Location in the section
     */
    double location() const;

    /**
     * Return the nodesets as a JSON string.
     */
    std::string toJSON() const;

  private:
    std::unique_ptr<detail::CompartmentSetElement> impl_;
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
     * Get the Elements
     */
    std::vector<CompartmentSetElement> getElements();

    /**
     * Return the gids of the compartment set elements.
     */
    std::vector<uint64_t> gids() const;

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
