
/**
 * @file
 * Declaration of a column associative indexing policy.
 */

#ifndef __MEM_CACHE_INDEXING_POLICIES_COLUMN_ASSOCIATIVE_HH__
#define __MEM_CACHE_INDEXING_POLICIES_COLUMN_ASSOCIATIVE_HH__

#include <vector>

#include "mem/cache/tags/indexing_policies/base.hh"
#include "params/ColumnAssociative.hh"

class ReplaceableEntry;

/**
 * A column associative indexing policy.
 */
class ColumnAssociative : public BaseIndexingPolicy
{
  private:
    /**
     * The number of skewing functions implemented. Should be updated if more
     * functions are added. If more than this number of skewing functions are
     * needed (i.e., assoc > this value), we programatically generate new ones,
     * which may be sub-optimal.
     *
    const int NUM_SKEWING_FUNCTIONS = 8; */

    /**
     * The amount to shift a set index to get its MSB.
     */
    const int msbShift;

    /**
     * The hash function itself.
     *
     * @param addr The address to be hashed.
     * @param The hashed address.
     */
    Addr hash(const Addr addr, const uint32_t way) const;
    Addr dehash(const Addr tag, const uint32_t way) const;
    mutable std::vector<std::vector<Addr>> ways;

    /**
     * Address skewing function selection. It selects and applies one of the
     * skewing functions functions based on the way provided.
     *
     * @param addr Address to be skewed. Should contain the set and tag bits.
     * @param way The cache way, used to select a hash function.
     * @return The skewed address.
     *
    Addr skew(const Addr addr, const uint32_t way) const;

    /**
     * Address deskewing function (inverse of the skew function) of the given
     * way.
     * @sa skew()
     *
     * @param addr Address to be deskewed. Should contain the set and tag bits.
     * @param way The cache way, used to select a hash function.
     * @return The deskewed address.
     *
    Addr deskew(const Addr addr, const uint32_t way) const; */

    /**
     * Apply the hash functions to calculate address' set given a way.
     *
     * @param addr The address to calculate the set for.
     * @param way The way to get the set from.
     * @return The set index for given combination of address and way.
     */
    uint32_t extractSet(const Addr addr, const uint32_t way) const;

  public:
    /** Convenience typedef. */
     typedef ColumnAssociativeParams Params;

    /**
     * Construct and initialize this policy.
     */
    ColumnAssociative(const Params *p);

    /**
     * Destructor.
     */
    ~ColumnAssociative() {};

    /**
     * Find all possible entries for insertion and replacement of an address.
     * Should be called immediately before ReplacementPolicy's findVictim()
     * not to break cache resizing.
     *
     * @param addr The addr to a find possible entries for.
     * @return The possible entries.
     */
    std::vector<ReplaceableEntry*> getPossibleEntries(const Addr addr) const
                                                                   override;

    /**
     * Regenerate an entry's address from its tag and assigned set and way.
     * Uses the inverse of the skewing function.
     *
     * @param tag The tag bits.
     * @param entry The entry.
     * @return the entry's address.
     */
    Addr regenerateAddr(const Addr tag, const ReplaceableEntry* entry) const
                                                                   override;
};

#endif //__MEM_CACHE_INDEXING_POLICIES_COLUMN_ASSOCIATIVE_HH__
