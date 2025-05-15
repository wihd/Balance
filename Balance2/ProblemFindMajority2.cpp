//
//  ProblemFindMajority2.cpp
//  Balance2
//
//  Created by William Hurwood on 5/15/25.
//
#include <cassert>
#include "ProblemFindMajority2.hpp"
#include "StateTemplates.h"
#include "Partition2.hpp"

// Our class must be a valid Problem (i.e. must be compatible with the manager)
static_assert(Problem<ProblemFindMajority2>);


// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ProblemFindMajority

ProblemFindMajority2::ProblemFindMajority2(uint8_t coin_count, bool is_almost_balanced) :
	coin_count(coin_count),
	minimum_count(1),				// Must be at least one coin of each variety
	maximum_count(coin_count - 1),	// Must be at least one coin that is not of each variety
	threshold((coin_count + 1) / 2)
{
	// There MUST be an odd number of coins
	assert(coin_count % 2 == 1);

	// If caller specifies that problem is almost balanced then caller is adding constaint that the minority
	// variety has one fewer coin than majority variety.  This is additional information and so makes the
	// problem easier to solve (even though only remaining case is the hardest case).
	if (is_almost_balanced)
	{
		minimum_count = threshold - 1;
		maximum_count = threshold;
	}
}

inline bool ProblemFindMajority2::is_majority(const Distribution& distribution)
{
	// Determine whether H is in majority for a given distribution
	uint8_t h_count = 0;
	for (auto c : distribution)
	{
		h_count += c;
		
		// Once we have reached threshold we know the outcome
		if (h_count >= threshold)
		{
			return true;
		}
	}
	
	// If we never reached the threshold then we are not in the majority
	return false;
}

ProblemFindMajority2::StateType ProblemFindMajority2::make_root(Partition2* partition)
{
	// At the root there is one part which might contain any of the allowed number of H coins
	assert(partition && partition->size() == 1 && partition->coin_count() == coin_count);
	Distributions result;
	for (auto i = minimum_count; i <= maximum_count; ++i)
	{
		result.push_back({i});
	}
	return {result, partition};
}
