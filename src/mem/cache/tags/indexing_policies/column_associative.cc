
/**
 * @file
 * Definitions of a column associative indexing policy.
 */

#include "mem/cache/tags/indexing_policies/column_associative.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "base/logging.hh"
#include "mem/cache/replacement_policies/replaceable_entry.hh"

ColumnAssociative::ColumnAssociative(const Params *p)
    : BaseIndexingPolicy(p),
      msbShift(floorLog2(numSets) + floorLog2(assoc) - 1)
{
}

Addr
ColumnAssociative::hash(const Addr addr, const uint32_t way) const
{
    // It appears that addr1 and addr2 from the "skew" function
    // (below, from skew-associative) are what we want.
    // So we copy them, and xor the lower bits
    // with a rotating shift by k of the upper bits for way k
    Addr addr1 = bits<Addr>(addr, msbShift, 0);
    Addr addr2 = bits<Addr>(addr, 2 * (msbShift + 1) - 1, msbShift + 1);
    Addr shifted_addr2 = addr2;
    if (way >= 1) {
        shifted_addr2 = insertBits<Addr, uint8_t>(addr2 >> way, msbShift,
                                   msbShift - way + 1,
                                   bits<Addr>(addr2, way - 1, 0));
    }
    return addr1 ^ shifted_addr2;
}
/*
Addr
ColumnAssociative::skew(const Addr addr, const uint32_t way) const
{
    // Assume an address of size A bits can be decomposed into
    // {addr3, addr2, addr1, addr0}, where:
    //   addr0 (M bits) = Block offset;
    //   addr1 (N bits) = Set bits in conventional cache;
    //   addr3 (A - M - 2*N bits), addr2 (N bits) = Tag bits.
    // We use addr1 and addr2, as proposed in the original paper
    Addr addr1 = bits<Addr>(addr, msbShift, 0);
    const Addr addr2 = bits<Addr>(addr, 2 * (msbShift + 1) - 1, msbShift + 1);

    // Select and apply skewing function for given way
    switch (way % NUM_SKEWING_FUNCTIONS) {
      case 0:
        addr1 = hash(addr1) ^ hash(addr2) ^ addr2;
        break;
      case 1:
        addr1 = hash(addr1) ^ hash(addr2) ^ addr1;
        break;
      case 2:
        addr1 = hash(addr1) ^ dehash(addr2) ^ addr2;
        break;
      case 3:
        addr1 = hash(addr1) ^ dehash(addr2) ^ addr1;
        break;
      case 4:
        addr1 = dehash(addr1) ^ hash(addr2) ^ addr2;
        break;
      case 5:
        addr1 = dehash(addr1) ^ hash(addr2) ^ addr1;
        break;
      case 6:
        addr1 = dehash(addr1) ^ dehash(addr2) ^ addr2;
        break;
      case 7:
        addr1 = dehash(addr1) ^ dehash(addr2) ^ addr1;
        break;
      default:
        panic("A skewing function has not been implemented for this way.");
    }

    // If we have more than 8 ways, just pile them up on hashes. This is not
    // the optimal solution, and can be improved by adding more skewing
    // functions to the previous selector
    for (uint32_t i = 0; i < way/NUM_SKEWING_FUNCTIONS; i++) {
        addr1 = hash(addr1);
    }

    return addr1;
}

Addr
ColumnAssociative::deskew(const Addr addr, const uint32_t way) const
{
    // Get relevant bits of the addr
    Addr addr1 = bits<Addr>(addr, msbShift, 0);
    const Addr addr2 = bits<Addr>(addr, 2 * (msbShift + 1) - 1, msbShift + 1);

    // If we have more than NUM_SKEWING_FUNCTIONS ways, unpile the hashes
    if (way >= NUM_SKEWING_FUNCTIONS) {
        for (uint32_t i = 0; i < way/NUM_SKEWING_FUNCTIONS; i++) {
            addr1 = dehash(addr1);
        }
    }

    // Select and apply skewing function for given way
    switch (way % 8) {
      case 0:
        return dehash(addr1 ^ hash(addr2) ^ addr2);
      case 1:
        addr1 = addr1 ^ hash(addr2);
        for (int i = 0; i < msbShift; i++) {
            addr1 = hash(addr1);
        }
        return addr1;
      case 2:
        return dehash(addr1 ^ dehash(addr2) ^ addr2);
      case 3:
        addr1 = addr1 ^ dehash(addr2);
        for (int i = 0; i < msbShift; i++) {
            addr1 = hash(addr1);
        }
        return addr1;
      case 4:
        return hash(addr1 ^ hash(addr2) ^ addr2);
      case 5:
        addr1 = addr1 ^ hash(addr2);
        for (int i = 0; i <= msbShift; i++) {
            addr1 = hash(addr1);
        }
        return addr1;
      case 6:
        return hash(addr1 ^ dehash(addr2) ^ addr2);
      case 7:
        addr1 = addr1 ^ dehash(addr2);
        for (int i = 0; i <= msbShift; i++) {
            addr1 = hash(addr1);
        }
        return addr1;
      default:
        panic("A skewing function has not been implemented for this way.");
    }
}
*/

uint32_t
ColumnAssociative::extractSet(const Addr addr, const uint32_t way) const
{
    return hash(addr >> setShift, way);
}

Addr
ColumnAssociative::regenerateAddr(const Addr tag,
                                  const ReplaceableEntry* entry) const
{
    Addr index = (entry->getSet() << floorLog2(assoc)) | entry->getWay();
    Addr addr2 = bits<Addr>(tag, msbShift, 0);
    Addr shifted_addr2 = addr2;
    if (way >= 1) {
        shifted_addr2 = insertBits<Addr, uint8_t>(addr2 >> way, msbShift,
                                   msbShift - way + 1,
                                   bits<Addr>(addr2, way - 1, 0));
    }
    return (tag << tagShift) | ((index ^ shifted_addr2) << setShift);
}


std::vector<ReplaceableEntry*>
ColumnAssociative::getPossibleEntries(const Addr addr) const
{
    std::vector<ReplaceableEntry*> entries;

    // Parse all ways
    for (uint32_t way = 0; way < assoc; ++way) {
        // Apply hash to get set, and get way entry in it
        Addr index = extractSet(addr, way);
        entries.push_back(sets[index >> (floorLog2(assoc))]
                              [index & (assoc-1)]);
    }

    return entries;
}

ColumnAssociative *
ColumnAssociativeParams::create()
{
    return new ColumnAssociative(this);
}
