//
//  Partition2.cpp
//  Balance2
//
//  Created by William Hurwood on 5/14/25.
//

#include <cassert>
#include <algorithm>
#include "Partition2.hpp"
#include "Weighing2.hpp"
#include "Output2.hpp"

// Initialise static values
decltype(Partition2::cache) Partition2::cache{};

// We use a local generator class to track the creation of weighings below a partition
class Generator
{
public:
	Generator(const Partition2& input);
	Partition2::Child get() const;
	bool advance();

private:
	const Partition2& input;				// The partition whose weighings we are computing
	std::vector<uint8_t> left;				// For each input part, the number of coins placed in left pan
	std::vector<uint8_t> right;				// For each input part, the number of coins placed in right pan
	
	// Helper methods
	bool select_right();
	bool advance_left();
	bool advance_right();
	void fill_left(uint8_t count, size_t index = 0);
	void fill_right(uint8_t count, size_t index = 0);
	uint8_t pan_count() const { return std::reduce(left.begin(), left.end()); }
};

Generator::Generator(const Partition2& input) :
	input(input),
	left(input.size()),
	right(input.size())
{
	// The first weighing selects one coin for each pan, taking it from earlier available part
	assert(input.coin_count() >= 2);
	left[0] = 1;
	if (input[0] >= 2)
	{
		right[0] = 1;
	}
	else
	{
		right[1] = 1;
	}
}

Partition2::Child Generator::get() const
{
	// Under assumption that caller already knows the generator has another weighing
	// report the value of that weighing
	
	// The natural order that we generate output parts is not the order we are required to use
	// We will accumulate the data in a local structure so its easier to sort
	struct Item
	{
		int input_part;				// Input partition part from which this output part is refined
		Placement placement;		// Pan into which this set of coins is placed
		uint8_t part_size;			// Number of coins in this output part
		
		bool operator<(const Item& other) const
		{
			// Sort by part_size then input_part then placement
			if (part_size != other.part_size)
			{
				return part_size < other.part_size;
			}
			if (input_part != other.input_part)
			{
				return input_part < other.input_part;
			}
			return placement < other.placement;
		}
	};

	// Each input part will yield between one and three output parts
	// Compute the provanences and input part sizes in order of output
	std::vector<Item> items;
	for (size_t i = 0; i != input.size(); ++i)
	{
		auto count = input[i];
		if (left[i] > 0)
		{
			items.emplace_back(i, Placement::LeftPan, left[i]);
			count -= left[i];
		}
		if (right[i] > 0)
		{
			items.emplace_back(i, Placement::RightPan, right[i]);
			count -= right[i];
		}
		if (count > 0)
		{
			items.emplace_back(i, Placement::SetAside, count);
		}
	}

	// Sort the items to favour the partition size ordering over input item part number
	std::sort(items.begin(), items.end());
	
	// Separate the partition and weighing information
	std::vector<uint8_t> output_parts;
	std::vector<Weighing2::Part> provenances;
	output_parts.reserve(items.size());
	provenances.resize(items.size());
	for (auto& item : items)
	{
		output_parts.push_back(item.part_size);
		provenances.emplace_back(item.input_part, item.placement);
	}
	
	// Switch to a cached instance of each object so we only have one of each
	return
	{
		Weighing2::get_instance(std::move(provenances)),
		Partition2::get_instance(std::move(output_parts))
	};
}

bool Generator::advance()
{
	// Switch known valid generator to another weighing, returning true if successful
	// Attempt to find another weighing by changing the right pan, keeping left pan fixed
	if (!advance_right())
	{
		// There were no more options to advance right pan, so advance left pan instead
		if (!advance_left())
		{
			// Since we could not make a different selection for left pan, it is impossible to make another
			// selection for entire weighing.
			return false;
		}
		else if (!select_right())
		{
			// The large comment in Weighing.cpp justifies the claim that if it is impossible to select right
			// lexiographically smaller than left (we know that there are enough coins if we had a free selection)
			// then it will be impossible for any left lexiographically smaller than its current value
			// So we should go directly to case where we start with a larger selection
			uint8_t new_pan_count = pan_count() + 1;
			if (2 * new_pan_count > input.coin_count())
			{
				// There are not enough coins to use selections of this size
				return false;
			}
			else
			{
				// Set left to be the first coins - it has to be possible to fill on right
				fill_left(new_pan_count);
				select_right();
			}
		}
	}
	
	// We were able to make new selection
	return true;
}

