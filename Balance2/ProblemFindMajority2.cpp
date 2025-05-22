//
//  ProblemFindMajority2.cpp
//  Balance2
//
//  Created by William Hurwood on 5/15/25.
//
#include <cassert>
#include <ranges>
#include <algorithm>
#include <numeric>
#include "ProblemFindMajority2.hpp"
#include "StateTemplates.h"
#include "Partition2.hpp"
#include "Weighing2.hpp"
#include "Output2.hpp"

// Our class must be a valid Problem (i.e. must be compatible with the manager)
static_assert(Problem<ProblemFindMajority2>);


// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Distribute coins

// During a weighing an input part may be split into up to three parts
// We create little Splitter iterators to desribe what could happen to coins in the part
// A splitter does not care about the location of each part
// Since there are never more than three outputs to the split, we will handwrite code for each possibility
// rather than writing a generic n-way splitter.  We expect that one or two way splits will be common
// so this should provide useful optimisation.


// We need a base class to allow code to select splitter at run time
class Splitter
{
public:
	Splitter(std::vector<uint8_t>& indexes) : indexes(indexes) {}
	virtual ~Splitter() {}

	// Specify number of H coins in distribution to be split
	void set_count(uint8_t new_count) { count = new_count; }
	
	// Reset the iterator to start again
	virtual void reset() = 0;
	
	// It the iterator is currently pointing to a valid split of the count assign it into the distribution, then
	// advance the iterator and return true
	// Alternatively return false
	virtual bool advance(ProblemFindMajority2::Distribution& distribution, const Partition2& output_partition) = 0;
	
	// Reset the splitter and immediately advance to its first entry
	void restart(ProblemFindMajority2::Distribution& distribution, const Partition2& output_partition)
	{
		reset();
		advance(distribution, output_partition);
	}

protected:
	std::vector<uint8_t>& indexes;		// Vector listing the indexes of our output parts
	uint8_t count = 0;					// Number of H coins split over the output
};

// Simplest splitter only has one output, so it does not do any splitting
class SplitterOne : public Splitter
{
public:
	using Splitter::Splitter;
	void reset() override { has_visited = false; }
	bool advance(ProblemFindMajority2::Distribution& distribution, const Partition2& output_partition) override;

private:
	bool has_visited = false;
};

bool SplitterOne::advance(ProblemFindMajority2::Distribution& distribution, const Partition2&)
{
	// Since there is only one way to split the input, all we care about is whether this is first invocation or not
	// The output_partition is ignored, since input and output part size must be identical
	if (has_visited)
	{
		return false;
	}
	else
	{
		has_visited = true;
		distribution[indexes[0]] = count;
		return true;
	}
}

// If there are two output parts then we get count+1 ways of splitting
class SplitterTwo : public Splitter
{
public:
	using Splitter::Splitter;
	void reset() override { next_a_count = 0; }
	bool advance(ProblemFindMajority2::Distribution& distribution, const Partition2& output_partition) override;
	
private:
	uint8_t next_a_count = 0;
};

bool SplitterTwo::advance(ProblemFindMajority2::Distribution& distribution, const Partition2& partition)
{
	// Is the next distribution viable?
	// We must reject the distribution if it puts more H coins into a part than there are coins in the part
	while (next_a_count <= count)
	{
		distribution[indexes[0]] = next_a_count;
		distribution[indexes[1]] = count - next_a_count;
		++next_a_count;
		if (distribution[indexes[0]] <= partition[indexes[0]] && distribution[indexes[1]] <= partition[indexes[1]])
		{
			return true;
		}
	}
	return false;
}

// The most complex case is when the part is split three ways
class SplitterThree : public Splitter
{
public:
	using Splitter::Splitter;
	void reset() override { next_a_count = 0; next_b_count = 0; }
	bool advance(ProblemFindMajority2::Distribution& distribution, const Partition2& output_partition) override;
	
private:
	uint8_t next_a_count = 0;
	uint8_t next_b_count = 0;
};

bool SplitterThree::advance(ProblemFindMajority2::Distribution& distribution, const Partition2& partition)
{
	// Is the next distribution viable?
	// We must reject the distribution if it puts more H coins into a part than there are coins in the part
	while (next_a_count + next_b_count <= count)
	{
		distribution[indexes[0]] = next_a_count;
		distribution[indexes[1]] = next_b_count;
		distribution[indexes[2]] = count - next_a_count - next_b_count;
		
		// If incrementing a is still viable then that's what we do next
		if (++next_a_count + next_b_count > count)
		{
			// Reset a and increment b - since this is a three way split we have no fallback if this is not viable
			next_a_count = 0;
			++next_b_count;
		}
		if (distribution[indexes[0]] <= partition[indexes[0]] &&
			distribution[indexes[1]] <= partition[indexes[1]] &&
			distribution[indexes[2]] <= partition[indexes[2]])
		{
			return true;
		}
	}
	return false;
}


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

