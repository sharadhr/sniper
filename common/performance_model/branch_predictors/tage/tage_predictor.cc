//
// Created by sharadh on 15/2/22.
//
// This code would have been a LOT cleaner with C++20 and std::ranges;
// there are way too many calls to .begin()/.end() here.

#include <algorithm>
#include <numeric>
#include <chrono>
#include <thread>
#include "tage_predictor.h"
#include "fixed_types.h"
#include "simulator.h"

TagePredictor::TagePredictor(const String &name, core_id_t core_id, UInt32 entries, UInt8 components, UInt8 alpha, UInt8 L_1, UInt8 tag_width)
        : BranchPredictor(name, core_id),
          tagged_tables_entries{entries},
          entry_bits{10u},
          tag_width{tag_width},
          base_predictor{entries << 2},
          tagged_components{static_cast<std::size_t>(components - 1), TaggedTable{entries, tag_width}} //,
          // multiplexer{[](bool prev_prediction, const std::pair<bool, bool> &tagged_results)
          //             {
          //                 return tagged_results.second ? prev_prediction : tagged_results
          //                         .first;
          //             }}
{
    // Sleep so that debugger can be hooked in
    // std::this_thread::sleep_for(std::chrono::seconds(10));

    // generate the geometric series
    std::generate_n(std::back_inserter(geometric_series), components - 1, [&alpha, &L_1, i = 1u]() mutable
    {
        return i++ == 1 ? L_1 : static_cast<UInt32>(std::pow(alpha, i - 2) * L_1);
    });
    // we resize the history register to the *largest* value in the geometric series
    global_history_register = dyn_bitset{geometric_series.back()};

    auto ghr_size{global_history_register.size()};

    // now generate a vector of indexing groups with sizes index_bits,
    // and number GHR / index_bits
    std::generate_n(std::back_inserter(index_groups),
                    (ghr_size / entry_bits) + (ghr_size % entry_bits == 0),
                    [&]()
                    { return dyn_bitset{entry_bits}; });

    // now generate the sizes for the CSR1 tag groups, with sizes tag_width,
    // and number GHR / tag_width
    std::generate_n(std::back_inserter(tag_csr1_groups),
                    (ghr_size / tag_width) + (ghr_size % tag_width == 0),
                    [&]()
                    { return dyn_bitset{tag_width}; });

    // Finally, generate the sizes for the CSR2 tag groups, with sizes tag_width - 1,
    // and number GHR / (tag_width - 1)
    std::generate_n(std::back_inserter(tag_csr2_groups),
                    (ghr_size / (tag_width - 1u)) + (ghr_size % tag_width == 0),
                    [&]()
                    { return dyn_bitset{tag_width - 1u}; });
}

bool TagePredictor::predict(bool indirect, IntPtr ip, IntPtr target)
{
    auto max_unsigned = std::numeric_limits<std::size_t>::max();
    // increment branch count; predict() is only called on encounter of a
    // branch instruction
    ++branch_count;

    // get a base prediction
    auto base_prediction{base_predictor.predict(indirect, ip, target)};

    auto indices{xorFoldedResult(index_groups, ip)};

    // compute the circular shift registers 1 and 2 for each tagged component
    auto csr1{xorFoldedResult(tag_csr1_groups, 0)};
    auto csr2{xorFoldedResult(tag_csr2_groups, 0)};
    std::for_each(csr2.begin(), csr2.end(),
                  [&](dyn_bitset &group)
                  {
                      group.resize(tag_width);
                      group <<= 1;
                  });

    // compute the tags for each tagged component
    dyn_bitset pc{tag_width, ip};
    std::vector<dyn_bitset> computed_tags{};
    std::generate_n(std::back_inserter(computed_tags), tagged_components.size(),
                    [&, i = 0]() mutable
                    {
                        auto result{pc ^ csr1[i] ^ csr2[i]};
                        ++i;
                        return result;
                    });

    // Finally, generate the predictions for the tagged components
    // The pair of bool is: [prediction, tag_hit]
    predictions.clear();
    std::generate_n(std::back_inserter(predictions), tagged_components.size(),
                    [&, i = 0]() mutable
                    {
                        return tagged_components[i].predict(indices[i].to_ulong(), computed_tags[i]);
                    });

    // get provider component, start from longest history length
    for (main_provider_i = tagged_components.size() - 1; main_provider_i != max_unsigned; --main_provider_i)
    {
        if (predictions[main_provider_i].second) break;
    }

    // if there is no hits, aka underflow to -1, return base prediction
    if (main_provider_i == max_unsigned)
    {
        alt_prediction = base_prediction;
        return base_prediction;
    }

    // get alternate provider; again, if -1, use base prediction
    for (alt_provider_i = main_provider_i - 1; alt_provider_i != max_unsigned; --alt_provider_i)
    {
        if (predictions[alt_provider_i].second) break;
    }

    // Therefore, we have our final prediction and the alternate prediction
    final_prediction = predictions[main_provider_i].first;
    main_provider_entry_index = indices[main_provider_i].to_ulong();
    alt_prediction = alt_provider_i != max_unsigned ? predictions[alt_provider_i].first : base_prediction;

    // combine the predictions with multiplexers and return the final prediction
    // final_prediction = std::accumulate(predictions.begin(), predictions.end(),
    //                                    base_prediction, multiplexer);

    // return the final prediction
    return final_prediction;
}

