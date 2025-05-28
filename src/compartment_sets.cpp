#include "../extlib/filesystem.hpp"

#include <unordered_set>

#include "utils.h"  // readFile

#include <bbp/sonata/compartment_sets.h>
namespace bbp {
namespace sonata {

namespace fs = ghc::filesystem;

namespace detail {

using json = nlohmann::json;

class CompartmentSetElement {
    std::uint64_t gid_;
    std::string section_name_;
    std::uint64_t section_index_;
    double location_;

public:

    explicit CompartmentSetElement(uint64_t gid,
                                   const std::string& section_name,
                                   uint64_t section_index,
                                   double location)
        : gid_(gid),
          section_name_(section_name),
          section_index_(section_index),
          location_(location) {
        if (location < 0.0 || location > 1.0) {
            throw SonataError(fmt::format("Location must be between 0 and 1 inclusive, got {}", location));
        }
    }

    explicit CompartmentSetElement(const std::string& content)
        : CompartmentSetElement(json::parse(content)) {}

    CompartmentSetElement(const nlohmann::json& j) {
        if (!j.is_array() || j.size() != 4) {
            throw SonataError("CompartmentSetElement must be an array of exactly 4 elements: [gid, \"section_name\", section_index, location]");
        }

        gid_ = get_uint64_or_throw(j[0]);

        if (!j[1].is_string()) {
            throw SonataError("Second element (section_name) must be a string");
        }
        section_name_ = j[1].get<std::string>();

        section_index_ = get_uint64_or_throw(j[2]);

        if (!j[3].is_number()) {
            throw SonataError("Fourth element (location) must be a number");
        }
        const double location = j[3].get<double>();
        if (location < 0.0 || location > 1.0) {
            throw SonataError(fmt::format("Location must be between 0 and 1 inclusive, got {}", location));
        }
        location_ = location;
    }

    uint64_t gid() const {
        return gid_;
    }

    const std::string& sectionName() const {
        return section_name_;
    }

    uint64_t sectionIndex() const {
        return section_index_;
    }

    double location() const {
        return location_;
    }

    nlohmann::json to_json() const {
        return nlohmann::json::array({gid_, section_name_, section_index_, location_});
    }
};


class CompartmentSet {
    std::string population_;
    std::vector<CompartmentSetElement> compartment_set_elements_;

public:

    explicit CompartmentSet(const std::string& content)
        : CompartmentSet(json::parse(content)) {}

    explicit CompartmentSet(const nlohmann::json& j) {
        if (!j.is_object()) {
            throw SonataError("CompartmentSet must be an object");
        }

        if (j.contains("population")) {
            if (!j.at("population").is_string()) {
                throw SonataError("'population' must be a string");
            }
            population_ = j.at("population").get<std::string>();
        } else {
            throw SonataError("CompartmentSet must contain 'population' key");
        }

        if (j.contains("compartment_set")) {
            if (!j.at("compartment_set").is_array()) {
                throw SonataError("'compartment_set' must be an array");
            }

            for (const auto& el : j.at("compartment_set")) {
                compartment_set_elements_.emplace_back(el);
            }
        } else {
            throw SonataError("CompartmentSet must contain 'compartment_set' key");
        }
    }

    std::vector<uint64_t> gids() const {
        std::vector<uint64_t> result;
        std::unordered_set<uint64_t> seen;

        result.reserve(compartment_set_elements_.size());
        for (const auto& elem : compartment_set_elements_) {
            uint64_t id = elem.gid();
            if (seen.insert(id).second) { // insert returns {iterator, bool}
                result.push_back(id);
            }
        }
        return result;
    }

    const std::string& population() const {
        return population_;
    }

    std::vector<CompartmentSetElement>& getElements() {
    return compartment_set_elements_;
    }

    const std::vector<CompartmentSetElement>& getElements() const {
        return compartment_set_elements_;
    }

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["population"] = population_;

        j["compartment_set"] = nlohmann::json::array();
        for (const auto& elem : compartment_set_elements_) {
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

// CompartmentSetElement python API

CompartmentSetElement::CompartmentSetElement(const uint64_t gid,
                                             const std::string& section_name,
                                             const uint64_t section_index,
                                             const double location)
    : impl_(new detail::CompartmentSetElement(gid, section_name, section_index, location)) {}

CompartmentSetElement::CompartmentSetElement(const std::string& content)
    : impl_(new detail::CompartmentSetElement(content)) {}

CompartmentSetElement::CompartmentSetElement(std::unique_ptr<detail::CompartmentSetElement>&& impl)
    : impl_(std::move(impl)) {}

CompartmentSetElement::CompartmentSetElement(CompartmentSetElement&&) noexcept = default;
CompartmentSetElement& CompartmentSetElement::operator=(CompartmentSetElement&&) noexcept = default;
CompartmentSetElement::~CompartmentSetElement() = default;

uint64_t CompartmentSetElement::gid() const {
    return impl_->gid();
}
const std::string& CompartmentSetElement::sectionName() const {
    return impl_->sectionName();
}
uint64_t CompartmentSetElement::sectionIndex() const {
    return impl_->sectionIndex();
}

double CompartmentSetElement::location() const {
    return impl_->location();
}

std::string CompartmentSetElement::toJSON() const {
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

std::vector<CompartmentSetElement> CompartmentSet::getElements() {
    std::vector<CompartmentSetElement> view;
    view.reserve(impl_->getElements().size());
    for (auto& el : impl_->getElements()) {
        view.emplace_back(std::make_unique<detail::CompartmentSetElement>(el));
    }
    return view;
}

std::vector<uint64_t> CompartmentSet::gids() const {
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