ProblemFindMajority2::StateTypeRef ProblemFindMajority2::make_root()
{
	// At the root there is one part which might contain any of the allowed number of H coins
	Partition2* root_partition = Partition2::get_root(coin_count);
	Distributions result;
	for (auto i = minimum_count; i <= maximum_count; ++i)
	{
		result.push_back({i});
	}
	return std::make_unique<StateType>(result, root_partition);
}

// Confirm that a weighing that does not refine the input partition is stable with respect to part order
bool check_part_order(Weighing2* weighing)
{
	uint8_t index = 0;
	for (auto& part : *weighing)
	{
		if (part.part != index)
		{
			return false;
		}
		++index;
	}
	return true;
}

OutcomeArray<ProblemFindMajority2::StateTypeRef> ProblemFindMajority2::apply_weighing(const StateType& state,
																					  Weighing2* weighing,
																					  Partition2* partition)
{
	// We are given a state and a weighing leading to an output partition
	// We must compute up to three states and return them, one for each outcome of the weighing
	// We will omit an output state if the corresponding outcome is impossible based on what we already know
	// If possible we will perform two optimisations intended to reduce number of states we consider
	// 1. We may be able to merge some parts in the output partition (fewer parts means fewer future weighings)
	// 2. We may return a state that is isomorphic to the actual state (reducing number of states considered)
	
	// Each distribution in the input state will be mapped to a distribution over the output partition
	// If the output partition is refined (i.e. some parts are split) then a distribution may be mapped to
	// several output distributions (since the H coins may be split over the output parts in various ways).
	
	// The weighing refines the partitions if and only if the output partition has more parts
	if (partition->size() != state.partition->size())
	{
		// At least one part must be split.  Even a part is not split its index may change.
		// So we need to compute a new Distributions object

		// We will start by creating a vector listing for each input part the output part(s) over which it is split
		// In the earlier version this was not needed since output parts from the same input part were placed
		// together, but the requirement that part sizes do not decrement means this is no longer true
		std::vector<std::vector<uint8_t>> split_indexes(state.partition->size());
		for (uint8_t i = 0; i != weighing->size(); ++i)
		{
			split_indexes[(*weighing)[i].part].push_back(i);
		}
		
		// We now create a vector of splitters, which will handle splitting and reordering
		std::vector<std::unique_ptr<Splitter>> splitters;
		for (auto& split_index : split_indexes)
		{
			switch (split_index.size())
			{
				case 1:
					splitters.push_back(std::make_unique<SplitterOne>(split_index));
					break;
				case 2:
					splitters.push_back(std::make_unique<SplitterTwo>(split_index));
					break;
				case 3:
					splitters.push_back(std::make_unique<SplitterThree>(split_index));
					break;
				default:
					// Split counts should always be 1, 2 or 3
					assert(false);
			}
		}
		
		// Apply the splitters to determine what happens to each distribution
		Distributions split_distributions;
		Distribution current(partition->size(), 0);
		for (auto& distribution : state.distributions)
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
				splitter->restart(current, *partition);
			}
			
			// Loop over ways of splitting this distribution
			auto advanced_splitter = splitters.begin();
			while (advanced_splitter != splitters.end())
			{
				// Copy the current distribution to split_distributions
				split_distributions.emplace_back(current.begin(), current.end());
				
				// We walk through the splitters, trying to find one that can be advanced
				for (advanced_splitter = splitters.begin(); advanced_splitter != splitters.end(); ++advanced_splitter)
				{
					// Are we able to advance this splitter?
					if ((*advanced_splitter)->advance(current, *partition))
					{
						// All the splitters that had finished must be restarted
						// Note that since all the splitters write into different parts of current it does not
						// matter that we advanced them in uneven order
						for (auto it = splitters.begin(); it != advanced_splitter; ++it)
						{
							(*it)->restart(current, *partition);
						}
						
						// Break to leave any remaining splitters in place
						// Also we must leave advance_splitter pointing to an actual splitter
						break;
					}
				}
			} // next splitter distribution
		} // next input distribution
		
		// We now have list of distributions to process
		return apply_weighing_to_distributions(split_distributions, *weighing, partition);
	}
	else
	{
		// This weighing does not split any parts in the distributions
		// The logic to construct weighings should be stable with respect to parts unless part size forces
		// it to change the order.  Since part size did not change the order should not change either.
		// Thus we can pass the input distributions onto the next function without any modifications.
		assert(check_part_order(weighing));
		return apply_weighing_to_distributions(state.distributions, *weighing, partition);
	}
}

