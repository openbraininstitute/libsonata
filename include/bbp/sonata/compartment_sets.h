#pragma once

#include <bbp/sonata/nodes.h>

namespace bbp {
namespace sonata {
namespace detail {
class CompartmentLocation;
class CompartmentSet;
class CompartmentSets;
}  // namespace detail

/**
 * CompartmentLocation public API.
 * 
 * This class uniquely identifies a compartment by a set of gid, section_idx and offset:
 * 
 * - gid: Global ID of the cell (Neuron) to which the compartment belongs. No 
 * overlaps among populations.
 * - section_idx: Absolute section index. Progressive index that uniquely identifies the section.
 *  There is a mapping between neuron section names (i.e. dend[10]) and this index.
 * - offset: Offset of the compartment along the section. The offset is a value between 0 and 1
 */
class SONATA_API CompartmentLocation
{
  public:
    CompartmentLocation() = delete;
    CompartmentLocation(const int64_t gid, const int64_t section_idx, const double offset);
    explicit CompartmentLocation(const std::string& content);
    explicit CompartmentLocation(std::unique_ptr<detail::CompartmentLocation>&& impl);
    CompartmentLocation(const CompartmentLocation& other);
    CompartmentLocation& operator=(const CompartmentLocation& other);
    CompartmentLocation(CompartmentLocation&&) noexcept;
    CompartmentLocation& operator=(CompartmentLocation&&) noexcept;
    ~CompartmentLocation();

    bool operator==(const CompartmentLocation& other) const noexcept;
    bool operator!=(const CompartmentLocation& other) const noexcept;

    uint64_t gid() const;
    uint64_t sectionIdx() const;
    double offset() const;

    std::string toJSON() const;

  private:
    std::unique_ptr<detail::CompartmentLocation> impl_;
};


// class SONATA_API CompartmentSet
// {
//   public:
//     /**
//      * Create CompartmentSet from JSON
//      *
//      * See also:
//      * TODO
//      *
//      * \param content is the JSON compartment_set value
//      * \throw if content cannot be parsed
//      */
//     CompartmentSet(const std::string& content);
//     CompartmentSet(std::unique_ptr<detail::CompartmentSet>&& impl);
//     CompartmentSet(detail::CompartmentSet&& impl);
//     CompartmentSet(CompartmentSet&&) noexcept;
//     CompartmentSet(const CompartmentSet& other) = delete;
//     CompartmentSet& operator=(CompartmentSet&&) noexcept;
//     ~CompartmentSet();

    
//     std::size_t size() const;

//     CompartmentLocation operator[](std::size_t index) const;
  
//     /**
//      * Population name
//      */
//     const std::string& population() const;

//     /**
//      * Get the CompartmentLocations.
//      */
//     std::vector<CompartmentLocation> locations(
//         const Selection& selection = Selection({})) const;

//     /**
//      * Return the gids of the compartment locations.
//      */
//     Selection gids() const;

//     /**
//      * Return the nodesets as a JSON string.
//      */
//     std::string toJSON() const;

//   private:
//     std::unique_ptr<detail::CompartmentSet> impl_;
// };

// class SONATA_API CompartmentSets
// {
// public:
//     // Keep these exactly as-is:
//     CompartmentSets(const std::string& content);
//     CompartmentSets(std::unique_ptr<detail::CompartmentSets>&& impl);
//     CompartmentSets(detail::CompartmentSets&& impl);
//     CompartmentSets(CompartmentSets&&) noexcept;
//     CompartmentSets(const CompartmentSets& other) = delete;
//     CompartmentSets& operator=(CompartmentSets&&) noexcept;
//     ~CompartmentSets();
    
//     static CompartmentSets fromFile(const std::string& path);

//     // Read-only dict-like API, Python-style names:
//     size_t size() const;
//     bool contains(const std::string& name) const;

//     std::vector<std::string> keys() const;
//     std::vector<CompartmentSet> values() const;
//     std::vector<std::pair<std::string, CompartmentSet>> items() const;

//     CompartmentSet get(const std::string& name) const;

//     std::string toJSON() const;

// private:
//     std::unique_ptr<detail::CompartmentSets> impl_;
// };

}  // namespace sonata
}  // namespace bbp