bool Generator::select_right()
{
	// Try to select coins for right pan consistent with left pan selection
	// We do not assume that right[] is set to all zeros on entry
	
	// First try to set every part in right to same in left
	uint8_t count = pan_count();
	for (size_t index = 0; index != right.size(); ++index)
	{
		if (input[index] >= 2 * left[index])
		{
			right[index] = left[index];
			count -= right[index];
		}
		else
		{
			// We cannot match left selection for this part, since there were too few coins left in the part
			right[index] = std::min(count, static_cast<uint8_t>(input[index] - left[index]));
			count -= right[index];
			
			// Now that we have selected fewer coins at index we can select maximal available coins in all
			// other parts without violating lexical constraint
			// We do not use fill_right() since we do not know that we will be able to select count coins
			for (auto i = index + 1; i != right.size(); ++i)
			{
				right[i] = std::min(count, static_cast<uint8_t>(input[i] - left[i]));
				count -= right[i];
			}
			
			// If we were able to select all coins then this must be lexiographically first selection
			if (count == 0)
			{
				return true;
			}
			
			// The largest possible (lexiographically) selection did not work
			// However a better selection might be possible.  There may be some i (i < index) where
			// we could select one fewer coins.  This might allow us to select more for later indexes
			// because we would satisfy the lexical concern earlier
			++count;

			// Search back looking for indexes where we could have choosen more
			// NB There is no point in looking at index==0 since we need to have an earlier part to decrement
			while (index > 1)
			{
				// Previously we selected right[index] coins here
				// But without lexiographical constraint we could have selected input[index] - left[index]
				--index;
				count += right[index];
				right[index] = std::min(count, static_cast<uint8_t>(input[index] - left[index]));
				count -= right[index];
				
				// If we have reduced count to zero we have a chance
				if (count == 0)
				{
					// If we can remove one from an index before current index then we will have a solution
					while (index > 0)
					{
						if (right[--index] > 0)
						{
							// Reduce size of selection for this part
							--right[index];

							// However the solution could easily be wrong because we added coins in wrong order
							// (To be lex maximal we should have added in order of incrementing index)
							// We know count up this index is not enough (since right[i] <= left[i] in this range)
							count = pan_count();
							for (size_t i = 0; i <= index; ++i)
							{
								count -= right[i];
							}
							assert(count >= 1);
							
							// Now we fill again.  The point is that because we placed some coins earlier
							// this fill will run out of coins before it reaches the end
							fill_right(count, index + 1);
							return true;
						}
					}
					
					// We must have used the first non-zero part to get enough coins
					// This means that we cannot find a selection that satisfies the lexiograph restriction
					return false;
				}
			}
			return false;
		}
	}
	
	// Identical selection works - it must be the lexically largest permitted selection
	return true;
}

bool Generator::advance_left()
{
	// We assume that left contains a valid selection.  In addition to satisfying physical constaint
	// it is known that it is possible to make a valid selection for right pan as well.
	// We change the left vector to a different valid selection, returning false if this cannot be done.
	
	// Initially we keep the current number of coins in the pan fixed, but try selecting them from a different
	// part of the partition.  The new selection will be lexiographically smaller.
	
	// We start by searching from back of vector looking for a part where there is room to increase the number
	// of coins selected.  We count the number of coins that we have passed over
	uint8_t count = 0;
	size_t index = left.size() - 1;
	while (left[index] == input[index])
	{
		count += left[index];
		
		// The input assumption that it is possible to make a valid assumption for right pan implies
		// that there must be a part for which left pan did not select all coins in partition
		// So we should not need to check at run time that index has not underflowed
		assert(index > 0);
		--index;
	}
	count += left[index];
	
	// Now we search for an earlier part that has selected some coins in left pan
	// It is possible that no such part exists
	while (index > 0)
	{
		--index;
		if (left[index] > 0)
		{
			// We advance left pan by decrementing this slot, and selecting coins as early as possible after it
			// Since we already passed a slot with spare capacity we must be able to place count coins
			--left[index];
			fill_left(count + 1, index + 1);
			return true;
		}
	}
	
	// There are no more selections for left with the same size
	// count must have cleared all of the previous coins so it tells us how many coins were initially in the left pan
	// We can try again with a larger selection, but we will only do so if there are enough coins in partition
	// to permit this.
	++count;
	if (2 * count > input.coin_count())
	{
		return false;
	}
	
	// Restart with one more coin than before
	fill_left(count);
	return true;
}