OutcomeArray<ProblemFindMajority2::StateTypeRef> ProblemFindMajority2::apply_weighing_to_distributions(
	const Distributions& distributions, Weighing2& weighing, Partition2* partition)
{
	// This is a helper function for apply_weighing()
	// The distributions passed in will have been split to use the output partition
	// In some cases these may be the input distributions, so this function cannot modify or move them
	
	// Assign each distribution to one of the three possible outcomes
	OutcomeArray<std::vector<const Distribution*>> outcome_distributions;
	for (auto& current : distributions)
	{
		// We must allocate it to one of the three outcome sets based on the outcome of the weighing
		// when this distribution is used
		// Count number of H coins placed in each pan by this distribution
		int count_left = 0;
		int count_right = 0;
		for (size_t i = 0; i != weighing.size(); ++i)
		{
			switch (weighing[i].placement)
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
		
		// Insert (a pointer to) current distribution into one of the three output sets based on what would happen
		if (count_left > count_right)
		{
			outcome_distributions[Outcome::LeftHeavier].push_back(&current);
		}
		else if (count_right > count_left)
		{
			outcome_distributions[Outcome::RightHeavier].push_back(&current);
		}
		else
		{
			outcome_distributions[Outcome::Balances].push_back(&current);
		}
	}
	
	// We have computed the output state, but we might be able to simplify it
	OutcomeArray<ProblemFindMajority2::StateTypeRef> result;
	for (int i = Outcome::Begin; i != Outcome::End; ++i)
	{
		result[i] = simplify_partition(outcome_distributions[i], partition);
	}
	return result;
}

// Define helper types and functions for simplify_partition
using GroupType = std::pair<std::vector<size_t>, uint8_t>;
constexpr uint8_t UniquePartMarker = 0xff;
bool compare_groups(const GroupType& a, const GroupType& b)
{
	// Sort groups into ascending order by part size, otherwise stable with respect to input part index
	if (a.second != b.second)
	{
		return a.second < b.second;
	}
	
	// We use UniquePartMarker as a magic number, so we skip over it if it is present
	auto a_it = a.first.begin();
	if (*a_it == UniquePartMarker)
	{
		++a_it;
	}
	auto b_it = b.first.begin();
	if (*b_it == UniquePartMarker)
	{
		++b_it;
	}
	return *a_it < *b_it;
}

ProblemFindMajority2::StateTypeRef ProblemFindMajority2::simplify_partition(
	const std::vector<const Distribution*>& distributions, Partition2* partition)
{
	// If there are no distributions then this outcome is impossible
	// There is no point in having a non-null state for an impossible outcome
	if (distributions.empty())
	{
		return {};
	}
	
	// We would obtain the correct solution if we just returned the distribtions + partition we already have
	// But we will attempt several optimisations which have potential to considerably reduce work done
	
	// First we observe that we are not required to use the output partion we were given
	// If can identify several coins whose distribution is identical then in future weighings we can
	// treat these coins as indistiguishable.  To be more precise although we can distinguish between
	// these coins there is no reason to do so.  For example if we have proven that several coins are H
	// then we can put these coins into one part.  If three coins get grouped together it halves the
	// number of weighings we need to consider, so this can be a significant improvement.

	// Identify the columns that could be joined together
	std::vector<GroupType> groups;
	for (size_t index = 0; index != partition->size(); ++index)
	{
		// To be able to merge parts we require that for all distributions, all of the coins the part
		// are either H or L.  This means that there is no point in distinguishing between these coins.
		uint8_t part_size = (*partition)[index];
		bool is_candidate = true;
		
		// If the part only contains one coin then the coins in the part are vacuously not worth distinshing
		// But if there is more than one coin in the part we must examine all distributions
		if (part_size > 1)
		{
			for (auto distribution_ptr : distributions)
			{
				if ((*distribution_ptr)[index] != 0 && (*distribution_ptr)[index] != part_size)
				{
					// This distribution does not treat all the coins in the part as identical
					// So this part cannot be joined
					is_candidate = false;
					break;
				}
			}
		}
		
		if (is_candidate)
		{
			// Look at the groups we have already identified
			for (auto& group : groups)
			{
				// Compare coins distributed on our part with one of the part(s) already in the group
				size_t sample_index = group.first.front();
				
				// If the groups first index is magic number UniquePartMarker then we cannot join with it
				if (sample_index == UniquePartMarker)
				{
					continue;
				}
				
				// Can we join with tihs group?
				bool can_join = true;
				for (auto distribution_ptr : distributions)
				{
					// Both parts must have non-zero H coins in the same pattern
					bool is_zero = (*distribution_ptr)[index] == 0;
					bool sample_is_zero = (*distribution_ptr)[sample_index] == 0;
					if ((is_zero && !sample_is_zero) || (!is_zero && sample_is_zero))
					{
						can_join = false;
						break;
					}
				}
				if (can_join)
				{
					// Add this index to the group
					group.first.push_back(index);
					group.second += part_size;
					is_candidate = false;
					break;
				}
			}

			// Is this part still marked as a candidate to join?
			if (is_candidate)
			{
				// We insert it as a fresh group with one part in it so far
				groups.emplace_back(std::vector<size_t>{index}, part_size);
			}
		}
		else
		{
			// If this part cannot be joined we just add it to list of groups
			groups.emplace_back(std::vector<size_t>{UniquePartMarker, index}, part_size);
		}
	}
	
	// Have we ended up with fewer groups than we started with parts?
	if (groups.size() != partition->size())
	{
		// It is worth switching to a different partition
		// We start by sorting the groups because the merged parts must be in ascending order by size
		std::sort(groups.begin(), groups.end(), compare_groups);
		
		// Get the cached instance of the joined partition
		std::vector<uint8_t> joined_parts;
		for (auto& group : groups)
		{
			joined_parts.push_back(group.second);
		}
		auto joined_partion = Partition2::get_instance(std::move(joined_parts));
		
		// We must allocate new distributions
		Distributions joined_distributions(distributions.size());
		for (size_t i = 0; i != distributions.size(); ++i)
		{
			const Distribution& input = *distributions[i];
			Distribution& output = joined_distributions[i];
			
			// Each group specifies the part(s) to be joined to make the next value in the output distribution
			for (auto& group : groups)
			{
				auto it = group.first.begin();
				if (*it == UniquePartMarker)
				{
					// This group was not joined, so we can copy over a single part
					++it;
					output.push_back(input[*it]);
				}
				else
				{
					// Create new entry in output distribution
					output.push_back(input[*it]);
					
					// Join in other distributions
					for (++it; it != group.first.end(); ++it)
					{
						output.back() += input[*it];
					}
				}
			}
		}
				
		// Call next level function that will look for more optimisations
		return simplify_state(std::move(joined_distributions), joined_partion);
	}
	else
	{
		// We will stick with the original output partition
		// However the next function requires a distributions vector that it can both move and modify
		// In the other branch this was easy (it made a new distributions anyway)
		// In this branch we need to make a copy (we didn't do it earlier in case we entered other branch)
		Distributions copy_distributions;
		copy_distributions.reserve(distributions.size());
		for (auto& d : distributions)
		{
			copy_distributions.emplace_back(d->begin(), d->end());
		}
		
		return simplify_state(std::move(copy_distributions), partition);
	}
}

// Helper class to help compare two parts with respect to a distribution
class PartCompareHelper
{
public:
	PartCompareHelper(const ProblemFindMajority2::Distributions& distributions,
					  size_t index,
					  uint8_t part_size) :
	distributions(distributions), values{part_size}, index(index) {}
	uint8_t operator[](size_t i);
	
private:
	const ProblemFindMajority2::Distributions& distributions;	// Distributions
	std::vector<size_t> values;									// Compare values we have computed
	size_t index;												// Index number of this part
};

uint8_t PartCompareHelper::operator[](size_t i)
{
	// If we have already computed a value for entry i, return cached value
	// Note that since the first value is the part_size, it will sort by part_size first of all
	if (i < values.size())
	{
		return values[i];
	}
	
	// We expect caller to ask for each value in turn, so we only need compute the next value
	assert(i == values.size());
	
	// We will count the number of distributions that have size i+1
	bool saw_larger = false;
	size_t counter = 0;
	for (auto& distribution : distributions)
	{
		if (distribution[index] == i - 1)
		{
			++counter;
		}
		else if (distribution[index] >= i)
		{
			saw_larger = true;
		}
	}
	
	// Cache this number
	values.push_back(counter);
	
	// If there were no bigger values then there is no point in looking again
	if (!saw_larger)
	{
		// Push a value based on the index number into the vector, but large enough to be unique
		// This will have two effects
		// i)  By making number bigger than distributions size() we know that it will disambiguate all parts
		// ii) We keep part order stable when there is no reason to sort them
		// Stability is important because we do not want to make changes when it is immaterial
		values.push_back(distributions.size() + 1 + index);
	}
	return counter;
}

// We will use a functor object to allow us to reduce a vector with a constant
// Hmm, I'm not convinced here that this is simpler than just explicitly summing the value!
// Once we have this operator we can use it easily, but writing the operator requires multiple cases
struct SumBinaryOp
{
	size_t operator()(size_t a, const ProblemFindMajority2::Distribution& b) const
	{
		return std::reduce(b.begin(), b.end(), a);
	}
	/*
	 https://en.cppreference.com/w/cpp/algorithm/reduce tells us that we must define all four combinations
	 of combining the output type (size_t) with data tyoe (const Distribution&).
	 However we found it compiles with just the one.  I suspect that if we were using a parallel execution
	 policy (which makes no sense in our program) then it might need other cases.  But we don't need them.
	 
	size_t operator()(const ProblemFindMajority2::Distribution& a, const ProblemFindMajority2::Distribution& b) const
	{
		return std::reduce(b.begin(), b.end(), std::reduce(a.begin(), a.end()));
	}
	size_t operator()(const ProblemFindMajority2::Distribution& a, size_t b) const
	{
		return std::reduce(a.begin(), a.end(), b);
	}
	size_t operator()(size_t a, size_t b) const { return a + b; }
	 */
};

ProblemFindMajority2::StateTypeRef ProblemFindMajority2::simplify_state(Distributions&& distributions,
																		Partition2* partition)
{
	// Caller has supplied us with a non-empty list of distributions and a simplified output partition
	// We could just return this as a state
	// But we observe that many states are isomorphic in the sense that if we reordered the parts
	// then we would obtain the same state.  We will attempt to put the state into a canonical form
	
	// We observe that our problem is completely symmetric with regard to L and H coins
	// So we can swap L and H without changing the difficulty of solving the problem
	// If one coin occurs more than the other in the distributions we can exchange the coin types to
	// ensure that H is in the majority.
	size_t h_count = std::reduce(distributions.begin(), distributions.end(), 0, SumBinaryOp());
	size_t l_count = distributions.size() * coin_count - h_count;
	bool swap_coins = (l_count > h_count);
	if (h_count == l_count)
	{
		// The sum of the counts was same for L and H and so does not favour one over other
		// We will try the sum of the squares as well
		// Note that although we can compute the sum of H with std::reduce(), seems not worth messing
		// with reduce_transformation or the like to get l_count.
		h_count = l_count = 0;
		for (auto i = 0; i != partition->size(); ++i)
		{
			auto part_size = (*partition)[i];
			for (auto& d : distributions)
			{
				h_count += d[i] * d[i];
				l_count += (part_size - d[i]) * (part_size - d[i]);
			}
		}
		swap_coins = (l_count > h_count);
	}

	// Did we decide to swap the coins?
	if (swap_coins)
	{
		// Since we own the distributions we will modify them in place
		for (auto i = 0; i != partition->size(); ++i)
		{
			auto part_size = (*partition)[i];
			for (auto& d : distributions)
			{
				d[i] = part_size - d[i];
			}
		}
	}
	
	// Next we sort the parts
	// If two parts have different sizes then their relative order is fixed by partition
	// But if two parts have the same size then switching them around has no effect on solving the problem
	// So we will sort them into ascending order by the histogram of the H counts in distributions
	// Since aome parts will never need the histogram, and others will only need a few entries we will
	// use helper classes that compute histogram on demand
	std::vector<PartCompareHelper> helpers;
	for (size_t i = 0; i != partition->size(); ++i)
	{
		helpers.emplace_back(distributions, i, (*partition)[i]);
	}
	
	// We will actually sort an array of index numbers
	std::vector<size_t> sorted_indexes(partition->size());
	std::iota(sorted_indexes.begin(), sorted_indexes.end(), 0);
	std::sort(sorted_indexes.begin(), sorted_indexes.end(),
			  [&helpers](auto a, auto b) -> bool {
		// Bug fix: 2025-05-18: Sometimes this function is invoked with a == b
		// It seems wrong to me - surely an efficient sort will not compare a value with itself
		// But when it happens the loop below will never end, so we must eliminate this case
		if (a == b)
		{
			return false;
		}
		
		// Otherwise keep fetching values from both indexes until values are different
		// Since each sequence of values ends in a value not otherwise used it must find a difference
		// before it reaches end of sequence, even though different a and b have different lengths
		size_t i = 0;
		size_t a_value = 0;
		size_t b_value = 0;
		do
		{
			a_value = helpers[a][i];
			b_value = helpers[b][i];
			++i;
		} while (a_value == b_value);
		return a_value < b_value;
	});
	
	// Did the sorting actually make any difference?
	if (!std::is_sorted(sorted_indexes.begin(), sorted_indexes.end()))
	{
		// The sorting will not have induced any change in the partition
		// But it will require us to sort the distributions
		// We will sort them in place, since we own the distributions
		// Thus we only need one temporary row
		Distribution temp(partition->size());
		for (auto& d : distributions)
		{
			for (size_t i = 0; i != partition->size(); ++i)
			{
				temp[i] = d[sorted_indexes[i]];
			}
			d.swap(temp);
		}
	}
	
	// The order of the distributions is immaterial, so we will sort them lexiographically
	// The point here is to ensure that two distributions we compare as equal if they are equal
	std::sort(distributions.begin(), distributions.end());
	
	// Finally we can return a state, moving the distributions array into it
	return std::make_unique<StateType>(std::move(distributions), partition);
}

bool ProblemFindMajority2::is_solved(const StateType& state)
{
	// Return true if this state answers the problem.
	// Unlike earlier version we do not need to be concerned that state might be impossible, since we omit such states
	// We count the number of H coins in each distribution
	// The problem is resolved if H is in majority or in the minority for each case
	assert(!state.distributions.empty());
	bool seen_majority = false;
	bool seen_minority = false;
	for (auto& distribution : state.distributions)
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
	// Alternatively there might not have been any distributions (in which case this is an impossible
	// configuration, not a solved one).  But it still counts as resolved.
	return true;
}

void ProblemFindMajority2::write_description(Output2& output)
{
	// Output a description of the problem we want to solve
	output.println("Problem:   Identify majority coin variety from {} coins, each variety has [{}, {}] coins",
				   coin_count, minimum_count, maximum_count);
}

void ProblemFindMajority2::write_solved_node(Output2& output, const StateType& state)
{
	// Output a description of why a node is solved
	// We should introduce the description with the given outcome name
	auto& distributions = state.distributions;
	assert(!state.distributions.empty());
	assert(is_solved(state));
	
	// If there is only one description we will write it on one line
	if (distributions.size() == 1)
	{
		output.println("State:     <Solved: Majority {}>  Heavy-Coins-per-Part: {}",
					   is_majority(distributions[0]) ? "Heavy" : "Light",
					   distributions[0]);
	}
	else
	{
		// We need multiple lines to describe the state
		output.println("State:     <Majority {}>  Multiple-Distributions: {} {{",
					   is_majority(distributions[0]) ? "Heavy" : "Light",
					   distributions.size());
		output.indent();
		for (auto& distribution : distributions)
		{
			output.println("Heavy-Coins-per-Part: {}", distribution);
		}
		output.outdent();
		output << "}";
	}
}

void ProblemFindMajority2::write_ambiguous_state(Output2& output, const StateType& state)
{
	// The caller provides a state (and corresponding partition although not needed here)
	// The caller already knows (via other calls to the problem) that there is insufficient information
	// in the state to solve the problem.  We generate a text description stating what we known
	int count_minority = 0;
	int count_majority = 0;
	for (auto& d : state.distributions)
	{
		if (is_majority(d))
		{
			++count_majority;
		}
		else
		{
			++count_minority;
		}
	}
	
	// We always display it with multiple lines, putting partition on its own line
	// We don't provide an outcome name - the assumption is that we will also report information about children
	output.println("State:     Ambiguous: Heavy Majority: {};  Light Majority: {}  {{",
				   count_majority, count_minority);
	output.indent();
	for (auto& distribution : state.distributions)
	{
		output.println("{} Majority with Heavy-Coins-per-Part: {}",
					   is_majority(distribution) ? "Heavy" : "Light",
					   distribution);
	}
	output.outdent();
	output << "}";
}