void TagePredictor::update(bool predicted, bool actual, bool indirect, IntPtr ip, IntPtr target)
{
    auto max_unsigned = std::numeric_limits<std::size_t>::max();
    // update counters
    updateCounters(predicted, actual);

    // update history register and register groups
    shiftAllLeft(actual);

    // update base predictor
    base_predictor.update(predicted, actual, indirect, ip, target);

    // update useful counter of provider component
    if (main_provider_i != max_unsigned)
    {
        tagged_components[main_provider_i].update(main_provider_entry_index, predicted, alt_prediction,
                                                  actual);

        // reset useful MSB/LSB if necessary
        if (branch_count % (512 * 1024) == 0)
            tagged_components[main_provider_i].resetUsefulMsb();
        else if (branch_count % (512 * 1024) == (256 * 1024))
            tagged_components[main_provider_i].resetUsefulLsb();
    }


    // on incorrect prediction, and if the provider was NOT using the longest history...
    if (predicted != actual && tagged_components.size() - main_provider_i > 1)
    {
        auto j{main_provider_i + 1};
        for (; j < tagged_components.size(); ++j)
        {
            auto entry{tagged_components[j][main_provider_entry_index]};

            // if there exists some j such that entry u_j = 0...
            if (entry.counter == SaturatingPredictor<3>{})
            {
                // allocate a component: ctr set to weak taken, useful set to zero
                entry = TaggedTable::TaggedEntry{entry_bits};
            }
        }

        // else decrement all u counters
        if (j == tagged_components.size())
        {
            for (auto i{main_provider_i + 1}; i < tagged_components.size(); ++i)
            {
                --tagged_components[i][main_provider_entry_index].useful;
            }
        }
    }
}

void TagePredictor::shiftAllLeft(bool inserted)
{
    // shift the GHR left
    global_history_register << 1;
    global_history_register[0] = inserted;

    // Shift the bits in the index group left
    shiftGroupsLeftAndSet(index_groups, inserted);
    shiftGroupsLeftAndSet(tag_csr1_groups, inserted);
    shiftGroupsLeftAndSet(tag_csr2_groups, inserted);
}

void TagePredictor::shiftGroupsLeftAndSet(std::vector<dyn_bitset> &bit_groups, bool inserted) const
{
    for (auto i{0u}; i < bit_groups.size(); ++i)
    {
        // one left shift for everything, discarding MSB...
        bit_groups[i] << 1;

        // if most significant block, mask extra bits
        dyn_bitset mask{entry_bits, true};

        if (i == 0) bit_groups[i] &= dyn_bitset{bit_groups[i].size(), 1};

        // Set the LSB to the MSB of the next group...
        if (i < bit_groups.size() - 1) bit_groups[i][0] = bit_groups[i + 1][bit_groups[i].size() - 1];

            // if last group, set to the inserted value
        else bit_groups[i][0] = inserted;
    }
}

template<typename T>
boost::dynamic_bitset<> TagePredictor::xorFoldNGroups(std::vector<dyn_bitset> &bit_groups, T n)
{
    if (n == 1u) return bit_groups.back();

    return std::accumulate(std::next(bit_groups.rbegin()), bit_groups.rbegin() + n,
                           bit_groups.back(), std::bit_xor{});
}

std::vector<boost::dynamic_bitset<>> TagePredictor::xorFoldedResult(std::vector<dyn_bitset> &bit_groups, IntPtr ip)
{
    static auto mask{(1 << bit_groups[0].size()) - 1};
    std::vector<dyn_bitset> result{};
    std::generate_n(std::back_inserter(result), bit_groups.size(),
                    [i = 1, this, &ip, &bit_groups]() mutable
                    {
                        if (ip)
                        {
                            std::vector<dyn_bitset> vec_with_pc{bit_groups};
                            vec_with_pc.emplace_back(dyn_bitset{bit_groups[0].size(), ip & mask});
                            mask << bit_groups[0].size();
                            return xorFoldNGroups(vec_with_pc, i++);
                        } else return xorFoldNGroups(bit_groups, i++);
                    });
    return result;
}






