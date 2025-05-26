#include <algorithm>  // std::find, std::transform
#include <cassert>
#include <cmath>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fstream>

#include "../extlib/filesystem.hpp"

#include <nlohmann/json.hpp>
#include <utility>

#include "utils.h"  // readFile

#include <bbp/sonata/compartment_sets.h>

namespace bbp {
namespace sonata {

namespace fs = ghc::filesystem;

namespace detail {

using json = nlohmann::json;

class CompartmentSets
{

  public:
    explicit CompartmentSets(const json& j) {
        if (!j.is_object()) {
            throw SonataError("Top level compartment_set must be an object");
        }
    }
    
    static const fs::path& validate_path(const fs::path& path) {
        if (!fs::exists(path)) {
            throw SonataError(fmt::format("Path does not exist: {}", std::string(path)));
        }
        return path;
    }

    explicit CompartmentSets(const fs::path& path)
        : CompartmentSets(json::parse(std::ifstream(validate_path(path)))) {}

    static std::unique_ptr<CompartmentSets> fromFile(const std::string& path_) {
        fs::path path(path_);
        return std::make_unique<detail::CompartmentSets>(path);
    }

    explicit CompartmentSets(const std::string& content)
        : CompartmentSets(json::parse(content)) {}

    std::string toJSON() const {
        std::string ret{"{\n"};
        ret += "}";

        return ret;
    }
};

}  // namespace detail
}  // namespace sonata
}  // namespace bbp