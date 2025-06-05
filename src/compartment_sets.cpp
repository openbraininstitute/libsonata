

#include "../extlib/filesystem.hpp"

#include "utils.h"  // readFile

#include <unordered_set>

#include <bbp/sonata/compartment_sets.h>
namespace bbp {
namespace sonata {

namespace fs = ghc::filesystem;

namespace detail {

using json = nlohmann::json;


class CompartmentLocation
{
    private:
        std::uint64_t node_id_;
        std::uint64_t section_index_;
        double offset_;

        void setNodeId(int64_t node_id) {
            if (node_id < 0) {
                throw SonataError(fmt::format("Node id must be non-negative, got {}", node_id));
            }
            node_id_ = static_cast<uint64_t>(node_id);
        }
        void setSectionIndex(int64_t section_index) {
            if (section_index < 0) {
                throw SonataError(
                    fmt::format("Section index must be non-negative, got {}", section_index));
            }
            section_index_ = static_cast<uint64_t>(section_index);
        }
        void setOffset(double offset) {
            if (offset < 0.0 || offset > 1.0) {
                throw SonataError(
                    fmt::format("Offset must be between 0 and 1 inclusive, got {}", offset));
            }
            offset_ = offset;
        }

  public:
    static constexpr double offsetTolerance = 1e-4;
    // static constexpr double offsetToleranceInv = 1.0 / offsetTolerance;

    CompartmentLocation(const CompartmentLocation& other) = default;
    CompartmentLocation(CompartmentLocation&&) noexcept = default;
    CompartmentLocation& operator=(CompartmentLocation&&) noexcept = default;
    CompartmentLocation(int64_t node_id, int64_t section_index, double offset) {
        setNodeId(node_id);
        setSectionIndex(section_index);
        setOffset(offset);
    }

    CompartmentLocation(const std::string& content)
        : CompartmentLocation(json::parse(content)) {}

    CompartmentLocation(const nlohmann::json& j) {
        if (!j.is_array() || j.size() != 3) {
            throw SonataError(
                "CompartmentLocation must be an array of exactly 3 elements: [node_id, section_index, "
                "offset]");
        }

        setNodeId(get_int64_or_throw(j[0]));
        setSectionIndex(get_int64_or_throw(j[1]));

        if (!j[2].is_number()) {
            throw SonataError("Fourth element (offset) must be a number");
        }
        setOffset(j[2].get<double>());
    }

    uint64_t nodeId() const {
        return node_id_;
    }

    uint64_t sectionIndex() const {
        return section_index_;
    }

    double offset() const {
        return offset_;
    }

    nlohmann::json to_json() const {
        return nlohmann::json::array({node_id_, section_index_, offset_});
    }

    bool operator==(const CompartmentLocation& other) const {
        return node_id_ == other.node_id_ && section_index_ == other.section_index_ &&
               std::abs(offset_ - other.offset_) < offsetTolerance;
    }
    bool operator!=(const CompartmentLocation& other) const {
        return !(*this == other);
    }

    std::unique_ptr<detail::CompartmentLocation> clone() const {
        return std::unique_ptr<detail::CompartmentLocation>(new CompartmentLocation(*this));
    }
};

class CompartmentSetFilteredIterator {
public:
    using base_iterator = std::vector<CompartmentLocation>::const_iterator;
    using value_type = detail::CompartmentLocation;
    using reference = const value_type&;
    using pointer = const value_type*;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
private:
    base_iterator current_;
    base_iterator end_;
    bbp::sonata::Selection selection_; // copied

    void skip_to_valid() {
        while (current_ != end_) {
            if (selection_.empty() || selection_.contains(current_->nodeId())) {
                break;
            }
            ++current_;
        }
    }

public:

    CompartmentSetFilteredIterator(base_iterator current,
                    base_iterator end,
                    bbp::sonata::Selection selection)
        : current_(current), end_(end), selection_(std::move(selection)) {
        skip_to_valid();
    }

    reference operator*() const {
        return *current_;
    }

    pointer operator->() const {
        return &(*current_);
    }

