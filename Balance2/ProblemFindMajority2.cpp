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
	Splitter(std::vector<uint8_t>&& indexes) : indexes(std::move(indexes)) {}
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
	std::vector<uint8_t> indexes;		// Vector listing the indexes of our output parts
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

ProblemFindMajority2::ProblemFindMajority2(uint8_t coin_count,
										   bool is_almost_balanced,
										   EnumJoinStrategy join_strategy) :
	coin_count(coin_count),
	minimum_count(1),				// Must be at least one coin of each variety
	maximum_count(coin_count - 1),	// Must be at least one coin that is not of each variety
	threshold((coin_count + 1) / 2),
	join_strategy(join_strategy)
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

// Minor class that generates split distributions
class SplitGenerator
{
	using Distribution = ProblemFindMajority2::Distribution;
	using Distributions = ProblemFindMajority2::Distributions;
	using DistributionIt = Distributions::const_iterator;
	
public:
	SplitGenerator(const Distributions& distributions, Weighing2* weighing, Partition2* partition);
	
	// Since there is always at least one split, after construction operator*() will be interesting
	// Then invoke operator bool() until it returns false - so use a do loop
	const Distribution& operator*() const { return result; }
	operator bool();

private:
	std::vector<std::unique_ptr<Splitter>> splitters;
	Distribution result;
	Partition2* partition;
	DistributionIt current;
	DistributionIt sentinel;
	void init_result();
};

SplitGenerator::SplitGenerator(const Distributions& distributions, Weighing2* weighing, Partition2* partition)
	: result(partition->size()), current(distributions.begin()), sentinel(distributions.end()), partition(partition)
{
	// We will start by creating a vector listing for each input part the output part(s) over which it is split
	auto input_size = distributions.front().size();
	splitters.reserve(input_size);

	// In the earlier version this was not needed since output parts from the same input part were placed
	// together, but the requirement that part sizes do not decrement means this is no longer true
	std::vector<std::vector<uint8_t>> split_indexes(input_size);
	for (uint8_t i = 0; i != weighing->size(); ++i)
	{
		split_indexes[(*weighing)[i].part].push_back(i);
	}
	
	// We now create a vector of splitters, which will handle splitting and reordering
	for (auto& split_index : split_indexes)
	{
		switch (split_index.size())
		{
			case 1:
				splitters.push_back(std::make_unique<SplitterOne>(std::move(split_index)));
				break;
			case 2:
				splitters.push_back(std::make_unique<SplitterTwo>(std::move(split_index)));
				break;
			case 3:
				splitters.push_back(std::make_unique<SplitterThree>(std::move(split_index)));
				break;
			default:
				// Split counts should always be 1, 2 or 3
				assert(false);
		}
	}
	
	// Initialise result using the first split of first distribution
	init_result();
}

SplitGenerator::operator bool()
{
	// We do not expect to be called again once we have examined every distibution
	assert(current != sentinel);
	
	// Examine each splitter in turn, looking for one that provides another split
	for (auto splitter = splitters.begin(); splitter != splitters.end(); ++splitter)
	{
		// Are we able to advance this splitter?
		if ((*splitter)->advance(result, *partition))
		{
			// All the splitters that had finished must be restarted
			// Note that since all the splitters write into different parts of current it does not
			// matter that we advanced them in uneven order
			for (auto it = splitters.begin(); it != splitter; ++it)
			{
				(*it)->restart(result, *partition);
			}

			// We have found a new distribution, and set it to result
			return true;
		}
	}

	// If possible start again with another distribution
	if (++current == sentinel)
	{
		return false;
	}
	else
	{
		init_result();
		return true;
	}
}

