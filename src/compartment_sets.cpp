#include "../extlib/filesystem.hpp"

#include <unordered_set>
#include <functional>
#include <cmath>

#include "utils.h"  // readFile

#include <bbp/sonata/compartment_sets.h>
namespace bbp {
namespace sonata {

namespace fs = ghc::filesystem;

namespace detail {

using json = nlohmann::json;

class CompartmentLocation {
    std::uint64_t gid_;
    std::uint64_t section_idx_;
    double offset_;

    void setGid(int64_t gid) {
        if (gid < 0) {
            throw SonataError(fmt::format("GID must be non-negative, got {}", gid));
        }
        gid_ = static_cast<uint64_t>(gid);
    }
    void setSectionIdx(int64_t section_idx) {
        if (section_idx < 0) {
            throw SonataError(fmt::format("Section index must be non-negative, got {}", section_idx));
        }
        section_idx_ = static_cast<uint64_t>(section_idx);
    }
    void setOffset(double offset) {
        if (offset < 0.0 || offset > 1.0) {
            throw SonataError(fmt::format("Offset must be between 0 and 1 inclusive, got {}", offset));
        }
        offset_ = offset;
    }

public:
    static constexpr double offsetTolerance = 1e-4;
    static constexpr double offsetToleranceInv = 1.0 / offsetTolerance;

    explicit CompartmentLocation(int64_t gid, int64_t section_idx, double offset) {
        setGid(gid);
        setSectionIdx(section_idx);
        setOffset(offset);
    }

    explicit CompartmentLocation(const std::string& content)
        : CompartmentLocation(json::parse(content)) {}

    CompartmentLocation(const nlohmann::json& j) {
        if (!j.is_array() || j.size() != 3) {
            throw SonataError("CompartmentLocation must be an array of exactly 3 elements: [gid, section_idx, offset]");
        }

        setGid(get_uint64_or_throw(j[0]));
        setSectionIdx(get_uint64_or_throw(j[1]));

        if (!j[2].is_number()) {
            throw SonataError("Fourth element (offset) must be a number");
        }
        setOffset(j[2].get<double>());
    }

    uint64_t gid() const {
        return gid_;
    }

    uint64_t sectionIdx() const {
        return section_idx_;
    }

    double offset() const {
        return offset_;
    }

    nlohmann::json to_json() const {
        return nlohmann::json::array({gid_, section_idx_, offset_});
    }

    bool operator==(const CompartmentLocation& other) const {
        return gid_ == other.gid_
            && section_idx_ == other.section_idx_
            && std::abs(offset_ - other.offset_) < offsetTolerance;
    }
};

// Custom hash for CompartmentLocation
struct CompartmentLocationHash {
    std::size_t operator()(const CompartmentLocation& loc) const noexcept {
        std::size_t h1 = std::hash<uint64_t>{}(loc.gid());
        std::size_t h2 = std::hash<uint64_t>{}(loc.sectionIdx());

        // Quantize offset to 4 decimal places
        double offset = loc.offset();
        uint64_t quantized_offset = static_cast<uint64_t>(std::round(offset * CompartmentLocation::offsetToleranceInv));

        std::size_t h3 = std::hash<uint64_t>{}(quantized_offset);

        // Combine hashes (boost style)
        std::size_t seed = 0;
        seed ^= h1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return seed;
    }
};

class CompartmentSet {
    std::string population_;
    std::vector<CompartmentLocation> compartment_locations_;

public:

    explicit CompartmentSet(const std::string& content)
        : CompartmentSet(json::parse(content)) {}

    explicit CompartmentSet(const nlohmann::json& j) {
        if (!j.is_object()) {
            throw SonataError("CompartmentSet must be an object");
        }

        // Extract and check 'population' key once
        auto pop_it = j.find("population");
        if (pop_it == j.end() || !pop_it->is_string()) {
            throw SonataError("CompartmentSet must contain 'population' key of string type");
        }
        population_ = pop_it->get<std::string>();

        // Extract and check 'compartment_set' key once
        auto comp_it = j.find("compartment_set");
        if (comp_it == j.end() || !comp_it->is_array()) {
            throw SonataError("CompartmentSet must contain 'compartment_set' key of array type");
        }

        compartment_locations_.reserve(comp_it->size());
        for (const auto& el : *comp_it) {
            compartment_locations_.emplace_back(el);
        }
        compartment_locations_.shrink_to_fit();
    }


    Selection gids() const {
        std::vector<uint64_t> result;
        std::unordered_set<uint64_t> seen;

        result.reserve(compartment_locations_.size());
        for (const auto& elem : compartment_locations_) {
            uint64_t id = elem.gid();
            if (seen.insert(id).second) { // insert returns {iterator, bool}
                result.push_back(id);
            }
        }
        sort(result.begin(), result.end());
        return Selection::fromValues(result.begin(), result.end());
    }

    const std::string& population() const {
        return population_;
    }

    std::vector<std::unique_ptr<CompartmentLocation>>
    getCompartmentLocations(const Selection& selection) const {
        std::vector<std::unique_ptr<CompartmentLocation>> result;
        result.reserve(compartment_locations_.size());

        if (selection.empty()) {
            for (const auto& el : compartment_locations_) {
                result.emplace_back(std::make_unique<detail::CompartmentLocation>(el));
            }
        } else {
            for (const auto& el : compartment_locations_) {
                if (selection.contains(el.gid())) {
                    result.emplace_back(std::make_unique<detail::CompartmentLocation>(el));
                }
            }
        }
        result.shrink_to_fit();

        return result;
    }

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["population"] = population_;