    CompartmentSetFilteredIterator& operator++() {
        ++current_;
        skip_to_valid();
        return *this;
    }

    CompartmentSetFilteredIterator operator++(int) {
        CompartmentSetFilteredIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    bool operator==(const CompartmentSetFilteredIterator& other) const {
        return current_ == other.current_;
    }

    bool operator!=(const CompartmentSetFilteredIterator& other) const {
        return !(*this == other);
    }

    std::unique_ptr<CompartmentSetFilteredIterator> clone() const {
        return std::make_unique<CompartmentSetFilteredIterator>(*this);
    }
};
class CompartmentSet {
public:
    using container_t = std::vector<CompartmentLocation>;
    class FilteredIterator;
private:
    // Private constructor for filter factory method

    std::string population_;
    container_t compartment_locations_;
    

    /**
     * Copy-construction is private. Used only for cloning.
     */
    CompartmentSet(const CompartmentSet& other) = default;
    CompartmentSet(const std::string& population, const container_t& compartment_locations): population_(population), compartment_locations_(compartment_locations) {}

public:
    
    // Construct from JSON string (delegates to JSON constructor)
    explicit CompartmentSet(const std::string& content)
        : CompartmentSet(nlohmann::json::parse(content)) {}

    // Construct from JSON object
    explicit CompartmentSet(const nlohmann::json& j) {
        if (!j.is_object()) {
            throw SonataError("CompartmentSet must be an object");
        }

        auto pop_it = j.find("population");
        if (pop_it == j.end() || !pop_it->is_string()) {
            throw SonataError("CompartmentSet must contain 'population' key of string type");
        }
        population_ = pop_it->get<std::string>();

        auto comp_it = j.find("compartment_set");
        if (comp_it == j.end() || !comp_it->is_array()) {
            throw SonataError("CompartmentSet must contain 'compartment_set' key of array type");
        }

        compartment_locations_.reserve(comp_it->size());
        for (auto&& el : *comp_it) {
            compartment_locations_.emplace_back(std::forward<decltype(el)>(el));
        }
        compartment_locations_.shrink_to_fit();
    }

    ~CompartmentSet() = default;
    CompartmentSet& operator=(const CompartmentSet&) = delete;
    CompartmentSet(CompartmentSet&&) noexcept = default;
    CompartmentSet& operator=(CompartmentSet&&) noexcept = default;

    std::pair<CompartmentSetFilteredIterator, CompartmentSetFilteredIterator>
    filtered_crange(bbp::sonata::Selection selection = Selection({})) const {
        CompartmentSetFilteredIterator begin_it(compartment_locations_.cbegin(),
                                compartment_locations_.cend(),
                                selection);
        CompartmentSetFilteredIterator end_it(compartment_locations_.cend(),
                                compartment_locations_.cend(),
                                std::move(selection));
        return {begin_it, end_it};
    }

    // Size with optional filter
    std::size_t size(const bbp::sonata::Selection& selection = bbp::sonata::Selection({})) const {
        if (selection.empty()) {
            return compartment_locations_.size();
        }

        return static_cast<std::size_t>(std::count_if(compartment_locations_.begin(),
                                                     compartment_locations_.end(),
            [&](const CompartmentLocation& loc) {
                return selection.contains(loc.nodeId());
            }));
    }

    std::size_t empty() const {
        return compartment_locations_.empty();
    }

    std::unique_ptr<CompartmentLocation> operator[](std::size_t index) const {
        return compartment_locations_.at(index).clone();
    }