bool Generator::advance_right()
{
	// Assume that on entry left and right are both set to valid selections
	// Advance right to the next entry which is lexiographically smaller than its current entry but which uses
	// same number of coins and respects coins already taken by left
	// Return true if this can be done
	// Note that we do not need to do anything to ensure that the new right selection is leciographically
	// smaller than left selection.  It must be true by transitivity.

	// Search for last part with capacity to take more coins
	uint8_t count = 0;
	size_t index = right.size() - 1;
	while (right[index] + left[index] == input[index])
	{
		count += right[index];

		// Unlike left pan (where we know there is spare capacity for right pan) it is possible that there is no
		// spare capacity.  This would happen if the weighing includes all coins in the partition
		if (index == 0)
		{
			// Since there is no spare capacity there are no other ways to advance right pan's content
			return false;
		}
		--index;
	}
	count += right[index];
	
	// Now we search for an earlier part that has selected some coins in right pan
	// It is possible that no such part exists
	while (index > 0)
	{
		--index;
		if (right[index] > 0)
		{
			// We advance right pan by decrementing this slot, and selecting coins as early as possible after it
			// Since we already passed a slot with spare capacity we must be able to place count coins
			--right[index];
			fill_right(count + 1, index + 1);
			return true;
		}
	}
	
	// There are no more options for right pan with this number of coins
	// Since both pans must have same number of coins we give up here (advance_left() tries again with more coins)
	return false;
}

void Generator::fill_left(uint8_t count, size_t index)
{
	// Place `count` coins in left pan, starting from part `index`
	// We will make no assumptions about existing values in these slots
	// We expect that caller will have validated that there is room for these coins
	for (; index != left.size(); ++index)
	{
		// Select as many coins as possible
		left[index] = std::min(count, input[index]);
		count -= left[index];
	}
	assert(count == 0);
}

void Generator::fill_right(uint8_t count, size_t index)
{
	// Place count coins in right pan, starting from index part
	// Caller already knows that there is room to place the coins
	for (; index != right.size(); ++index)
	{
		// Select as many coins as possible
		// C++ note: To avoid integer type promotion we must explicitly use uint8_t for this expression
		right[index] = std::min(count, static_cast<uint8_t>(input[index] - left[index]));
		count -= right[index];
	}
	assert(count == 0);
}

const std::vector<Partition2::Child>& Partition2::get_children()
{
	// We are asked to give the list of children (i.e. distinct weighings) of this partition
	// We only compute this once.  Since it is never empty we can determine if we computed it before
	if (children.empty())
	{
		// We need a generator class to track the effort of computing the weighings
		Generator generator(*this);
		
		// Generate each of the weighings - note that there is always at least one
		// The generation code will not select any weighing in which exchanging left and right pans
		// will produce a distinct weighing in the list.  Each pair will only have one representative weighing.
		do {
			children.push_back(generator.get());
		} while (generator.advance());
	}
	
	return children;
}

Partition2* Partition2::get_instance(std::vector<uint8_t>&& parts)
{
	// Get the cached instance of a partition with the given parts, creating it if necessary
	Partition2 probe(std::move(parts));
	auto iterator = cache.find(&probe);
	if (iterator != cache.end())
	{
		// Switch to partition instance already in cache
		return iterator->get();
	}
	else
	{
		// Create and return a cached instance of the partition
		auto instance = std::make_unique<Partition2>(std::move(probe));
		auto result = instance.get();
		cache.insert(std::move(instance));
		return result;
	}
}

void Partition2::write(Output2& output, const Weighing2* weighing) const
{
	// Optionally caller may provide a weighing vector to explain how partition was computed
	if (weighing)
	{
		// Each part consists of the members of some input part that were sent to some bucket
		// Count for each input part how many ways it is divided
		// We know there cannot be more input parts than output parts
		std::vector<int> counters(parts.size());
		for (auto& p : *weighing)
		{
			counters[p.part] += 1;
		}
		
		std::vector<std::string> part_provenances;
		for (auto& p : *weighing)
		{
			if (counters[p.part] == 1)
			{
				part_provenances.push_back(std::format("p[{}]", p.part));
			}
			else
			{
				part_provenances.push_back(std::format("p[{}]@{}", p.part, placement_names[p.placement]));
			}
		}

		// C++ 20 Note: The {::s} format makes it omit string delimiters around the parts
		output.println("Partition: {{ {} part{};  Sizes: {};  Provenances: {::s} }}",
					   parts.size(), parts.size() == 1 ? "" : "s", parts, part_provenances);
	}
	else
	{
		// C++20 Note: Built in support for formatting vectors (and other ranges)
		output.println("Partition: {{ {} part{};  Sizes: {} }}",
					   parts.size(), parts.size() == 1 ? "" : "s", parts);
	}
}
