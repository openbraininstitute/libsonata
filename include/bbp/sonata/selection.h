#pragma once

#include "common.h"

#include <cstdint>
#include <iterator>  // std::forward_iterator_tag
#include <utility>   // std::move
#include <vector>

namespace bbp {
namespace sonata {

class SONATA_API Selection
{
  public:
    using Value = uint64_t;
    using Values = std::vector<Value>;
    using Range = std::array<Value, 2>;
    using Ranges = std::vector<Range>;

    /**
     * Create Selection from a list of ranges
     * @param ranges is a list of ranges constituting Selection
     */
    Selection(Ranges ranges);

    template <typename Iterator>
    static Selection fromValues(Iterator first, Iterator last);
    static Selection fromValues(const Values& values);

    /**
     * Get a list of ranges constituting Selection
     */
    const Ranges& ranges() const;

    /**
     * Array of IDs constituting Selection
     */
    Values flatten() const;

    /**
     * Total number of elements constituting Selection
     */
    size_t flatSize() const;

    bool empty() const;

    /**
     * Check if Selection contains a given node id
     * @param node id to check
     * @return true if Selection contains the node id, false otherwise
     */
    bool contains(Value node_id) const;

    /**
     * Forward iterator over individual element values in range-order.
     * Yields the same sequence as flatten() without allocating a vector.
     */
    class const_iterator
    {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Value;
        using difference_type = std::ptrdiff_t;
        using pointer = const Value*;
        using reference = Value;

        const_iterator() = default;

        Value operator*() const {
            return current_;
        }

        const_iterator& operator++() {
            ++current_;
            if (current_ >= std::get<1>(*range_it_)) {
                ++range_it_;
                if (range_it_ != range_end_) {
                    current_ = std::get<0>(*range_it_);
                }
            }
            return *this;
        }

        const_iterator operator++(int) {
            auto tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const const_iterator& other) const {
            return range_it_ == other.range_it_;
        }

        bool operator!=(const const_iterator& other) const {
            return !(*this == other);
        }

      private:
        friend class Selection;

        const_iterator(Ranges::const_iterator range_it, Ranges::const_iterator range_end)
            : range_it_(range_it)
            , range_end_(range_end)
            , current_(range_it != range_end ? std::get<0>(*range_it) : 0) {}

        Ranges::const_iterator range_it_{};
        Ranges::const_iterator range_end_{};
        Value current_ = 0;
    };

    /**
     * Iterator to the first element of the selection
     */
    const_iterator begin() const {
        return const_iterator(ranges_.cbegin(), ranges_.cend());
    }

    /**
     * Past-the-end iterator
     */
    const_iterator end() const {
        return const_iterator(ranges_.cend(), ranges_.cend());
    }

  private:
    Ranges ranges_;
};

bool SONATA_API operator==(const Selection&, const Selection&);
bool SONATA_API operator!=(const Selection&, const Selection&);

Selection SONATA_API operator&(const Selection&, const Selection&);
Selection SONATA_API operator|(const Selection&, const Selection&);

template <typename Iterator>
Selection Selection::fromValues(Iterator first, Iterator last) {
    Selection::Ranges ranges;

    Selection::Range range{0, 0};
    while (first != last) {
        const auto v = *first;
        if (v == std::get<1>(range)) {
            ++std::get<1>(range);
        } else {
            if (std::get<0>(range) < std::get<1>(range)) {
                ranges.push_back(range);
            }
            std::get<0>(range) = v;
            std::get<1>(range) = v + 1;
        }
        ++first;
    }

    if (std::get<0>(range) < std::get<1>(range)) {
        ranges.push_back(range);
    }

    return Selection(std::move(ranges));
}

}  // namespace sonata
}  // namespace bbp