    Selection nodeIds() const {
        std::vector<uint64_t> result;
        std::unordered_set<uint64_t> seen;

        result.reserve(compartment_locations_.size());
        for (const auto& elem : compartment_locations_) {
            uint64_t id = elem.nodeId();
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

    nlohmann::json to_json() const {
        nlohmann::json j;
        j["population"] = population_;

        j["compartment_set"] = nlohmann::json::array();
        for (const auto& elem : compartment_locations_) {
            j["compartment_set"].push_back(elem.to_json());
        }

        return j;
    }

    std::unique_ptr<CompartmentSet> clone() const {
        return std::unique_ptr<CompartmentSet>(new CompartmentSet(*this));
    }

    std::unique_ptr<CompartmentSet> filter(const bbp::sonata::Selection& selection = bbp::sonata::Selection({})) const {
        if (selection.empty()) {
            return clone();
        }
        std::vector<CompartmentLocation> filtered;
        filtered.reserve(compartment_locations_.size());
        for (const auto& el : compartment_locations_) {
            if (selection.contains(el.nodeId())) {
                filtered.emplace_back(el);
            }
        }
        return std::unique_ptr<CompartmentSet>(new CompartmentSet(population_, std::move(filtered)));
    }

    bool operator==(const CompartmentSet& other) const {
        return (population_ == other.population_) &&
            (compartment_locations_ == other.compartment_locations_);
    }

    bool operator!=(const CompartmentSet& other) const {
        return !(*this == other);
    }
};
class CompartmentSets
{
private:
    std::map<std::string, std::shared_ptr<detail::CompartmentSet>> data_;

public:
    CompartmentSets(const json& j) {
        if (!j.is_object()) {
            throw SonataError("Top level compartment_set must be an object");
        }

        for (const auto& el : j.items()) {
            data_.emplace(el.key(), std::make_shared<detail::CompartmentSet>(el.value()));
        }
    }

    static const fs::path& validate_path(const fs::path& path) {
        if (!fs::exists(path)) {
            throw SonataError(fmt::format("Path does not exist: {}", std::string(path)));
        }
        return path;
    }

    CompartmentSets(const fs::path& path)
        : CompartmentSets(json::parse(std::ifstream(validate_path(path)))) {}

    static CompartmentSets fromFile(const std::string& path_) {
        fs::path path(path_);
        return path;
    }

    CompartmentSets(const std::string& content)
        : CompartmentSets(json::parse(content)) {}


    std::shared_ptr<detail::CompartmentSet> at(const std::string& key) const {
        return data_.at(key);
    }

    std::size_t size() const {
        return data_.size();
    }

    bool contains(const std::string& key) const {
        return data_.find(key) != data_.end();
    }

    bool empty() const {
        return data_.empty();
    }

    std::vector<std::string> keys() const {
        std::vector<std::string> result;
        result.reserve(data_.size());
        std::transform(data_.begin(), data_.end(), std::back_inserter(result),
                    [](const auto& kv) { return kv.first; });
        return result;
    }

    std::vector<std::shared_ptr<detail::CompartmentSet>> values() const {
        std::vector<std::shared_ptr<detail::CompartmentSet>> result;
        result.reserve(data_.size());
        std::transform(data_.begin(), data_.end(), std::back_inserter(result),
            [](const auto& kv) { return kv.second; });
        return result;
    }

    std::vector<std::pair<std::string, std::shared_ptr<detail::CompartmentSet>>> items() const {
        std::vector<std::pair<std::string, std::shared_ptr<detail::CompartmentSet>>> result;
        result.reserve(data_.size());
        std::copy(data_.begin(), data_.end(), std::back_inserter(result));
        return result;
    }

    nlohmann::json to_json() const {
        nlohmann::json j;
        for (const auto& entry : data_) {
            j[entry.first] = entry.second->to_json();
        }
        return j;
    }

    bool operator==(const CompartmentSets& other) {
        if (data_.size() != other.data_.size()) {
            return false;
        }

        for (const auto& kv : data_) {
            const auto& key = kv.first;
            const auto& this_set = kv.second;

            auto it = other.data_.find(key);
            if (it == other.data_.end()) {
                return false;
            }

            const auto& other_set = it->second;
            if (this_set == nullptr && other_set == nullptr) {
                continue;
            }
            if (this_set == nullptr || other_set == nullptr) {
                return false;
            }
            if (*this_set != *other_set) {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const CompartmentSets& other) {
        return !(*this == other);
    }
};

}  // namespace detail

// CompartmentLocation public API
CompartmentLocation::CompartmentLocation() = default;
CompartmentLocation::CompartmentLocation(const int64_t node_id,
                                         const int64_t section_index,
                                         const double offset)
    : impl_(new detail::CompartmentLocation(node_id, section_index, offset)) { }
CompartmentLocation::CompartmentLocation(const std::string& content)
    : impl_(new detail::CompartmentLocation(content)) {}
CompartmentLocation::CompartmentLocation(std::unique_ptr<detail::CompartmentLocation>&& impl)
    : impl_(std::move(impl)) {}

CompartmentLocation::CompartmentLocation(const CompartmentLocation& other) : impl_(other.impl_->clone()){}
CompartmentLocation& CompartmentLocation::operator=(const CompartmentLocation& other) {
    if (this != &other) {
        auto tmp = other.impl_->clone();  // create copy first, if it throws impl is not assigned
        impl_ = std::move(tmp);           // then assign
    }
    return *this;
}
CompartmentLocation::CompartmentLocation(CompartmentLocation&&) noexcept = default;
CompartmentLocation& CompartmentLocation::operator=(CompartmentLocation&&) noexcept = default;
CompartmentLocation::~CompartmentLocation() = default;

bool CompartmentLocation::operator==(const CompartmentLocation& other) const noexcept {
    return *impl_ == *(other.impl_);
}

bool CompartmentLocation::operator!=(const CompartmentLocation& other) const noexcept {
    return *impl_ != *(other.impl_);
}

uint64_t CompartmentLocation::nodeId() const {
    return impl_->nodeId();
}

uint64_t CompartmentLocation::sectionIndex() const {
    return impl_->sectionIndex();
}

double CompartmentLocation::offset() const {
    return impl_->offset();
}

std::string CompartmentLocation::toJSON() const {
    return impl_->to_json().dump();
}

// CompartmentSetFilteredIterator public API

// Constructor
CompartmentSetFilteredIterator::CompartmentSetFilteredIterator(std::unique_ptr<detail::CompartmentSetFilteredIterator> impl)
    : impl_(std::move(impl)) {}

// Copy constructor
CompartmentSetFilteredIterator::CompartmentSetFilteredIterator(const CompartmentSetFilteredIterator& other)
    : impl_(other.impl_ ? other.impl_->clone() : nullptr) {}

// Copy assignment operator
CompartmentSetFilteredIterator& CompartmentSetFilteredIterator::operator=(const CompartmentSetFilteredIterator& other) {
    if (this != &other) {
        impl_ = other.impl_ ? other.impl_->clone() : nullptr;
    }
    return *this;
}

// Move constructor
CompartmentSetFilteredIterator::CompartmentSetFilteredIterator(CompartmentSetFilteredIterator&&) noexcept = default;

// Move assignment operator
CompartmentSetFilteredIterator& CompartmentSetFilteredIterator::operator=(CompartmentSetFilteredIterator&&) noexcept = default;


CompartmentSetFilteredIterator::~CompartmentSetFilteredIterator() = default;

CompartmentLocation CompartmentSetFilteredIterator::operator*() const {
    return CompartmentLocation(impl_->operator*().clone());
}

CompartmentSetFilteredIterator& CompartmentSetFilteredIterator::operator++() {
    ++(*impl_);
    return *this;
}

CompartmentSetFilteredIterator CompartmentSetFilteredIterator::operator++(int) {
    CompartmentSetFilteredIterator tmp(std::make_unique<detail::CompartmentSetFilteredIterator>(*impl_));
    ++(*impl_);
    return tmp;
}

bool CompartmentSetFilteredIterator::operator==(const CompartmentSetFilteredIterator& other) const {
    return *impl_ == *other.impl_;
}

bool CompartmentSetFilteredIterator::operator!=(const CompartmentSetFilteredIterator& other) const {
    return !(*this == other);
}

// CompartmentSet public API

CompartmentSet::CompartmentSet(const std::string& json_content)
    : impl_(std::make_shared<detail::CompartmentSet>(json_content)) {}

CompartmentSet::CompartmentSet(std::shared_ptr<detail::CompartmentSet>&& impl)
    : impl_(std::move(impl)) {}


std::pair<CompartmentSetFilteredIterator, CompartmentSetFilteredIterator>
CompartmentSet::filtered_crange(bbp::sonata::Selection selection) const {
    const auto internal_result = impl_->filtered_crange(std::move(selection));

    // Wrap clones of detail iterators in public API iterators
    return {
        CompartmentSetFilteredIterator(internal_result.first.clone()),
        CompartmentSetFilteredIterator(internal_result.second.clone())
    };
}

std::size_t CompartmentSet::size(const bbp::sonata::Selection& selection) const {
    return impl_->size(selection);
}

bool CompartmentSet::empty() const {
    return impl_->empty();
}

const std::string& CompartmentSet::population() const {
    return impl_->population();
}

CompartmentLocation CompartmentSet::operator[](std::size_t index) const {
    return CompartmentLocation((*impl_)[index]);
}

bbp::sonata::Selection CompartmentSet::nodeIds() const {
    return impl_->nodeIds();
}

CompartmentSet CompartmentSet::filter(const bbp::sonata::Selection& selection) const {
    return CompartmentSet(impl_->filter(selection));
}

bool CompartmentSet::operator==(const CompartmentSet& other) const {
    return *impl_ == *(other.impl_);
}

bool CompartmentSet::operator!=(const CompartmentSet& other) const {
    return *impl_ != *(other.impl_);
}

std::string CompartmentSet::toJSON() const {
    return impl_->to_json().dump();
}

// CompartmentSets public API

CompartmentSets::CompartmentSets(const std::string& content)
    : impl_(new detail::CompartmentSets(content)) {}
CompartmentSets::CompartmentSets(std::unique_ptr<detail::CompartmentSets>&& impl)
    : impl_(std::move(impl)) {}
CompartmentSets::CompartmentSets(detail::CompartmentSets&& impl)
    : CompartmentSets(std::make_unique<detail::CompartmentSets>(impl)) {}

CompartmentSets::CompartmentSets(CompartmentSets&&) noexcept = default;
CompartmentSets& CompartmentSets::operator=(CompartmentSets&&) noexcept = default;
CompartmentSets::~CompartmentSets() = default;



CompartmentSets CompartmentSets::fromFile(const std::string& path) {
    return detail::CompartmentSets::fromFile(path);
}

CompartmentSet CompartmentSets::at(const std::string& key) const {
    return CompartmentSet(impl_->at(key));
}

// Number of compartment sets
std::size_t CompartmentSets::size() const {
    return impl_->size();
}

// is empty?
bool CompartmentSets::empty() const {
    return impl_->empty();
}

// Check if key exists
bool CompartmentSets::contains(const std::string& key) const {
    return impl_->contains(key);
}

// Get keys as set or vector (use vector here)
std::vector<std::string> CompartmentSets::keys() const {
    return impl_->keys();
}

// Get all compartment sets as vector
std::vector<CompartmentSet> CompartmentSets::values() const {
    const auto vals = impl_->values();
    std::vector<CompartmentSet> result;
    result.reserve(vals.size());
    std::transform(vals.begin(), vals.end(), std::back_inserter(result),
        [](std::shared_ptr<detail::CompartmentSet> ptr) { return CompartmentSet(std::move(ptr)); });

    return result;
}

// Get items (key + compartment set) as vector of pairs
std::vector<std::pair<std::string, CompartmentSet>> CompartmentSets::items() const {
    auto items_vec = impl_->items();

    std::vector<std::pair<std::string, CompartmentSet>> result;
    result.reserve(items_vec.size());

    std::transform(
        items_vec.begin(), items_vec.end(),
        std::back_inserter(result),
        [](auto kv) { // pass by value to own the shared_ptr
            return std::make_pair(std::move(kv.first), CompartmentSet(std::move(kv.second)));
        });

    return result;
}
// Serialize all compartment sets to JSON string
std::string CompartmentSets::toJSON() const {
    return impl_->to_json().dump();
}

bool CompartmentSets::operator==(const CompartmentSets& other) const {
    return *impl_ == *(other.impl_);
}

bool CompartmentSets::operator!=(const CompartmentSets& other) const {
    return *impl_ != *(other.impl_);
}

}  // namespace sonata
}  // namespace bbp