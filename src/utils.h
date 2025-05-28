/*************************************************************************
 * Copyright (C) 2018-2020 Blue Brain Project
 *
 * This file is part of 'libsonata', distributed under the terms
 * of the GNU Lesser General Public License version 3.
 *
 * See top-level COPYING.LESSER and COPYING files for details.
 *************************************************************************/

#pragma once

#include <algorithm>  // std::transform
#include <iterator>   // std::inserter
#include <set>
#include <string>
#include <vector>

#include <bbp/sonata/population.h>
#include <nlohmann/json.hpp>
#include <fmt/format.h>

std::string readFile(const std::string& path);

namespace bbp {
namespace sonata {

template <typename T, class UnaryPredicate>
bbp::sonata::Selection _getMatchingSelection(const std::vector<T>& values, UnaryPredicate pred) {
    using bbp::sonata::Selection;
    Selection::Values ids;
    Selection::Value id = 0;
    for (const auto& v : values) {
        if (pred(v)) {
            ids.push_back(id);
        }
        ++id;
    }
    return Selection::fromValues(ids);
}

template <typename T>
std::set<std::string> getMapKeys(const T& map) {
    std::set<std::string> ret;
    std::transform(map.begin(), map.end(), std::inserter(ret, ret.end()), [](const auto& pair) {
        return pair.first;
    });
    return ret;
}

using json = nlohmann::json;
inline uint64_t get_uint64_or_throw(const json& el) {
    if (!el.is_number()) {
        throw SonataError(fmt::format("expected unsigned integer, got {}", el.dump()));
    }
    auto v = el.get<double>();
    if (v < 0) {
        throw SonataError(fmt::format("expected unsigned integer, got {}", v));
    }

    if (std::floor(v) != v) {
        throw SonataError(fmt::format("expected integer, got float {}", v));
    }
    return static_cast<uint64_t>(v);
}
}  // namespace sonata
}  // namespace bbp
