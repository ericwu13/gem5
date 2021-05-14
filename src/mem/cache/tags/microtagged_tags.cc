/*
 * Copyright (c) 2012-2014 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2003-2005,2014 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Definitions of a conventional tag store.
 */

#include "mem/cache/tags/microtagged_tags.hh"

#include <string>

#include "base/intmath.hh"

MicrotaggedTags::MicrotaggedTags(const Params *p)
    :BaseTags(p), allocAssoc(p->assoc), blks(p->size / p->block_size),
     sequentialAccess(p->sequential_access),
     replacementPolicy(p->replacement_policy)
{
    // Check parameters
    if (blkSize < 4 || !isPowerOf2(blkSize)) {
        fatal("Block size must be at least 4 and a power of 2");
    }
}

Addr
MicrotaggedTags::hash(const Addr addr) const
{
    // Get relevant bits
    // Addr p1 = bits<Addr>(addr, 19, 15) ^ bits<Addr>(addr, 24, 20);
    // // Addr p2 = reverseBits(bits<Addr>(addr, 27, 25)) >> 60;
    // Addr p3 = p2 ^ bits<Addr>(addr, 14, 12);

    // return (p1 << 5) | p3;
    Addr lower_top5 = bits<Addr>(addr, 19, 15);
    Addr upper_top5 = bits<Addr>(addr, 24, 20);
    Addr top5 = (lower_top5 ^ upper_top5) << 3;
    Addr lower3 = 0;
    lower3 |= (bits<Addr>(addr, 25) ^ bits<Addr>(addr, 14)) << 2;
    lower3 |= (bits<Addr>(addr, 26) ^ bits<Addr>(addr, 13)) << 1;
    lower3 |= bits<Addr>(addr, 27) ^ bits<Addr>(addr, 12);

    return top5 | lower3;
}

void
MicrotaggedTags::tagsInit()
{
    // Initialize all blocks
    for (unsigned blk_index = 0; blk_index < numBlocks; blk_index++) {
        // Locate next cache block
        CacheBlk* blk = &blks[blk_index];

        // Link block to indexing policy
        indexingPolicy->setEntry(blk, blk_index);

        // Associate a data chunk to the block
        blk->data = &dataBlks[blkSize*blk_index];

        // Associate a replacement data entry to the block
        blk->replacementData = replacementPolicy->instantiateEntry();
    }
}

void
MicrotaggedTags::invalidate(CacheBlk *blk)
{
    BaseTags::invalidate(blk);

    // Decrease the number of tags in use
    stats.tagsInUse--;

    // Invalidate replacement data
    replacementPolicy->invalidate(blk->replacementData);
}

CacheBlk*
MicrotaggedTags::findBlock(Addr addr, bool is_secure) const
{
    // Extract block tag
    Addr tag = extractTag(addr);
    Addr microtag = hash(addr);

    // Find possible entries that may contain the given address
    const std::vector<ReplaceableEntry*> entries =
        indexingPolicy->getPossibleEntries(addr);

    // Search for block
    for (const auto& location : entries) {
        CacheBlk* blk = static_cast<CacheBlk*>(location);
        if (blk->microtag == microtag) {
            if ((blk->tag == tag) && blk->isValid() &&
                (blk->isSecure() == is_secure)) {
                return blk;
            } else {
                return nullptr;
            }
        }
    }

    // Did not find block
    // inform("block not found: microtag (%p), tag (%p)\n", microtag, tag);
    return nullptr;
}

MicrotaggedTags*
MicrotaggedTagsParams::create()
{
    // There must be a indexing policy
    fatal_if(!indexing_policy, "An indexing policy is required");

    return new MicrotaggedTags(this);
}
