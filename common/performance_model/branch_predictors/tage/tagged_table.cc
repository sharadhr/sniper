//
// Created by sharadh on 15/2/22.
//

#include "tagged_table.h"

TaggedTable::TaggedTable(UInt32 entries, UInt8 tag_width) : m_table{entries, TaggedEntry{tag_width}} {}

TaggedTable::TaggedEntry& TaggedTable::operator[](std::size_t index)
{
    return m_table[index];
}

std::pair<bool, bool> TaggedTable::predict(std::size_t index, TaggedTable::dyn_bitset& computed)
{
    return {m_table[index].counter.predict(), m_table[index].tag == computed};
}

void TaggedTable::update(std::size_t index, bool final_prediction, bool alt_prediction, bool actual)
{
    if (alt_prediction != final_prediction)
        m_table[index].useful.update(final_prediction == actual);
}

void TaggedTable::resetUsefulLsb()
{
    for (auto& entry : m_table)
    {
        entry.useful.resetLsb();
    }
}

void TaggedTable::resetUsefulMsb()
{
    for (auto& entry : m_table)
    {
        entry.useful.resetMsb();
    }
}