void SplitGenerator::init_result()
{
	// Initialise the result distribution based on the current distribution by resetting each splitter
	assert(current != sentinel);
	for (auto [h_count, splitter] : std::views::zip(*current, splitters))
	{
		splitter->set_count(h_count);
	}

	// Restart all of the splitters (which will overwrite all entries in `current` to a valid output distribution)
	for (auto& splitter : splitters)
	{
		splitter->restart(result, *partition);
	}
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
	size_t input_size = state.partition->size();
	if (partition->size() != input_size)
	{
		// At least one part must be split.  Even a part is not split its index may change.
		// So we need to compute a new Distributions object
		// We use a helper class to generate the distributions, and extract them into a vector
		Distributions split_distributions;
		SplitGenerator generator(state.distributions, weighing, partition);
		do
		{
			split_distributions.emplace_back((*generator).begin(), (*generator).end());
		} while (generator);
		
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

// Helper function to decide the outcome of a weighing for a specific distribution
inline Outcome apply_weighing_to_distribution(const ProblemFindMajority2::Distribution& distribution,
									   Weighing2& weighing)
{
	// Determine the outcome of the given weighing to the given distribution
	// Count number of H coins placed in each pan by this distribution
	int count_left = 0;
	int count_right = 0;
	for (size_t i = 0; i != weighing.size(); ++i)
	{
		switch (weighing[i].placement)
		{
			case LeftPan:
				count_left += distribution[i];
				break;
			case RightPan:
				count_right += distribution[i];
				break;
			case SetAside:
				break;
		}
	}

	// The counts tell us which pan is heavier
	if (count_left > count_right)
	{
		return Outcome::LeftHeavier;
	}
	else if (count_right > count_left)
	{
		return Outcome::RightHeavier;
	}
	else
	{
		return Outcome::Balances;
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
		outcome_distributions[apply_weighing_to_distribution(current, weighing)].push_back(&current);
	}
	
	// We have computed the output state, but we might be able to simplify it
	OutcomeArray<ProblemFindMajority2::StateTypeRef> result;
	for (int i = Outcome::Begin; i != Outcome::End; ++i)
	{
		result[i] = simplify_partition(outcome_distributions[i], partition);
	}
	return result;
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
	// But we will attempt several optimisations which have potential to considerably reduce work done.
	// Many distribution+partition combinations are isomorphic in sense that that a solution to one
	// corresponds directly (in particular has identical depth) to a solution to the other.
	// When this happens we want to always use the **same** distribution and partition to reduce the number
	// of cases we consider.  It isn't enough to allow us to enumerate all states (its still exponential!)
	// but we did reduce number of states for tractable instances by a factor of more than 40.
	
	// This function will identify parts that can be joined together.  Sometimes, most notablely when
	// we have identified precise variety of several coins, we will find several parts where using one part
	// and splitting it to the original parts results in the same distributions.  In that case we have
	// multiple ways of representing the same state so we should switch to a single representation.
	// Its clear that the single representation we should use is the one with fewest parts since it
	// consumes less memory now, and will have a much lower branching factor in the future.
	
	// But the effort required to search for parts to join is considerable.  In the worst case we have
	// an algorithm that is cubic in number of parts, log-linear in number of distributions.
	// Also we do not see that many cases where the more obscure joins are possible.
	// So we use an enumeration to control what to do.
	
	// The distributions passed into this function are constant.  If we do join parts we will need to
	// create new distributions.  Since the downstream functions will find it convenient to edit
	// distributions in place (and since ultimately we will return a uniquely owned state) we find
	// it convenient to create owned distributions here which will be moved into final object.
	Partition2* joined_partition = nullptr;
	Distributions joined_distributions;
	joined_distributions.reserve(distributions.size());

	// We have been configured with join algorithm to use
	using enum EnumJoinStrategy;
	switch (join_strategy)
	{
		case None:
			break;
		case SameVariety:
			joined_partition = join_same_variety(distributions, partition, joined_distributions);
			break;
		case All:
			joined_partition = join_all(distributions, partition, joined_distributions);
			break;
		case Validate:
		{	// For testing we run both All and SameVariety
			// We have never yet seen a case where it asserts because All returned different to SameVariety
			// So either there is a bug, or the problem rarely if ever benefits from these joining
			Distributions same_distributions;
			auto same_partition = join_same_variety(distributions, partition, same_distributions);
			Distributions all_distributions;
			auto all_partition = join_all(distributions, partition, all_distributions);
			if (!same_partition)
			{
				// Want to find any case where all identifies a join not found in same
				// Note that all algorithm always returns a result, but it may be input partition
				assert(all_partition == partition);
				return simplify_state(std::move(all_distributions), partition);
			}
			assert(same_partition == all_partition);
			auto same_result = simplify_state(Distributions(same_distributions), same_partition);
			auto all_result = simplify_state(Distributions(all_distributions), all_partition);
			assert(same_result->distributions == all_result->distributions);
			return same_result;
		}
	}
	
	// Were we unable to compute a new partition by joining parts from the original one
	if (!joined_partition)
	{
		// We will use the original partion, but we still need to copy out the distribution
		joined_partition = partition;
		for (auto& d : distributions)
		{
			joined_distributions.emplace_back(d->begin(), d->end());
		}
	}

	// Call next level function that will look for more optimisations
	return simplify_state(std::move(joined_distributions), joined_partition);
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

Partition2* ProblemFindMajority2::join_same_variety(const std::vector<const Distribution*>& distributions,
													Partition2* partition,
													Distributions& output_distributions)
{
	// Identify parts that can be joined, and if we find some returned the joined distribution
	// If we cannot join anything we should return nullptr and leave output_distributions alone
	// We perform a much faster search (than `join_all`) because we only join parts that in every distribution
	// are listed as being restricted to a single variety.  This is most likely case and allows us to find
	// all of the joins in log-linear complexity over number of parts.

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
	
	// If we have same number of groups as we had parts then we did not find anything to join
	if (groups.size() == partition->size())
	{
		return nullptr;
	}
	
	// Sorting the groups because the merged parts must be in ascending order by size
	std::sort(groups.begin(), groups.end(), compare_groups);
	
	// For each input distribution we need to compute the
	for (auto& input : distributions)
	{
		Distribution& output = output_distributions.emplace_back();
		
		// Each group specifies the part(s) to be joined to make the next value in the output distribution
		for (auto& group : groups)
		{
			auto it = group.first.begin();
			if (*it == UniquePartMarker)
			{
				// This group was not joined, so we can copy over a single part
				++it;
				output.push_back((*input)[*it]);
			}
			else
			{
				// Create new entry in output distribution
				output.push_back((*input)[*it]);
				
				// Join in other distributions
				for (++it; it != group.first.end(); ++it)
				{
					output.back() += (*input)[*it];
				}
			}
		}
	}
			
	// Extract the joined partition and return the cached instance of this partition
	std::vector<uint8_t> joined_parts;
	for (auto& group : groups)
	{
		joined_parts.push_back(group.second);
	}
	return Partition2::get_instance(std::move(joined_parts));
}

// Helper class used to implement join_all
// This class keeps track of some of the parameters
class JoinAllHelper
{
	using Distributions = ProblemFindMajority2::Distributions;
	using Distribution = ProblemFindMajority2::Distribution;
	
public:
	JoinAllHelper(Distributions& distributions, Partition2* partition)
		: distributions(distributions), partition(partition), indexes(distributions.size())
	{
		std::iota(indexes.begin(), indexes.end(), 0);
		seen = indexes.begin();
	}
	Partition2* get_partition() { return partition; }
	bool check_pair(size_t first_part, size_t second_part);
private:
	Distributions& distributions;				// The vector of all distributions we are considering
	Partition2* partition;						// The partition we are imposing
	std::vector<int> indexes;					// Vector used as a heap to track the rows
	std::vector<int>::iterator seen;			// Part of the vector beyond its heap
	Distribution* current = nullptr;			// Sample distribution from current part
	size_t part_a = 0;							// One of the parts we would like to join
	size_t part_b = 0;							// Other part we would like to join
	
	// Helper functions
	size_t make_joined_partition();
	void make_joined_distribution(size_t new_part, std::vector<uint8_t>& joined_column);
	bool compare_indexes(int i, int j)
	{
		// Determine whether distribution i or j should come first
		// where sort distributions lexiographically by size within each part
		// except that we move part_a and part_b to the back of the compare order
		if (i == j)
		{
			// A distribution is never smaller than itself
			return false;
		}
		
		auto& d_i = distributions[i];
		auto& d_j = distributions[j];
		
		// Sort lexigraphically, except not on parts a and b
		for (size_t part = 0; part != partition->size(); ++part)
		{
			if (part != part_a && part != part_b && d_i[part] != d_j[part])
			{
				return d_i[part] < d_j[part];
			}
		}
		
		// Now sort by a and b in order
		if (d_i[part_a] != d_j[part_a])
		{
			return d_i[part_a] < d_j[part_a];
		}
		assert(d_i[part_b] != d_j[part_b]);		// We maintain invariant that all distributions are different
		return d_i[part_b] < d_j[part_b];
	}
	
	bool is_current_group()
	{
		// Test if the distribution whose index is top of the heap is from the current group
		assert(seen != indexes.begin());
		auto& distribution = distributions[indexes.front()];
		for (size_t part = 0; part != partition->size(); ++part)
		{
			if (part != part_a && part != part_b && distribution[part] != (*current)[part])
			{
				return false;
			}
		}
		
		// All parts other than the parts we are comparing matched, so it is the same group
		return true;
	}
};

bool JoinAllHelper::check_pair(size_t first_part, size_t second_part)
{
	// We are now going to consider trying to join the two given parts
	// Return false if we determine these parts cannot be joined
	assert(first_part < second_part);
	part_a = first_part;
	part_b = second_part;
	
	// To decide that two parts can be joined will require us to examine all of the rows in order
	// But usually we will abort after just looking at a few rows
	// We use a heap in the hope that the heap is faster to build that a full sort
	std::make_heap(indexes.begin(), indexes.end(), [this](int i, int j) { return compare_indexes(i, j); });
	seen = indexes.end();
	
	// Create a vector recording the values that would be in a putative joined column
	std::vector<uint8_t> joined_column;
	
	// Each time round this loop we will consume one or more rows from the same group
	// We will exit early if we ever find that the rows in a group are inconsistent with being joinable
	do
	{
		// Start a new group of related rows, under assumption that there is at least one more row unexamined
		// We are starting a new group of rows that have same values for parts other than a/b as the current row
		current = &distributions[indexes.front()];
		auto a = (*current)[part_a];
		auto b = (*current)[part_b];
		joined_column.push_back(a + b);
		
		// As we advance through the sorted distributions with the same fixed vector we expect
		// only a_value and b_value to change.  Since we sort by a before b, if this is joinable
		// then each subsequent row will increase a value by 1 and decrese b value by 1.
		// It will stop when either b_value is zero or a_value is the size of part a.
		
		// We require that it started at the beginning.  So either a_value must be 0 now,
		// or b_value must be size of part[b]
		if (a != 0 && b != (*partition)[part_b])
		{
			return false;
		}
		
		// Remove the current distribution from the heap
		std::pop_heap(indexes.begin(), seen, [this](int i, int j) { return compare_indexes(i, j); });
		--seen;
		
		// The current row might be part of a larger group - try to consume more rows
		while (seen != indexes.begin() && is_current_group())
		{
			// Check we can safely change a_value and b_value as required
			if (a == (*partition)[part_a] || b == 0)
			{
				return false;
			}
			
			// Check that the new top row has correct values
			if (distributions[indexes.front()][part_a] != ++a || distributions[indexes.front()][part_b] != --b)
			{
				return false;
			}
			
			// The row we read is consistent with the current pair, so we may consume it
			joined_column.push_back(0xff);
			std::pop_heap(indexes.begin(), seen, [this](int i, int j) { return compare_indexes(i, j); });
			--seen;
		}
		
		// There were no more rows in the current group
		// It should have been the case that either we could not decrement b_value again, or could not increment the other
		if (a != (*partition)[part_a] && b != 0)
		{
			return false;
		}
	} while (seen != indexes.begin());
	
	// We got to the end, so we may join the columns
	make_joined_distribution(make_joined_partition(), joined_column);
	
	// The indexes might now be incorrect because we might have fewer rows - erase any extra ones but preserve order
	if (indexes.size() > distributions.size())
	{
		std::erase_if(indexes, [&](int index) { return index >= distributions.size(); });
	}
	return true;
}

size_t JoinAllHelper::make_joined_partition()
{
	// We have decided that we will join part_a and part_b
	// We compute the new partition (and store it within this class)
	// We return the part number of the joined part within it.
	std::vector<uint8_t> parts;
	for (size_t i = 0; i != part_a; ++i)
	{
		parts.push_back((*partition)[i]);
	}
	for (size_t i = part_a + 1; i != part_b; ++i)
	{
		parts.push_back((*partition)[i]);
	}
	
	// We can determine size of the new part
	uint8_t new_part_size = (*partition)[part_a] + (*partition)[part_b];
	
	// Insert remaining existing parts that are smaller than new part
	size_t i = part_b + 1;
	while (i != partition->size() && (*partition)[i] < new_part_size)
	{
		parts.push_back((*partition)[i]);
		++i;
	}
	
	// Insert the new part here
	size_t result = parts.size();
	parts.push_back(new_part_size);
	
	// Finally copy over any remaining parts
	while (i != partition->size())
	{
		parts.push_back((*partition)[i]);
		++i;
	}
	
	// Replace the partition, and return part number of the new column
	partition = Partition2::get_instance(std::move(parts));
	return result;
}

void JoinAllHelper::make_joined_distribution(size_t new_part, std::vector<uint8_t>& joined_column)
{
	// We replace the distribution with a joined distribution
	Distribution temp;

	// Walk over the entries in the joined column
	// Because we popped the heap entires to the back of the heap vector we can obtain the corresponding index
	// by looking at this vector in reverse order
	auto index_id = indexes.rbegin();
	for (uint8_t value : joined_column)
	{
		// Select the distribution we will edit
		auto& distribution = distributions[*index_id];
		++index_id;
		
		// Did we decide to remove this row?
		if (value == 0xff)
		{
			// We cannot erase row now since this will upset index - we will just set it empty vector
			distribution.clear();
		}
		else
		{
			// Build a new distribution for this row
			temp.clear();
			for (size_t i = 0; i != part_a; ++i)
			{
				temp.push_back(distribution[i]);
			}
			for (size_t i = part_a + 1; i != part_b; ++i)
			{
				temp.push_back(distribution[i]);
			}
			
			// Insert remaining parts (NB since partition has been replaced we use distribution.size())
			for (size_t i = part_b + 1; i != distribution.size(); ++i)
			{
				// The new_part index number is with repect to new partition
				if (temp.size() == new_part)
				{
					temp.push_back(value);
				}
				temp.push_back(distribution[i]);
			}

			// New part might also be after all of the existing parts
			if (temp.size() == new_part)
			{
				temp.push_back(value);
			}
			
			// Put the temp distribution into place
			distribution.swap(temp);
		}
	}

	// If we cleared some rows we must erase them now
	// C++20 Note: Helper algorithm to simplify erasing rows that satisfy some predicate
	std::erase_if(distributions, [](const auto& d) { return d.empty(); });
}

Partition2* ProblemFindMajority2::join_all(const std::vector<const Distribution*>& distributions,
										   Partition2* partition,
										   Distributions& output_distributions)
{
	// We search for all possible joins between parts
	// Observe that if parts (a), (b) and (c) can be joined then so can (a) and (b), then (a, b) with (c)
	// So we will search for pairs of parts to join.  We will view the parts in partition order
	// so if we make a join the newly created column will be placed further to right.
	// We do not attempt to optimise for multiple joins (which is unlikely to occur!)

	// When we join parts we will need to rebuild the distributions.
	// Since this might happen several times it will be easier to copy out the distributions here
	for (auto& d : distributions)
	{
		output_distributions.emplace_back(d->begin(), d->end());
	}
	
	// Use helper class to track whether or not columns are joinable and the corresponding intermediate state
	JoinAllHelper helper(output_distributions, partition);
	
	// Note that we could replace partition during this loop, but the index numbers remain valid
	// So we must use < not != when looking to see if we have reached end of loop
	for (size_t a = 0; a < partition->size(); ++a)
	{
		for (size_t b = a + 1; b < partition->size(); ++b)
		{
			if (helper.check_pair(a, b))
			{
				// The distribution was changed in place, need to update partition as well (for loops)
				partition = helper.get_partition();
				
				// The original parts a and b no longer exist
				// A new part was created and its part number must be >= part a's part number
				// So we can keep using a for the next pair (although index now denotes a different part)
				// But we need to restart b at a+1.  Since will be incremented just set it to a here.
				// Note that although a still denotes a part, a+1 might not.  But if it does not then
				// both loops will exit immediately (it would mean we joined last two parts).
				b = a;
			}
		}
	}
	return partition;
}

// Helper class used to compare two parts
// It caches a vector to compare lexiographically
// We put the part_size at the beginning (so part_size controls order)
// Otherwise we put the content of the column, but sorted (so two columns with same values compare equal)
class PartCompareHelper
{
public:
	PartCompareHelper(const ProblemFindMajority2::Distributions& distributions,
					  size_t index,
					  uint8_t part_size) : values{part_size}
	{
		// We start values vector with the part_size
		// Append the values from the corresponding column, in sorted order
		values.reserve(distributions.size() + 1);
		for (auto& distribution : distributions)
		{
			values.push_back(distribution[index]);
		}
		std::sort(++values.begin(), values.end());
	}
	auto operator<=>(const PartCompareHelper&) const = default;
	
private:
	std::vector<size_t> values;									// Compare values we have computed
};

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

void reorder_distribution(ProblemFindMajority2::Distributions& input,
						  std::vector<size_t>& sorted_indexes,
						  ProblemFindMajority2::Distributions& output)
{
	// We copy the distributions from input into output
	// We assume on entry that output already has already allocated all necessary space so we can just overwrite it
	// We sort the columns into the given order as we copy them
	// We sort the rows after that
	size_t row_size = sorted_indexes.size();
	for (size_t i = 0; i != input.size(); ++i)
	{
		auto& distribution_in = input[i];
		auto& distribution_out = output[i];
		for (size_t j = 0; j != row_size; ++j)
		{
			distribution_out[j] = distribution_in[sorted_indexes[j]];
		}
	}
	
	// Remove row order uncertainty by sorting it
	std::sort(output.begin(), output.end());
}

typedef std::vector<std::pair<std::vector<size_t>::iterator, std::vector<size_t>::iterator>> RangesType;

struct ColumnCompare
{
	bool operator()(size_t a, size_t b) const
	{
		// We compare two integers by lexiographically comparing the values found in the corresponding columns
		if (a == b)
		{
			// Shortcut when comparing index with itself
			return false;
		}
		for (auto&d : distributions)
		{
			if (d[a] != d[b])
			{
				return d[a] < d[b];
			}
		}
		// Columns are identical, so we return false
		return false;
	}
	ProblemFindMajority2::Distributions& distributions;
};

bool advance_sorted_index(const RangesType& ranges, const ColumnCompare& comparator)
{
	// We want to consider every permutation of each of the ranges
	// We change sorted_indexes, and return false if we are back to the start
	for (auto& range : ranges)
	{
		// Can we find a new permutation by changing this range?
		if (std::next_permutation(range.first, range.second, comparator))
		{
			return true;
		}
		// If next_permutation was done on the given range it reset it back to the beginning
	}
	
	// All of the ranges were reset, so there are no more permutations
	return false;
}

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
	
	// Next observe that the names attached to parts are unimportant
	// So we can reorder parts without changing anything.  We want to put parts into a canonical order.
	// This means that we can exchange columns within the distribution without changing the state.
	// Note that each column is associated with a part size (from the partition, not the distribution)
	// and two states that differ solely by part_size are still different.  So we cannot exchange columns
	// that correspond to parts with different sizes.
	
	// But if two parts have the same size then switching them around has no effect on solving the problem.
	// If two parts have different histograms (i.e. the values on the column are not the same values in
	// a different order) then no matter how the rows are sorted the columns can never match.
	// So it makes sense to order parts with respect to the sorted values on thier columns.
	
	// To avoid repeating the column sorting we will use helper classes of type PartCompareHelper
	std::vector<PartCompareHelper> helpers;
	for (size_t i = 0; i != partition->size(); ++i)
	{
		helpers.emplace_back(distributions, i, (*partition)[i]);
	}

	// To avoid unnecessary swaps we will actually sort an array of integers.
	std::vector<size_t> sorted_indexes(partition->size());
	std::iota(sorted_indexes.begin(), sorted_indexes.end(), 0);
	std::sort(sorted_indexes.begin(), sorted_indexes.end(),
			  [&helpers](auto a, auto b) -> bool { return helpers[a] < helpers[b]; });

	// This approach helps, but does not fully solve the problem.  Consider following two distributions:
	//	Partition: 2 2 3						2 2 3
	//	           -----						-----
	//			   2 0 1						0 2 1
	//			   0 2 2						2 0 2
	// The first and second columns have identical histograms so their order is non-deterministic.
	// But if we exchange the columns in second one then the states become identical.
	// I went looking for algorithm, but there is no known effective algorithm.  In particular one can model
	// a graph as its agency matric.  If two graphs are isomorphic then their matrixes, after row/column
	// exchanges are identical.  But determining if two graphs are isomorphic is neither known to be in P
	// nor to be NP-complete.  (It is suspected that it is an NP-indeterminate problem.)
	// In any case we will not find an efficient solution.  Given that this program is inherently
	// infeasible, we will consider all of these cases individually.  Our hope is that since we found
	// an example of the problem so easily that we will save more on having fewer states than it costs
	// on trying to reduce each state to a simple representative.
	
	// We identify ranges within sorted_indexes where we have a non-trivial range of indexes that compare as equal
	// Although there are range/views to replace the pair I'm not clear that it buys me much here
	RangesType ranges;
	auto b = sorted_indexes.begin();
	auto e = std::next(b);
	while (e != sorted_indexes.end())
	{
		// Do the iterators point to equal values
		if (helpers[*b] == helpers[*e])
		{
			// Keep advancing e until it is different to a
			while (++e != sorted_indexes.end() && helpers[*b] == helpers[*e]) {}
			
			// We have found a range of at least two indexes that equal for the helper
			ranges.emplace_back(b, e);
			
			// If we already reached end there are no more columns to consider
			if (e == sorted_indexes.end())
			{
				break;
			}
		}

		// Start looking for another range at e
		b = e;
		++e;
	}
	
	// Did we find any ranges?
	if (ranges.empty())
	{
		// There is only one possible order for the parts, so we will edit `distributions` in place if necessary
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
		
		// The order of the distributions has no significance, so we sort it prior to returning
		std::sort(distributions.begin(), distributions.end());
		return std::make_unique<StateType>(std::move(distributions), partition);
	}

	// Otherwise we want to consider an exponential number of states and return the "best"
	// The best one is the one with smallest distribution
	Distributions best_seen(distributions.size(), Distribution(partition->size()));
	Distributions workspace(distributions.size(), Distribution(partition->size()));

	// Currently indexes within sorted_indexes that compared as equal are in non-deterministic order
	// Ensure that are sorted with respect to values of the coresponding column
	ColumnCompare column_comparator{distributions};
	for (auto& range : ranges)
	{
		std::sort(range.first, range.second, column_comparator);
	}
	
	// We can write the first order directly into best_seen
	reorder_distribution(distributions, sorted_indexes, best_seen);
	
	// Keep considering new orders of sorted_indexes
	// The number can grow very large, so we cap it at 5040 (=7!) permutations
	size_t counter = 1;
	while (advance_sorted_index(ranges, column_comparator) && counter <= 5040)
	{
		// Construct, in workspace the distributions for the latest ordering
		reorder_distribution(distributions, sorted_indexes, workspace);
		
		// If this one is better than the one we already had switch to it
		if (workspace < best_seen)
		{
			best_seen.swap(workspace);
		}
		++counter;
	}

	// The vector in best_seen is the one we want
	return std::make_unique<StateType>(best_seen, partition);
}

bool ProblemFindMajority2::apply_weighing_lite(const StateType& state, Weighing2* weighing, Partition2* partition)
{
	// This is a greatly simplified implementation of apply_weighing() which we hope will allow us to try
	// to brute force a slightly larger number of coins (say 11 instead of 9)
	// We expect to invoke this method on a state that is not resolved
	// The method will return true if all three outcomes of the weighing are resolved (i.e. represent
	// solved states or are impossible).  Otherwise it will return false.
	
	// We can answer this question much faster since we do not need to be concerned with simplifying the state
	// or indeed even with constructing its members.  As soon as we have found two outcomes with different
	// majorities we know it is false.
	
	// We hope to save some time this way.
	// But the real saving is that we will not attempt to save any states after four levels of weighing.
	// We hope this will allow us to find a solution with far fewer saved states.
	
	// For each outcome we track if we have seen majority H (+1) majority L (-1) or neither (0) distributions
	OutcomeArray<int> seen{0, 0, 0};
	
	// The weighing refines the partitions if and only if the output partition has more parts
	size_t input_size = state.partition->size();
	if (partition->size() != input_size)
	{
		// At least one part must be split.  Even a part is not split its index may change.
		// So we need to compute a new Distributions object
		// We use a helper class to generate the distributions
		SplitGenerator generator(state.distributions, weighing, partition);
		do
		{
			// There is no need to store the split distributions - we just apply them now
			auto& seen_outcome = seen[apply_weighing_to_distribution(*generator, *weighing)];
			if (is_majority(*generator))
			{
				if (seen_outcome == -1)
				{
					return false;
				}
				else
				{
					seen_outcome = 1;
				}
			}
			else
			{
				if (seen_outcome == 1)
				{
					return false;
				}
				else
				{
					seen_outcome = -1;
				}
			}
		} while (generator);
	}
	else
	{
		// There is no need to split parts, so we just process each input distribution
		assert(check_part_order(weighing));
		for (auto& d : state.distributions)
		{
			auto& seen_outcome = seen[apply_weighing_to_distribution(d, *weighing)];
			if (is_majority(d))
			{
				if (seen_outcome == -1)
				{
					return false;
				}
				else
				{
					seen_outcome = 1;
				}
			}
			else
			{
				if (seen_outcome == 1)
				{
					return false;
				}
				else
				{
					seen_outcome = -1;
				}
			}
		}
	}
	
	// Each outcome must have had at most one majority decision (it might have had none)
	return true;
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
