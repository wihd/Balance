//
//  ProblemFindMajority.hpp
//  Balance
//
//  Created by William Hurwood on 5/5/25.
//

// Suppose we are given n coins, where n is odd
// The coins have two varieties: L (slightly lighter) and H (slightly heavier)
// The goal is to determine, by weighing, which variety has the majority

#ifndef ProblemFindMajority_hpp
#define ProblemFindMajority_hpp

#include <vector>
#include "Manager.hpp"
class Output;

/**
 For each part we track the numbers of H coins that could be present in the part to be consistent with
 the observed weighings.
 */
class ProblemFindMajority
{
public:
	ProblemFindMajority(uint8_t coin_count, bool is_almost_balanced = true);
	
	// A Distribution specifies for each part the number of H coins that might be in the part
	using Distribution = std::vector<uint8_t>;

	// The state is a vector of distributions
	// It will be empty if it is impossible for the observed weighings to occur
	using StateType = std::vector<Distribution>;
	
	// Implement methods required to be a problem
	StateType make_root_data();
	OutcomeArray<StateType> apply_weighing(const Partition& input_partition,
										   const StateType& input_state,
										   const Weighing& weighing,
										   const Partition& output_partition,
										   const PartitionProvenance& provanence);
	bool is_resolved(const Partition&, const StateType& state);
	bool is_impossible(const Partition&, const StateType& state) { return state.empty(); }
	void write_description(Output& output);
	void write_solved_node(Output& output, const Partition&, const StateType& state, const char* outcome_name);
	
private:
	// Total number of coins
	uint8_t coin_count;
	
	// Smallest number of coins that could be present (from one variety)
	uint8_t minimum_count;
	
	// Largest number of coins that could be present (from one variety)
	uint8_t maximum_count;
	
	// This is the smallest number of coins of a variety to put it into majority
	uint8_t threshold;
	
	// Helper functions
	bool is_majority(const Distribution& distribution);
};

inline bool ProblemFindMajority::is_majority(const Distribution& distribution)
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


#endif /* ProblemFindMajority_hpp */
