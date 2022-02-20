//
// Created by sharadh on 15/2/22.
//

#pragma once

#include <bitset>
#include <vector>
#include <boost/dynamic_bitset.hpp>
#include "fixed_types.h"
#include "saturating_predictor.h"
#include "branch_predictor.h"

// Does NOT inherit from BranchPredictor because the signatures of
// predict() and update() are so different
class TaggedTable
{
public:
    using dyn_bitset = boost::dynamic_bitset<>;

    struct TaggedEntry
    {
        explicit TaggedEntry(UInt8 tag_width = 9u, UInt8 counter = 0b100, UInt8 useful = 0u) :
        tag{tag_width, 0},
        counter{counter},
        useful{useful}
        {};

        // We can't have runtime templates, so need to manually change the values inside the <> to change the bit widths
        dyn_bitset tag{9u};
        SaturatingPredictor<3> counter{};
        SaturatingPredictor<2> useful{};
    };

    explicit TaggedTable(UInt32 entries = 1024u, UInt8 tag_width = 9u);

    TaggedEntry &operator[](std::size_t index);

    std::pair<bool, bool> predict(std::size_t index, dyn_bitset &computed);

    void update(std::size_t index, bool final_prediction, bool alt_prediction, bool actual);

    void resetUsefulLsb();

    void resetUsefulMsb();

private:
    std::vector<TaggedEntry> m_table{256};
};




