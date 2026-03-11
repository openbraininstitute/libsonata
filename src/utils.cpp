/*************************************************************************
 * Copyright (C) 2018-2020 Blue Brain Project
 *
 * This file is part of 'libsonata', distributed under the terms
 * of the GNU Lesser General Public License version 3.
 *
 * See top-level COPYING.LESSER and COPYING files for details.
 *************************************************************************/

#include "utils.h"

#include "../extlib/filesystem.hpp"

#include <fstream>
#include <unordered_set>

std::string readFile(const std::string& path) {
    namespace fs = ghc::filesystem;

    if (!fs::is_regular_file(path)) {
        throw std::runtime_error("Path `" + path + "` is not a file");
    }

    std::ifstream file(path);

    if (file.fail()) {
        throw std::runtime_error("Could not open file `" + path + "`");
    }

    std::string contents;

    file.seekg(0, std::ios::end);
    contents.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    contents.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    return contents;
}

namespace bbp {
namespace sonata {

json parseJSONWithDuplicateKeyCheck(const std::string& content) {
    // Use parser callback to throw exception for duplicate keys
    struct ObjectScope {
        std::string name;
        std::unordered_set<std::string> keys;
    };
    std::vector<ObjectScope> object_stack;  // Track current json object
    std::string current_key;                // Store the last key seen
    auto callback = [&object_stack, &current_key](int /*depth*/,
                                                  nlohmann::json::parse_event_t event,
                                                  nlohmann::json& parsed) -> bool {
        switch (event) {
        case nlohmann::json::parse_event_t::object_start:
            object_stack.push_back({current_key.empty() ? "root" : current_key, {}});
            break;

        case nlohmann::json::parse_event_t::object_end:
            if (!object_stack.empty()) {
                object_stack.pop_back();
            }
            break;

        case nlohmann::json::parse_event_t::key:
            current_key = parsed.get<std::string>();
            if (!object_stack.empty() && !object_stack.back().keys.insert(current_key).second) {
                throw SonataError(fmt::format("Duplicate key '{}' in '{}'",
                                              current_key,
                                              object_stack.back().name));
            }
            break;

        default:
            break;
        }
        return true;
    };
    return json::parse(content, callback);
}

}  // namespace sonata
}  // namespace bbp