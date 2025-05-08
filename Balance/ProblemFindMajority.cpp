//
//  ProblemFindMajority.cpp
//  Balance
//
//  Created by William Hurwood on 5/5/25.
//
#include <cassert>
#include <set>
#include <memory>
#include <ranges>
#include "ProblemFindMajority.hpp"

// C++ 20 note: We assert that ProblemFindMajority can be used as a Problem
static_assert(Problem<ProblemFindMajority>);

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Distribute coins

// During a weighing a part may be split into up to three parts
// We create little Splitter iterators to desribe what could happen to coins in the part
// A splitter does not care about the location of each part
// Since there are never more than three outputs to the split, we will handwrite code for each possibility
// rather than writing a generic n-way splitter.  We expect that one or two way splits will be common
// so this should provide useful optimisation.
class Splitter
{
public:
	Splitter(uint8_t index) : base_index(index) {}
	virtual ~Splitter() {}

	// Specify number of H coins in distribution to be split
	void set_count(uint8_t new_count) { count = new_count; }
	
	// Reset the iterator to start again
	virtual void reset() = 0;
	
	// It the iterator is currently pointing to a valid split of the count assign it into the distribution, then
	// advance the iterator and return true
	// Alternatively return false
	virtual bool advance(ProblemFindMajority::Distribution& distribution) = 0;
	
	// Reset the splitter and immediately advance to its first entry
	void restart(ProblemFindMajority::Distribution& distribution) { reset(); advance(distribution); }

protected:
	int base_index;			// First output index used by this splitter
	uint8_t count = 0;		// Number of H coins split over the output
};

// Simplest splitter only has one output, so it does not do any splitting
class SplitterOne : public Splitter
{
public:
	SplitterOne(int part_index) : Splitter(part_index) {}
	void reset() override { has_visited = false; }
	bool advance(ProblemFindMajority::Distribution& distribution) override;
	
private:
	bool has_visited = false;
};

bool SplitterOne::advance(ProblemFindMajority::Distribution& distribution)
{
	// Since there is only one way to split the input, all we care about is whether this is first invocation or not
	if (has_visited)
	{
		return false;
	}
	else
	{
		has_visited = true;
		distribution[base_index] = count;
		return true;
	}
}

// If there are two output parts then we get count+1 ways of splitting
class SplitterTwo : public Splitter
{
public:
	SplitterTwo(int first_part_index) : Splitter(first_part_index) {}
	void reset() override { next_a_count = 0; }
	bool advance(ProblemFindMajority::Distribution& distribution) override;
	
private:
	uint8_t next_a_count = 0;
};

bool SplitterTwo::advance(ProblemFindMajority::Distribution& distribution)
{
	// Is the next distribution viable?
	if (next_a_count <= count)
	{
		distribution[base_index] = next_a_count;
		distribution[base_index+1] = count - next_a_count;
		++next_a_count;
		return true;
	}
	return false;
}

// The most complex case is when the part is split three ways
class SplitterThree : public Splitter
{
public:
	SplitterThree(int first_part_index) : Splitter(first_part_index) {}
	void reset() override { next_a_count = 0; next_b_count = 0; }
	bool advance(ProblemFindMajority::Distribution& distribution) override;
	
private:
	uint8_t next_a_count = 0;
	uint8_t next_b_count = 0;
};

bool SplitterThree::advance(ProblemFindMajority::Distribution& distribution)
{
	// Is the next distribution viable?
	if (next_a_count + next_b_count <= count)
	{
		distribution[base_index] = next_a_count;
		distribution[base_index+1] = next_b_count;
		distribution[base_index+2] = count - next_a_count - next_b_count;
		
		// If incrementing a is still viable then that's what we do next
		if (++next_a_count + next_b_count > count)
		{
			// Reset a and increment b - since this is a three way split we have no fallback if this is not viable
			next_a_count = 0;
			++next_b_count;
		}
		return true;
	}
	return false;
}




// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ProblemFindMajority

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
	
	// It is very easy to obtain the same output distribution from multiple input distributions
	// Internally we store the distributions as a set to remove duplicates as we go along
	OutcomeArray<std::set<Distribution>> distributions;
	
	// We will construct an array of splitters, one for each input part
	// These splitters will track the various ways that we can divide the H coins that might be assigned to the part
	// Observe that the splitters depend only on the provanence so we will reuse this vector for each distribution
	std::vector<std::unique_ptr<Splitter>> splitters;
	splitters.reserve(input_partition.size());
	
	// Take advantage of fact that provenance lists output parts from same input together
	// So all we care about is the number of times we see same input part number
	int split_count = 0;
	
	for (int i = 0; i != provanence.size(); ++i)
	{
		// We have found another way of splitting the current input part
		++split_count;
		
		// Have we reached end of output parts for this input part?
		if (i + 1 == provanence.size() || provanence[i].part != provanence[i+1].part)
		{
			// Create a splitter for the number of parts
			switch (split_count)
			{
				case 1:
					splitters.push_back(std::make_unique<SplitterOne>(i));
					break;
				case 2:
					splitters.push_back(std::make_unique<SplitterTwo>(i-1));
					break;
				case 3:
					splitters.push_back(std::make_unique<SplitterThree>(i-2));
					break;
				default:
					// Split counts should always be 1, 2 or 3
					assert(false);
			}
			split_count = 0;
		}
	}
	assert(splitters.size() == input_partition.size());

	// We will keep updating the same distribution object
	Distribution current(output_partition.size(), 0);
	
	// We consider each possible input distribution in turn
	for (auto& distribution : input_state)
	{
		// Configure the splitters with the number of H coins in each input part
		// C++23 Note: We can have option to iterate over two collections together
		for (auto [h_count, splitter] : std::views::zip(distribution, splitters))
		{
			splitter->set_count(h_count);
		}
		
		// Restart all of the splitters (which will overwrite all entries in `current` to a valid output distribution)
		for (auto& splitter : splitters)
		{
			splitter->restart(current);
		}
		
		// Each time we go round this loop we will have a new distribution
		auto advanced_splitter = splitters.begin();
		while (advanced_splitter != splitters.end())
		{
			// `current` now contains a valid output distribution made by splitting the input
			// We must allocate it to one of the three outcome sets based on the outcome of the weighing
			// when this distribution is used
			// Count number of H coins placed in each pan by this distribution
			int count_left = 0;
			int count_right = 0;
			for (int i = 0; i != provanence.size(); ++i)
			{
				switch (provanence[i].placement)
				{
					case LeftPan:
						count_left += current[i];
						break;
					case RightPan:
						count_right += current[i];
						break;
					case SetAside:
						break;
				}
			}
			
			// Insert (a copy of) current into one of the three output sets based on what would happen
			if (count_left > count_right)
			{
				distributions[Outcome::LeftHeavier].insert(current);
			}
			else if (count_right > count_left)
			{
				distributions[Outcome::RightHeavier].insert(current);
			}
			else
			{
				distributions[Outcome::Balances].insert(current);
			}
			
			// We walk through the splitters, trying to find one that can be advanced
			for (advanced_splitter = splitters.begin(); advanced_splitter != splitters.end(); ++advanced_splitter)
			{
				// Are we able to advance this splitter?
				if ((*advanced_splitter)->advance(current))
				{
					// All the splitters that had finished must be restarted
					// Note that since all the splitters write into different parts of current it does not
					// matter that we advanced them in uneven order
					for (auto it = splitters.begin(); it != advanced_splitter; ++it)
					{
						(*it)->restart(current);
					}
					
					// Break to leave any remaining splitters in place
					// Also we must leave advance_splitter pointing to an actual splitter
					break;
				}
			}
		}
	}

	// Once we have built the output distributions there is no point in using the set any more
	OutcomeArray<StateType> result;
	for (int i = Outcome::Begin; i != Outcome::End; ++i)
	{
		result[i].assign(distributions[i].begin(), distributions[i].end());
	}
	return result;
}

bool ProblemFindMajority::is_resolved(const Partition&, const StateType& state)
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
