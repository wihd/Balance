//
//  ProblemFindMajority.cpp
//  Balance
//
//  Created by William Hurwood on 5/5/25.
//
#include <cassert>
#include "ProblemFindMajority.hpp"

// C++ 20 note: We assert that ProblemFindMajority can be used as a Problem
static_assert(Problem<ProblemFindMajority>);

ProblemFindMajority::ProblemFindMajority(uint8_t coin_count, bool is_almost_balanced) :
	coin_count(coin_count),
	minimum_count(1),				// Must be at least one coin of each variety
	maximum_count(coin_count - 1),
	threshold((coin_count + 1) / 2)
{
	// There MUST be an odd number of coins
	assert(coin_count % 2 == 1);
	
	// Caller might specify that number of coins is almost balanced
	// Although this is hardest individual case it actually constains the problem and so simplifies it
	// Observe that if there were only one coin of some variety we must weigh against it
	if (is_almost_balanced)
	{
		minimum_count = threshold - 1;
		maximum_count = threshold;
	}
}

ProblemFindMajority::StateType ProblemFindMajority::make_root_data()
{
	// At the root there is one part which might contain any of the allowed number of H coins
	StateType result;
	for (auto i = minimum_count; i <= maximum_count; ++i)
	{
		result.push_back({i});
	}
	return result;
}

OutcomeArray<ProblemFindMajority::StateType> ProblemFindMajority::apply_weighing(const Partition& input_partition,
																				 const StateType& input_state,
																				 const Weighing& weighing,
																				 const Partition& output_partition,
																				 const PartitionProvenance& provanence)
{
	// We start with the given input partition and its state
	// We will apply the given weighing to the partition
	// For reference, manager tells us the induced output partition and the provance of its parts
	// For each of the three possible outcomes of the weighing we must compute what we can conclude about the
	// possible layout of the H coins to be consistent with the observation.
	OutcomeArray<StateType> result;

	// We consider each possible input distribution in turn
//	for (auto& distribution : input_state)
//	{
//		
//	}
	return result;
}

bool ProblemFindMajority::is_resolved(const Partition& input_partition, const StateType& state)
{
	// If there are no valid distributions then this state represents an impossible sequence of weighings
	// In that case it is resolved
	if (state.empty())
	{
		return true;
	}
	
	// Otherwise we count the number of H coins in each distribution
	// The problem is resolved if H is in majority or in the minority for each case
	bool seen_majority = false;
	bool seen_minority = false;
	for (auto& distribution : state)
	{
		if (is_majority(distribution))
		{
			if (seen_minority)
			{
				// We have seen both majority and minority distributions, so we have not resolved the problem
				return false;
			}
			seen_majority = true;
		}
		else if (seen_majority)
		{
			return false;
		}
		else
		{
			seen_minority = true;
		}
	}
	
	// All of the remaining distributions agreed on whether or not H was in the majority
	// So the problem is solved after this sequence of weighings
	return true;
}
