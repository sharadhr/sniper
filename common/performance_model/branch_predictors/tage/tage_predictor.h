//
// Created by sharadh on 15/2/22.
//

#pragma once

#include <cmath>
#include "tagged_table.h"
#include "fixed_types.h"
#include "branch_predictor.h"
#include "simple_bimodal_table.h"
#include <boost/dynamic_bitset.hpp>

class TagePredictor : public BranchPredictor
{
    using dyn_bitset = boost::dynamic_bitset<>;
public:
    /**
     * @param name
     * @param core_id
     * @param entries The number of entries in the partially-tagged tables; the number of entries in the base predictor is 4x this value
     * @param components The number of <strong>total</strong> components, including the untagged base predictor
     * @param alpha The ratio in the geometric series
     * @param L_1 The initial value in the geometric series
     */
    explicit TagePredictor(const String &name, core_id_t core_id,
                           UInt32 entries = 1024u, UInt8 components = 8u, UInt8 alpha = 2, UInt8 L_1 = 5,
                           UInt8 tag_width = 9u);

    bool predict(bool indirect, IntPtr ip, IntPtr target) override;

    void update(bool predicted, bool actual, bool indirect, IntPtr ip, IntPtr target) override;

private:
    UInt32 tagged_tables_entries{1024u};
    UInt8 entry_bits{10u};
    UInt8 tag_width{9u};

    SimpleBimodalTable base_predictor{tagged_tables_entries << 2};
    std::vector<TaggedTable> tagged_components{7, TaggedTable{tagged_tables_entries, 9u}};
    std::vector<UInt32> geometric_series{};

    dyn_bitset global_history_register{};
    bool final_prediction{};
    bool alt_prediction{};
    std::size_t main_provider_i{};
    std::size_t alt_provider_i{};
    std::size_t main_provider_entry_index{};
    UInt64 branch_count{};

    std::vector<dyn_bitset> index_groups{};
    std::vector<dyn_bitset> tag_csr1_groups{};
    std::vector<dyn_bitset> tag_csr2_groups{};

    void shiftAllLeft(bool inserted);
    void shiftGroupsLeftAndSet(std::vector<dyn_bitset> &bit_groups, bool inserted) const;

    template<typename T>
    dyn_bitset xorFoldNGroups(std::vector<dyn_bitset> &bit_groups, T n);
    std::vector<boost::dynamic_bitset<>> xorFoldedResult(std::vector<dyn_bitset> &bit_groups, IntPtr ip);

    // std::function<bool(bool, const std::pair<bool, bool>&)> multiplexer;
};