        j["compartment_set"] = nlohmann::json::array();
        for (const auto& elem : compartment_locations_) {
            j["compartment_set"].push_back(elem.to_json());
        }

        return j;
    }
};
class CompartmentSets
{

std::map<std::string, CompartmentSet> compartment_sets_;

  public:
    explicit CompartmentSets(const json& j) {
        if (!j.is_object()) {
            throw SonataError("Top level compartment_set must be an object");
        }
        for (const auto& el : j.items()) {
            compartment_sets_.emplace(el.key(), el.value());
        }
    }
    
    static const fs::path& validate_path(const fs::path& path) {
        if (!fs::exists(path)) {
            throw SonataError(fmt::format("Path does not exist: {}", std::string(path)));
        }
        return path;
    }

    static std::unique_ptr<CompartmentSets> fromFile(const std::string& path_) {
        fs::path path(path_);
        return std::make_unique<detail::CompartmentSets>(path);
    }

    explicit CompartmentSets(const fs::path& path)
        : CompartmentSets(json::parse(std::ifstream(validate_path(path)))) {}

    explicit CompartmentSets(const std::string& content)
        : CompartmentSets(json::parse(content)) {}

    size_t size() const {
        return compartment_sets_.size();
    }
    bool contains(const std::string& name) const {
        return compartment_sets_.find(name) != compartment_sets_.end();
    }

    std::set<std::string> names() const {
        return getMapKeys(compartment_sets_);
    }

    CompartmentSet getCompartmentSet(const std::string& name) const {
        auto it = compartment_sets_.find(name);
        if (it == compartment_sets_.end()) {
            throw SonataError(fmt::format("CompartmentSet '{}' not found", name));
        }
        return it->second;
    }

    nlohmann::json to_json() const {
        nlohmann::json j;
        for (const auto& entry : compartment_sets_) {
            j[entry.first] = entry.second.to_json();
        }
        return j;
    }
};



}  // namespace detail

// CompartmentLocation python API

CompartmentLocation::CompartmentLocation(const int64_t gid,
                                             const int64_t section_idx,
                                             const double offset)
    : impl_(new detail::CompartmentLocation(gid, section_idx, offset)) {}

CompartmentLocation::CompartmentLocation(const std::string& content)
    : impl_(new detail::CompartmentLocation(content)) {}

CompartmentLocation::CompartmentLocation(std::unique_ptr<detail::CompartmentLocation>&& impl)
    : impl_(std::move(impl)) {}

CompartmentLocation::CompartmentLocation(CompartmentLocation&&) noexcept = default;
CompartmentLocation& CompartmentLocation::operator=(CompartmentLocation&&) noexcept = default;
CompartmentLocation::~CompartmentLocation() = default;

bool CompartmentLocation::operator==(const CompartmentLocation& other) const noexcept {
    return *impl_ == *(other.impl_);
}

uint64_t CompartmentLocation::gid() const {
    return impl_->gid();
}

uint64_t CompartmentLocation::sectionIdx() const {
    return impl_->sectionIdx();
}

double CompartmentLocation::offset() const {
    return impl_->offset();
}

std::string CompartmentLocation::toJSON() const {
    return impl_->to_json().dump(4); // Pretty print with 4 spaces
}

// CompartmentSet python API

CompartmentSet::CompartmentSet(const std::string& content)
    : impl_(new detail::CompartmentSet(content)) {}

CompartmentSet::CompartmentSet(std::unique_ptr<detail::CompartmentSet>&& impl)
    : impl_(std::move(impl)) {}

CompartmentSet::CompartmentSet(CompartmentSet&&) noexcept = default;
CompartmentSet& CompartmentSet::operator=(CompartmentSet&&) noexcept = default;
CompartmentSet::~CompartmentSet() = default;

const std::string& CompartmentSet::population() const {
    return impl_->population();
}

std::vector<CompartmentLocation> CompartmentSet::getCompartmentLocations(const Selection& selection) const {
    std::vector<CompartmentLocation> view;
    auto raw_locs = impl_->getCompartmentLocations(selection);
    view.reserve(raw_locs.size());
    for (auto& el : raw_locs) {
        view.emplace_back(std::move(el));  // take ownership
    }
    return view;
}

Selection CompartmentSet::gids() const {
    return impl_->gids();
}

std::string CompartmentSet::toJSON() const {
    return impl_->to_json().dump(4); // Pretty print with 4 spaces
}

// CompartmentSets python API

CompartmentSets::CompartmentSets(const std::string& content)
    : impl_(new detail::CompartmentSets(content)) {}

CompartmentSets::CompartmentSets(std::unique_ptr<detail::CompartmentSets>&& impl)
    : impl_(std::move(impl)) {}

CompartmentSets::CompartmentSets(CompartmentSets&&) noexcept = default;
CompartmentSets& CompartmentSets::operator=(CompartmentSets&&) noexcept = default;
CompartmentSets::~CompartmentSets() = default;

CompartmentSets CompartmentSets::fromFile(const std::string& path) {
    return CompartmentSets(detail::CompartmentSets::fromFile(path));
}

size_t CompartmentSets::size() const {
    return impl_->size();
}

bool CompartmentSets::contains(const std::string& name) const {
    return impl_->contains(name);
}

std::set<std::string> CompartmentSets::names() const {
    return impl_->names();
}

CompartmentSet CompartmentSets::getCompartmentSet(const std::string& name) {
    return CompartmentSet(std::make_unique<detail::CompartmentSet>(impl_->getCompartmentSet(name)));
}

std::string CompartmentSets::toJSON() const {
    return impl_->to_json().dump(4); // Pretty print with 4 spaces
}


}  // namespace sonata
}  // namespace bbp