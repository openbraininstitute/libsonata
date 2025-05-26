#pragma once

#include <bbp/sonata/nodes.h>

namespace bbp {
namespace sonata {
namespace detail {
class CompartmentSets;
}  // namespace detail

class SONATA_API CompartmentSets
{
  public:
    /**
     * Create compartmentset from JSON
     *
     * See also:
     * TODO
     *
     * \param content is the JSON node_sets value
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
     * Return the nodesets as a JSON string.
     */
    std::string toJSON() const;

  private:
    std::unique_ptr<detail::CompartmentSets> impl_;
};

}  // namespace sonata
}  // namespace bbp
