//
//  ProblemFindMajority2.hpp
//  Balance2
//
//  Created by William Hurwood on 5/15/25.
//

// Suppose we are given n coins, where n is odd
// The coins have two varieties: L (slightly lighter) and H (slightly heavier)
// The goal is to determine, by weighing, which variety has the majority

#ifndef ProblemFindMajority2_hpp
#define ProblemFindMajority2_hpp

#include <vector>
class Partition2;

/**
 Problem class that determines which of two varieties of coins are in the majority.
 Specifically we expect there to be $c$ coins where $c$ is odd.
 We are told that there are two types of coins: L (slighly lighter) and H (slightly heavier).
 We may be given some addition restrictions on number of each coin.
 Since $c$ one of the varieties must be in the majority.  The goal is to determine which one.
 */
class ProblemFindMajority2
{
public:
	// A Distribution specifies for each part the number of H coins that might be in the part
	using Distribution = std::vector<uint8_t>;
	using Distributions = std::vector<Distribution>;

	// An instance of this class represents what we know about the problem after a series of weighings
	struct StateType
	{
		// We record all of the possible distributions of H coins within our parts
		Distributions distributions;
		
		// The distribution is applied with respect to a partition
		// Note that although it is true that each weighing has an output partition, the partition
		// of a state might not be the same as the output partition.  This happens if we can identify
		// that multiple singleton parts in the output distribution are identical.
		Partition2* partition;

		// Instruct compiler to generate operators to compare StateType objects
		auto operator<=>(const StateType& other) const = default;
	};
	
	ProblemFindMajority2(uint8_t coin_count, bool is_almost_balanced = true);
	
	// Implement methods required to be a problem
	StateType make_root(Partition2* partition);

private:
	uint8_t coin_count;					// Solve the problem for this number of coins
	uint8_t minimum_count;				// Each variety must have at least this number of coins
	uint8_t maximum_count;				// Each variety must have at most this number of coins
	uint8_t threshold;					// If variety has this number of coins it is majority
	
	// Helper functions
	bool is_majority(const Distribution& distribution);
};

#endif /* ProblemFindMajority2_hpp */
